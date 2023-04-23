//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "../Precompiled.h"

#include "../Input/PointerAdapter.h"

#include "../Input/Input.h"
#include "../Input/InputEvents.h"
#include "../Core/CoreEvents.h"
#include "../Graphics/Graphics.h"
#include "../UI/UI.h"

namespace Urho3D
{
PointerAdapter::PointerAdapter(Context* context)
    : Object(context)
    , directionAdapter_(MakeShared<DirectionAggregator>(context))
{
    // Propagate default subscription mask to direction adapter.
    SetSubscriptionMask(GetSubscriptionMask());

    const auto* input = GetSubsystem<Input>();
    pointerPosition_ = input->GetMousePosition().ToVector2();

    const auto* graphics = GetSubsystem<Graphics>();
    if (graphics)
    {
        maxCursorSpeed_ = Max(graphics->GetWidth(), graphics->GetHeight()) / 2.0f;
    }
}


/// Set enabled flag. The object subscribes for events when enabled.
void PointerAdapter::SetEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        directionAdapter_->SetEnabled(enabled);
        enabled_ = enabled;
        if (enabled_)
        {
            UpdateSubscriptions(enabledSubscriptions_);
        }
        else
        {
            UpdateSubscriptions(PointerAdapterMask::None);
        }
    }
}

void PointerAdapter::SetSubscriptionMask(PointerAdapterFlags mask)
{
    enabledSubscriptions_ = mask;

    // Propagate keyboard settings to the underlying direction adapter.
    DirectionAggregatorFlags flags{DirectionAggregatorMask::None};
    if (enabledSubscriptions_.Test(PointerAdapterMask::Keyboard))
        flags.Set(DirectionAggregatorMask::Keyboard);
    if (enabledSubscriptions_.Test(PointerAdapterMask::Joystick))
        flags.Set(DirectionAggregatorMask::Joystick);
    directionAdapter_->SetSubscriptionMask(flags);

    if (IsEnabled())
        UpdateSubscriptions(enabledSubscriptions_);
}


void PointerAdapter::UpdateSubscriptions(PointerAdapterFlags flags)
{
    auto* input = GetSubsystem<Input>();

    const auto toSubscribe = flags & ~subscriptionFlags_;
    const auto toUnsubscribe = subscriptionFlags_ & ~flags;

    if (!subscriptionFlags_ && flags)
    {
        SubscribeToEvent(E_UPDATE, &PointerAdapter::HandleUpdate);
    }
    else if (subscriptionFlags_ && !flags)
    {
        UnsubscribeFromEvent(E_UPDATE);
    }

    subscriptionFlags_ = flags;
    if (toSubscribe & PointerAdapterMask::Mouse)
    {
        SubscribeToEvent(input, E_MOUSEMOVE, &PointerAdapter::HandleMouseMove);
        SubscribeToEvent(input, E_MOUSEBUTTONUP, &PointerAdapter::HandleMouseButtonUp);
        SubscribeToEvent(input, E_MOUSEBUTTONDOWN, &PointerAdapter::HandleMouseButtonDown);
    }
    else if (toUnsubscribe & PointerAdapterMask::Mouse)
    {
        UnsubscribeFromEvent(E_MOUSEMOVE);
    }
    if (toSubscribe & PointerAdapterMask::Touch)
    {
        SubscribeToEvent(input, E_TOUCHBEGIN, &PointerAdapter::HandleTouchBegin);
        SubscribeToEvent(input, E_TOUCHMOVE, &PointerAdapter::HandleTouchMove);
        SubscribeToEvent(input, E_TOUCHEND, &PointerAdapter::HandleTouchEnd);
    }
    else if (toUnsubscribe & PointerAdapterMask::Touch)
    {
        UnsubscribeFromEvent(E_TOUCHBEGIN);
        UnsubscribeFromEvent(E_TOUCHMOVE);
        UnsubscribeFromEvent(E_TOUCHEND);
        activeTouchId_.reset();
    }
    if (toSubscribe & PointerAdapterMask::Joystick)
    {
        SubscribeToEvent(input, E_JOYSTICKBUTTONDOWN, &PointerAdapter::HandleJoystickButton);
        SubscribeToEvent(input, E_JOYSTICKBUTTONUP, &PointerAdapter::HandleJoystickButton);
    }
    else if (toUnsubscribe & PointerAdapterMask::Joystick)
    {
        UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
        UnsubscribeFromEvent(E_JOYSTICKBUTTONUP);
    }
}

void PointerAdapter::HandleUpdate(StringHash eventType, VariantMap& args)
{
    auto aggregatedDirection = directionAdapter_->GetDirection();

    using namespace Update;
    auto timestep = args[P_TIMESTEP].GetFloat();

    auto velocity = aggregatedDirection * maxCursorSpeed_;
    auto targetSpeed = velocity.Length();
    if (targetSpeed > maxCursorSpeed_)
    {
        auto limitedSpeed = Lerp(cursorSpeed_, targetSpeed, cursorAcceleration_);
        velocity = velocity * (limitedSpeed / targetSpeed);
    }

    UpdatePointer(pointerPosition_ + velocity * timestep, pointerPressed_, true);
}

void PointerAdapter::HandleMouseMove(StringHash eventType, VariantMap& args)
{
    // Ignore mouse movement if touch is active.
    if (activeTouchId_.has_value())
        return;

    using namespace MouseMove;
    UpdatePointer(IntVector2(args[P_X].GetInt(), args[P_Y].GetInt()).ToVector2(), pointerPressed_, false);
}

