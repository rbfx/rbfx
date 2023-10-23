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

#include "../Input/DirectionalPadAdapter.h"

#include "../Core/Context.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"
#include "../Core/CoreEvents.h"

namespace Urho3D
{

DirectionalPadAdapter::AggregatedState::AggregatedState(Scancode scancode)
    : scancode_(scancode)
    , timeToRepeat_(ea::numeric_limits<float>::max())
{
}

bool DirectionalPadAdapter::AggregatedState::Append(InputType inputType)
{
    for (unsigned i = 0; i < activeSources_.size(); ++i)
    {
        if (activeSources_[i] == inputType)
        {
            return false;
        }
    }
    activeSources_.push_back(inputType);
    return activeSources_.size() == 1;
}
bool DirectionalPadAdapter::AggregatedState::Remove(InputType inputType)
{
    for (unsigned i = 0; i < activeSources_.size(); ++i)
    {
        if (activeSources_[i] == inputType)
        {
            activeSources_.erase_unsorted(activeSources_.begin() + i);
            if (activeSources_.empty())
            {
                timeToRepeat_ = ea::numeric_limits<float>::max();
                return true;
            }
        }
    }
    return false;
}

bool DirectionalPadAdapter::AggregatedState::RemoveIf(const ea::function<bool(InputType)>& pred)
{
    if (activeSources_.empty())
    {
        return false;
    }
    for (unsigned i = activeSources_.size(); i--;)
    {
        if (pred(activeSources_[i]))
        {
            activeSources_.erase_unsorted(activeSources_.begin() + i);
        }
    }
    if (activeSources_.empty())
    {
        timeToRepeat_ = ea::numeric_limits<float>::max();
        return true;
    }
    return false;
}


/// Construct.
DirectionalPadAdapter::DirectionalPadAdapter(Context* context)
    : Object(context)
    , input_(context->GetSubsystem<Input>())
    , up_(SCANCODE_UP)
    , down_(SCANCODE_DOWN)
    , left_(SCANCODE_LEFT)
    , right_(SCANCODE_RIGHT)
{
    if (input_)
    {
        const unsigned numJoysticks = input_->GetNumJoysticks();
        for (unsigned i = 0; i < numJoysticks; ++i)
        {
            auto* joystick = input_->GetJoystickByIndex(i);
            if (joystick->GetNumAxes() == 3 && joystick->GetNumButtons() == 0 && joystick->GetNumHats() == 0)
            {
                ignoreJoystickId_ = joystick->joystickID_;
            }
        }
    }
}

/// Set enabled flag. The object subscribes for events when enabled.
void DirectionalPadAdapter::SetEnabled(bool enabled)
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
            UpdateSubscriptions(DirectionalPadAdapterMask::None);
        }
    }
}

void DirectionalPadAdapter::SetSubscriptionMask(DirectionalPadAdapterFlags mask)
{
    enabledSubscriptions_ = mask;
    if (IsEnabled())
        UpdateSubscriptions(enabledSubscriptions_);
}

/// Set axis upper threshold. Axis value greater than threshold is interpreted as key press.
void DirectionalPadAdapter::SetAxisUpperThreshold(float threshold) { axisUpperThreshold_ = threshold; }

/// Set axis lower threshold. Axis value lower than threshold is interpreted as key press.
void DirectionalPadAdapter::SetAxisLowerThreshold(float threshold) { axisLowerThreshold_ = threshold; }

/// Set repeat delay in seconds.
void DirectionalPadAdapter::SetRepeatDelay(float delayInSeconds)
{
    repeatDelay_ = Urho3D::Max(delayInSeconds, ea::numeric_limits<float>::epsilon());
}

/// Set repeat interval in seconds.
void DirectionalPadAdapter::SetRepeatInterval(float intervalInSeconds)
{
    repeatInterval_ = Urho3D::Max(intervalInSeconds, ea::numeric_limits<float>::epsilon());
}

bool DirectionalPadAdapter::GetKeyDown(Key key) const
{
    return GetScancodeDown(input_->GetScancodeFromKey(key));
}

bool DirectionalPadAdapter::GetScancodeDown(Scancode scancode) const
{
    switch (scancode)
    {
    case SCANCODE_UP: return up_.IsActive();
    case SCANCODE_DOWN: return down_.IsActive();
    case SCANCODE_LEFT: return left_.IsActive();
    case SCANCODE_RIGHT: return right_.IsActive();
    default: break;
    }
    return false;
}

void DirectionalPadAdapter::SendKeyDown(Scancode key, bool repeat)
{
    using namespace KeyDown;

    VariantMap args;

    switch (key)
    {
    case SCANCODE_UP: args[P_KEY] = KEY_UP; break;
    case SCANCODE_DOWN: args[P_KEY] = KEY_DOWN; break;
    case SCANCODE_LEFT: args[P_KEY] = KEY_LEFT; break;
    case SCANCODE_RIGHT: args[P_KEY] = KEY_RIGHT; break;
    default: args[P_KEY] = KEY_UNKNOWN; break;
    }

    args[P_SCANCODE] = key;
    args[P_BUTTONS] = 0;
    args[P_QUALIFIERS] = 0;
    args[P_REPEAT] = repeat;
    SendEvent(E_KEYDOWN, args);
}

