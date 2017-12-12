//
//  DiffTraversal.cpp
//  assignment-client/src/entities
//
//  Created by Andrew Meadows 2017.08.08
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DiffTraversal.h"

#include <OctreeUtils.h>


DiffTraversal::Waypoint::Waypoint(EntityTreeElementPointer& element) : _nextIndex(0) {
    assert(element);
    _weakElement = element;
}

void DiffTraversal::Waypoint::getNextVisibleElementFirstTime(DiffTraversal::VisibleElement& next,
        const DiffTraversal::View& view) {
    // NOTE: no need to set next.intersection in the "FirstTime" context
    if (_nextIndex == -1) {
        // root case is special:
        // its intersection is always INTERSECT,
        // we never bother checking for LOD culling, and
        // we can skip it if the content hasn't changed
        ++_nextIndex;
        next.element = _weakElement.lock();
        return;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement) {
                    if (!view.usesViewFrustum) {
                        // No LOD truncation if we aren't using the view frustum
                        next.element = nextElement;
                        return;
                    } else if (view.viewFrustum.cubeIntersectsKeyhole(nextElement->getAACube())) {
                        // check for LOD truncation
                        float distance = glm::distance(view.viewFrustum.getPosition(), nextElement->getAACube().calcCenter()) + MIN_VISIBLE_DISTANCE;
                        float angularDiameter = nextElement->getAACube().getScale() / distance;
                        if (angularDiameter > MIN_ELEMENT_ANGULAR_DIAMETER * view.lodScaleFactor) {
                            next.element = nextElement;
                            return;
                        }
                    }
                }
            }
        }
    }
    next.element.reset();
}

