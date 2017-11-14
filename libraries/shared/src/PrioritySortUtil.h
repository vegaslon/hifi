//
//  PrioritySortUtil.h
//
//  Created by Andrew Meadows on 2017-11-08
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_PrioritySortUtil_h
#define hifi_PrioritySortUtil_h

#include <glm/glm.hpp>
#include "ViewFrustum.h"

/*   PrioritySortUtil is a helper for sorting 3D things relative to a ViewFrustum.  To use:

(1) Derive a class from pure-virtual PrioritySortUtil::Sortable that wraps a copy of
    the Thing you want to prioritize and sort:

    class SortableWrapper: public PrioritySortUtil::Sortable {
    public:
        SortableWrapper(const Thing& thing) : _thing(thing) { }
        glm::vec3 getPosition() const override { return _thing->getPosition(); }
        float getRadius() const override { return 0.5f * _thing->getBoundingRadius(); }
        uint64_t getTimestamp() const override { return _thing->getLastTime(); }
        const Thing& getThing() const { return _thing; }
    private:
        Thing _thing;
    };

(2) Make a PrioritySortUtil::PriorityQueue<Thing> and add them to the queue:

    PrioritySortUtil::Prioritizer prioritizer(viewFrustum);
    std::priority_queue< PrioritySortUtil::Sortable<Thing> > sortedThings;
    for (thing in things) {
        float priority = prioritizer.computePriority(PrioritySortUtil::PrioritizableThing(thing));
        sortedThings.push(PrioritySortUtil::Sortable<Thing> entry(thing, priority));
    }

(3) Loop over your priority queue and do timeboxed work:

    uint64_t cutoffTime = usecTimestampNow() + TIME_BUDGET;
    while (!sortedThings.empty()) {
        const Thing& thing = sortedThings.top();
        // ...do work on thing...
        sortedThings.pop();
        if (usecTimestampNow() > cutoffTime) {
            break;
        }
    }
*/

namespace PrioritySortUtil {

    constexpr float DEFAULT_ANGULAR_COEF { 1.0f };
    constexpr float DEFAULT_CENTER_COEF { 0.5f };
    constexpr float DEFAULT_AGE_COEF { 0.25f / (float)(USECS_PER_SECOND) };

    class Sortable {
    public:
        virtual glm::vec3 getPosition() const = 0;
        virtual float getRadius() const = 0;
        virtual uint64_t getTimestamp() const = 0;

        void setPriority(float priority) { _priority = priority; }
        bool operator<(const Sortable& other) const { return _priority < other._priority; }
    private:
        float _priority { 0.0f };
    };

    template <typename T>
    class PriorityQueue {
    public:
        PriorityQueue() = delete;

        PriorityQueue(const ViewFrustum& view) : _view(view) { }

        PriorityQueue(const ViewFrustum& view, float angularWeight, float centerWeight, float ageWeight)
                : _view(view), _angularWeight(angularWeight), _centerWeight(centerWeight), _ageWeight(ageWeight)
        { }

        void setView(const ViewFrustum& view) { _view = view; }

        void setWeights(float angularWeight, float centerWeight, float ageWeight) {
            _angularWeight = angularWeight;
            _centerWeight = centerWeight;
            _ageWeight = ageWeight;
        }

        size_t size() const { return _queue.size(); }
        void push(T thing) {
            thing.setPriority(computePriority(thing));
            _queue.push(thing);
        }
        const T& top() const { return _queue.top(); }
        void pop() { return _queue.pop(); }
        bool empty() const { return _queue.empty(); }

    private:
        float computePriority(const T& thing) const {
            // priority = weighted linear combination of multiple values:
            //   (a) angular size
            //   (b) proximity to center of view
            //   (c) time since last update
            // where the relative "weights" are tuned to scale the contributing values into units of "priority".

            glm::vec3 position = thing.getPosition();
            glm::vec3 offset = position - _view.getPosition();
            float distance = glm::length(offset) + 0.001f; // add 1mm to avoid divide by zero
            float radius = thing.getRadius();

            float priority = _angularWeight * (radius / distance)
                + _centerWeight * (glm::dot(offset, _view.getDirection()) / distance)
                + _ageWeight * (float)(usecTimestampNow() - thing.getTimestamp());

            // decrement priority of things outside keyhole
            if (distance - radius > _view.getCenterRadius()) {
                if (!_view.sphereIntersectsFrustum(position, radius)) {
                    constexpr float OUT_OF_VIEW_PENALTY = -10.0f;
                    priority += OUT_OF_VIEW_PENALTY;
                }
            }
            return priority;
        }

        ViewFrustum _view;
        std::priority_queue<T> _queue;
        float _angularWeight { DEFAULT_ANGULAR_COEF };
        float _centerWeight { DEFAULT_CENTER_COEF };
        float _ageWeight { DEFAULT_AGE_COEF };
    };
} // namespace PrioritySortUtil

#endif // hifi_PrioritySortUtil_h

