// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Physics2D/Constraint2D.h"

namespace Urho3D
{

/// 2D mouse constraint component.
class URHO3D_API ConstraintMouse2D : public Constraint2D
{
    URHO3D_OBJECT(ConstraintMouse2D, Constraint2D);

public:
    /// Construct.
    explicit ConstraintMouse2D(Context* context);
    /// Destruct.
    ~ConstraintMouse2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set target.
    /// @property
    void SetTarget(const Vector2& target);
    /// Set max force.
    /// @property
    void SetMaxForce(float maxForce);
    /// Set frequency Hz.
    /// @property
    void SetFrequencyHz(float frequencyHz);
    /// Set damping ratio.
    /// @property
    void SetDampingRatio(float dampingRatio);

    /// Return target.
    /// @property
    const Vector2& GetTarget() const { return target_; }

    /// Return max force.
    /// @property
    float GetMaxForce() const { return jointDef_.maxForce; }

    /// Return frequency Hz.
    /// @property
    float GetFrequencyHz() const { return jointDef_.frequencyHz; }

    /// Return damping ratio.
    /// @property
    float GetDampingRatio() const { return jointDef_.dampingRatio; }

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2MouseJointDef jointDef_;
    /// Target.
    Vector2 target_;
};

}
