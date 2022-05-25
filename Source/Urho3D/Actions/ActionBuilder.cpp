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
#include "Misc.h"
#include "Parallel.h"

namespace Urho3D
{
using namespace Actions;

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

/// Run action in parallel to currrent one.
ActionBuilder ActionBuilder::Also(Actions::FiniteTimeAction* parallelAction)
{
    if (!parallelAction)
    {
        return *this;
    }

    if (action_)
    {
        SharedPtr<Parallel> parallel;
        parallel.DynamicCast(action_);
        if (!parallel)
        {
            parallel = MakeShared<Parallel>(context_);
            parallel->AddAction(action_->Cast<FiniteTimeAction>());
        }
        parallel->AddAction(parallelAction);

        return ActionBuilder(context_, parallel);
    }

    return ActionBuilder(context_, parallelAction);
}

/// Build MoveBy action.
ActionBuilder ActionBuilder::MoveBy(float duration, const Vector3& offset)
{
    auto action = MakeShared<Actions::MoveBy>(context_);
    action->SetDuration(duration);
    action->SetPositionDelta(offset);
    return Then(action);
}

ActionBuilder ActionBuilder::MoveBy2D(float duration, const Vector2& offset)
{
    auto action = MakeShared<Actions::MoveBy2D>(context_);
    action->SetDuration(duration);
    action->SetPositionDelta(offset);
    return Then(action);
}

ActionBuilder ActionBuilder::Hide()
{
    auto action = MakeShared<Actions::Hide>(context_);
    return Then(action);
}

ActionBuilder ActionBuilder::Show()
{
    auto action = MakeShared<Actions::Show>(context_);
    return Then(action);
}

ActionBuilder ActionBuilder::Blink(float duration, unsigned numOfBlinks)
{
    auto action = MakeShared<Actions::Blink>(context_);
    action->SetDuration(duration);
    action->SetNumOfBlinks(numOfBlinks);
    return Then(action);
}

ActionBuilder ActionBuilder::Blink(float duration, unsigned numOfBlinks, const ea::string_view& attributeName)
{
    auto action = MakeShared<Actions::Blink>(context_);
    action->SetDuration(duration);
    action->SetNumOfBlinks(numOfBlinks);
    action->SetAttributeName(attributeName);
    return Then(action);
}

/// Combine with BackIn action.
ActionBuilder ActionBuilder::BackIn()
{
    const auto action = MakeShared<EaseBackIn>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with BackOut action.
ActionBuilder ActionBuilder::BackOut()
{
    const auto action = MakeShared<EaseBackOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with BackInOut action.
ActionBuilder ActionBuilder::BackInOut()
{
    const auto action = MakeShared<EaseBackInOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with BounceOut action.
ActionBuilder ActionBuilder::BounceOut()
{
    const auto action = MakeShared<EaseBounceOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with BounceIn action.
ActionBuilder ActionBuilder::BounceIn()
{
    const auto action = MakeShared<EaseBounceIn>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with BounceInOut action.
ActionBuilder ActionBuilder::BounceInOut()
{
    const auto action = MakeShared<EaseBounceInOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with SineOut action.
ActionBuilder ActionBuilder::SineOut()
{
    const auto action = MakeShared<EaseSineOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with SineIn action.
ActionBuilder ActionBuilder::SineIn()
{
    const auto action = MakeShared<EaseSineIn>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with SineInOut action.
ActionBuilder ActionBuilder::SineInOut()
{
    const auto action = MakeShared<EaseSineInOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with ExponentialOut action.
ActionBuilder ActionBuilder::ExponentialOut()
{
    const auto action = MakeShared<EaseExponentialOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with ExponentialIn action.
ActionBuilder ActionBuilder::ExponentialIn()
{
    const auto action = MakeShared<EaseExponentialIn>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with ExponentialInOut action.
ActionBuilder ActionBuilder::ExponentialInOut()
{
    const auto action = MakeShared<EaseExponentialInOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    return ActionBuilder(context_, action);
}

/// Combine with ElasticIn action.
ActionBuilder ActionBuilder::ElasticIn(float period)
{
    const auto action = MakeShared<EaseElasticIn>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    action->SetPeriod(period);
    return ActionBuilder(context_, action);
}

/// Combine with ElasticOut action.
ActionBuilder ActionBuilder::ElasticOut(float period)
{
    const auto action = MakeShared<EaseElasticOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    action->SetPeriod(period);
    return ActionBuilder(context_, action);
}

/// Combine with ElasticInOut action.
ActionBuilder ActionBuilder::ElasticInOut(float period)
{
    const auto action = MakeShared<EaseElasticInOut>(context_);
    action->SetInnerAction(action_->Cast<FiniteTimeAction>());
    action->SetPeriod(period);
    return ActionBuilder(context_, action);
}

/// Combine with RemoveSelf action.
ActionBuilder ActionBuilder::RemoveSelf() { return Then(MakeShared<Actions::RemoveSelf>(context_)); }

/// Combine with DelayTime action.
ActionBuilder ActionBuilder::DelayTime(float duration)
{
    auto action = MakeShared<Actions::DelayTime>(context_);
    action->SetDuration(duration);
    return Then(action);
}

} // namespace Urho3D
