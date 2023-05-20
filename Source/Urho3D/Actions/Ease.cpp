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

#include "Actions.h"

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
            //innerAction_->Update(action_->Ease(dt));
        }
    }

private:
    SharedPtr<ActionEase> action_;
    SharedPtr<FiniteTimeActionState> innerAction_;
};

} // namespace

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

} // namespace Actions
} // namespace Urho3D
