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

#include "Move.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "AttributeActionState.h"
#include "FiniteTimeActionState.h"
#include "Urho3D/IO/ArchiveSerializationBasic.h"

namespace Urho3D
{
namespace Actions
{

namespace
{
class MoveByState : public AttributeActionState
{
    Vector3 positionDelta_;
    Vector3 startPosition_;
    Vector3 previousPosition_;

public:
    MoveByState(MoveBy* action, Object* target)
        : AttributeActionState(action, target, "Position")
    {
        positionDelta_ = action->GetPositionDelta();
        if (attribute_)
        {
            if (attribute_->type_ == VAR_VECTOR3)
                previousPosition_ = startPosition_ = Get<Vector3>();
            else if (attribute_->type_ == VAR_INTVECTOR3)
                previousPosition_ = startPosition_ = Vector3(Get<IntVector3>());
            else
            {
                URHO3D_LOGERROR(
                    Format("Attribute {} is not of type VAR_VECTOR3 or VAR_INTVECTOR3.", attribute_->name_));
                attribute_ = nullptr;
                return;
            }
        }
    }

    void Update(float time, Variant& value) override
    {
        if (attribute_->type_ == VAR_VECTOR3)
        {
            const auto currentPos = value.GetVector3();
            auto diff = currentPos - previousPosition_;
            startPosition_ = startPosition_ + diff;
            const auto newPos = startPosition_ + positionDelta_ * time;
            value = newPos;
            previousPosition_ = newPos;
        }
        else
        {
            const auto currentPos = Vector3(value.GetIntVector3());
            auto diff = currentPos - previousPosition_;
            startPosition_ = startPosition_ + diff;
            const auto newPos = startPosition_ + positionDelta_ * time;
            const IntVector3 newIntPos = IntVector3(newPos.x_, newPos.y_, newPos.z_);
            value = newIntPos;
            previousPosition_ = Vector3(newIntPos);
        }
    }
};

class MoveBy2DState : public AttributeActionState
{
    Vector2 positionDelta_;
    Vector2 startPosition_;
    Vector2 previousPosition_;

public:
    MoveBy2DState(MoveBy2D* action, Object* target)
        : AttributeActionState(action, target, "Position")
    {
        positionDelta_ = action->GetPositionDelta();
        if (attribute_)
        {
            if (attribute_->type_ == VAR_VECTOR2)
                previousPosition_ = startPosition_ = Get<Vector2>();
            else if (attribute_->type_ == VAR_INTVECTOR2)
                previousPosition_ = startPosition_ = Vector2(Get<IntVector2>());
            else
            {
                URHO3D_LOGERROR(
                    Format("Attribute {} is not of type VAR_VECTOR2 or VAR_INTVECTOR2.", attribute_->name_));
                attribute_ = nullptr;
                return;
            }
        }
    }

    void Update(float time, Variant& value) override
    {
        if (attribute_->type_ == VAR_VECTOR2)
        {
            const auto currentPos = value.GetVector2();
            auto diff = currentPos - previousPosition_;
            startPosition_ = startPosition_ + diff;
            const auto newPos = startPosition_ + positionDelta_ * time;
            value = newPos;
            previousPosition_ = newPos;
        }
        else
        {
            const auto currentPos = Vector2(value.GetIntVector2());
            auto diff = currentPos - previousPosition_;
            startPosition_ = startPosition_ + diff;
            const auto newPos = startPosition_ + positionDelta_ * time;
            const IntVector2 newIntPos = IntVector2(newPos.x_, newPos.y_);
            value = newIntPos;
            previousPosition_ = Vector2(newIntPos);
        }
    }
};

class JumpByState : public AttributeActionState
{
    Vector3 positionDelta_;
    bool triggered_;

public:
    JumpByState(JumpBy* action, Object* target)
        : AttributeActionState(action, target, "Position")
        , positionDelta_(action->GetPositionDelta())
        , triggered_(false)
    {
    }

