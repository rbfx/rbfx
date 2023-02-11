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

#include "ActionManager.h"
#include "Attribute.h"
#include "Ease.h"
#include "Misc.h"
#include "Move.h"
#include "Parallel.h"
#include "Repeat.h"
#include "Sequence.h"
#include "ShaderParameter.h"
#include "Urho3D/Core/Context.h"

namespace Urho3D
{
using namespace Actions;

ActionBuilder::ActionBuilder(Context* context)
    : context_(context)
{
}

/// Continue with provided action.
ActionBuilder& ActionBuilder::Then(const SharedPtr<Actions::FiniteTimeAction>& nextAction)
{
    if (!nextAction)
    {
        return *this;
    }

    if (action_)
    {
        const auto action = MakeShared<Urho3D::Sequence>(context_);
        action->SetFirstAction(action_);
        action->SetSecondAction(nextAction);
        action_ = action;
        return *this;
    }

    action_ = nextAction;
    return *this;
}

/// Run action in parallel to current one.
ActionBuilder& ActionBuilder::Also(const SharedPtr<Actions::FiniteTimeAction>& parallelAction)
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
            parallel->AddAction(action_);
        }
        parallel->AddAction(parallelAction);
        action_ = parallel;
        return *this;
    }
    action_ = parallelAction;
    return *this;
}

/// Build MoveBy action.
ActionBuilder& ActionBuilder::MoveBy(float duration, const Vector3& offset, ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::MoveBy>(context_);
    action->SetAttributeName(attributeName);
    action->SetDuration(duration);
    action->SetPositionDelta(offset);
    return Then(action);
}

ActionBuilder& ActionBuilder::MoveBy(float duration, const Vector2& offset, ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::MoveBy>(context_);
    action->SetAttributeName(attributeName);
    action->SetDuration(duration);
    action->SetPositionDelta(offset.ToVector3());
    return Then(action);
}

ActionBuilder& ActionBuilder::JumpBy(const Vector3& offset, ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::JumpBy>(context_);
    action->SetAttributeName(attributeName);
    action->SetPositionDelta(offset);
    return Then(action);
}

ActionBuilder& ActionBuilder::JumpBy(const Vector2& offset, ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::JumpBy>(context_);
    action->SetAttributeName(attributeName);
    action->SetPositionDelta(offset.ToVector3());
    return Then(action);
}

/// Continue with ScaleBy action.
ActionBuilder& ActionBuilder::ScaleBy(float duration, const Vector3& delta, ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::ScaleBy>(context_);
    action->SetAttributeName(attributeName);
    action->SetDuration(duration);
    action->SetScaleDelta(delta);
    return Then(action);
}

/// Continue with ScaleBy action.
ActionBuilder& ActionBuilder::ScaleBy(float duration, const Vector2& delta, ea::string_view attributeName)
{
    return ScaleBy(duration, Vector3(delta.x_, delta.y_, 1.0f), attributeName);
}

/// Continue with RotateBy action.
ActionBuilder& ActionBuilder::RotateBy(float duration, const Quaternion& delta, ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::RotateBy>(context_);
    action->SetAttributeName(attributeName);
    action->SetDuration(duration);
    action->SetRotationDelta(delta);
    return Then(action);
}

/// Continue with RotateAround action.
ActionBuilder& ActionBuilder::RotateAround(float duration, const Vector3& pivot, const Quaternion& delta)
{
    const auto action = MakeShared<Actions::RotateAround>(context_);
    action->SetDuration(duration);
    action->SetRotationDelta(delta);
    action->SetPivot(pivot);
    return Then(action);
}

ActionBuilder& ActionBuilder::Hide(ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::Hide>(context_);
    action->SetAttributeName(attributeName);
    return Then(action);
}

ActionBuilder& ActionBuilder::Show(ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::Show>(context_);
    action->SetAttributeName(attributeName);
    return Then(action);
}

ActionBuilder& ActionBuilder::Enable(ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::Enable>(context_);
    action->SetAttributeName(attributeName);
    return Then(action);
}

ActionBuilder& ActionBuilder::Disable(ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::Disable>(context_);
    action->SetAttributeName(attributeName);
    return Then(action);
}

ActionBuilder& ActionBuilder::Blink(float duration, unsigned numOfBlinks, ea::string_view attributeName)
{
    const auto action = MakeShared<Actions::Blink>(context_);
    action->SetDuration(duration);
    action->SetNumOfBlinks(numOfBlinks);
    action->SetAttributeName(attributeName);
    return Then(action);
}

/// Continue with AttributeTo action.
ActionBuilder& ActionBuilder::AttributeTo(float duration, ea::string_view attributeName, const Variant& to)
{
    const auto action = MakeShared<Actions::AttributeTo>(context_);
    action->SetDuration(duration);
    action->SetAttributeName(attributeName);
    action->SetTo(to);
    return Then(action);
}

/// Continue with AttributeFromTo action.
ActionBuilder& ActionBuilder::AttributeFromTo(
    float duration, ea::string_view attributeName, const Variant& from, const Variant& to)
{
    const auto action = MakeShared<Actions::AttributeFromTo>(context_);
    action->SetDuration(duration);
    action->SetAttributeName(attributeName);
    action->SetFrom(from);
    action->SetTo(to);
    return Then(action);
}

