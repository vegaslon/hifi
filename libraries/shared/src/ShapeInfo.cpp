//
//  ShapeInfo.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ShapeInfo.h"

#include <math.h>

#include "NumericalConstants.h" // for MILLIMETERS_PER_METER

// Originally within EntityItemProperties.cpp
const char* shapeTypeNames[] = {
    "none",
    "box",
    "sphere",
    "capsule-x",
    "capsule-y",
    "capsule-z",
    "cylinder-x",
    "cylinder-y",
    "cylinder-z",
    "hull",
    "plane",
    "compound",
    "simple-hull",
    "simple-compound",
    "static-mesh"
};

static const size_t SHAPETYPE_NAME_COUNT = (sizeof(shapeTypeNames) / sizeof((shapeTypeNames)[0]));

// Bullet doesn't support arbitrarily small shapes
const float MIN_HALF_EXTENT = 0.005f; // 0.5 cm

QString ShapeInfo::getNameForShapeType(ShapeType type) {
    if (((int)type <= 0) || ((int)type >= (int)SHAPETYPE_NAME_COUNT)) {
        type = (ShapeType)0;
    }

    return shapeTypeNames[(int)type];
}

void ShapeInfo::clear() {
    _url.clear();
    _pointCollection.clear();
    _triangleIndices.clear();
    _halfExtents = glm::vec3(0.0f);
    _offset = glm::vec3(0.0f);
    _hashKey.clear();
    _type = SHAPE_TYPE_NONE;
}

void ShapeInfo::setParams(ShapeType type, const glm::vec3& halfExtents, QString url) {
    _url = "";
    _type = type;
    setHalfExtents(halfExtents);
    switch(type) {
        case SHAPE_TYPE_NONE:
            _halfExtents = glm::vec3(0.0f);
            break;
        case SHAPE_TYPE_BOX:
        case SHAPE_TYPE_HULL:
            break;
        case SHAPE_TYPE_SPHERE: {
                float radius = glm::length(halfExtents) / SQUARE_ROOT_OF_3;
                radius = glm::max(radius, MIN_HALF_EXTENT);
                _halfExtents = glm::vec3(radius);
            }
            break;
        case SHAPE_TYPE_CIRCLE: {
            _halfExtents = glm::vec3(_halfExtents.x, MIN_HALF_EXTENT, _halfExtents.z);
        }
        break;
        case SHAPE_TYPE_COMPOUND:
        case SHAPE_TYPE_SIMPLE_HULL:
        case SHAPE_TYPE_SIMPLE_COMPOUND:
        case SHAPE_TYPE_STATIC_MESH:
            _url = QUrl(url);
            break;
        default:
            break;
    }
    _hashKey.clear();
}

void ShapeInfo::setBox(const glm::vec3& halfExtents) {
    _url = "";
    _type = SHAPE_TYPE_BOX;
    setHalfExtents(halfExtents);
    _hashKey.clear();
}

void ShapeInfo::setSphere(float radius) {
    _url = "";
    _type = SHAPE_TYPE_SPHERE;
    radius = glm::max(radius, MIN_HALF_EXTENT);
    _halfExtents = glm::vec3(radius);
    _hashKey.clear();
}

void ShapeInfo::setPointCollection(const ShapeInfo::PointCollection& pointCollection) {
    _pointCollection = pointCollection;
    _hashKey.clear();
}

void ShapeInfo::setCapsuleY(float radius, float halfHeight) {
    _url = "";
    _type = SHAPE_TYPE_CAPSULE_Y;
    radius = glm::max(radius, MIN_HALF_EXTENT);
    halfHeight = glm::max(halfHeight, 0.0f);
    _halfExtents = glm::vec3(radius, halfHeight, radius);
    _hashKey.clear();
}

void ShapeInfo::setOffset(const glm::vec3& offset) {
    _offset = offset;
    _hashKey.clear();
}

uint32_t ShapeInfo::getNumSubShapes() const {
    switch (_type) {
        case SHAPE_TYPE_NONE:
            return 0;
        case SHAPE_TYPE_COMPOUND:
        case SHAPE_TYPE_SIMPLE_COMPOUND:
            return _pointCollection.size();
        case SHAPE_TYPE_SIMPLE_HULL:
        case SHAPE_TYPE_STATIC_MESH:
            assert(_pointCollection.size() == 1);
            // yes fall through to default
        default:
            return 1;
    }
}

int ShapeInfo::getLargestSubshapePointCount() const {
    int numPoints = 0;
    for (int i = 0; i < _pointCollection.size(); ++i) {
        int n = _pointCollection[i].size();
        if (n > numPoints) {
            numPoints = n;
        }
    }
    return numPoints;
}

float computeCylinderVolume(const float radius, const float height) {
    return PI * radius * radius * 2.0f * height;
}

float computeCapsuleVolume(const float radius, const float cylinderHeight) {
    return PI * radius * radius * (cylinderHeight + 4.0f * radius / 3.0f);
}

