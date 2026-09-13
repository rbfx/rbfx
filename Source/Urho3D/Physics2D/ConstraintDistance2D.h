// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Physics2D/Constraint2D.h"

namespace Urho3D
{

/// 2D distance constraint component.
class URHO3D_API ConstraintDistance2D : public Constraint2D
{
    URHO3D_OBJECT(ConstraintDistance2D, Constraint2D);

public:
    /// Construct.
    explicit ConstraintDistance2D(Context* context);
    /// Destruct.
    ~ConstraintDistance2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set owner body anchor.
    /// @property
    void SetOwnerBodyAnchor(const Vector2& anchor);
    /// Set other body anchor.
    /// @property
    void SetOtherBodyAnchor(const Vector2& anchor);
    /// Set frequency Hz.
    /// @property
    void SetFrequencyHz(float frequencyHz);
    /// Set damping ratio.
    /// @property
    void SetDampingRatio(float dampingRatio);
    /// Set length.
    /// @property
    void SetLength(float length);

    /// Return owner body anchor.
    /// @property
    const Vector2& GetOwnerBodyAnchor() const { return ownerBodyAnchor_; }

    /// Return other body anchor.
    /// @property
    const Vector2& GetOtherBodyAnchor() const { return otherBodyAnchor_; }

    /// Return frequency Hz.
    /// @property
    float GetFrequencyHz() const { return jointDef_.frequencyHz; }

    /// Return damping ratio.
    /// @property
    float GetDampingRatio() const { return jointDef_.dampingRatio; }

    /// Return length.
    /// @property
    float GetLength() const { return jointDef_.length; }

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    b2DistanceJointDef jointDef_;
    /// Owner body anchor.
    Vector2 ownerBodyAnchor_;
    /// Other body anchor.
    Vector2 otherBodyAnchor_;
};

}
