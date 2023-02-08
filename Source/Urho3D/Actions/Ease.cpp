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

#include "Ease.h"

#include "../Core/Context.h"
#include "../IO/ArchiveSerializationBasic.h"
#include "../IO/Log.h"
#include "ActionManager.h"
#include "FiniteTimeActionState.h"

namespace Urho3D
{
namespace Actions
{

namespace
{
class ActionEaseState : public FiniteTimeActionState
{

public:
    ActionEaseState(ActionEase* action, Object* target)
        : FiniteTimeActionState(action, target)
        , action_(action)
    {
        innerAction_.DynamicCast(StartAction(action->GetInnerAction(), target));
    }

    /// Called every frame with it's delta time.
    void Update(float dt) override
    {
        if (innerAction_)
        {
            innerAction_->Update(action_->Ease(dt));
        }
    }

private:
    SharedPtr<ActionEase> action_;
    SharedPtr<FiniteTimeActionState> innerAction_;
};

} // namespace

/// Construct.
ActionEase::ActionEase(Context* context)
    : BaseClassName(context)
{
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

/// Set inner action.
void ActionEase::SetInnerAction(FiniteTimeAction* action)
{
    innerAction_ = GetOrDefault(action);
    SetDuration(innerAction_->GetDuration());
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> ActionEase::Reverse() const
{
    auto result = MakeShared<ActionEase>(context_);
    if (innerAction_)
    {
        result->SetInnerAction(innerAction_->Reverse());
    }
    return result;
}

/// Apply easing function to the time argument.
float ActionEase::Ease(float time) const { return time; }

/// Serialize content from/to archive. May throw ArchiveException.
void ActionEase::SerializeInBlock(Archive& archive)
{
    // Skipping FiniteTimeAction::SerializeInBlock on purpose to skip duration serialization
    BaseAction::SerializeInBlock(archive);
    SerializeValue(archive, "innerAction", innerAction_);
}

/// -------------------------------------------------------------------
/// Construct.
EaseElastic::EaseElastic(Context* context)
    : BaseClassName(context)
{
}

/// Serialize content from/to archive. May throw ArchiveException.
void EaseElastic::SerializeInBlock(Archive& archive)
{
    // Skipping FiniteTimeAction::SerializeInBlock on purpose
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "period_", period_, 0.3f);
}

/// -------------------------------------------------------------------
/// Construct.
EaseBackIn::EaseBackIn(Context* context)
    : BaseClassName(context)
{
    SetInnerAction(nullptr);
}

SharedPtr<FiniteTimeAction> EaseBackIn::Reverse() const
{
    auto result = MakeShared<EaseBackOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBackIn::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }

/// -------------------------------------------------------------------
/// Construct.
EaseBackOut::EaseBackOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBackOut::Reverse() const
{
    auto result = MakeShared<EaseBackIn>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBackOut::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }

/// -------------------------------------------------------------------
/// Construct.
EaseElasticIn::EaseElasticIn(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseElasticIn::Reverse() const
{
    auto result = MakeShared<EaseElasticOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    result->SetPeriod(GetPeriod());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseElasticIn::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }

/// -------------------------------------------------------------------
/// Construct.
EaseElasticInOut::EaseElasticInOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseElasticInOut::Reverse() const
{
    auto result = MakeShared<EaseElasticInOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    result->SetPeriod(GetPeriod());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseElasticInOut::StartAction(Object* target)
{
    return MakeShared<ActionEaseState>(this, target);
}

/// -------------------------------------------------------------------
/// Construct.
EaseElasticOut::EaseElasticOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseElasticOut::Reverse() const
{
    auto result = MakeShared<EaseElasticIn>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    result->SetPeriod(GetPeriod());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseElasticOut::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }


/// -------------------------------------------------------------------
/// Construct.
EaseBackInOut::EaseBackInOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBackInOut::Reverse() const
{
    auto result = MakeShared<EaseBackInOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBackInOut::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }

/// -------------------------------------------------------------------
/// Construct.
EaseBounceOut::EaseBounceOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBounceOut::Reverse() const
{
    auto result = MakeShared<EaseBounceIn>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBounceOut::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }

/// -------------------------------------------------------------------
/// Construct.
EaseBounceIn::EaseBounceIn(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBounceIn::Reverse() const
{
    auto result = MakeShared<EaseBounceOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBounceIn::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }

/// -------------------------------------------------------------------
/// Construct.
EaseBounceInOut::EaseBounceInOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBounceInOut::Reverse() const
{
    auto result = MakeShared<EaseBounceInOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBounceInOut::StartAction(Object* target)
{
    return MakeShared<ActionEaseState>(this, target);
}

/// -------------------------------------------------------------------
/// Construct.
EaseSineOut::EaseSineOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseSineOut::Reverse() const
{
    auto result = MakeShared<EaseSineIn>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseSineOut::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }

/// -------------------------------------------------------------------
/// Construct.
EaseSineIn::EaseSineIn(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseSineIn::Reverse() const
{
    auto result = MakeShared<EaseSineOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseSineIn::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }

/// -------------------------------------------------------------------
/// Construct.
EaseSineInOut::EaseSineInOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseSineInOut::Reverse() const
{
    auto result = MakeShared<EaseSineInOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseSineInOut::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }

/// -------------------------------------------------------------------
/// Construct.
EaseExponentialOut::EaseExponentialOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseExponentialOut::Reverse() const
{
    auto result = MakeShared<EaseExponentialIn>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseExponentialOut::StartAction(Object* target)
{
    return MakeShared<ActionEaseState>(this, target);
}

/// -------------------------------------------------------------------
/// Construct.
EaseExponentialIn::EaseExponentialIn(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseExponentialIn::Reverse() const
{
    auto result = MakeShared<EaseExponentialOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseExponentialIn::StartAction(Object* target)
{
    return MakeShared<ActionEaseState>(this, target);
}

/// -------------------------------------------------------------------
/// Construct.
EaseExponentialInOut::EaseExponentialInOut(Context* context)
    : BaseClassName(context)
{
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseExponentialInOut::Reverse() const
{
    auto result = MakeShared<EaseExponentialInOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseExponentialInOut::StartAction(Object* target)
{
    return MakeShared<ActionEaseState>(this, target);
}

} // namespace Actions
} // namespace Urho3D