void PointerAdapter::HandleMouseButtonUp(StringHash eventType, VariantMap& args)
{
    // Ignore mouse movement if touch is active.
    if (activeTouchId_.has_value())
        return;

    using namespace MouseButtonUp;
    UpdatePointer(pointerPosition_, false, false);
}

void PointerAdapter::HandleMouseButtonDown(StringHash eventType, VariantMap& args)
{
    // Ignore mouse movement if touch is active.
    if (activeTouchId_.has_value())
        return;

    using namespace MouseButtonDown;
    UpdatePointer(pointerPosition_, true, false);
}


void PointerAdapter::HandleTouchBegin(StringHash eventType, VariantMap& args)
{
    // Do nothing if already is tracking touch
    if (activeTouchId_.has_value())
        return;

    const auto* input = GetSubsystem<Input>();

    // Start tracking touch
    using namespace TouchBegin;
    auto* touchState = input->GetTouchById(args[P_TOUCHID].GetInt());
    if (touchState && touchState->touchedElement_ == directionAdapter_->GetUIElement())
    {
        activeTouchId_ = touchState->touchID_;
        UpdatePointer(IntVector2(args[P_X].GetInt(), args[P_Y].GetInt()).ToVector2(), true, true);
    }
}

void PointerAdapter::HandleTouchMove(StringHash eventType, VariantMap& args)
{
    // Do nothing if not tracking touch
    if (!activeTouchId_.has_value())
        return;
    // Validate touch id
    using namespace TouchMove;
    const int touchId = args[P_TOUCHID].GetInt();
    if (touchId != activeTouchId_.value())
        return;

    UpdatePointer(IntVector2(args[P_X].GetInt(), args[P_Y].GetInt()).ToVector2(), true, true);
}

void PointerAdapter::HandleTouchEnd(StringHash eventType, VariantMap& args)
{
    // Do nothing if not tracking touch
    if (!activeTouchId_.has_value())
        return;
    // Stop tracking touch
    using namespace TouchBegin;
    const int touchId = args[P_TOUCHID].GetInt();
    if (touchId != activeTouchId_.value())
        return;

    
    UpdatePointer(IntVector2(args[P_X].GetInt(), args[P_Y].GetInt()).ToVector2(), false, true);

    activeTouchId_.reset();
}

void PointerAdapter::HandleJoystickButton(StringHash eventType, VariantMap& args)
{
    auto down = eventType == E_JOYSTICKBUTTONDOWN;
    using namespace JoystickButtonDown;
    if (args[P_BUTTON].GetInt() == 0)
    {
        UpdatePointer(pointerPosition_, down, false);
    }
}

void PointerAdapter::UpdatePointer(const Vector2& position, bool press, bool moveMouse)
{
    const auto input = GetSubsystem<Input>();

    if (!pointerPosition_.Equals(position))
    {
        const auto prevPosition = pointerPosition_.ToIntVector2();

        pointerPosition_ = position;

        const auto newPosition = pointerPosition_.ToIntVector2();

        if (moveMouse)
        {
            input->SetMousePosition(newPosition);
        }

        if (newPosition != prevPosition)
        {
            using namespace MouseMove;

            VariantMap& eventData = GetEventDataMap();

            eventData[P_X] = newPosition.x_;
            eventData[P_Y] = newPosition.y_;
            eventData[P_DX] = newPosition.x_ - prevPosition.x_;
            eventData[P_DY] = newPosition.y_ - prevPosition.y_;
            eventData[P_BUTTONS] = pointerPressed_ ? MOUSEB_LEFT : 0;
            eventData[P_QUALIFIERS] = input->GetQualifiers();
            SendEvent(E_MOUSEMOVE, eventData);
        }
    }

    if (press != pointerPressed_)
    {
        pointerPressed_ = press;
        if (pointerPressed_)
        {
            using namespace MouseButtonDown;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_BUTTON] = MOUSEB_LEFT;
            eventData[P_BUTTONS] = pointerPressed_ ? MOUSEB_LEFT : 0;
            eventData[P_QUALIFIERS] = input->GetQualifiers();
            eventData[P_CLICKS] = 1;
            SendEvent(E_MOUSEBUTTONDOWN, eventData);
        }
        else
        {
            using namespace MouseButtonUp;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_BUTTON] = 0;
            eventData[P_BUTTONS] = pointerPressed_ ? MOUSEB_LEFT : 0;
            eventData[P_QUALIFIERS] = input->GetQualifiers();
            SendEvent(E_MOUSEBUTTONUP, eventData);
        }
    }
}

/// Set UI element to filter touch events. Only touch events originated in the element going to be handled.
void PointerAdapter::SetUIElement(UIElement* element)
{
    directionAdapter_->SetUIElement(element);
}

void PointerAdapter::SetCursorSpeed(float cursorSpeed)
{
    cursorSpeed_ = cursorSpeed;
}

void PointerAdapter::SetCursorAcceleration(float cursorAcceleration)
{
    cursorAcceleration_ = cursorAcceleration;
}

IntVector2 PointerAdapter::GetUIPointerPosition() const
{
    const auto ui = GetSubsystem<UI>();
    const auto pos = GetPointerPosition();
    if (ui)
        return ui->ConvertSystemToUI(pos);
    return pos;
}

DirectionAggregator* PointerAdapter::GetDirectionAggregator() const
{
    return directionAdapter_;
}

} // namespace Urho3D
