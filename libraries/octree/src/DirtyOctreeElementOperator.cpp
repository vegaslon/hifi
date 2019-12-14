//
//  DirtyOctreeElementOperator.cpp
//  libraries/entities/src
//
//  Created by Andrew Meawdows 2016.02.04
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DirtyOctreeElementOperator.h"

DirtyOctreeElementOperator::DirtyOctreeElementOperator(const OctreeElementPointer& element)
    :   _element(element) {
    assert(_element.get());
    _element->bumpChangedContent();
    _point = _element->getAACube().calcCenter();
}

bool DirtyOctreeElementOperator::preRecursion(const OctreeElementPointer& element) {
    if (element == _element) {
        return false;
    }
    return element->getAACube().contains(_point);
}

bool DirtyOctreeElementOperator::postRecursion(const OctreeElementPointer& element) {
    element->markWithChangedTime();
    return true;
}
