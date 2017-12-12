//
//  OctreeQuery.h
//  libraries/octree/src
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeQuery_h
#define hifi_OctreeQuery_h

#include <inttypes.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QJsonObject>
#include <QtCore/QReadWriteLock>

#include <NodeData.h>


class OctreeQuery : public NodeData {
    Q_OBJECT

public:
    OctreeQuery(bool randomizeConnectionID = false);
    virtual ~OctreeQuery() {}

    int getBroadcastData(unsigned char* destinationBuffer);
    int parseData(ReceivedMessage& message) override;

    // getters for camera details
    const glm::vec3& getCameraPosition() const { return _cameraPosition; }
    const glm::quat& getCameraOrientation() const { return _cameraOrientation; }
    float getCameraFov() const { return _cameraFov; }
    float getCameraAspectRatio() const { return _cameraAspectRatio; }
    float getCameraNearClip() const { return _cameraNearClip; }
    float getCameraFarClip() const { return _cameraFarClip; }
    const glm::vec3& getCameraEyeOffsetPosition() const { return _cameraEyeOffsetPosition; }
    float getCameraCenterRadius() const { return _cameraCenterRadius; }

    glm::vec3 calculateCameraDirection() const;

    // setters for camera details
    void setCameraPosition(const glm::vec3& position) { _cameraPosition = position; }
    void setCameraOrientation(const glm::quat& orientation) { _cameraOrientation = orientation; }
    void setCameraFov(float fov) { _cameraFov = fov; }
    void setCameraAspectRatio(float aspectRatio) { _cameraAspectRatio = aspectRatio; }
    void setCameraNearClip(float nearClip) { _cameraNearClip = nearClip; }
    void setCameraFarClip(float farClip) { _cameraFarClip = farClip; }
    void setCameraEyeOffsetPosition(const glm::vec3& eyeOffsetPosition) { _cameraEyeOffsetPosition = eyeOffsetPosition; }
    void setCameraCenterRadius(float radius) { _cameraCenterRadius = radius; }
    
    // getters/setters for JSON filter
    QJsonObject getJSONParameters() { QReadLocker locker { &_jsonParametersLock }; return _jsonParameters; }
    void setJSONParameters(const QJsonObject& jsonParameters)
        { QWriteLocker locker { &_jsonParametersLock }; _jsonParameters = jsonParameters; }
    
    // related to Octree Sending strategies
    int getMaxQueryPacketsPerSecond() const { return _maxQueryPPS; }
    float getOctreeSizeScale() const { return _octreeElementSizeScale; }
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }
    
    bool getUsesFrustum() { return _usesFrustum; }
    void setUsesFrustum(bool usesFrustum) { _usesFrustum = usesFrustum; }

    void incrementConnectionID() { ++_connectionID; }

    bool hasReceivedFirstQuery() const  { return _hasReceivedFirstQuery; }

signals:
    void incomingConnectionIDChanged();

public slots:
    void setMaxQueryPacketsPerSecond(int maxQueryPPS) { _maxQueryPPS = maxQueryPPS; }
    void setOctreeSizeScale(float octreeSizeScale) { _octreeElementSizeScale = octreeSizeScale; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust) { _boundaryLevelAdjust = boundaryLevelAdjust; }

protected:
    // camera details for the avatar
    glm::vec3 _cameraPosition { glm::vec3(0.0f) };
    glm::quat _cameraOrientation { glm::quat() };
    float _cameraFov;
    float _cameraAspectRatio;
    float _cameraNearClip;
    float _cameraFarClip;
    float _cameraCenterRadius;
    glm::vec3 _cameraEyeOffsetPosition { glm::vec3(0.0f) };

    // octree server sending items
    int _maxQueryPPS = DEFAULT_MAX_OCTREE_PPS;
    float _octreeElementSizeScale = DEFAULT_OCTREE_SIZE_SCALE; /// used for LOD calculations
    int _boundaryLevelAdjust = 0; /// used for LOD calculations
    
    uint8_t _usesFrustum = true;
    uint16_t _connectionID; // query connection ID, randomized to start, increments with each new connection to server
    
    QJsonObject _jsonParameters;
    QReadWriteLock _jsonParametersLock;

    bool _hasReceivedFirstQuery { false };
    
private:
    // privatize the copy constructor and assignment operator so they cannot be called
    OctreeQuery(const OctreeQuery&);
    OctreeQuery& operator= (const OctreeQuery&);
};

#endif // hifi_OctreeQuery_h
