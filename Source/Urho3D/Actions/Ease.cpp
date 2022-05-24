//
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

#include "../Math/EaseMath.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../IO/ArchiveSerializationBasic.h"
#include "FiniteTimeActionState.h"

namespace Urho3D {

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

/// Set inner action.
void ActionEase::SetInnerAction(FiniteTimeAction* action)
{
    innerAction_ = action;
    if (innerAction_)
    {
        SetDuration(innerAction_->GetDuration());
    }
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
    FiniteTimeAction::SerializeInBlock(archive);
    SerializeValue(archive, "innerAction", innerAction_);
}

/// Construct.
EaseBackIn::EaseBackIn(Context* context)
    : BaseClassName(context)
{
}

SharedPtr<FiniteTimeAction> EaseBackIn::Reverse() const
{
    auto result = MakeShared<EaseBackOut>(context_);
    result->SetInnerAction(GetInnerAction()->Reverse());
    return result;
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBackIn::StartAction(Object* target) { return MakeShared<ActionEaseState>(this, target); }


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

}
