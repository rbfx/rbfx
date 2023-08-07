//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../Input/DirectionAggregator.h"

#include "../Core/Context.h"
#include "../Graphics/Graphics.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"

#include <SDL.h>

namespace Urho3D
{

/// Construct.
DirectionAggregator::DirectionAggregator(Context* context)
    : Object(context)
    , input_(context->GetSubsystem<Input>())
{
    // By default the full axis range is about 1 inch
    const auto* graphics = context->GetSubsystem<Graphics>();
    if (graphics && graphics->GetDisplayDPI().x_ > 0)
        touchSensitivity_ = 2.0f / graphics->GetDisplayDPI().x_;
    else
        touchSensitivity_ = 2.0f / 96.0f;

    this->ignoreJoystickId_ = input_->FindAccelerometerJoystickId();
}

DirectionAggregator::~DirectionAggregator()
{
}

/// Set enabled flag. The object subscribes for events when enabled.
void DirectionAggregator::SetEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        if (enabled_)
        {
            UpdateSubscriptions(enabledSubscriptions_);
        }
        else
        {
            UpdateSubscriptions(DirectionAggregatorMask::None);
        }
    }
}

void DirectionAggregator::SetSubscriptionMask(DirectionAggregatorFlags mask)
{
    enabledSubscriptions_ = mask;
    if (IsEnabled())
        UpdateSubscriptions(enabledSubscriptions_);
}

/// Set UI element to filter touch events. Only touch events originated in the element going to be handled.
void DirectionAggregator::SetUIElement(UIElement* element) { uiElement_ = element; }

/// Set dead zone to mitigate axis drift.
void DirectionAggregator::SetDeadZone(float deadZone) { axisAdapter_.SetDeadZone(deadZone); }

void DirectionAggregator::UpdateSubscriptions(DirectionAggregatorFlags flags)
{
    const auto toSubscribe = flags & ~subscriptionFlags_;
    const auto toUnsubscribe = subscriptionFlags_ & ~flags;

    if (!subscriptionFlags_ && flags)
    {
        SubscribeToEvent(input_, E_INPUTFOCUS, &DirectionAggregator::HandleInputFocus);
    }
    else if (subscriptionFlags_ && !flags)
    {
        UnsubscribeFromEvent(input_, E_INPUTFOCUS);
    }

    subscriptionFlags_ = flags;
    if (toSubscribe & DirectionAggregatorMask::Keyboard)
    {
        SubscribeToEvent(input_, E_KEYUP, &DirectionAggregator::HandleKeyUp);
        SubscribeToEvent(input_, E_KEYDOWN, &DirectionAggregator::HandleKeyDown);
    }
    else if (toUnsubscribe & DirectionAggregatorMask::Keyboard)
    {
        UnsubscribeFromEvent(E_KEYUP);
        UnsubscribeFromEvent(E_KEYDOWN);

        auto predicate = [](const AxisState& key) { return key.input_ == InputType::Keyboard; };
        ea::erase_if(horizontalAxis_, predicate);
        ea::erase_if(verticalAxis_, predicate);
    }
    if (toSubscribe & DirectionAggregatorMask::Joystick)
    {
        SubscribeToEvent(input_, E_JOYSTICKAXISMOVE, &DirectionAggregator::HandleJoystickAxisMove);
        SubscribeToEvent(input_, E_JOYSTICKHATMOVE, &DirectionAggregator::HandleJoystickHatMove);
        SubscribeToEvent(
            input_, E_JOYSTICKDISCONNECTED, &DirectionAggregator::HandleJoystickDisconnected);
    }
    else if (toUnsubscribe & DirectionAggregatorMask::Joystick)
    {
        UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
        UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
        UnsubscribeFromEvent(E_JOYSTICKDISCONNECTED);

        auto predicate = [](const AxisState& key) { return key.input_ >= InputType::JoystickAxis; };
        ea::erase_if(horizontalAxis_, predicate);
        ea::erase_if(verticalAxis_, predicate);
    }
    if (toSubscribe & DirectionAggregatorMask::Touch)
    {
        SubscribeToEvent(input_, E_TOUCHBEGIN, &DirectionAggregator::HandleTouchBegin);
        SubscribeToEvent(input_, E_TOUCHMOVE, &DirectionAggregator::HandleTouchMove);
        SubscribeToEvent(input_, E_TOUCHEND, &DirectionAggregator::HandleTouchEnd);
    }
    else if (toUnsubscribe & DirectionAggregatorMask::Touch)
    {
        UnsubscribeFromEvent(E_TOUCHBEGIN);
        UnsubscribeFromEvent(E_TOUCHMOVE);
        UnsubscribeFromEvent(E_TOUCHEND);

        auto predicate = [](const AxisState& key) { return key.input_ == InputType::Touch; };
        ea::erase_if(horizontalAxis_, predicate);
        ea::erase_if(verticalAxis_, predicate);
        activeTouchId_.reset();
    }
}

