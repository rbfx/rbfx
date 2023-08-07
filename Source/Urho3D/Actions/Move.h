//
// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "AttributeAction.h"

namespace Urho3D
{
namespace Actions
{
/// Move by 3D or 2D offset action. Target should have attribute "Position" of type Vector3, Vector2, IntVector2 or IntVector3.
class URHO3D_API MoveBy : public AttributeAction
{
    URHO3D_OBJECT(MoveBy, AttributeAction)
public:
    /// Construct.
    explicit MoveBy(Context* context);

    /// Set position delta.
    void SetPositionDelta(const Vector3& delta);

    /// Get position delta.
    const Vector3& GetPositionDelta() const { return delta_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Vector3 delta_{Vector3::ZERO};
};

/// Move by 3D or 2D offset action. Target should have attribute "Position" of type Vector3, Vector2, IntVector2 or
/// IntVector3.
class URHO3D_API MoveByQuadratic : public MoveBy
{
    URHO3D_OBJECT(MoveByQuadratic, MoveBy)
public:
    /// Construct.
    explicit MoveByQuadratic(Context* context);

    /// Set control point delta.
    void SetControlDelta(const Vector3& delta);

    /// Get control point delta.
    const Vector3& GetControlDelta() const { return control_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Vector3 control_{Vector3::ZERO};
};


/// Move instantly by 3D offset action. Target should have attribute "Position" of type Vector3 or IntVector3.
class URHO3D_API JumpBy : public AttributeAction
{
    URHO3D_OBJECT(JumpBy, AttributeAction)
public:
    /// Construct.
    explicit JumpBy(Context* context);

    /// Set position delta.
    void SetPositionDelta(const Vector3& delta);

    /// Get position delta.
    const Vector3& GetPositionDelta() const { return delta_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Vector3 delta_{Vector3::ZERO};
};

/// Scale by 3D offset action. Target should have attribute "Scale" of type Vector3.
class URHO3D_API ScaleBy : public AttributeAction
{
    URHO3D_OBJECT(MoveBy, AttributeAction)
public:
    /// Construct.
    explicit ScaleBy(Context* context);

    /// Set scale delta.
    void SetScaleDelta(const Vector3& delta);

    /// Get scale delta.
    const Vector3& GetScaleDelta() const { return delta_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Vector3 delta_{Vector3::ONE};
};

/// Rotate by 3D delta action. Target should have attribute "Rotation" of type Quaternion.
class URHO3D_API RotateBy : public AttributeAction
{
    URHO3D_OBJECT(RotateBy, AttributeAction)
public:
    /// Construct.
    explicit RotateBy(Context* context);

    /// Set rotation delta.
    void SetRotationDelta(const Quaternion& delta);

    /// Get rotation delta.
    const Quaternion& GetRotationDelta() const { return delta_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Quaternion delta_{Quaternion::IDENTITY};
};


/// Rotate around 3D point action. Target should have "Position" of type Vector3 and "Rotation" of type Quaternion attributes.
class URHO3D_API RotateAround : public AttributeAction
{
    URHO3D_OBJECT(RotateAround, AttributeAction)
public:
    /// Construct.
    explicit RotateAround(Context* context);

    /// Set rotation delta.
    void SetRotationDelta(const Quaternion& delta);

    /// Get rotation delta.
    const Quaternion& GetRotationDelta() const { return delta_; }

    /// Set rotation pivot.
    void SetPivot(const Vector3& pivot);

    /// Get rotation pivot.
    const Vector3& GetPivot() const { return pivot_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Quaternion delta_{Quaternion::IDENTITY};
    Vector3 pivot_{Vector3::ZERO};
};
} // namespace Actions
} // namespace Urho3D