void DirectionalPadAdapter::SendKeyUp(Scancode key)
{
    using namespace KeyUp;

    VariantMap args;

    using namespace KeyUp;
    switch (key)
    {
    case SCANCODE_UP: args[P_KEY] = KEY_UP; break;
    case SCANCODE_DOWN: args[P_KEY] = KEY_DOWN; break;
    case SCANCODE_LEFT: args[P_KEY] = KEY_LEFT; break;
    case SCANCODE_RIGHT: args[P_KEY] = KEY_RIGHT; break;
    default: args[P_KEY] = KEY_UNKNOWN; break;
    }
    args[P_SCANCODE] = key;
    args[P_BUTTONS] = 0;
    args[P_QUALIFIERS] = 0;
    SendEvent(E_KEYUP, args);
}

void DirectionalPadAdapter::HandleInputFocus(StringHash eventType, VariantMap& args)
{
    using namespace InputFocus;
    const unsigned focus = args[P_FOCUS].GetBool();
    if (!focus)
    {
        // Clear all input and send even if necessary.
        ea::function<bool(InputType)> pred = [](InputType i) { return true; };
        RemoveIf(up_, pred);
        RemoveIf(down_, pred);
        RemoveIf(left_, pred);
        RemoveIf(right_, pred);
    }
}

void DirectionalPadAdapter::HandleUpdate(StringHash eventType, VariantMap& args)
{
    using namespace Update;
    const float timeStep = args[P_TIMESTEP].GetFloat();
    ea::array<AggregatedState*, 4> states = {&up_,&down_, &left_, &right_};
    for (AggregatedState* state: states)
    {
        if (state->IsActive())
        {
            state->timeToRepeat_ -= timeStep;
            if (state->timeToRepeat_ < 0)
            {
                state->timeToRepeat_ = repeatInterval_;
                SendKeyDown(state->scancode_, true);
            }
        }
    }
}

void DirectionalPadAdapter::HandleKeyDown(StringHash eventType, VariantMap& args)
{
    using namespace KeyDown;
    switch (args[P_SCANCODE].GetUInt())
    {
    case SCANCODE_W:
    case SCANCODE_UP:
        Append(up_, InputType::Keyboard);
        break;
    case SCANCODE_S:
    case SCANCODE_DOWN:
        Append(down_, InputType::Keyboard);
        break;
    case SCANCODE_A:
    case SCANCODE_LEFT:
        Append(left_, InputType::Keyboard);
        break;
    case SCANCODE_D:
    case SCANCODE_RIGHT:
        Append(right_, InputType::Keyboard);
        break;
    }
}

void DirectionalPadAdapter::HandleKeyUp(StringHash eventType, VariantMap& args)
{
    using namespace KeyUp;
    switch (args[P_SCANCODE].GetUInt())
    {
    case SCANCODE_W:
    case SCANCODE_UP:
        Remove(up_, InputType::Keyboard);
        break;
    case SCANCODE_S:
    case SCANCODE_DOWN:
        Remove(down_, InputType::Keyboard);
        break;
    case SCANCODE_A:
    case SCANCODE_LEFT:
        Remove(left_, InputType::Keyboard);
        break;
    case SCANCODE_D:
    case SCANCODE_RIGHT:
        Remove(right_, InputType::Keyboard);
        break;
    }
}

void DirectionalPadAdapter::HandleJoystickAxisMove(StringHash eventType, VariantMap& args)
{
    using namespace JoystickAxisMove;


    auto joystickId = args[P_JOYSTICKID].GetInt();
    if (joystickId == ignoreJoystickId_)
        return;

    auto eventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis) + joystickId);

    auto axisIndex = args[P_AXIS].GetUInt();
    auto value = args[P_POSITION].GetFloat();

    // Up-Down
    if (axisIndex == 1)
    {
        if (value > axisUpperThreshold_)
        {
            Append(down_, eventId);
            Remove(up_, eventId);
        }
        else if (value < -axisUpperThreshold_)
        {
            Remove(down_, eventId);
            Append(up_, eventId);
        }
        else if (value > -axisLowerThreshold_ && value < axisLowerThreshold_)
        {
            Remove(up_, eventId);
            Remove(down_, eventId);
        }
    }
    // Left-Right
    if (axisIndex == 0)
    {
        if (value > axisUpperThreshold_)
        {
            Append(right_, eventId);
            Remove(left_, eventId);
        }
        else if (value < -axisUpperThreshold_)
        {
            Append(left_, eventId);
            Remove(right_, eventId);
        }
        else if (value > -axisLowerThreshold_ && value < axisLowerThreshold_)
        {
            Remove(right_, eventId);
            Remove(left_, eventId);
        }
    }
}

