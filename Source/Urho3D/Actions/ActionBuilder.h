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

#pragma once

#include "AttributeAction.h"
#include "CallFunc.h"

namespace Urho3D
{
namespace Actions
{
class ActionCallHandler;
}

class ActionManager;

/// Action as resource
class URHO3D_API ActionBuilder
{
public:
    /// Construct.
    explicit ActionBuilder(Context* context);

    /// Continue with provided action.
    ActionBuilder& Then(const SharedPtr<Actions::FiniteTimeAction>& nextAction);

    /// Run action in parallel to current one.
    ActionBuilder& Also(const SharedPtr<Actions::FiniteTimeAction>& parallelAction);

    /// Continue with MoveBy action.
    ActionBuilder& MoveBy(float duration, const Vector3& offset, ea::string_view attributeName = Actions::POSITION_ATTRIBUTE);

    /// Continue with MoveBy action.
    ActionBuilder& MoveBy(float duration, const Vector2& offset, ea::string_view attributeName = Actions::POSITION_ATTRIBUTE);

    /// Continue with MoveBy action with quad .
    ActionBuilder& MoveByQuadratic(float duration, const Vector3& controlOffset, const Vector3& targetOffset,
        ea::string_view attributeName = Actions::POSITION_ATTRIBUTE);

    /// Continue with MoveBy action.
    ActionBuilder& MoveByQuadratic(float duration, const Vector2& controlOffset, const Vector2& targetOffset,
        ea::string_view attributeName = Actions::POSITION_ATTRIBUTE);

    /// Continue with JumpBy action.
    ActionBuilder& JumpBy(const Vector3& offset, ea::string_view attributeName = Actions::POSITION_ATTRIBUTE);

    /// Continue with MoveBy action.
    ActionBuilder& JumpBy(const Vector2& offset, ea::string_view attributeName = Actions::POSITION_ATTRIBUTE);

    /// Continue with ScaleBy action.
    ActionBuilder& ScaleBy(
        float duration, const Vector3& delta, ea::string_view attributeName = Actions::SCALE_ATTRIBUTE);

    /// Continue with ScaleBy action.
    ActionBuilder& ScaleBy(
        float duration, const Vector2& delta, ea::string_view attributeName = Actions::SCALE_ATTRIBUTE);

    /// Continue with RotateBy action.
    ActionBuilder& RotateBy(
        float duration, const Quaternion& delta, ea::string_view attributeName = Actions::ROTATION_ATTRIBUTE);

    /// Continue with RotateAround action.
    ActionBuilder& RotateAround(float duration, const Vector3& pivot, const Quaternion& delta);

    /// Continue with Hide action.
    ActionBuilder& Hide(ea::string_view attributeName = Actions::ISVISIBLE_ATTRIBUTE);

    /// Continue with Show action.
    ActionBuilder& Show(ea::string_view attributeName = Actions::ISVISIBLE_ATTRIBUTE);

    /// Continue with Enable action.
    ActionBuilder& Enable(ea::string_view attributeName = Actions::ISENABLED_ATTRIBUTE);

    /// Continue with Disable action.
    ActionBuilder& Disable(ea::string_view attributeName = Actions::ISENABLED_ATTRIBUTE);

    /// Continue with AttributeBlink action.
    ActionBuilder& Blink(float duration, unsigned numOfBlinks, ea::string_view attributeName = Actions::ISENABLED_ATTRIBUTE);

    /// Continue with AttributeTo action.
    ActionBuilder& AttributeTo(float duration, ea::string_view attributeName, const Variant& to);

    /// Continue with AttributeFromTo action.
    ActionBuilder& AttributeFromTo(
        float duration, ea::string_view attributeName, const Variant& from, const Variant& to);

    /// Continue with ShaderParameterTo action.
    ActionBuilder& ShaderParameterTo(float duration, ea::string_view parameter, const Variant& to);

    /// Continue with ShaderParameterFromTo action.
    ActionBuilder& ShaderParameterFromTo(
        float duration, ea::string_view parameter, const Variant& from, const Variant& to);

    /// Continue with SendEvent action.
    ActionBuilder& SendEvent(ea::string_view eventType, const StringVariantMap& data);

    /// Continue with CallFunc action.
    ActionBuilder& CallFunc(Actions::ActionCallHandler* handler);

    /// Continue with CallFunc action.
    template <typename T>
    ActionBuilder& CallFunc(
        T* receiver, typename Actions::ActionCallHandlerImpl<T>::HandlerFunctionPtr func, void* userData = nullptr)
    {
        return CallFunc(new Actions::ActionCallHandlerImpl<T>(receiver, func, userData));
    }

    /// Combine with BackIn action.
    ActionBuilder& BackIn();

    /// Combine with BackOut action.
    ActionBuilder& BackOut();

    /// Combine with BackInOut action.
    ActionBuilder& BackInOut();

    /// Combine with BounceOut action.
    ActionBuilder& BounceOut();

    /// Combine with BounceIn action.
    ActionBuilder& BounceIn();

    /// Combine with BounceInOut action.
    ActionBuilder& BounceInOut();

    /// Combine with SineOut action.
    ActionBuilder& SineOut();

    /// Combine with SineIn action.
    ActionBuilder& SineIn();

    /// Combine with SineInOut action.
    ActionBuilder& SineInOut();

    /// Combine with ExponentialOut action.
    ActionBuilder& ExponentialOut();

    /// Combine with ExponentialIn action.
    ActionBuilder& ExponentialIn();

    /// Combine with ExponentialInOut action.
    ActionBuilder& ExponentialInOut();

    /// Combine with ElasticIn action.
    ActionBuilder& ElasticIn(float period = 0.3f);

    /// Combine with ElasticOut action.
    ActionBuilder& ElasticOut(float period = 0.3f);

    /// Combine with ElasticInOut action.
    ActionBuilder& ElasticInOut(float period = 0.3f);

    /// Combine with RemoveSelf action.
    ActionBuilder& RemoveSelf();

    /// Combine with DelayTime action.
    ActionBuilder& DelayTime(float duration);

    /// Repeat current action.
    ActionBuilder& Repeat(unsigned times);

    /// Repeat current action forever (until canceled).
    ActionBuilder& RepeatForever();

    /// Complete action building and produce result.
    SharedPtr<Actions::FiniteTimeAction> Build() { return action_; }

    /// Run current action on object.
    /// Use Build() instead of Run() if you run the action more than once to reduce allocations.
    Actions::ActionState* Run(Object* target) const;

    /// Run current action on object via action manager.
    /// Use Build() instead of Run() if you run the action more than once to reduce allocations.
    Actions::ActionState* Run(ActionManager* actionManager, Object* target) const;

private:
    /// Urho3D context.
    Context* context_{};
    /// Action on top of stack (current).
    SharedPtr<Actions::FiniteTimeAction> action_;
};

} // namespace Urho3D