/// Continue with ShaderParameterTo action.
ActionBuilder& ActionBuilder::ShaderParameterTo(float duration, ea::string_view parameter, const Variant& to)
{
    const auto action = MakeShared<Actions::ShaderParameterTo>(context_);
    action->SetDuration(duration);
    action->SetName(parameter);
    action->SetTo(to);
    return Then(action);
}

/// Continue with ShaderParameterFromTo action.
ActionBuilder& ActionBuilder::ShaderParameterFromTo(
    float duration, ea::string_view parameter, const Variant& from, const Variant& to)
{
    const auto action = MakeShared<Actions::ShaderParameterFromTo>(context_);
    action->SetDuration(duration);
    action->SetName(parameter);
    action->SetFrom(from);
    action->SetTo(to);
    return Then(action);
}

/// Continue with SendEvent action.
ActionBuilder& ActionBuilder::SendEvent(ea::string_view eventType, const StringVariantMap& data)
{
    const auto action = MakeShared<Actions::SendEvent>(context_);
    action->SetEventType(eventType);
    action->SetEventData(data);
    return Then(action);
}

/// Continue with CallFunc action.
ActionBuilder& ActionBuilder::CallFunc(Actions::ActionCallHandler* handler)
{
    const auto action = MakeShared<Actions::CallFunc>(context_);
    action->SetCallHandler(handler);
    return Then(action);
}

/// Combine with BackIn action.
ActionBuilder& ActionBuilder::BackIn()
{
    const auto action = MakeShared<EaseBackIn>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with BackOut action.
ActionBuilder& ActionBuilder::BackOut()
{
    const auto action = MakeShared<EaseBackOut>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with BackInOut action.
ActionBuilder& ActionBuilder::BackInOut()
{
    const auto action = MakeShared<EaseBackInOut>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with BounceOut action.
ActionBuilder& ActionBuilder::BounceOut()
{
    const auto action = MakeShared<EaseBounceOut>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with BounceIn action.
ActionBuilder& ActionBuilder::BounceIn()
{
    const auto action = MakeShared<EaseBounceIn>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with BounceInOut action.
ActionBuilder& ActionBuilder::BounceInOut()
{
    const auto action = MakeShared<EaseBounceInOut>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with SineOut action.
ActionBuilder& ActionBuilder::SineOut()
{
    const auto action = MakeShared<EaseSineOut>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with SineIn action.
ActionBuilder& ActionBuilder::SineIn()
{
    const auto action = MakeShared<EaseSineIn>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with SineInOut action.
ActionBuilder& ActionBuilder::SineInOut()
{
    const auto action = MakeShared<EaseSineInOut>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with ExponentialOut action.
ActionBuilder& ActionBuilder::ExponentialOut()
{
    const auto action = MakeShared<EaseExponentialOut>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with ExponentialIn action.
ActionBuilder& ActionBuilder::ExponentialIn()
{
    const auto action = MakeShared<EaseExponentialIn>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with ExponentialInOut action.
ActionBuilder& ActionBuilder::ExponentialInOut()
{
    const auto action = MakeShared<EaseExponentialInOut>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Combine with ElasticIn action.
ActionBuilder& ActionBuilder::ElasticIn(float period)
{
    const auto action = MakeShared<EaseElasticIn>(context_);
    action->SetInnerAction(action_);
    action->SetPeriod(period);
    action_ = action;
    return *this;
}

/// Combine with ElasticOut action.
ActionBuilder& ActionBuilder::ElasticOut(float period)
{
    const auto action = MakeShared<EaseElasticOut>(context_);
    action->SetInnerAction(action_);
    action->SetPeriod(period);
    action_ = action;
    return *this;
}

/// Combine with ElasticInOut action.
ActionBuilder& ActionBuilder::ElasticInOut(float period)
{
    const auto action = MakeShared<EaseElasticInOut>(context_);
    action->SetInnerAction(action_);
    action->SetPeriod(period);
    action_ = action;
    return *this;
}

/// Combine with RemoveSelf action.
ActionBuilder& ActionBuilder::RemoveSelf() { return Then(MakeShared<Actions::RemoveSelf>(context_)); }

/// Combine with DelayTime action.
ActionBuilder& ActionBuilder::DelayTime(float duration)
{
    const auto action = MakeShared<Actions::DelayTime>(context_);
    action->SetDuration(duration);
    return Then(action);
}

/// Repeat current action.
ActionBuilder& ActionBuilder::Repeat(unsigned times)
{
    const auto action = MakeShared<Actions::Repeat>(context_);
    action->SetInnerAction(action_);
    action->SetTimes(times);
    action_ = action;
    return *this;
}

/// Repeat current action forever (until canceled).
ActionBuilder& ActionBuilder::RepeatForever()
{
    const auto action = MakeShared<Actions::RepeatForever>(context_);
    action->SetInnerAction(action_);
    action_ = action;
    return *this;
}

/// Run current action on object.
/// Use Build() instead of Run() if you run the action more than once to reduce allocations.
Actions::ActionState* ActionBuilder::Run(Object* target) const
{
    return Run(context_->GetSubsystem<ActionManager>(), target);
}

/// Run current action on object via action manager.
/// Use Build() instead of Run() if you run the action more than once to reduce allocations.
Actions::ActionState* ActionBuilder::Run(ActionManager* actionManager, Object* target) const
{
    if (actionManager)
    {
        return actionManager->AddAction(action_, target);
    }
    return nullptr;
}

} // namespace Urho3D
