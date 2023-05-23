//
// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2023 the rbfx project.
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

#include "ActionStates.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/StaticModel.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/EaseMath.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/UI/UIElement.h"

namespace Urho3D
{
namespace Actions
{
namespace Detail
{
void MoveByState::Vec3State::Init(const MoveByState* state)
{
    positionDelta_ = state->GetDelta();
    previousPosition_ = startPosition_ = state->Get<Vector3>();
}

void MoveByState::Vec3State::Update(float time, Variant& value)
{
    const auto currentPos = value.GetVector3();
    auto diff = currentPos - previousPosition_;
    startPosition_ = startPosition_ + diff;
    const auto newPos = startPosition_ + positionDelta_ * time;
    value = newPos;
    previousPosition_ = newPos;
}

void MoveByState::IntVec3State::Init(const MoveByState* state)
{
    positionDelta_ = state->GetDelta();
    previousPosition_ = startPosition_ = state->Get<IntVector3>();
}

void MoveByState::IntVec3State::Update(float time, Variant& value)
{
    const auto currentPos = value.GetIntVector3();
    const auto diff = currentPos - previousPosition_;
    startPosition_ = startPosition_ + diff;
    const auto newPos = startPosition_ + (positionDelta_ * time).ToIntVector3();
    value = newPos;
    previousPosition_ = newPos;
}

void MoveByState::Vec2State::Init(const MoveByState* state)
{
    positionDelta_ = state->GetDelta().ToVector2();
    previousPosition_ = startPosition_ = state->Get<Vector2>();
}

void MoveByState::Vec2State::Update(float time, Variant& value)
{
    const auto currentPos = value.GetVector2();
    const auto diff = currentPos - previousPosition_;
    startPosition_ = startPosition_ + diff;
    const auto newPos = startPosition_ + positionDelta_ * time;
    value = newPos;
    previousPosition_ = newPos;
}

void MoveByState::IntVec2State::Init(const MoveByState* state)
{
    positionDelta_ = state->GetDelta().ToVector2();
    previousPosition_ = startPosition_ = state->Get<IntVector2>();
}

void MoveByState::IntVec2State::Update(float time, Variant& value)
{
    const auto currentPos = value.GetIntVector2();
    const auto diff = currentPos - previousPosition_;
    startPosition_ = startPosition_ + diff;
    const auto newPos = startPosition_ + (positionDelta_ * time).ToIntVector2();
    value = newPos;
    previousPosition_ = newPos;
}

MoveByState::MoveByState(MoveBy* action, Object* target)
    : AttributeActionState(action, target)
{

    if (auto attribute = GetAttribute())
    {
        switch (attribute->type_)
        {
        case VAR_VECTOR2: state_.emplace<Vec2State>(); break;
        case VAR_VECTOR3: state_.emplace<Vec3State>(); break;
        case VAR_INTVECTOR2: state_.emplace<IntVec2State>(); break;
        case VAR_INTVECTOR3: state_.emplace<IntVec3State>(); break;
        default: URHO3D_LOGERROR(Format("Attribute {} is not of valid type.", action->GetAttributeName())); break;
        }
        auto callInit = [=](auto& state) { state.Init(this); };
        ea::visit(callInit, state_);
    }
}

void MoveByState::Update(float dt, Variant& value)
{
    auto callUpdate = [&](auto& state) { state.Update(dt, value); };
    ea::visit(callUpdate, state_);
}

void MoveByQuadraticState::Vec3State::Init(const MoveByQuadraticState* state)
{
    positionDelta_ = state->GetDelta();
    controlDelta_ = state->GetControl();
    previousPosition_ = startPosition_ = state->Get<Vector3>();
}

void MoveByQuadraticState::Vec3State::Update(float time, Variant& value)
{
    const auto _time = 1.0f - time;
    const auto currentPos = value.GetVector3();
    auto diff = currentPos - previousPosition_;
    startPosition_ = startPosition_ + diff;
    const auto newPos = startPosition_ + controlDelta_ * 2.0f * time * _time + positionDelta_ * time * time;
    value = newPos;
    previousPosition_ = newPos;
}

void MoveByQuadraticState::IntVec3State::Init(const MoveByQuadraticState* state)
{
    positionDelta_ = state->GetDelta();
    controlDelta_ = state->GetControl();
    previousPosition_ = startPosition_ = state->Get<IntVector3>();
}

void MoveByQuadraticState::IntVec3State::Update(float time, Variant& value)
{
    const auto _time = 1.0f - time;
    const auto currentPos = value.GetIntVector3();
    const auto diff = currentPos - previousPosition_;
    startPosition_ = startPosition_ + diff;
    const auto newPos =
        startPosition_ + (controlDelta_ * 2.0f * time * _time + positionDelta_ * time * time).ToIntVector3();
    value = newPos;
    previousPosition_ = newPos;
}

void MoveByQuadraticState::Vec2State::Init(const MoveByQuadraticState* state)
{
    positionDelta_ = state->GetDelta().ToVector2();
    controlDelta_ = state->GetControl().ToVector2();
    previousPosition_ = startPosition_ = state->Get<Vector2>();
}

void MoveByQuadraticState::Vec2State::Update(float time, Variant& value)
{
    const auto _time = 1.0f - time;
    const auto currentPos = value.GetVector2();
    const auto diff = currentPos - previousPosition_;
    startPosition_ = startPosition_ + diff;
    const auto newPos = startPosition_ + controlDelta_ * 2.0f * time * _time + positionDelta_ * time * time;
    value = newPos;
    previousPosition_ = newPos;
}

void MoveByQuadraticState::IntVec2State::Init(const MoveByQuadraticState* state)
{
    positionDelta_ = state->GetDelta().ToVector2();
    controlDelta_ = state->GetControl().ToVector2();
    previousPosition_ = startPosition_ = state->Get<IntVector2>();
}

void MoveByQuadraticState::IntVec2State::Update(float time, Variant& value)
{
    const auto _time = 1.0f - time;
    const auto currentPos = value.GetIntVector2();
    const auto diff = currentPos - previousPosition_;
    startPosition_ = startPosition_ + diff;
    const auto newPos =
        startPosition_ + (controlDelta_ * 2.0f * time * _time + positionDelta_ * time * time).ToIntVector2();
    value = newPos;
    previousPosition_ = newPos;
}

MoveByQuadraticState::MoveByQuadraticState(MoveByQuadratic* action, Object* target)
    : AttributeActionState(action, target)
{
    if (auto attribute = GetAttribute())
    {
        switch (attribute->type_)
        {
        case VAR_VECTOR2: state_.emplace<Vec2State>(); break;
        case VAR_VECTOR3: state_.emplace<Vec3State>(); break;
        case VAR_INTVECTOR2: state_.emplace<IntVec2State>(); break;
        case VAR_INTVECTOR3: state_.emplace<IntVec3State>(); break;
        default: URHO3D_LOGERROR(Format("Attribute {} is not of valid type.", action->GetAttributeName())); break;
        }
        auto callInit = [=](auto& state) { state.Init(this); };
        ea::visit(callInit, state_);
    }
}

void MoveByQuadraticState::Update(float dt, Variant& value)
{
    auto callUpdate = [&](auto& state) { state.Update(dt, value); };
    ea::visit(callUpdate, state_);
}

JumpByState::JumpByState(JumpBy* action, Object* target)
    : AttributeActionState(action, target)
{
    if (auto attribute = GetAttribute())
    {
        switch (attribute->type_)
        {
        case VAR_VECTOR2: state_.emplace<State<Vector2>>(); break;
        case VAR_VECTOR3: state_.emplace<State<Vector3>>(); break;
        case VAR_INTVECTOR2: state_.emplace<State<IntVector2>>(); break;
        case VAR_INTVECTOR3: state_.emplace<State<IntVector3>>(); break;
        default: URHO3D_LOGERROR(Format("Attribute {} is not of valid type.", action->GetAttributeName())); break;
        }
        auto callInit = [=](auto& state) { state.Init(this); };
        ea::visit(callInit, state_);
    }
}

void JumpByState::Update(float dt, Variant& value)
{
    auto callUpdate = [&](auto& state) { state.Update(dt, value); };
    ea::visit(callUpdate, state_);
}

ScaleByState::ScaleByState(ScaleBy* action, Object* target)
    : AttributeActionState(action, target)
{
    if (auto attribute = GetAttribute())
    {
        switch (attribute->type_)
        {
        case VAR_VECTOR2: state_.emplace<State<Vector2>>(); break;
        case VAR_VECTOR3: state_.emplace<State<Vector3>>(); break;
        default: URHO3D_LOGERROR(Format("Attribute {} is not of valid type.", action->GetAttributeName())); break;
        }
        auto callInit = [=](auto& state) { state.Init(this); };
        ea::visit(callInit, state_);
    }
}

void ScaleByState::Update(float time, Variant& value)
{
    auto callUpdate = [&](auto& state) { state.Update(time, value); };
    ea::visit(callUpdate, state_);
}

RotateByState::RotateByState(RotateBy* action, Object* target)
    : AttributeActionState(action, target)
{
    rotationDelta_ = action->GetDelta();
    previousRotation_ = startRotation_ = Get<Quaternion>();
    if (auto attribute = GetAttribute())
    {
        if (attribute->type_ != VAR_QUATERNION)
        {
            URHO3D_LOGERROR(Format("Attribute {} is not of valid type.", action->GetAttributeName()));
        }
    }
}

void RotateByState::Update(float time, Variant& value)
{
    const auto currentRotation = value.GetQuaternion();
    auto diff = previousRotation_.Inverse() * currentRotation;
    startRotation_ = startRotation_ * diff;
    const auto newRotation = startRotation_ * Quaternion::IDENTITY.Slerp(rotationDelta_, time);
    value = newRotation;
    previousRotation_ = newRotation;
}

RotateAroundState::RotateAroundState(RotateAround* action, Object* target)
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
            URHO3D_LOGERROR(
                Format("Attribute {} is not of type {}.", RotationAttributeName, Variant::GetTypeName(VAR_QUATERNION)));
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
            URHO3D_LOGERROR(
                Format("Attribute {} is not of type {}.", PositionAttributeName, Variant::GetTypeName(VAR_VECTOR3)));
            positionAttribute_ = nullptr;
            return;
        }
    }
    rotationDelta_ = action->GetDelta();
    pivot_ = action->GetPivot();
    Variant rotationVariant;
    rotationAttribute_->accessor_->Get(serializable, rotationVariant);
    previousRotation_ = startRotation_ = rotationVariant.GetQuaternion();
}

