//
// Copyright (c) 2021 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "ActionBuilder.h"

#include "Ease.h"
#include "Move.h"
#include "Sequence.h"

namespace Urho3D
{
/// Construct.
ActionBuilder::ActionBuilder(Context* context, BaseAction* action)
    : context_(context)
    , action_(action)
{
}

ActionBuilder::ActionBuilder(Context* context)
    : context_(context)
{
}

/// Continue with provided action.
ActionBuilder ActionBuilder::Then(FiniteTimeAction* nextAction)
{
    if (!nextAction)
    {
        return *this;
    }

    if (action_)
    {
        auto action = MakeShared<Urho3D::Sequence>(context_);
        action->SetFirstAction(action_->Cast<FiniteTimeAction>());
        action->SetSecondAction(nextAction);
        return ActionBuilder(context_, action);
    }

    return ActionBuilder(context_, nextAction);
}

/// Build MoveBy action.
ActionBuilder ActionBuilder::MoveBy(float duration, const Vector3& offset)
{
    auto action = MakeShared<Urho3D::MoveBy>(context_);
    action->SetDuration(duration);
    action->SetPositionDelta(offset);
    return Then(action);
}

/// Combine with BackIn action.
ActionBuilder ActionBuilder::BackIn() {
    auto action = MakeShared<Urho3D::EaseBackIn>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with BackOut action.
ActionBuilder ActionBuilder::BackOut()
{
    auto action = MakeShared<Urho3D::EaseBackIn>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

ActionBuilder::operator SharedPtr<BaseAction>() { return action_; }

} // namespace Urho3D
