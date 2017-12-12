//
//  ZoneEntityItem.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QDebug>

#include <ByteCountCoding.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "ZoneEntityItem.h"
#include "EntityEditFilters.h"

bool ZoneEntityItem::_zonesArePickable = false;
bool ZoneEntityItem::_drawZoneBoundaries = false;


const ShapeType ZoneEntityItem::DEFAULT_SHAPE_TYPE = SHAPE_TYPE_BOX;
const QString ZoneEntityItem::DEFAULT_COMPOUND_SHAPE_URL = "";
const bool ZoneEntityItem::DEFAULT_FLYING_ALLOWED = true;
const bool ZoneEntityItem::DEFAULT_GHOSTING_ALLOWED = true;
const QString ZoneEntityItem::DEFAULT_FILTER_URL = "";

EntityItemPointer ZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity(new ZoneEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

ZoneEntityItem::ZoneEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    _type = EntityTypes::Zone;

    _shapeType = DEFAULT_SHAPE_TYPE;
    _compoundShapeURL = DEFAULT_COMPOUND_SHAPE_URL;

    _backgroundMode = BACKGROUND_MODE_INHERIT;
}

EntityItemProperties ZoneEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class

    // Contains a QString property, must be synchronized
    withReadLock([&] {
        _keyLightProperties.getProperties(properties);
    });
    
    _stageProperties.getProperties(properties);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeType);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(compoundShapeURL, getCompoundShapeURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(backgroundMode, getBackgroundMode);

    // Contains a QString property, must be synchronized
    withReadLock([&] {
        _skyboxProperties.getProperties(properties);
    });

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(flyingAllowed, getFlyingAllowed);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(ghostingAllowed, getGhostingAllowed);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(filterURL, getFilterURL);

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(hazeMode, getHazeMode);
    _hazeProperties.getProperties(properties);

    return properties;
}

bool ZoneEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "ZoneEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }

    return somethingChanged;
}

bool ZoneEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setSubClassProperties(properties); // set the properties in our base class

    // Contains a QString property, must be synchronized
    withWriteLock([&] {
        _keyLightPropertiesChanged = _keyLightProperties.setProperties(properties);
    });

    _stagePropertiesChanged = _stageProperties.setProperties(properties);

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, setShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(compoundShapeURL, setCompoundShapeURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(backgroundMode, setBackgroundMode);

    // Contains a QString property, must be synchronized
    withWriteLock([&] {
        _skyboxPropertiesChanged = _skyboxProperties.setProperties(properties);
    });

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(flyingAllowed, setFlyingAllowed);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(ghostingAllowed, setGhostingAllowed);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(filterURL, setFilterURL);
	
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(hazeMode, setHazeMode);
    _hazePropertiesChanged = _hazeProperties.setProperties(properties);

    somethingChanged = somethingChanged || _keyLightPropertiesChanged || _stagePropertiesChanged || _skyboxPropertiesChanged || _hazePropertiesChanged;

    return somethingChanged;
}

int ZoneEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    int bytesFromKeylight;
    withWriteLock([&] {
        bytesFromKeylight = _keyLightProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData, _keyLightPropertiesChanged);
    });

    somethingChanged = somethingChanged || _keyLightPropertiesChanged;
    bytesRead += bytesFromKeylight;
    dataAt += bytesFromKeylight;

    int bytesFromStage = _stageProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, 
                                                                               propertyFlags, overwriteLocalData, _stagePropertiesChanged);
    somethingChanged = somethingChanged || _stagePropertiesChanged;
    bytesRead += bytesFromStage;
    dataAt += bytesFromStage;

    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, setShapeType);
    READ_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
    READ_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, BackgroundMode, setBackgroundMode);

    int bytesFromSkybox;
    withWriteLock([&] {
        bytesFromSkybox = _skyboxProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData, _skyboxPropertiesChanged);
    });
    somethingChanged = somethingChanged || _skyboxPropertiesChanged;
    bytesRead += bytesFromSkybox;
    dataAt += bytesFromSkybox;

    READ_ENTITY_PROPERTY(PROP_FLYING_ALLOWED, bool, setFlyingAllowed);
    READ_ENTITY_PROPERTY(PROP_GHOSTING_ALLOWED, bool, setGhostingAllowed);
    READ_ENTITY_PROPERTY(PROP_FILTER_URL, QString, setFilterURL);

    READ_ENTITY_PROPERTY(PROP_HAZE_MODE, uint32_t, setHazeMode);

    int bytesFromHaze = _hazeProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
        propertyFlags, overwriteLocalData, _hazePropertiesChanged);

    somethingChanged = somethingChanged || _hazePropertiesChanged;
    bytesRead += bytesFromHaze;
    dataAt += bytesFromHaze;

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
EntityPropertyFlags ZoneEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    withReadLock([&] {
        requestedProperties += _keyLightProperties.getEntityProperties(params);
    });

    requestedProperties += _stageProperties.getEntityProperties(params);

    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += PROP_COMPOUND_SHAPE_URL;
    requestedProperties += PROP_BACKGROUND_MODE;

    withReadLock([&] {
        requestedProperties += _skyboxProperties.getEntityProperties(params);
    });

    requestedProperties += PROP_FLYING_ALLOWED;
    requestedProperties += PROP_GHOSTING_ALLOWED;
    requestedProperties += PROP_FILTER_URL;

    requestedProperties += PROP_HAZE_MODE;
    requestedProperties += _hazeProperties.getEntityProperties(params);

    return requestedProperties;
}

void ZoneEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;

    _keyLightProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                         propertyFlags, propertiesDidntFit, propertyCount, appendState);

    _stageProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
                                    propertyFlags, propertiesDidntFit, propertyCount, appendState);


    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeType());
    APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, getCompoundShapeURL());
    APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, (uint32_t)getBackgroundMode()); // could this be a uint16??

    _skyboxProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
        propertyFlags, propertiesDidntFit, propertyCount, appendState);

    APPEND_ENTITY_PROPERTY(PROP_FLYING_ALLOWED, getFlyingAllowed());
    APPEND_ENTITY_PROPERTY(PROP_GHOSTING_ALLOWED, getGhostingAllowed());
    APPEND_ENTITY_PROPERTY(PROP_FILTER_URL, getFilterURL());

    APPEND_ENTITY_PROPERTY(PROP_HAZE_MODE, (uint32_t)getHazeMode());
    _hazeProperties.appendSubclassData(packetData, params, modelTreeElementExtraEncodeData, requestedProperties,
        propertyFlags, propertiesDidntFit, propertyCount, appendState);
}

void ZoneEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   ZoneEntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                  position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "                dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "             getLastEdited:" << debugTime(getLastEdited(), now);
    qCDebug(entities) << "               _backgroundMode:" << EntityItemProperties::getBackgroundModeString(_backgroundMode);
    qCDebug(entities) << "               _hazeMode:" << EntityItemProperties::getHazeModeString(_hazeMode);

    _keyLightProperties.debugDump();
    _skyboxProperties.debugDump();
    _hazeProperties.debugDump();
    _stageProperties.debugDump();
}

ShapeType ZoneEntityItem::getShapeType() const {
    // Zones are not allowed to have a SHAPE_TYPE_NONE... they are always at least a SHAPE_TYPE_BOX
    if (_shapeType == SHAPE_TYPE_COMPOUND) {
        return hasCompoundShapeURL() ? SHAPE_TYPE_COMPOUND : DEFAULT_SHAPE_TYPE;
    } else {
        return _shapeType == SHAPE_TYPE_NONE ? DEFAULT_SHAPE_TYPE : _shapeType;
    }
}

void ZoneEntityItem::setCompoundShapeURL(const QString& url) {
    withWriteLock([&] {
        _compoundShapeURL = url;
        if (_compoundShapeURL.isEmpty() && _shapeType == SHAPE_TYPE_COMPOUND) {
            _shapeType = DEFAULT_SHAPE_TYPE;
        }
    });
}

bool ZoneEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElementPointer& element, float& distance, 
                         BoxFace& face, glm::vec3& surfaceNormal,
                         void** intersectedObject, bool precisionPicking) const {

    return _zonesArePickable;
}

void ZoneEntityItem::setFilterURL(QString url) {
    withWriteLock([&] {
        _filterURL = url;
    });
    if (DependencyManager::isSet<EntityEditFilters>()) {
        auto entityEditFilters = DependencyManager::get<EntityEditFilters>();
        qCDebug(entities) << "adding filter " << url << "for zone" << getEntityItemID();
        entityEditFilters->addFilter(getEntityItemID(), url);
    }
}

QString ZoneEntityItem::getFilterURL() const { 
    QString result;
    withReadLock([&] {
        result = _filterURL;
    });
    return result;
}

bool ZoneEntityItem::hasCompoundShapeURL() const { 
    return !getCompoundShapeURL().isEmpty();
}

QString ZoneEntityItem::getCompoundShapeURL() const { 
    QString result;
    withReadLock([&] {
        result = _compoundShapeURL;
    });
    return result;
}

void ZoneEntityItem::resetRenderingPropertiesChanged() {
    withWriteLock([&] {
        _keyLightPropertiesChanged = false;
        _backgroundPropertiesChanged = false;
        _skyboxPropertiesChanged = false;
        _hazePropertiesChanged = false;
        _stagePropertiesChanged = false;
    });
}

void ZoneEntityItem::setHazeMode(const uint32_t value) {
    if (value < COMPONENT_MODE_ITEM_COUNT) {
        _hazeMode = value;
        _hazePropertiesChanged = true;
    }
}

uint32_t ZoneEntityItem::getHazeMode() const {
    return _hazeMode;
}
