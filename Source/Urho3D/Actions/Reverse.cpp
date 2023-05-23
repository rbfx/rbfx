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

#include "Actions.h"

#include "AttributeActionState.h"
#include "Urho3D/IO/ArchiveSerializationBasic.h"

namespace Urho3D
{
namespace Actions
{

/// ------------------------------------------------------------------------------
void MoveBy::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<MoveBy*>(action)->SetDelta(-delta_);
}

/// ------------------------------------------------------------------------------
void MoveByQuadratic::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<MoveByQuadratic*>(action)->SetDelta(-GetDelta());
    static_cast<MoveByQuadratic*>(action)->SetControl(-GetControl());
}

/// ------------------------------------------------------------------------------
void JumpBy::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<JumpBy*>(action)->SetDelta(-delta_);
}

/// ------------------------------------------------------------------------------
///
void ScaleBy::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<ScaleBy*>(action)->SetDelta(Vector3(1.0f / delta_.x_, 1.0f / delta_.y_, 1.0f / delta_.z_));
}

/// ------------------------------------------------------------------------------
void RotateBy::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<RotateBy*>(action)->SetDelta(delta_.Inverse());
}


/// ------------------------------------------------------------------------------

void RotateAround::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<RotateAround*>(action)->SetDelta(delta_.Inverse());
    static_cast<RotateAround*>(action)->SetPivot(GetPivot());
}

void Blink::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<Blink*>(action)->SetNumOfBlinks(GetNumOfBlinks());
}

/// Get action duration.
float ActionEase::GetDuration() const
{
    if (innerAction_)
    {
        return innerAction_->GetDuration();
    }
    return ea::numeric_limits<float>::epsilon();
}

void ActionEase::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<ActionEase*>(action)->SetInnerAction(innerAction_->Reverse());
}

/// Populate fields in reversed action.
void EaseElastic::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBackIn::Reverse() const
{
    auto action = MakeShared<EaseBackOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBackOut::Reverse() const
{
    auto action = MakeShared<EaseBackIn>(context_);
    ReverseImpl(action);
    return action;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBounceIn::Reverse() const
{
    auto action = MakeShared<EaseBounceOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBounceOut::Reverse() const
{
    auto action = MakeShared<EaseBounceIn>(context_);
    ReverseImpl(action);
    return action;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseSineIn::Reverse() const
{
    auto action = MakeShared<EaseSineOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseSineOut::Reverse() const
{
    auto action = MakeShared<EaseSineIn>(context_);
    ReverseImpl(action);
    return action;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseElasticIn::Reverse() const
{
    auto action = MakeShared<EaseElasticOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseElasticOut::Reverse() const
{
    auto action = MakeShared<EaseElasticIn>(context_);
    ReverseImpl(action);
    return action;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseExponentialIn::Reverse() const
{
    auto action = MakeShared<EaseExponentialOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseExponentialOut::Reverse() const
{
    auto action = MakeShared<EaseExponentialIn>(context_);
    ReverseImpl(action);
    return action;
}

void AttributeFromTo::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<AttributeFromTo*>(action)->SetFrom(GetTo());
    static_cast<AttributeFromTo*>(action)->SetTo(GetFrom());
}

void AttributeBlink::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<AttributeBlink*>(action)->SetFrom(GetTo());
    static_cast<AttributeBlink*>(action)->SetTo(GetFrom());
    static_cast<AttributeBlink*>(action)->SetNumOfBlinks(GetNumOfBlinks());
}

void AttributeTo::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<AttributeTo*>(action)->SetTo(GetTo());
}

void SetAttribute::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<SetAttribute*>(action)->SetValue(GetValue());
}


void ShaderParameterAction::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<ShaderParameterAction*>(action)->SetName(GetName());
}

void ShaderParameterTo::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<ShaderParameterTo*>(action)->SetTo(GetTo());
}

void ShaderParameterFromTo::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<ShaderParameterFromTo*>(action)->SetFrom(GetFrom());
    static_cast<ShaderParameterFromTo*>(action)->SetTo(GetTo());
}

} // namespace Actions
} // namespace Urho3D

