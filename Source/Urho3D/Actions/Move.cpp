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
class MoveByVec3State : public AttributeActionState
{
    Vector3 positionDelta_;
    Vector3 startPosition_;
    Vector3 previousPosition_;

public:
    MoveByVec3State(MoveBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
    {
        positionDelta_ = action->GetPositionDelta();
        previousPosition_ = startPosition_ = Get<Vector3>();
    }

    void Update(float time, Variant& value) override
    {
        const auto currentPos = value.GetVector3();
        auto diff = currentPos - previousPosition_;
        startPosition_ = startPosition_ + diff;
        const auto newPos = startPosition_ + positionDelta_ * time;
        value = newPos;
        previousPosition_ = newPos;
    }
};

class MoveByIntVec3State : public AttributeActionState
{
    Vector3 positionDelta_;
    IntVector3 startPosition_;
    IntVector3 previousPosition_;

public:
    MoveByIntVec3State(MoveBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
    {
        positionDelta_ = action->GetPositionDelta();
        previousPosition_ = startPosition_ = Get<IntVector3>();
    }

    void Update(float time, Variant& value) override
    {
        const auto currentPos = value.GetIntVector3();
        const auto diff = currentPos - previousPosition_;
        startPosition_ = startPosition_ + diff;
        const auto newPos = startPosition_ + (positionDelta_ * time).ToIntVector3();
        value = newPos;
        previousPosition_ = newPos;
    }
};

class MoveByVec2State : public AttributeActionState
{
    Vector2 positionDelta_;
    Vector2 startPosition_;
    Vector2 previousPosition_;

public:
    MoveByVec2State(MoveBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
    {
        positionDelta_ = action->GetPositionDelta().ToVector2();
        previousPosition_ = startPosition_ = Get<Vector2>();
    }

    void Update(float time, Variant& value) override
    {
        const auto currentPos = value.GetVector2();
        const auto diff = currentPos - previousPosition_;
        startPosition_ = startPosition_ + diff;
        const auto newPos = startPosition_ + positionDelta_ * time;
        value = newPos;
        previousPosition_ = newPos;
    }
};

class MoveByIntVec2State : public AttributeActionState
{
    Vector2 positionDelta_;
    IntVector2 startPosition_;
    IntVector2 previousPosition_;

public:
    MoveByIntVec2State(MoveBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
    {
        positionDelta_ = action->GetPositionDelta().ToVector2();
        previousPosition_ = startPosition_ = Get<IntVector2>();
    }

    void Update(float time, Variant& value) override
    {
        const auto currentPos = value.GetIntVector2();
        const auto diff = currentPos - previousPosition_;
        startPosition_ = startPosition_ + diff;
        const auto newPos = startPosition_ + (positionDelta_ * time).ToIntVector2();
        value = newPos;
        previousPosition_ = newPos;
    }
};

class JumpByVec3State : public AttributeActionState
{
    Vector3 positionDelta_;
    bool triggered_;

public:
    JumpByVec3State(JumpBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
        , positionDelta_(action->GetPositionDelta())
        , triggered_(false)
    {
    }

    void Update(float time, Variant& value) override
    {
        if (triggered_)
            return;
        triggered_ = true;
        value = value.GetVector3() + positionDelta_;
    }
};

class JumpByIntVec3State : public AttributeActionState
{
    Vector3 positionDelta_;
    bool triggered_;

public:
    JumpByIntVec3State(JumpBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
        , positionDelta_(action->GetPositionDelta())
        , triggered_(false)
    {
    }

    void Update(float time, Variant& value) override
    {
        if (triggered_)
            return;
        triggered_ = true;
        const auto res = value.GetIntVector3().ToVector3() + positionDelta_;
        value = res.ToIntVector3();
    }
};

class JumpByVec2State : public AttributeActionState
{
    Vector3 positionDelta_;
    bool triggered_;

public:
    JumpByVec2State(JumpBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
        , positionDelta_(action->GetPositionDelta())
        , triggered_(false)
    {
    }

    void Update(float time, Variant& value) override
    {
        if (triggered_)
            return;
        triggered_ = true;
        value = value.GetVector2() + positionDelta_.ToVector2();
    }
};

class JumpByIntVec2State : public AttributeActionState
{
    Vector3 positionDelta_;
    bool triggered_;

public:
    JumpByIntVec2State(JumpBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
        , positionDelta_(action->GetPositionDelta())
        , triggered_(false)
    {
    }

    void Update(float time, Variant& value) override
    {
        if (triggered_)
            return;
        triggered_ = true;
        value = value.GetIntVector2() + positionDelta_.ToIntVector2();
    }
};

class ScaleByState : public AttributeActionState
{
    Vector3 scaleDelta_;
    Vector3 startScale_;
    Vector3 previousScale_;

public:
    ScaleByState(ScaleBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
    {
        scaleDelta_ = action->GetScaleDelta();
        previousScale_ = startScale_ = Get<Vector3>();
    }

    void Update(float time, Variant& value) override
    {
        const auto currentScale = value.GetVector3();
        const auto diff = currentScale/previousScale_;
        startScale_ = startScale_ * diff;
        const auto newScale = startScale_ * Vector3::ONE.Lerp(scaleDelta_, time);
        value = newScale;
        previousScale_ = newScale;
    }
};


class ScaleByVec2State : public AttributeActionState
{
    Vector2 scaleDelta_;
    Vector2 startScale_;
    Vector2 previousScale_;

public:
    ScaleByVec2State(ScaleBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
    {
        scaleDelta_ = action->GetScaleDelta().ToVector2();
        previousScale_ = startScale_ = Get<Vector2>();
    }

    void Update(float time, Variant& value) override
    {
        const auto currentScale = value.GetVector2();
        const auto diff = currentScale / previousScale_;
        startScale_ = startScale_ * diff;
        const auto newScale = startScale_ * Vector2::ONE.Lerp(scaleDelta_, time);
        value = newScale;
        previousScale_ = newScale;
    }
};

} // namespace

/// ------------------------------------------------------------------------------
/// Construct.
MoveBy::MoveBy(Context* context)
    : BaseClassName(context, POSITION_ATTRIBUTE)
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
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector3::ZERO);
}

/// Create new action state from the action.
SharedPtr<ActionState> MoveBy::StartAction(Object* target)
{
    if (auto attribute = GetAttribute(target))
    {
        switch (attribute->type_)
        {
        case VAR_VECTOR2: return MakeShared<MoveByVec2State>(this, target, attribute);
        case VAR_VECTOR3: return MakeShared<MoveByVec3State>(this, target, attribute);
        case VAR_INTVECTOR2: return MakeShared<MoveByIntVec2State>(this, target, attribute);
        case VAR_INTVECTOR3: return MakeShared<MoveByIntVec3State>(this, target, attribute);
        default: URHO3D_LOGERROR(Format("Attribute {} is not of valid type.", GetAttributeName())); break;
        }
    }
    return BaseClassName::StartAction(target);
}

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
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector3::ZERO);
}

/// Create new action state from the action.
SharedPtr<ActionState> JumpBy::StartAction(Object* target)
{
    if (auto attribute = GetAttribute(target))
    {
        switch (attribute->type_)
        {
        case VAR_VECTOR2: return MakeShared<JumpByVec2State>(this, target, attribute);
        case VAR_VECTOR3: return MakeShared<JumpByVec3State>(this, target, attribute);
        case VAR_INTVECTOR2: return MakeShared<JumpByIntVec2State>(this, target, attribute);
        case VAR_INTVECTOR3: return MakeShared<JumpByIntVec3State>(this, target, attribute);
        default: URHO3D_LOGERROR(Format("Attribute {} is not of valid type.", GetAttributeName())); break;
        }
    }
    return BaseClassName::StartAction(target);
}

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
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector3::ONE);
}

/// Create new action state from the action.
SharedPtr<ActionState> ScaleBy::StartAction(Object* target)
{
    if (auto attribute = GetAttribute(target))
    {
        switch (attribute->type_)
        {
        case VAR_VECTOR3: return MakeShared<ScaleByState>(this, target, attribute);
        case VAR_VECTOR2: return MakeShared<ScaleByVec2State>(this, target, attribute);
        default: URHO3D_LOGERROR(Format("Attribute {} is not of valid type.", GetAttributeName())); break;
        }
    }
    return BaseClassName::StartAction(target);
}


/// ------------------------------------------------------------------------------

namespace
{
class RotateByState : public AttributeActionState
{
    Quaternion rotationDelta_;
    Quaternion startRotation_;
    Quaternion previousRotation_;

public:
    RotateByState(RotateBy* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
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

/// Construct.
RotateBy::RotateBy(Context* context)
    : BaseClassName(context)
{
}

/// Set rotation delta.
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
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Quaternion::IDENTITY);
}

/// Create new action state from the action.
SharedPtr<ActionState> RotateBy::StartAction(Object* target)
{
    if (auto attribute = GetAttribute(target))
    {
        switch (attribute->type_)
        {
        case VAR_QUATERNION: return MakeShared<RotateByState>(this, target, attribute);
        default: URHO3D_LOGERROR(Format("Attribute {} is not of valid type.", GetAttributeName())); break;
        }
    }
    return BaseClassName::StartAction(target);
}

/// ------------------------------------------------------------------------------

namespace
{
class RotateAroundState : public FiniteTimeActionState
{
    Quaternion rotationDelta_;
    Quaternion startRotation_;
    Quaternion previousRotation_;
    Vector3 pivot_;
    AttributeInfo* rotationAttribute_{};
    AttributeInfo* positionAttribute_{};

public:
    RotateAroundState(RotateAround* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        auto serializable = target->Cast<Serializable>();
        if (!serializable)
        {
            URHO3D_LOGERROR(
                Format("Can animate only serializable class but {} is not serializable.", target->GetTypeName()));
            return;
        }
        {
            const static char* RotationAttributeName = "Rotation";
            rotationAttribute_ =
                target->GetContext()->GetReflection(target->GetType())->GetAttribute(RotationAttributeName);
            if (!rotationAttribute_)
            {
                URHO3D_LOGERROR(Format("Attribute {} not found in {}.", RotationAttributeName, target->GetTypeName()));
                return;
            }
            if (rotationAttribute_->type_ != VAR_QUATERNION)
            {
                URHO3D_LOGERROR(Format(
                    "Attribute {} is not of type {}.", RotationAttributeName, Variant::GetTypeName(VAR_QUATERNION)));
                rotationAttribute_ = nullptr;
                return;
            }
        }
        {
            const static char* PositionAttributeName = "Position";
            positionAttribute_ =
                target->GetContext()->GetReflection(target->GetType())->GetAttribute(PositionAttributeName);
            if (!positionAttribute_)
            {
                URHO3D_LOGERROR(Format("Attribute {} not found in {}.", PositionAttributeName, target->GetTypeName()));
                return;
            }
            if (positionAttribute_->type_ != VAR_VECTOR3)
            {
                URHO3D_LOGERROR(Format(
                    "Attribute {} is not of type {}.", PositionAttributeName, Variant::GetTypeName(VAR_VECTOR3)));
                positionAttribute_ = nullptr;
                return;
            }
        }
        rotationDelta_ = action->GetRotationDelta();
        pivot_ = action->GetPivot();
        Variant rotationVariant;
        rotationAttribute_->accessor_->Get(serializable, rotationVariant);
        previousRotation_ = startRotation_ = rotationVariant.GetQuaternion();
    }