float ShapeInfo::computeVolume() const {
    const float DEFAULT_VOLUME = 1.0f;
    float volume = DEFAULT_VOLUME;
    switch(_type) {
        case SHAPE_TYPE_BOX: {
            // factor of 8.0 because the components of _halfExtents are all halfExtents
            volume = 8.0f * _halfExtents.x * _halfExtents.y * _halfExtents.z;
            break;
        }
        case SHAPE_TYPE_SPHERE: {
            volume = 4.0f * PI * _halfExtents.x * _halfExtents.y * _halfExtents.z / 3.0f;
            break;
        }
        case SHAPE_TYPE_CYLINDER_X: {
            volume = computeCylinderVolume(_halfExtents.y, _halfExtents.x);
            break;
        }
        case SHAPE_TYPE_CYLINDER_Y: {
            volume = computeCylinderVolume(_halfExtents.x, _halfExtents.y);
            break;
        }
        case SHAPE_TYPE_CYLINDER_Z: {
            volume = computeCylinderVolume(_halfExtents.x, _halfExtents.z);
            break;
        }
        case SHAPE_TYPE_CAPSULE_X: {
            // Need to offset halfExtents.x by y to account for the system treating
            // the x extent of the capsule as the cylindrical height + spherical radius.
            const float cylinderHeight = 2.0f * (_halfExtents.x - _halfExtents.y);
            volume = computeCapsuleVolume(_halfExtents.y, cylinderHeight);
            break;
        }
        case SHAPE_TYPE_CAPSULE_Y: {
            // Need to offset halfExtents.y by x to account for the system treating
            // the y extent of the capsule as the cylindrical height + spherical radius.
            const float cylinderHeight = 2.0f * (_halfExtents.y - _halfExtents.x);
            volume = computeCapsuleVolume(_halfExtents.x, cylinderHeight);
            break;
        }
        case SHAPE_TYPE_CAPSULE_Z: {
            // Need to offset halfExtents.z by x to account for the system treating
            // the z extent of the capsule as the cylindrical height + spherical radius.
            const float cylinderHeight = 2.0f * (_halfExtents.z - _halfExtents.x);
            volume = computeCapsuleVolume(_halfExtents.x, cylinderHeight);
            break;
        }
        default:
            break;
    }
    assert(volume > 0.0f);
    return volume;
}

bool ShapeInfo::contains(const glm::vec3& point) const {
    switch(_type) {
        case SHAPE_TYPE_SPHERE:
            return glm::length(point) <= _halfExtents.x;
        case SHAPE_TYPE_CYLINDER_X:
            return glm::length(glm::vec2(point.y, point.z)) <= _halfExtents.z;
        case SHAPE_TYPE_CYLINDER_Y:
            return glm::length(glm::vec2(point.x, point.z)) <= _halfExtents.x;
        case SHAPE_TYPE_CYLINDER_Z:
            return glm::length(glm::vec2(point.x, point.y)) <= _halfExtents.y;
        case SHAPE_TYPE_CAPSULE_X: {
            if (glm::abs(point.x) <= _halfExtents.x) {
                return glm::length(glm::vec2(point.y, point.z)) <= _halfExtents.z;
            } else {
                glm::vec3 absPoint = glm::abs(point) - _halfExtents.x;
                return glm::length(absPoint) <= _halfExtents.z;
            }
        }
        case SHAPE_TYPE_CAPSULE_Y: {
            if (glm::abs(point.y) <= _halfExtents.y) {
                return glm::length(glm::vec2(point.x, point.z)) <= _halfExtents.x;
            } else {
                glm::vec3 absPoint = glm::abs(point) - _halfExtents.y;
                return glm::length(absPoint) <= _halfExtents.x;
            }
        }
        case SHAPE_TYPE_CAPSULE_Z: {
            if (glm::abs(point.z) <= _halfExtents.z) {
                return glm::length(glm::vec2(point.x, point.y)) <= _halfExtents.y;
            } else {
                glm::vec3 absPoint = glm::abs(point) - _halfExtents.z;
                return glm::length(absPoint) <= _halfExtents.y;
            }
        }
        case SHAPE_TYPE_BOX:
        default: {
            glm::vec3 absPoint = glm::abs(point);
            return absPoint.x <= _halfExtents.x
            && absPoint.y <= _halfExtents.y
            && absPoint.z <= _halfExtents.z;
        }
    }
}

const HashKey& ShapeInfo::getHash() const {
    // NOTE: we cache the key so we only ever need to compute it once for any valid ShapeInfo instance.
    if (_hashKey.isNull() && _type != SHAPE_TYPE_NONE) {
        // The key is not yet cached therefore we must compute it.

        _hashKey.hashUint64((uint64_t)_type);
        if (_type != SHAPE_TYPE_SIMPLE_HULL) {
            _hashKey.hashVec3(_halfExtents);
            _hashKey.hashVec3(_offset);
        } else {
            // TODO: we could avoid hashing all of these points if we were to supply the ShapeInfo with a unique
            // descriptive string.  Shapes that are uniquely described by their type and URL could just put their
            // url in the description.
            assert(_pointCollection.size() == (size_t)1);
            const PointList & points = _pointCollection.back();
            const int numPoints = (int)points.size();

            for (int i = 0; i < numPoints; ++i) {
                _hashKey.hashVec3(points[i]);
            }
        }

        QString url = _url.toString();
        if (!url.isEmpty()) {
            QByteArray baUrl = url.toLocal8Bit();
            uint32_t urlHash = qChecksum(baUrl.data(), baUrl.size());
            _hashKey.hashUint64((uint64_t)urlHash);
        }

        if (_type == SHAPE_TYPE_COMPOUND || _type == SHAPE_TYPE_SIMPLE_COMPOUND) {
            uint64_t numHulls = (uint64_t)_pointCollection.size();
            _hashKey.hashUint64(numHulls);
        } else if (_type == SHAPE_TYPE_SIMPLE_HULL) {
            _hashKey.hashUint64(1);
        }
    }
    return _hashKey;
}

void ShapeInfo::setHalfExtents(const glm::vec3& halfExtents) {
    _halfExtents = glm::max(halfExtents, glm::vec3(MIN_HALF_EXTENT));
    _hashKey.clear();
}
