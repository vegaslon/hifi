//
//  EntityEditPacketSender.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>
#include <QJsonDocument>
#include <PerfStat.h>
#include <OctalCode.h>
#include <udt/PacketHeaders.h>
#include "EntityEditPacketSender.h"
#include "EntitiesLogging.h"
#include "EntityItem.h"
#include "EntityItemProperties.h"
#include <AddressManager.h>

EntityEditPacketSender::EntityEditPacketSender() {
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerDirectListener(PacketType::EntityEditNack, this, "processEntityEditNackPacket");
}

void EntityEditPacketSender::processEntityEditNackPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    processNackPacket(*message, sendingNode);
}

void EntityEditPacketSender::adjustEditPacketForClockSkew(PacketType type, QByteArray& buffer, qint64 clockSkew) {
    if (type == PacketType::EntityAdd || type == PacketType::EntityEdit || type == PacketType::EntityPhysics) {
        EntityItem::adjustEditPacketForClockSkew(buffer, clockSkew);
    }
}

void EntityEditPacketSender::queueEditAvatarEntityMessage(PacketType type,
                                                          EntityTreePointer entityTree,
                                                          EntityItemID entityItemID,
                                                          const EntityItemProperties& properties) {
    assert(_myAvatar);
    if (!entityTree) {
        qCDebug(entities) << "EntityEditPacketSender::queueEditEntityMessage null entityTree.";
        return;
    }
    EntityItemPointer entity = entityTree->findEntityByEntityItemID(entityItemID);
    if (!entity) {
        qCDebug(entities) << "EntityEditPacketSender::queueEditAvatarEntityMessage can't find entity: " << entityItemID;
        return;
    }

    // the properties that get serialized into the avatar identity packet should be the entire set
    // rather than just the ones being edited.
    EntityItemProperties entityProperties = entity->getProperties();
    entityProperties.merge(properties);

    std::lock_guard<std::mutex> lock(_mutex);
    QScriptValue scriptProperties = EntityItemNonDefaultPropertiesToScriptValue(&_scriptEngine, entityProperties);
    QVariant variantProperties = scriptProperties.toVariant();
    QJsonDocument jsonProperties = QJsonDocument::fromVariant(variantProperties);

    // the ID of the parent/avatar changes from session to session.  use a special UUID to indicate the avatar
    QJsonObject jsonObject = jsonProperties.object();
    if (jsonObject.contains("parentID")) {
        if (QUuid(jsonObject["parentID"].toString()) == _myAvatar->getID()) {
            jsonObject["parentID"] = AVATAR_SELF_ID.toString();
        }
    }
    jsonProperties = QJsonDocument(jsonObject);

    QByteArray binaryProperties = jsonProperties.toBinaryData();
    _myAvatar->updateAvatarEntity(entityItemID, binaryProperties);

    entity->setLastBroadcast(usecTimestampNow());
    return;
}


void EntityEditPacketSender::queueEditEntityMessage(PacketType type,
                                                    EntityTreePointer entityTree,
                                                    EntityItemID entityItemID,
                                                    const EntityItemProperties& properties) {
    if (!_shouldSend) {
        return; // bail early
    }

    if (properties.getClientOnly() && properties.getOwningAvatarID() == _myAvatar->getID()) {
        // this is an avatar-based entity --> update our avatar-data rather than sending to the entity-server
        queueEditAvatarEntityMessage(type, entityTree, entityItemID, properties);
        return;
    }

    QByteArray bufferOut(NLPacket::maxPayloadSize(type), 0);

    if (type == PacketType::EntityAdd) {
        auto MAX_ADD_DATA_SIZE = NLPacket::maxPayloadSize(type) * 10; // a really big buffer
        bufferOut.resize(MAX_ADD_DATA_SIZE);
    }

    OctreeElement::AppendState encodeResult = OctreeElement::PARTIAL; // start the loop assuming there's more to send
    auto nodeList = DependencyManager::get<NodeList>();

    EntityPropertyFlags didntFitProperties;
    EntityItemProperties propertiesCopy = properties;

    if (properties.parentIDChanged() && properties.getParentID() == AVATAR_SELF_ID) {
        const QUuid myNodeID = nodeList->getSessionUUID();
        propertiesCopy.setParentID(myNodeID);
    }

    EntityPropertyFlags requestedProperties = propertiesCopy.getChangedProperties();

    while (encodeResult == OctreeElement::PARTIAL) {
        encodeResult = EntityItemProperties::encodeEntityEditPacket(type, entityItemID, propertiesCopy, bufferOut, requestedProperties, didntFitProperties);

        if (encodeResult != OctreeElement::NONE) {
            #ifdef WANT_DEBUG
                qCDebug(entities) << "calling queueOctreeEditMessage()...";
                qCDebug(entities) << "    id:" << entityItemID;
                qCDebug(entities) << "    properties:" << properties;
            #endif

            queueOctreeEditMessage(type, bufferOut);
            if (type == PacketType::EntityAdd && !properties.getCertificateID().isEmpty()) {
                emit addingEntityWithCertificate(properties.getCertificateID(), DependencyManager::get<AddressManager>()->getPlaceName());
            }
        }

        // if we still have properties to send, switch the message type to edit, and request only the packets that didn't fit
        if (encodeResult != OctreeElement::COMPLETED) {
            type = PacketType::EntityEdit;
            requestedProperties = didntFitProperties;
        }

        bufferOut.resize(NLPacket::maxPayloadSize(type)); // resize our output buffer for the next packet
    }
}

void EntityEditPacketSender::queueEraseEntityMessage(const EntityItemID& entityItemID) {
    if (!_shouldSend) {
        return; // bail early
    }

    // in case this was a clientOnly entity:
    if(_myAvatar) {
        _myAvatar->clearAvatarEntity(entityItemID);
    }

    QByteArray bufferOut(NLPacket::maxPayloadSize(PacketType::EntityErase), 0);

    if (EntityItemProperties::encodeEraseEntityMessage(entityItemID, bufferOut)) {
        queueOctreeEditMessage(PacketType::EntityErase, bufferOut);
    }
}