void RotateAroundState::Update(float time)
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

void RemoveSelfState::Update(float time)
{
    if (!triggered_)
    {
        triggered_ = true;
        const auto target = GetTarget();
        if (!target)
        {
            return;
        }
        if (Node* node = target->Cast<Node>())
        {
            node->Remove();
        }
        else if (UIElement* element = target->Cast<UIElement>())
        {
            element->Remove();
        }
    }
}

void CloneMaterialsState::Update(float time)
{
    if (!triggered_)
    {
        triggered_ = true;
        auto* target = dynamic_cast<StaticModel*>(GetTarget());
        if (!target)
        {
            URHO3D_LOGERROR("CloneMaterials action is not running on StaticModel");
            return;
        }
        for (unsigned i = 0; i < target->GetNumGeometries(); ++i)
        {
            target->SetMaterial(i, target->GetMaterial(i)->Clone());
        }
    }
}

ShowState::ShowState(Show* action, Object* target)
    : SetAttributeState(action, target, true)
{
}

HideState::HideState(Hide* action, Object* target)
    : SetAttributeState(action, target, false)
{
}

EnableState::EnableState(Enable* action, Object* target)
    : SetAttributeState(action, target, true)
{
}

DisableState::DisableState(Disable* action, Object* target)
    : SetAttributeState(action, target, false)
{
}

