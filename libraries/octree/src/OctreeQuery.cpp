//
//  OctreeQuery.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 10/24/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <random>

#include <QtCore/QJsonDocument>

#include <GLMHelpers.h>
#include <udt/PacketHeaders.h>

#include "OctreeConstants.h"
#include "OctreeQuery.h"

const float DEFAULT_FOV = 45.0f; // degrees
const float DEFAULT_ASPECT_RATIO = 1.0f;
const float DEFAULT_NEAR_CLIP = 0.1f;
const float DEFAULT_FAR_CLIP = 3.0f;

OctreeQuery::OctreeQuery(bool randomizeConnectionID) :
    _cameraFov(DEFAULT_FOV),
    _cameraAspectRatio(DEFAULT_ASPECT_RATIO),
    _cameraNearClip(DEFAULT_NEAR_CLIP),
    _cameraFarClip(DEFAULT_FAR_CLIP),
    _cameraCenterRadius(DEFAULT_FAR_CLIP)
{
    _maxQueryPPS = DEFAULT_MAX_OCTREE_PPS;

    if (randomizeConnectionID) {
        // randomize our initial octree query connection ID using random_device
        // the connection ID is 16 bits so we take a generated 32 bit value from random device and chop off the top
        std::random_device randomDevice;
        _connectionID = randomDevice();
    }
}

int OctreeQuery::getBroadcastData(unsigned char* destinationBuffer) {
    unsigned char* bufferStart = destinationBuffer;

    // pack the connection ID so the server can detect when we start a new connection
    memcpy(destinationBuffer, &_connectionID, sizeof(_connectionID));
    destinationBuffer += sizeof(_connectionID);
    
    // back a boolean (cut to 1 byte) to designate if this query uses the sent view frustum
    memcpy(destinationBuffer, &_usesFrustum, sizeof(_usesFrustum));
    destinationBuffer += sizeof(_usesFrustum);
    
    if (_usesFrustum) {
        // TODO: DRY this up to a shared method
        // that can pack any type given the number of bytes
        // and return the number of bytes to push the pointer
        
        // camera details
        memcpy(destinationBuffer, &_cameraPosition, sizeof(_cameraPosition));
        destinationBuffer += sizeof(_cameraPosition);
        destinationBuffer += packOrientationQuatToBytes(destinationBuffer, _cameraOrientation);
        destinationBuffer += packFloatAngleToTwoByte(destinationBuffer, _cameraFov);
        destinationBuffer += packFloatRatioToTwoByte(destinationBuffer, _cameraAspectRatio);
        destinationBuffer += packClipValueToTwoByte(destinationBuffer, _cameraNearClip);
        destinationBuffer += packClipValueToTwoByte(destinationBuffer, _cameraFarClip);
        memcpy(destinationBuffer, &_cameraEyeOffsetPosition, sizeof(_cameraEyeOffsetPosition));
        destinationBuffer += sizeof(_cameraEyeOffsetPosition);
    }
    
    // desired Max Octree PPS
    memcpy(destinationBuffer, &_maxQueryPPS, sizeof(_maxQueryPPS));
    destinationBuffer += sizeof(_maxQueryPPS);

    // desired voxelSizeScale
    memcpy(destinationBuffer, &_octreeElementSizeScale, sizeof(_octreeElementSizeScale));
    destinationBuffer += sizeof(_octreeElementSizeScale);

    // desired boundaryLevelAdjust
    memcpy(destinationBuffer, &_boundaryLevelAdjust, sizeof(_boundaryLevelAdjust));
    destinationBuffer += sizeof(_boundaryLevelAdjust);

    memcpy(destinationBuffer, &_cameraCenterRadius, sizeof(_cameraCenterRadius));
    destinationBuffer += sizeof(_cameraCenterRadius);
    
    // create a QByteArray that holds the binary representation of the JSON parameters
    QByteArray binaryParametersDocument;
    
    if (!_jsonParameters.isEmpty()) {
        binaryParametersDocument = QJsonDocument(_jsonParameters).toBinaryData();
    }
    
    // write the size of the JSON parameters
    uint16_t binaryParametersBytes = binaryParametersDocument.size();
    memcpy(destinationBuffer, &binaryParametersBytes, sizeof(binaryParametersBytes));
    destinationBuffer += sizeof(binaryParametersBytes);
    
    // pack the binary JSON parameters
    // NOTE: for now we assume that the filters that will be set are all small enough that we will not have a packet > MTU
    if (binaryParametersDocument.size() > 0) {
        memcpy(destinationBuffer, binaryParametersDocument.data(), binaryParametersBytes);
        destinationBuffer += binaryParametersBytes;
    }
    
    return destinationBuffer - bufferStart;
}