/// Get aggregated direction vector.
Vector2 DirectionAggregator::GetDirection() const
{
    Vector2 res = Vector2::ZERO;

    if (!horizontalAxis_.empty())
    {
        for (const AxisState& activeState : horizontalAxis_)
            res.x_ += activeState.value_;
        res.x_ *= 1.0f / static_cast<float>(horizontalAxis_.size());
    }
    if (!verticalAxis_.empty())
    {
        for (const AxisState& activeState : verticalAxis_)
            res.y_ += activeState.value_;
        res.y_ *= 1.0f / static_cast<float>(verticalAxis_.size());
    }

    return res;
}

void DirectionAggregator::HandleInputFocus(StringHash eventType, VariantMap& args)
{
    using namespace InputFocus;
    const unsigned focus = args[P_FOCUS].GetBool();
    if (!focus)
    {
        horizontalAxis_.clear();
        verticalAxis_.clear();
    }
}

void DirectionAggregator::HandleKeyDown(StringHash eventType, VariantMap& args)
{
    using namespace KeyDown;
    const unsigned scancode = args[P_SCANCODE].GetUInt();
    switch (scancode)
    {
    case SCANCODE_W:
    case SCANCODE_UP: UpdateAxis(verticalAxis_, {InputType::Keyboard, scancode, - 1.0f}); break;
    case SCANCODE_S:
    case SCANCODE_DOWN: UpdateAxis(verticalAxis_, {InputType::Keyboard, scancode, 1.0f}); break;
    case SCANCODE_A:
    case SCANCODE_LEFT: UpdateAxis(horizontalAxis_, {InputType::Keyboard, scancode, -1.0f}); break;
    case SCANCODE_D:
    case SCANCODE_RIGHT: UpdateAxis(horizontalAxis_, {InputType::Keyboard, scancode, 1.0f}); break;
    }
}

void DirectionAggregator::HandleKeyUp(StringHash eventType, VariantMap& args)
{
    using namespace KeyUp;
    const unsigned scancode = args[P_SCANCODE].GetUInt();
    switch (scancode)
    {
    case SCANCODE_W:
    case SCANCODE_UP: UpdateAxis(verticalAxis_, {InputType::Keyboard, scancode, 0.0f}); break;
    case SCANCODE_S:
    case SCANCODE_DOWN: UpdateAxis(verticalAxis_, {InputType::Keyboard, scancode, 0.0f}); break;
    case SCANCODE_A:
    case SCANCODE_LEFT: UpdateAxis(horizontalAxis_, {InputType::Keyboard, scancode, 0.0f}); break;
    case SCANCODE_D:
    case SCANCODE_RIGHT: UpdateAxis(horizontalAxis_, {InputType::Keyboard, scancode, 0.0f}); break;
    }
}

void DirectionAggregator::HandleJoystickAxisMove(StringHash eventType, VariantMap& args)
{
    using namespace JoystickAxisMove;

    const auto joystickId = args[P_JOYSTICKID].GetUInt();
    if (joystickId == ignoreJoystickId_)
        return;

    const auto axisIndex = args[P_AXIS].GetUInt();
    const auto value = args[P_POSITION].GetFloat();

    // Up-Down
    if (axisIndex == 1)
    {
        UpdateAxis(verticalAxis_, {InputType::JoystickAxis, joystickId, value});
    }
    // Left-Right
    if (axisIndex == 0)
    {
        UpdateAxis(horizontalAxis_, {InputType::JoystickAxis, joystickId, value});
    }
}

