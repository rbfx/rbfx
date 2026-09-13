// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Input/MoveAndOrbitComponent.h>
#include <Urho3D/Input/InputMap.h>

using namespace Urho3D;

const unsigned CTRL_FORWARD = 1;
const unsigned CTRL_BACK = 2;
const unsigned CTRL_LEFT = 4;
const unsigned CTRL_RIGHT = 8;
const unsigned CTRL_JUMP = 16;

const float MOVE_FORCE = 0.8f;
const float INAIR_MOVE_FORCE = 0.02f;
const float BRAKE_FORCE = 0.2f;
const float JUMP_FORCE = 7.0f;
const float INAIR_THRESHOLD_TIME = 0.1f;

/// Character component, responsible for physical movement according to controls, as well as animation.
class Character : public MoveAndOrbitComponent
{
    URHO3D_OBJECT(Character, MoveAndOrbitComponent);

public:
    /// Construct.
    explicit Character(Context* context);

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Handle startup. Called by LogicComponent base class.
    void Start() override;
    /// Handle physics world update. Called by LogicComponent base class.
    void FixedUpdate(float timeStep) override;

    /// Set input map.
    void SetInputMap(InputMap* inputMap);
    /// Return input map.
    InputMap* GetInputMap() const { return inputMap_; }
    /// Set input map attribute.
    void SetInputMapAttr(const ResourceRef& value);
    /// Return input map attribute.
    ResourceRef GetInputMapAttr() const;

private:
    /// Handle physics collision event.
    void HandleNodeCollision(StringHash eventType, VariantMap& eventData);

    /// Grounded flag for movement.
    bool onGround_;
    /// Jump flag.
    bool okToJump_;
    /// In air timer. Due to possible physics inaccuracy, character can be off ground for max. 1/10 second and still be allowed to move.
    float inAirTimer_;

    /// Input map.
    SharedPtr<InputMap> inputMap_;
};
