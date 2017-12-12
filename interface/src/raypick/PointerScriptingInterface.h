//
//  Created by Sam Gondelman 10/20/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PointerScriptingInterface_h
#define hifi_PointerScriptingInterface_h

#include <QtCore/QObject>

#include "DependencyManager.h"
#include <PointerManager.h>
#include <Pick.h>

/**jsdoc
 * The Pointers API lets you create and manage objects for repeatedly calculating intersections in different ways, as well as the visual representation of those objects.
 *  Pointers can also be configured to automatically generate PointerEvents.
 *
 * @namespace Pointers
 */

class PointerScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    unsigned int createLaserPointer(const QVariant& properties) const;
    unsigned int createStylus(const QVariant& properties) const;

    /**jsdoc
     * A set of properties that can be passed to {@link Pointers.createPointer} to create a new Pointer.  Also contains the relevant {@link Picks.PickProperties} to define the underlying Pick.
     *
     * Different {@link PickType}s use different properties, and within one PickType, the properties you choose can lead to a wide range of behaviors.  For example,
     *   with PickType.Ray, depending on which optional parameters you pass, you could create a Static Ray Pointer, a Mouse Ray Pointer, or a Joint Ray Pointer.
     *
     * @typedef {Object} Pointers.PointerProperties
     * @property {boolean} [hover=false] If this Pointer should generate hover events.
     * @property {boolean} [faceAvatar=false] Ray Pointers only.  If true, the end of the Pointer will always rotate to face the avatar.
     * @property {boolean} [centerEndY=true] Ray Pointers only.  If false, the end of the Pointer will be moved up by half of its height.
     * @property {boolean} [lockEnd=false] Ray Pointers only.  If true, the end of the Pointer will lock on to the center of the object at which the laser is pointing.
     * @property {boolean} [distanceScaleEnd=false] Ray Pointers only.  If true, the dimensions of the end of the Pointer will scale linearly with distance.
     * @property {boolean} [scaleWithAvatar=false] Ray Pointers only.  If true, the width of the Pointer's path will scale linearly with your avatar's scale.
     * @property {Pointers.RayPointerRenderState[]} [renderStates] Ray Pointers only.  A list of different visual states to switch between.
     * @property {Pointers.DefaultRayPointerRenderState[]} [defaultRenderStates] Ray Pointers only.  A list of different visual states to use if there is no intersection.
     * @property {Pointers.Trigger[]} [triggers] Ray Pointers only.  A list of different triggers mechanisms that control this Pointer's click event generation.
     */

    /**jsdoc
     * A set of properties used to define the visual aspect of a Ray Pointer in the case that the Pointer is intersecting something.
     *
     * @typedef {Object} Pointers.RayPointerRenderState
     * @property {string} name The name of this render state, used by {@link Pointers.setRenderState} and {@link Pointers.editRenderState}
     * @property {OverlayProperties} [start] All of the properties you would normally pass to {@Overlays.addOverlay}, plus the type (as a <code>type</code> field).
     * An overlay to represent the beginning of the Ray Pointer, if desired.
     * @property {OverlayProperties} [path] All of the properties you would normally pass to {@Overlays.addOverlay}, plus the type (as a <code>type</code> field), which <b>must</b> be "line3d".
     * An overlay to represent the path of the Ray Pointer, if desired.
     * @property {OverlayProperties} [end] All of the properties you would normally pass to {@Overlays.addOverlay}, plus the type (as a <code>type</code> field).
     * An overlay to represent the end of the Ray Pointer, if desired.
     */

    /**jsdoc
     * A set of properties used to define the visual aspect of a Ray Pointer in the case that the Pointer is not intersecting something.  Same as a {@link Pointers.RayPointerRenderState},
     * but with an additional distance field.
     *
     * @typedef {Object} Pointers.DefaultRayPointerRenderState
     * @augments Pointers.RayPointerRenderState
     * @property {number} distance The distance at which to render the end of this Ray Pointer, if one is defined.
     */

    /**jsdoc
     * A trigger mechanism for Ray Pointers.
     *
     * @typedef {Object} Pointers.Trigger
     * @property {Controller.Action} action This can be a built-in Controller action, like Controller.Standard.LTClick, or a function that evaluates to >= 1.0 when you want to trigger <code>button</code>.
     * @property {string} button Which button to trigger.  "Primary", "Secondary", "Tertiary", and "Focus" are currently supported.  Only "Primary" will trigger clicks on web surfaces.  If "Focus" is triggered,
     * it will try to set the entity or overlay focus to the object at which the Pointer is aimed.  Buttons besides the first three will still trigger events, but event.button will be "None".
     */

    /**jsdoc
     * Adds a new Pointer
     * @function Pointers.createPointer
     * @param {Picks.PickType} type A PickType that specifies the method of picking to use
     * @param {Pointers.PointerProperties} properties A PointerProperties object, containing all the properties for initializing this Pointer <b>and</b> the {@link Picks.PickProperties} for the Pick that
     *   this Pointer will use to do its picking.
     * @returns {number} The ID of the created Pointer.  Used for managing the Pointer.  0 if invalid.
     *
     * @example <caption>Create a left hand Ray Pointer that triggers events on left controller trigger click and changes color when it's intersecting something.</caption>
     *
     * var end = {
     *     type: "sphere",
     *     dimensions: {x:0.5, y:0.5, z:0.5},
     *     solid: true,
     *     color: {red:0, green:255, blue:0},
     *     ignoreRayIntersection: true
     * };
     * var end2 = {
     *     type: "sphere",
     *     dimensions: {x:0.5, y:0.5, z:0.5},
     *     solid: true,
     *     color: {red:255, green:0, blue:0},
     *     ignoreRayIntersection: true
     * };
     *
     * var renderStates = [ {name: "test", end: end} ];
     * var defaultRenderStates = [ {name: "test", distance: 10.0, end: end2} ];
     * var pointer = Pointers.createPointer(PickType.Ray, {
     *     joint: "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
     *     filter: Picks.PICK_OVERLAYS | Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_NONCOLLIDABLE,
     *     renderStates: renderStates,
     *     defaultRenderStates: defaultRenderStates,
     *     distanceScaleEnd: true,
     *     triggers: [ {action: Controller.Standard.LTClick, button: "Focus"}, {action: Controller.Standard.LTClick, button: "Primary"} ],
     *     hover: true,
     *     enabled: true
     * });
     * Pointers.setRenderState(pointer, "test");
     */
    Q_INVOKABLE unsigned int createPointer(const PickQuery::PickType& type, const QVariant& properties);
    /**jsdoc
     * Enables a Pointer.
     * @function Pointers.enablePointer
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     */
    Q_INVOKABLE void enablePointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->enablePointer(uid); }
    /**jsdoc
     * Disables a Pointer.
     * @function Pointers.disablePointer
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     */
    Q_INVOKABLE void disablePointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->disablePointer(uid); }
    /**jsdoc
     * Removes a Pointer.
     * @function Pointers.removePointer
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     */
    Q_INVOKABLE void removePointer(unsigned int uid) const { DependencyManager::get<PointerManager>()->removePointer(uid); }
    /**jsdoc
     * Edit some visual aspect of a Pointer.  Currently only supported for Ray Pointers.
     * @function Pointers.editRenderState
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {string} renderState The name of the render state you want to edit.
     * @param {RenderState} properties The new properties for <code>renderState</code>.  For Ray Pointers, a {@link Pointers.RayPointerRenderState}.
     */
    Q_INVOKABLE void editRenderState(unsigned int uid, const QString& renderState, const QVariant& properties) const;
    /**jsdoc
     * Set the render state of a Pointer.  For Ray Pointers, this means switching between their {@link Pointers.RayPointerRenderState}s, or "" to turn off rendering and hover/trigger events.
     *  For Stylus Pointers, there are three built-in options: "events on" (render and send events, the default), "events off" (render but don't send events), and "disabled" (don't render, don't send events).
     * @function Pointers.setRenderState
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {string} renderState The name of the render state to which you want to switch.
     */
    Q_INVOKABLE void setRenderState(unsigned int uid, const QString& renderState) const { DependencyManager::get<PointerManager>()->setRenderState(uid, renderState.toStdString()); }

    /**jsdoc
     * Get the most recent pick result from this Pointer.  This will be updated as long as the Pointer is enabled, regardless of the render state.
     * @function Pointers.getPrevPickResult
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @returns {PickResult} The most recent intersection result.  This will be slightly different for different PickTypes.  See {@link Picks.RayPickResult} and {@link Picks.StylusPickResult}.
     */
    Q_INVOKABLE QVariantMap getPrevPickResult(unsigned int uid) const;

    /**jsdoc
     * Sets whether or not to use precision picking.
     * @function Pointers.setPrecisionPicking
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {boolean} precisionPicking Whether or not to use precision picking
     */
    Q_INVOKABLE void setPrecisionPicking(unsigned int uid, bool precisionPicking) const { DependencyManager::get<PointerManager>()->setPrecisionPicking(uid, precisionPicking); }
    /**jsdoc
     * Sets the length of this Pointer.  No effect on Stylus Pointers.
     * @function Pointers.setLength
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {float} length The desired length of the Pointer.
     */
    Q_INVOKABLE void setLength(unsigned int uid, float length) const { DependencyManager::get<PointerManager>()->setLength(uid, length); }
    /**jsdoc
     * Sets a list of Entity IDs, Overlay IDs, and/or Avatar IDs to ignore during intersection.  Not used by Stylus Pointers.
     * @function Pointers.setIgnoreItems
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {Uuid[]} ignoreItems A list of IDs to ignore.
     */
    Q_INVOKABLE void setIgnoreItems(unsigned int uid, const QScriptValue& ignoreEntities) const;
    /**jsdoc
     * Sets a list of Entity IDs, Overlay IDs, and/or Avatar IDs to include during intersection, instead of intersecting with everything.  Stylus
     *   Pointers <b>only</b> intersect with objects in their include list.
     * @function Pointers.setIncludeItems
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {Uuid[]} includeItems A list of IDs to include.
     */
    Q_INVOKABLE void setIncludeItems(unsigned int uid, const QScriptValue& includeEntities) const;

    /**jsdoc
     * Lock a Pointer onto a specific object (overlay, entity, or avatar).  Optionally, provide an offset in object-space, otherwise the Pointer will lock on to the center of the object.
     *   Not used by Stylus Pointers.
     * @function Pointers.setLockEndUUID
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @param {QUuid} objectID The ID of the object to which to lock on.
     * @param {boolean} isOverlay False for entities or avatars, true for overlays
     * @param {Mat4} [offsetMat] The offset matrix to use if you do not want to lock on to the center of the object.
     */
    Q_INVOKABLE void setLockEndUUID(unsigned int uid, const QUuid& objectID, bool isOverlay, const glm::mat4& offsetMat = glm::mat4()) const { DependencyManager::get<PointerManager>()->setLockEndUUID(uid, objectID, isOverlay, offsetMat); }

    /**jsdoc
     * Check if a Pointer is associated with the left hand.
     * @function Pointers.isLeftHand
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @returns {boolean} True if the Pointer is a Joint Ray Pointer with joint == "_CONTROLLER_LEFTHAND" or "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND", or a Stylus Pointer with hand == 0
     */
    Q_INVOKABLE bool isLeftHand(unsigned int uid) { return DependencyManager::get<PointerManager>()->isLeftHand(uid); }
    /**jsdoc
     * Check if a Pointer is associated with the right hand.
     * @function Pointers.isRightHand
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @returns {boolean} True if the Pointer is a Joint Ray Pointer with joint == "_CONTROLLER_RIGHTHAND" or "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND", or a Stylus Pointer with hand == 1
     */
    Q_INVOKABLE bool isRightHand(unsigned int uid) { return DependencyManager::get<PointerManager>()->isRightHand(uid); }
    /**jsdoc
     * Check if a Pointer is associated with the system mouse.
     * @function Pointers.isMouse
     * @param {number} uid The ID of the Pointer, as returned by {@link Pointers.createPointer}.
     * @returns {boolean} True if the Pointer is a Mouse Ray Pointer, false otherwise.
     */
    Q_INVOKABLE bool isMouse(unsigned int uid) { return DependencyManager::get<PointerManager>()->isMouse(uid); }

};

#endif // hifi_PointerScriptingInterface_h