// called on the other nodes - assigns it to my views of the others
int OctreeQuery::parseData(ReceivedMessage& message) {
 
    const unsigned char* startPosition = reinterpret_cast<const unsigned char*>(message.getRawMessage());
    const unsigned char* sourceBuffer = startPosition;

    // unpack the connection ID
    uint16_t newConnectionID;
    memcpy(&newConnectionID, sourceBuffer, sizeof(newConnectionID));
    sourceBuffer += sizeof(newConnectionID);

    if (!_hasReceivedFirstQuery) {
        // set our flag to indicate that we've parsed for this query at least once
        _hasReceivedFirstQuery = true;

        // set the incoming connection ID as the current
        _connectionID = newConnectionID;
    } else {
        if (newConnectionID != _connectionID) {
            // the connection ID has changed - emit our signal so the server
            // knows that the client is starting a new session
            _connectionID = newConnectionID;
            emit incomingConnectionIDChanged();
        }
    }

    // check if this query uses a view frustum
    memcpy(&_usesFrustum, sourceBuffer, sizeof(_usesFrustum));
    sourceBuffer += sizeof(_usesFrustum);
    
    if (_usesFrustum) {
        // unpack camera details
        memcpy(&_cameraPosition, sourceBuffer, sizeof(_cameraPosition));
        sourceBuffer += sizeof(_cameraPosition);
        sourceBuffer += unpackOrientationQuatFromBytes(sourceBuffer, _cameraOrientation);
        sourceBuffer += unpackFloatAngleFromTwoByte((uint16_t*) sourceBuffer, &_cameraFov);
        sourceBuffer += unpackFloatRatioFromTwoByte(sourceBuffer,_cameraAspectRatio);
        sourceBuffer += unpackClipValueFromTwoByte(sourceBuffer,_cameraNearClip);
        sourceBuffer += unpackClipValueFromTwoByte(sourceBuffer,_cameraFarClip);
        memcpy(&_cameraEyeOffsetPosition, sourceBuffer, sizeof(_cameraEyeOffsetPosition));
        sourceBuffer += sizeof(_cameraEyeOffsetPosition);
    }

    // desired Max Octree PPS
    memcpy(&_maxQueryPPS, sourceBuffer, sizeof(_maxQueryPPS));
    sourceBuffer += sizeof(_maxQueryPPS);

    // desired _octreeElementSizeScale
    memcpy(&_octreeElementSizeScale, sourceBuffer, sizeof(_octreeElementSizeScale));
    sourceBuffer += sizeof(_octreeElementSizeScale);

    // desired boundaryLevelAdjust
    memcpy(&_boundaryLevelAdjust, sourceBuffer, sizeof(_boundaryLevelAdjust));
    sourceBuffer += sizeof(_boundaryLevelAdjust);
    
    memcpy(&_cameraCenterRadius, sourceBuffer, sizeof(_cameraCenterRadius));
    sourceBuffer += sizeof(_cameraCenterRadius);
    
    // check if we have a packed JSON filter
    uint16_t binaryParametersBytes;
    memcpy(&binaryParametersBytes, sourceBuffer, sizeof(binaryParametersBytes));
    sourceBuffer += sizeof(binaryParametersBytes);
    
    if (binaryParametersBytes > 0) {
        // unpack the binary JSON parameters
        QByteArray binaryJSONParameters { binaryParametersBytes, 0 };
        memcpy(binaryJSONParameters.data(), sourceBuffer, binaryParametersBytes);
        sourceBuffer += binaryParametersBytes;
        
        // grab the parameter object from the packed binary representation of JSON
        auto newJsonDocument = QJsonDocument::fromBinaryData(binaryJSONParameters);
        
        QWriteLocker jsonParameterLocker { &_jsonParametersLock };
        _jsonParameters = newJsonDocument.object();
    }
    
    return sourceBuffer - startPosition;
}

glm::vec3 OctreeQuery::calculateCameraDirection() const {
    glm::vec3 direction = glm::vec3(_cameraOrientation * glm::vec4(IDENTITY_FORWARD, 0.0f));
    return direction;
}