void DiffTraversal::Waypoint::getNextVisibleElementRepeat(
        DiffTraversal::VisibleElement& next, const DiffTraversal::View& view, uint64_t lastTime) {
    if (_nextIndex == -1) {
        // root case is special
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        if (element->getLastChangedContent() > lastTime) {
            next.element = element;
            next.intersection = ViewFrustum::INTERSECT;
            return;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && nextElement->getLastChanged() > lastTime) {
                    if (!view.usesViewFrustum) {
                        // No LOD truncation if we aren't using the view frustum
                        next.element = nextElement;
                        next.intersection = ViewFrustum::INSIDE;
                        return;
                    } else {
                        // check for LOD truncation
                        float distance = glm::distance(view.viewFrustum.getPosition(), nextElement->getAACube().calcCenter()) + MIN_VISIBLE_DISTANCE;
                        float angularDiameter = nextElement->getAACube().getScale() / distance;
                        if (angularDiameter > MIN_ELEMENT_ANGULAR_DIAMETER * view.lodScaleFactor) {
                            ViewFrustum::intersection intersection = view.viewFrustum.calculateCubeKeyholeIntersection(nextElement->getAACube());
                            if (intersection != ViewFrustum::OUTSIDE) {
                                next.element = nextElement;
                                next.intersection = intersection;
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}

void DiffTraversal::Waypoint::getNextVisibleElementDifferential(DiffTraversal::VisibleElement& next,
        const DiffTraversal::View& view, const DiffTraversal::View& lastView) {
    if (_nextIndex == -1) {
        // root case is special
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        next.element = element;
        next.intersection = ViewFrustum::INTERSECT;
        return;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement) {
                    AACube cube = nextElement->getAACube();
                    // check for LOD truncation
                    float distance = glm::distance(view.viewFrustum.getPosition(), cube.calcCenter()) + MIN_VISIBLE_DISTANCE;
                    float angularDiameter = cube.getScale() / distance;
                    if (angularDiameter > MIN_ELEMENT_ANGULAR_DIAMETER * view.lodScaleFactor) {
                        if (view.viewFrustum.calculateCubeKeyholeIntersection(cube) != ViewFrustum::OUTSIDE) {
                            next.element = nextElement;
                            next.intersection = ViewFrustum::OUTSIDE;
                            return;
                        }
                    }
                }
            }
        }
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}

DiffTraversal::DiffTraversal() {
    const int32_t MIN_PATH_DEPTH = 16;
    _path.reserve(MIN_PATH_DEPTH);
}

DiffTraversal::Type DiffTraversal::prepareNewTraversal(const ViewFrustum& viewFrustum, EntityTreeElementPointer root, 
        int32_t lodLevelOffset, bool usesViewFrustum) {
    assert(root);
    // there are three types of traversal:
    //
    //   (1) First = fresh view --> find all elements in view
    //   (2) Repeat = view hasn't changed --> find elements changed since last complete traversal
    //   (3) Differential = view has changed --> find elements changed or in new view but not old
    //
    // for each traversal type we assign the appropriate _getNextVisibleElementCallback
    //
    //   _getNextVisibleElementCallback = identifies elements that need to be traversed,
    //        updates VisibleElement ref argument with pointer-to-element and view-intersection
    //        (INSIDE, INTERSECT, or OUTSIDE)
    //
    // external code should update the _scanElementCallback after calling prepareNewTraversal
    //
    _currentView.usesViewFrustum = usesViewFrustum;
    float lodScaleFactor = powf(2.0f, lodLevelOffset);

    Type type;
    // If usesViewFrustum changes, treat it as a First traversal
    if (_completedView.startTime == 0 || _currentView.usesViewFrustum != _completedView.usesViewFrustum) {
        type = Type::First;
        _currentView.viewFrustum = viewFrustum;
        _currentView.lodScaleFactor = lodScaleFactor;
        _getNextVisibleElementCallback = [this](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementFirstTime(next, _currentView);
        };
    } else if (!_currentView.usesViewFrustum ||
               (_completedView.viewFrustum.isVerySimilar(viewFrustum) &&
                lodScaleFactor == _completedView.lodScaleFactor)) {
        type = Type::Repeat;
        _getNextVisibleElementCallback = [this](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementRepeat(next, _completedView, _completedView.startTime);
        };
    } else {
        type = Type::Differential;
        _currentView.viewFrustum = viewFrustum;
        _currentView.lodScaleFactor = lodScaleFactor;
        _getNextVisibleElementCallback = [this](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementDifferential(next, _currentView, _completedView);
        };
    }

    _path.clear();
    _path.push_back(DiffTraversal::Waypoint(root));
    // set root fork's index such that root element returned at getNextElement()
    _path.back().initRootNextIndex();

    _currentView.startTime = usecTimestampNow();

    return type;
}

void DiffTraversal::getNextVisibleElement(DiffTraversal::VisibleElement& next) {
    if (_path.empty()) {
        next.element.reset();
        next.intersection = ViewFrustum::OUTSIDE;
        return;
    }
    _getNextVisibleElementCallback(next);
    if (next.element) {
        int8_t nextIndex = _path.back().getNextIndex();
        if (nextIndex > 0) {
            _path.push_back(DiffTraversal::Waypoint(next.element));
        }
    } else {
        // we're done at this level
        while (!next.element) {
            // pop one level
            _path.pop_back();
            if (_path.empty()) {
                // we've traversed the entire tree
                _completedView = _currentView;
                return;
            }
            // keep looking for next
            _getNextVisibleElementCallback(next);
            if (next.element) {
                // we've descended one level so add it to the path
                _path.push_back(DiffTraversal::Waypoint(next.element));
            }
        }
    }
}

void DiffTraversal::setScanCallback(std::function<void (DiffTraversal::VisibleElement&)> cb) {
    if (!cb) {
        _scanElementCallback = [](DiffTraversal::VisibleElement& a){};
    } else {
        _scanElementCallback = cb;
    }
}

void DiffTraversal::traverse(uint64_t timeBudget) {
    uint64_t expiry = usecTimestampNow() + timeBudget;
    DiffTraversal::VisibleElement next;
    getNextVisibleElement(next);
    while (next.element) {
        if (next.element->hasContent()) {
            _scanElementCallback(next);
        }
        if (usecTimestampNow() > expiry) {
            break;
        }
        getNextVisibleElement(next);
    }
}