void DirectionAggregator::HandleJoystickHatMove(StringHash eventType, VariantMap& args)
{
    using namespace JoystickHatMove;
    const auto hatIndex = args[P_HAT].GetUInt();
    if (hatIndex != 0)
        return;

    const auto joystickId = args[P_JOYSTICKID].GetUInt();
    const auto position = args[P_POSITION].GetUInt();

    const float horizontalValue = ((position & HAT_RIGHT) ? 1.0f : 0.0f) + ((position & HAT_LEFT) ? -1.0f : 0.0f);
    UpdateAxis(horizontalAxis_, {InputType::JoystickDPad, joystickId, horizontalValue});
    const float verticalValue = ((position & HAT_DOWN) ? 1.0f : 0.0f) + ((position & HAT_UP) ? -1.0f : 0.0f);
    UpdateAxis(verticalAxis_, {InputType::JoystickDPad, joystickId, verticalValue});
}

void DirectionAggregator::HandleJoystickDisconnected(StringHash eventType, VariantMap& args)
{
    using namespace JoystickDisconnected;
    const auto joystickId = args[P_JOYSTICKID].GetUInt();

    // Cancel Axis states.
    const auto joyEventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis));
    UpdateAxis(verticalAxis_, {joyEventId, joystickId, 0.0f});
    UpdateAxis(horizontalAxis_, {joyEventId, joystickId, 0.0f});

    // Cancel DPad states.
    const auto dpadEventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad));
    UpdateAxis(verticalAxis_, {dpadEventId, joystickId, 0.0f});
    UpdateAxis(horizontalAxis_, {dpadEventId, joystickId, 0.0f});
}

void DirectionAggregator::HandleTouchBegin(StringHash eventType, VariantMap& args)
{
    // Do nothing if already is tracking touch
    if (activeTouchId_.has_value())
        return;

    // Start tracking touch
    using namespace TouchBegin;
    auto* touchState = input_->GetTouchById(args[P_TOUCHID].GetInt());
    if (touchState && touchState->touchedElement_ == uiElement_)
    {
        activeTouchId_ = touchState->touchID_;
        touchOrigin_ = IntVector2(args[P_X].GetInt(), args[P_Y].GetInt());
    }
}

void DirectionAggregator::HandleTouchMove(StringHash eventType, VariantMap& args)
{
    // Do nothing if not tracking touch
    if (!activeTouchId_.has_value())
        return;
    // Validate touch id
    using namespace TouchBegin;
    const int touchId = args[P_TOUCHID].GetInt();
    if (touchId != activeTouchId_.value())
        return;

    auto pos = IntVector2(args[P_X].GetInt(), args[P_Y].GetInt());

    const float dx = Clamp((pos.x_ - touchOrigin_.x_) * touchSensitivity_, -1.0f, 1.0f);
    const float dy = Clamp((pos.y_ - touchOrigin_.y_) * touchSensitivity_, -1.0f, 1.0f);

    // Reset origin position if touch left the -1..1 range
    touchOrigin_.x_ = pos.x_ - static_cast<int>(dx / touchSensitivity_);
    touchOrigin_.y_ = pos.y_ - static_cast<int>(dy / touchSensitivity_);

    UpdateAxis(horizontalAxis_, AxisState{InputType::Touch, 0, dx});
    UpdateAxis(verticalAxis_, AxisState{InputType::Touch, 0, dy});
}

void DirectionAggregator::HandleTouchEnd(StringHash eventType, VariantMap& args)
{
    // Do nothing if not tracking touch
    if (!activeTouchId_.has_value())
        return;
    // Stop tracking touch
    using namespace TouchBegin;
    const int touchId = args[P_TOUCHID].GetInt();
    if (touchId != activeTouchId_.value())
        return;

    activeTouchId_.reset();
    UpdateAxis(horizontalAxis_, AxisState{InputType::Touch, 0, 0.0f});
    UpdateAxis(verticalAxis_, AxisState{InputType::Touch, 0, 0.0f});
}

void DirectionAggregator::UpdateAxis(ea::fixed_vector<AxisState, 4>& activeStates, AxisState state)
{
    // Apply dead zone
    const float adjustedValue = axisAdapter_.Transform(state.value_);

    // Find and replace value
    for (AxisState& activeState : activeStates)
    {
        if ((activeState.input_ == state.input_) && (activeState.key_ == state.key_))
        {
            if (adjustedValue == 0.0f)
                activeStates.erase_unsorted(&activeState);
            else
                activeState.value_ = adjustedValue;
            return;
        }
    }
    // Add value if not found
    if (adjustedValue != 0.0f)
        activeStates.push_back(AxisState{state.input_, state.key_, adjustedValue});
}

} // namespace Urho3D