BlinkState::BlinkState(Blink* action, Object* target)
    : AttributeBlinkState(action, target, false, true, action->GetNumOfBlinks())
{
}

ActionEaseState::ActionEaseState(ActionEase* action, Object* target)
    : FiniteTimeActionState(action, target)
{
    if (action->GetInnerAction())
    {
        innerState_ = StartAction(action->GetInnerAction(), target);
    }
}

void ActionEaseState::Update(float time)
{
    if (innerState_)
    {
        innerState_->Update(Ease(time));
    }
}

float ActionEaseState::Ease(float time) const
{
    return time;
}

float EaseBackInState::Ease(float time) const
{
    return BackIn(time);
}

float EaseBackOutState::Ease(float time) const
{
    return BackOut(time);
}

float EaseBackInOutState::Ease(float time) const
{
    return BackInOut(time);
}

float EaseBounceInState::Ease(float time) const
{
    return BounceIn(time);
}

float EaseBounceOutState::Ease(float time) const
{
    return BounceOut(time);
}

float EaseBounceInOutState::Ease(float time) const
{
    return BounceInOut(time);
}

float EaseSineInState::Ease(float time) const
{
    return SineIn(time);
}

float EaseSineOutState::Ease(float time) const
{
    return SineOut(time);
}

float EaseSineInOutState::Ease(float time) const
{
    return SineInOut(time);
}

float EaseExponentialInState::Ease(float time) const
{
    return ExponentialIn(time);
}

float EaseExponentialOutState::Ease(float time) const
{
    return ExponentialOut(time);
}

float EaseExponentialInOutState::Ease(float time) const
{
    return ExponentialInOut(time);
}

float EaseElasticState::Ease(float time) const
{
    return time;
}

float EaseElasticInState::Ease(float time) const
{
    return ElasticIn(time, static_cast<EaseElasticIn*>(GetAction())->GetPeriod());
}