    void Update(float time) override
    {
        if (!positionAttribute_ || !rotationAttribute_)
            return;
        Variant positionVariant;
        Variant rotationVariant;
        positionAttribute_->accessor_->Get(static_cast<const Serializable*>(GetTarget()), positionVariant);
        rotationAttribute_->accessor_->Get(static_cast<const Serializable*>(GetTarget()), rotationVariant);

        const auto currentRotation = rotationVariant.GetQuaternion();
        const auto currentPosition = positionVariant.GetVector3();
        Matrix3x4 currentTR(currentPosition, rotationVariant.GetQuaternion(), 1.0f);
        Matrix3x4 currentITR{currentTR.Inverse()};
        const auto localPivot = currentITR * pivot_;

        auto diff = previousRotation_.Inverse() * currentRotation;
        startRotation_ = startRotation_ * diff;
        const auto newRotation = Quaternion::IDENTITY.Slerp(rotationDelta_, time) * startRotation_;
        rotationVariant = newRotation;
        previousRotation_ = newRotation;

        Matrix3x4 newTR(currentPosition, newRotation, 1.0f);
        auto newPivot = newTR * localPivot;
        positionVariant = currentPosition + (pivot_ - newPivot);

        positionAttribute_->accessor_->Set(static_cast<Serializable*>(GetTarget()), positionVariant);
        rotationAttribute_->accessor_->Set(static_cast<Serializable*>(GetTarget()), rotationVariant);
    }
};
} // namespace

/// Construct.
RotateAround::RotateAround(Context* context)
    : BaseClassName(context)
{
}

/// Set rotation delta.
void RotateAround::SetRotationDelta(const Quaternion& delta) { delta_ = delta; }

/// Set pivot.
void RotateAround::SetPivot(const Vector3& pivot) { pivot_ = pivot; }

SharedPtr<FiniteTimeAction> RotateAround::Reverse() const
{
    auto result = MakeShared<RotateAround>(context_);
    result->SetDuration(GetDuration());
    result->SetRotationDelta(delta_.Inverse());
    result->SetPivot(GetPivot());
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void RotateAround::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Quaternion::IDENTITY);
    SerializeOptionalValue(archive, "pivot", pivot_, Vector3::ZERO);
}

/// Create new action state from the action.
SharedPtr<ActionState> RotateAround::StartAction(Object* target) { return MakeShared<RotateAroundState>(this, target); }

} // namespace Actions
} // namespace Urho3D

