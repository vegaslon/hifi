//
//  Created by Sam Gondelman 10/17/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Pick.h"

const PickFilter PickFilter::NOTHING;

int pickTypeMetaTypeId = qRegisterMetaType<PickQuery::PickType>("PickType");

PickQuery::PickQuery(const PickFilter& filter, const float maxDistance, const bool enabled) :
    _filter(filter),
    _maxDistance(maxDistance),
    _enabled(enabled) {
}

void PickQuery::enable(bool enabled) {
    withWriteLock([&] {
        _enabled = enabled;
    });
}

PickFilter PickQuery::getFilter() const {
    return resultWithReadLock<PickFilter>([&] {
        return _filter;
    });
}

float PickQuery::getMaxDistance() const {
    return _maxDistance;
}

bool PickQuery::isEnabled() const {
    return resultWithReadLock<bool>([&] {
        return _enabled;
    });
}

void PickQuery::setPrecisionPicking(bool precisionPicking) {
    withWriteLock([&] {
        _filter.setFlag(PickFilter::PICK_COARSE, !precisionPicking);
    });
}

void PickQuery::setPickResult(const PickResultPointer& pickResult) {
    withWriteLock([&] {
        _prevResult = pickResult;
    });
}

QVector<QUuid> PickQuery::getIgnoreItems() const {
    return resultWithReadLock<QVector<QUuid>>([&] {
        return _ignoreItems;
    });
}

QVector<QUuid> PickQuery::getIncludeItems() const {
    return resultWithReadLock<QVector<QUuid>>([&] {
        return _includeItems;
    });
}

PickResultPointer PickQuery::getPrevPickResult() const {
    return resultWithReadLock<PickResultPointer>([&] {
        return _prevResult;
    });
}

void PickQuery::setIgnoreItems(const QVector<QUuid>& ignoreItems) {
    withWriteLock([&] {
        _ignoreItems = ignoreItems;
    });
}

void PickQuery::setIncludeItems(const QVector<QUuid>& includeItems) {
    withWriteLock([&] {
        _includeItems = includeItems;
    });
}