    void Update(float time, Variant& value) override
    {
        if (triggered_)
            return;
        triggered_ = true; 
        if (attribute_->type_ == VAR_VECTOR3)
        {
            value = value.GetVector3() + positionDelta_;
        }
        else if (attribute_->type_ == VAR_INTVECTOR3)
        {
            auto res = Vector3(value.GetIntVector3()) + positionDelta_;
            value = IntVector3(res.x_, res.y_, res.z_);
        }
        else
        {
            URHO3D_LOGERROR("Can't JumpBy: Position attribute is neither Vector3 nor IntVector3");
        }
    }
};

class JumpBy2DState : public AttributeActionState
{
    Vector2 positionDelta_;
    bool triggered_;

public:
    JumpBy2DState(JumpBy2D* action, Object* target)
        : AttributeActionState(action, target, "Position")
        , positionDelta_(action->GetPositionDelta())
        , triggered_(false)
    {
    }

    void Update(float time, Variant& value) override
    {
        if (triggered_)
            return;
        triggered_ = true;
        if (attribute_->type_ == VAR_VECTOR2)
        {
            value = value.GetVector2() + positionDelta_;
        }
        else if (attribute_->type_ == VAR_INTVECTOR2)
        {
            auto res = Vector2(value.GetIntVector2()) + positionDelta_;
            value = IntVector2(res.x_, res.y_);
        }
        else
        {
            URHO3D_LOGERROR("Can't JumpBy2D: Position attribute is neither Vector2 nor IntVector2");
        }
    }
};

class ScaleByState : public AttributeActionState
{
    Vector3 scaleDelta_;
    Vector3 startScale_;
    Vector3 previousScale_;

public:
    ScaleByState(ScaleBy* action, Object* target)
        : AttributeActionState(action, target, "Scale", VAR_VECTOR3)
    {
        scaleDelta_ = action->GetScaleDelta();
        previousScale_ = startScale_ = Get<Vector3>();
    }

    void Update(float time, Variant& value) override
    {
        const auto currentScale = value.GetVector3();
        auto diff = currentScale/previousScale_;
        startScale_ = startScale_ * diff;
        const auto newScale = startScale_ * Vector3::ONE.Lerp(scaleDelta_, time);
        value = newScale;
        previousScale_ = newScale;
    }
};


class RotateByState : public AttributeActionState
{
    Quaternion rotationDelta_;
    Quaternion startRotation_;
    Quaternion previousRotation_;

public:
    RotateByState(RotateBy* action, Object* target)
        : AttributeActionState(action, target, "Rotation", VAR_QUATERNION)
    {
        rotationDelta_ = action->GetRotationDelta();
        previousRotation_ = startRotation_ = Get<Quaternion>();
    }

