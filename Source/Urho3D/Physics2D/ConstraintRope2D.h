// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Physics2D/Constraint2D.h"

namespace Urho3D
{

/// 2D rope constraint component.
class URHO3D_API ConstraintRope2D : public Constraint2D
{
    URHO3D_OBJECT(ConstraintRope2D, Constraint2D);

public:
    /// Construct.
    explicit ConstraintRope2D(Context* context);
    /// Destruct.
    ~ConstraintRope2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set owner body anchor.
    /// @property
    void SetOwnerBodyAnchor(const Vector2& anchor);
    /// Set other body anchor.
    /// @property
    void SetOtherBodyAnchor(const Vector2& anchor);
    /// Set max length.
    /// @property
    void SetMaxLength(float maxLength);

    /// Return owner body anchor.
    /// @property
    const Vector2& GetOwnerBodyAnchor() const { return ownerBodyAnchor_; }

    /// Return other body anchor.
    /// @property
    const Vector2& GetOtherBodyAnchor() const { return otherBodyAnchor_; }

    /// Return max length.
    /// @property
    float GetMaxLength() const { return jointDef_.maxLength; }

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2RopeJointDef jointDef_;
    /// Owner body anchor.
    Vector2 ownerBodyAnchor_;
    /// Other body anchor.
    Vector2 otherBodyAnchor_;
};

}