float EaseElasticOutState::Ease(float time) const
{
    return ElasticOut(time, static_cast<EaseElasticOut*>(GetAction())->GetPeriod());
}

float EaseElasticInOutState::Ease(float time) const
{
    return ElasticInOut(time, static_cast<EaseElasticInOut*>(GetAction())->GetPeriod());
}

AttributeFromToState::AttributeFromToState(AttributeFromTo* action, Object* target)
    : AttributeActionState(action, target)
    , from_(action->GetFrom())
    , to_(action->GetTo())
{
}

void AttributeFromToState::Update(float time, Variant& value)
{
    value = from_.Lerp(to_, time);
}

AttributeToState::AttributeToState(AttributeTo* action, Object* target)
    : AttributeActionState(action, target)
    , to_(action->GetTo())
{
    if (attribute_)
    {
        attribute_->accessor_->Get(static_cast<const Serializable*>(target), from_);
    }
}

void AttributeToState::Update(float time, Variant& value)
{
    value = from_.Lerp(to_, time);
}

ShaderParameterActionState::ShaderParameterActionState(ShaderParameterAction* action, Object* target)
    : FiniteTimeActionState(action, target)
{
}

ShaderParameterFromToState::ShaderParameterFromToState(ShaderParameterFromTo* action, Object* target): FiniteTimeActionState(action, target)
                                                                                                       , from_(action->GetFrom())
                                                                                                       , to_(action->GetTo())
                                                                                                       , name_(action->GetName())
{
    material_ = ShaderParameterFromToState::GetMaterial(GetTarget());
}

Material* ShaderParameterFromToState::GetMaterial(Object* target)
{
    if (!target)
        return nullptr;
    auto* material = target->Cast<Material>();
    if (material)
        return material;
    if (auto* staticModel = target->Cast<StaticModel>())
        return staticModel->GetMaterial(0);

    if (auto* node = target->Cast<Node>())
    {
        if (auto* staticModel = node->GetComponent<StaticModel>())
            return staticModel->GetMaterial(0);
        if (auto* animatedModel = node->GetComponent<AnimatedModel>())
            return animatedModel->GetMaterial(0);
    }
    URHO3D_LOGERROR("Can't get matrial from {}", target->GetTypeName());
    return nullptr;
}

void ShaderParameterFromToState::Update(float time)
{
    if (material_)
    {
        material_->SetShaderParameter(name_, from_.Lerp(to_, time));
    }
}

ShaderParameterToState::ShaderParameterToState(ShaderParameterTo* action, Object* target): FiniteTimeActionState(action, target)
    , to_(action->GetTo())
    , name_(action->GetName())
{
    material_ = ShaderParameterFromToState::GetMaterial(GetTarget());
    if (material_)
    {
        from_ = material_->GetShaderParameter(name_);
        if (from_.GetType() != to_.GetType())
        {
            from_ = to_;
        }
    }
}

void ShaderParameterToState::Update(float time)
{
    if (material_)
    {
        material_->SetShaderParameter(name_, from_.Lerp(to_, time));
    }
}


AttributeBlinkState::AttributeBlinkState(
    AttributeAction* action, Object* target, Variant from, Variant to, unsigned times)
    : AttributeActionState(action, target)
    , from_(from)
    , to_(to)
{
    times_ = Max(1, times);
    Get(originalState_);
}

AttributeBlinkState::AttributeBlinkState(AttributeBlink* action, Object* target)
    : AttributeActionState(action, target)
    , from_(action->GetFrom())
    , to_(action->GetTo())
{
    times_ = Max(1, action->GetNumOfBlinks());
    Get(originalState_);
}

void AttributeBlinkState::Update(float time, Variant& var)
{
    const auto slice = 1.0f / static_cast<float>(times_);
    const auto m = Mod(time, slice);
    var = (m > slice / 2) ? from_ : to_;
}

void AttributeBlinkState::Stop()
{
    Set(originalState_);
}
SetAttributeState::SetAttributeState(AttributeAction* action, Object* target, const Variant& value)
    : AttributeActionState(action, target)
    , value_(value)
{
}

SetAttributeState::SetAttributeState(AttributeActionInstant* action, Object* target, const Variant& value)
    : AttributeActionState(action, target)
    , value_(value)
{
}

SetAttributeState::SetAttributeState(SetAttribute* action, Object* target)
    : AttributeActionState(action, target)
    , value_(action->GetValue())
{
}

void SetAttributeState::Update(float time, Variant& var)
{
    if (!triggered_)
    {
        var = value_;
        triggered_ = true;
    }
}

} // namespace Detail
} // namespace Actions
} // namespace Urho3D