    void Update(float time, Variant& value) override
    {
        const auto currentRotation = value.GetQuaternion();
        auto diff = previousRotation_.Inverse() * currentRotation;
        startRotation_ = startRotation_ * diff;
        const auto newRotation = startRotation_ * Quaternion::IDENTITY.Slerp(rotationDelta_, time);
        value = newRotation;
        previousRotation_ = newRotation;
    }
};

} // namespace

/// ------------------------------------------------------------------------------
/// Construct.
MoveBy::MoveBy(Context* context)
    : BaseClassName(context)
{
}

/// Set position delta.
void MoveBy::SetPositionDelta(const Vector3& pos) { delta_ = pos; }

SharedPtr<FiniteTimeAction> MoveBy::Reverse() const
{
    auto result = MakeShared<MoveBy>(context_);
    result->SetDuration(GetDuration());
    result->SetPositionDelta(-delta_);
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void MoveBy::SerializeInBlock(Archive& archive)
{
    FiniteTimeAction::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector3::ZERO);
}

/// Create new action state from the action.
SharedPtr<ActionState> MoveBy::StartAction(Object* target) { return MakeShared<MoveByState>(this, target); }

/// ------------------------------------------------------------------------------
/// Construct.
MoveBy2D::MoveBy2D(Context* context)
    : BaseClassName(context)
{
}

/// Set position delta.
void MoveBy2D::SetPositionDelta(const Vector2& pos) { delta_ = pos; }

SharedPtr<FiniteTimeAction> MoveBy2D::Reverse() const
{
    auto result = MakeShared<MoveBy2D>(context_);
    result->SetDuration(GetDuration());
    result->SetPositionDelta(-delta_);
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void MoveBy2D::SerializeInBlock(Archive& archive)
{
    FiniteTimeAction::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector2::ZERO);
}

/// Create new action state from the action.
SharedPtr<ActionState> MoveBy2D::StartAction(Object* target) { return MakeShared<MoveBy2DState>(this, target); }


/// ------------------------------------------------------------------------------
/// Construct.
JumpBy::JumpBy(Context* context)
    : BaseClassName(context)
{
}

/// Set position delta.
void JumpBy::SetPositionDelta(const Vector3& pos) { delta_ = pos; }

SharedPtr<FiniteTimeAction> JumpBy::Reverse() const
{
    auto result = MakeShared<JumpBy>(context_);
    result->SetDuration(GetDuration());
    result->SetPositionDelta(-delta_);
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void JumpBy::SerializeInBlock(Archive& archive)
{
    FiniteTimeAction::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector3::ZERO);
}

/// Create new action state from the action.
SharedPtr<ActionState> JumpBy::StartAction(Object* target) { return MakeShared<JumpByState>(this, target); }

/// ------------------------------------------------------------------------------
/// Construct.
JumpBy2D::JumpBy2D(Context* context)
    : BaseClassName(context)
{
}

/// Set position delta.
void JumpBy2D::SetPositionDelta(const Vector2& pos) { delta_ = pos; }

SharedPtr<FiniteTimeAction> JumpBy2D::Reverse() const
{
    auto result = MakeShared<JumpBy2D>(context_);
    result->SetDuration(GetDuration());
    result->SetPositionDelta(-delta_);
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void JumpBy2D::SerializeInBlock(Archive& archive)
{
    FiniteTimeAction::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector2::ZERO);
}

/// Create new action state from the action.
SharedPtr<ActionState> JumpBy2D::StartAction(Object* target) { return MakeShared<JumpBy2DState>(this, target); }

/// ------------------------------------------------------------------------------
/// Construct.
ScaleBy::ScaleBy(Context* context)
    : BaseClassName(context)
{
}

/// Set scale delta.
void ScaleBy::SetScaleDelta(const Vector3& delta) { delta_ = delta; }

SharedPtr<FiniteTimeAction> ScaleBy::Reverse() const
{
    auto result = MakeShared<ScaleBy>(context_);
    result->SetDuration(GetDuration());
    result->SetScaleDelta(Vector3(1.0f / delta_.x_, 1.0f / delta_.y_, 1.0f / delta_.z_));
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void ScaleBy::SerializeInBlock(Archive& archive)
{
    FiniteTimeAction::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector3::ONE);
}

/// Create new action state from the action.
SharedPtr<ActionState> ScaleBy::StartAction(Object* target) { return MakeShared<ScaleByState>(this, target); }


/// ------------------------------------------------------------------------------
/// Construct.
RotateBy::RotateBy(Context* context)
    : BaseClassName(context)
{
}

/// Set scale delta.
void RotateBy::SetRotationDelta(const Quaternion& delta) { delta_ = delta; }

SharedPtr<FiniteTimeAction> RotateBy::Reverse() const
{
    auto result = MakeShared<RotateBy>(context_);
    result->SetDuration(GetDuration());
    result->SetRotationDelta(delta_.Inverse());
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void RotateBy::SerializeInBlock(Archive& archive)
{
    FiniteTimeAction::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Quaternion::IDENTITY);
}

/// Create new action state from the action.
SharedPtr<ActionState> RotateBy::StartAction(Object* target) { return MakeShared<RotateByState>(this, target); }


} // namespace Actions
} // namespace Urho3D