void DirectionalPadAdapter::HandleJoystickHatMove(StringHash eventType, VariantMap& args)
{
    using namespace JoystickHatMove;
    const auto hatIndex = args[P_HAT].GetUInt();
    if (hatIndex != 0)
        return;

    const auto joystickId = args[P_JOYSTICKID].GetUInt();
    const auto eventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId);

    const auto position = args[P_POSITION].GetUInt();

    if (0 != (position & HAT_UP))
        Append(up_, eventId);
    else
        Remove(up_, eventId);

    if (0 != (position & HAT_DOWN))
        Append(down_, eventId);
    else
        Remove(down_, eventId);

    if (0 != (position & HAT_LEFT))
        Append(left_, eventId);
    else
        Remove(left_, eventId);

    if (0 != (position & HAT_RIGHT))
        Append(right_, eventId);
    else
        Remove(right_, eventId);

    const float horizontalValue = ((position & HAT_RIGHT) ? 1.0f : 0.0f) + ((position & HAT_LEFT) ? -1.0f : 0.0f);
    const float verticalValue = ((position & HAT_DOWN) ? 1.0f : 0.0f) + ((position & HAT_UP) ? -1.0f : 0.0f);
}

void DirectionalPadAdapter::HandleJoystickDisconnected(StringHash eventType, VariantMap& args)
{
    using namespace JoystickDisconnected;
    auto joystickId = args[P_JOYSTICKID].GetUInt();

    // Cancel Axis states.
    const auto joyEventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis) + joystickId);
    Remove(up_, joyEventId);
    Remove(down_, joyEventId);
    Remove(left_, joyEventId);
    Remove(right_, joyEventId);

    // Cancel DPad states.
    const auto dpadEventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId);
    Remove(up_, dpadEventId);
    Remove(down_, dpadEventId);
    Remove(left_, dpadEventId);
    Remove(right_, dpadEventId);
}

void DirectionalPadAdapter::Append(AggregatedState& activeInput, InputType input)
{
    if (activeInput.Append(input))
    {
        SendKeyDown(activeInput.scancode_, false);
        activeInput.timeToRepeat_ = repeatDelay_;
    }
}

void DirectionalPadAdapter::Remove(AggregatedState& activeInput, InputType input)
{
    if (activeInput.Remove(input))
    {
        SendKeyUp(activeInput.scancode_);
    }
}

void DirectionalPadAdapter::RemoveIf(
    AggregatedState& activeInput, const ea::function<bool(InputType)>& pred)
{

    if (activeInput.RemoveIf(pred))
    {
        SendKeyUp(activeInput.scancode_);
    }
}

void DirectionalPadAdapter::UpdateSubscriptions(DirectionalPadAdapterFlags flags)
{
    const auto toSubscribe = flags & ~subscriptionFlags_;
    const auto toUnsubscribe = subscriptionFlags_ & ~flags;

    if (!subscriptionFlags_ && flags)
    {
        SubscribeToEvent(input_, E_INPUTFOCUS, &DirectionalPadAdapter::HandleInputFocus);
    }
    else if (subscriptionFlags_ && !flags)
    {
        UnsubscribeFromEvent(input_, E_INPUTFOCUS);
    }

    subscriptionFlags_ = flags;
    if (toSubscribe & DirectionalPadAdapterMask::KeyRepeat)
    {
        SubscribeToEvent(E_UPDATE, &DirectionalPadAdapter::HandleUpdate);
    }
    else if (toUnsubscribe & DirectionalPadAdapterMask::Keyboard)
    {
        UnsubscribeFromEvent(E_UPDATE);
    }

    if (toSubscribe & DirectionalPadAdapterMask::Keyboard)
    {
        SubscribeToEvent(input_, E_KEYUP, &DirectionalPadAdapter::HandleKeyUp);
        SubscribeToEvent(input_, E_KEYDOWN, &DirectionalPadAdapter::HandleKeyDown);
    }
    else if (toUnsubscribe & DirectionalPadAdapterMask::Keyboard)
    {
        UnsubscribeFromEvent(E_KEYUP);
        UnsubscribeFromEvent(E_KEYDOWN);

        ea::function<bool(InputType)> pred = [](InputType i) { return i == InputType::Keyboard; };
        RemoveIf(up_, pred);
        RemoveIf(down_, pred);
        RemoveIf(left_, pred);
        RemoveIf(right_, pred);
    }
    if (toSubscribe & DirectionalPadAdapterMask::Joystick)
    {
        SubscribeToEvent(input_, E_JOYSTICKAXISMOVE, &DirectionalPadAdapter::HandleJoystickAxisMove);
        SubscribeToEvent(input_, E_JOYSTICKHATMOVE, &DirectionalPadAdapter::HandleJoystickHatMove);
        SubscribeToEvent(input_, E_JOYSTICKDISCONNECTED, &DirectionalPadAdapter::HandleJoystickDisconnected);
    }
    else if (toUnsubscribe & DirectionalPadAdapterMask::Joystick)
    {
        UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
        UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
        UnsubscribeFromEvent(E_JOYSTICKDISCONNECTED);

        ea::function<bool(InputType)> pred = [](InputType i) { return i >= InputType::JoystickAxis; };
        RemoveIf(up_, pred);
        RemoveIf(down_, pred);
        RemoveIf(left_, pred);
        RemoveIf(right_, pred);
    }
}

} // namespace Urho3D
