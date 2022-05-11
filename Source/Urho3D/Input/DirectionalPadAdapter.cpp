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

#include "../Input/DirectionalPadAdapter.h"
#include "../Core/Context.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"

namespace Urho3D
{
using namespace DirectionalPadAdapterDetail;

/// Construct.
DirectionalPadAdapter::DirectionalPadAdapter(Context* context)
    : Object(context)
    , input_(context->GetSubsystem<Input>())
{
    unsigned numJoysticks = input_->GetNumJoysticks();
    for (unsigned i = 0; i < numJoysticks; ++i)
    {
        auto* joystick = input_->GetJoystickByIndex(i);
        if (joystick->GetNumAxes() == 3 && joystick->GetNumButtons() == 0 && joystick->GetNumHats() == 0)
        {
            ignoreJoystickId_ = joystick->joystickID_;
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
            UpdateSubscriptions(SubscriptionMask::None);
        }
    }
}


/// Set keyboard enabled flag.
void DirectionalPadAdapter::SetKeyboardEnabled(bool enabled)
{
    if (enabled)
    {
        enabledSubscriptions_ |= SubscriptionMask::Keyboard;
    }
    else
    {
        enabledSubscriptions_ &= ~SubscriptionMask::Keyboard;
    }
    if (IsEnabled())
        UpdateSubscriptions(enabledSubscriptions_);
}

/// Set joystick enabled flag.
void DirectionalPadAdapter::SetJoystickEnabled(bool enabled)
{
    if (enabled)
    {
        enabledSubscriptions_ |= SubscriptionMask::Joystick;
    }
    else
    {
        enabledSubscriptions_ &= ~SubscriptionMask::Joystick;
    }
    if (IsEnabled())
        UpdateSubscriptions(enabledSubscriptions_);
}


bool DirectionalPadAdapter::GetKeyDown(Key key) const
{
    return GetScancodeDown(input_->GetScancodeFromKey(key));
}

bool DirectionalPadAdapter::GetScancodeDown(Scancode scancode) const
{
    switch (scancode)
    {
    case SCANCODE_UP: return !up_.empty();
    case SCANCODE_DOWN: return !down_.empty();
    case SCANCODE_LEFT: return !left_.empty();
    case SCANCODE_RIGHT: return !right_.empty();
    }
    return false;
}

void DirectionalPadAdapter::SendKeyDown(Scancode key)
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
    args[P_REPEAT] = false;
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

void DirectionalPadAdapter::HandleKeyDown(StringHash eventType, VariantMap& args)
{
    using namespace KeyDown;
    switch (args[P_SCANCODE].GetUInt())
    {
    case SCANCODE_W:
    case SCANCODE_UP:
        Append(up_, InputType::Keyboard, SCANCODE_UP);
        break;
    case SCANCODE_S:
    case SCANCODE_DOWN:
        Append(down_, InputType::Keyboard, SCANCODE_DOWN);
        break;
    case SCANCODE_A:
    case SCANCODE_LEFT:
        Append(left_, InputType::Keyboard, SCANCODE_LEFT);
        break;
    case SCANCODE_D:
    case SCANCODE_RIGHT:
        Append(right_, InputType::Keyboard, SCANCODE_RIGHT);
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
        Remove(up_, InputType::Keyboard, SCANCODE_UP);
        break;
    case SCANCODE_S:
    case SCANCODE_DOWN:
        Remove(down_, InputType::Keyboard, SCANCODE_DOWN);
        break;
    case SCANCODE_A:
    case SCANCODE_LEFT:
        Remove(left_, InputType::Keyboard, SCANCODE_LEFT);
        break;
    case SCANCODE_D:
    case SCANCODE_RIGHT:
        Remove(right_, InputType::Keyboard, SCANCODE_RIGHT);
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
        if (value > 0.6f)
        {
            Append(down_, eventId, SCANCODE_DOWN);
            Remove(up_, eventId, SCANCODE_UP);
        }
        else if (value < -0.6f)
        {
            Remove(down_, eventId, SCANCODE_DOWN);
            Append(up_, eventId, SCANCODE_UP);
        }
        else if (value > -0.4f && value < 0.4f)
        {
            Remove(up_, eventId, SCANCODE_UP);
            Remove(down_, eventId, SCANCODE_DOWN);
        }
    }
    // Left-Right
    if (axisIndex == 0)
    {
        if (value > 0.6f)
        {
            Append(right_, eventId, SCANCODE_RIGHT);
            Remove(left_, eventId, SCANCODE_LEFT);
        }
        else if (value < -0.6f)
        {
            Append(left_, eventId, SCANCODE_LEFT);
            Remove(right_, eventId, SCANCODE_RIGHT);
        }
        else if (value > -0.4f && value < 0.4f)
        {
            Remove(right_, eventId, SCANCODE_RIGHT);
            Remove(left_, eventId, SCANCODE_LEFT);
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
        Append(up_, eventId, SCANCODE_UP);
    else
        Remove(up_, eventId, SCANCODE_UP);

    if (0 != (position & HAT_DOWN))
        Append(down_, eventId, SCANCODE_DOWN);
    else
        Remove(down_, eventId, SCANCODE_DOWN);

    if (0 != (position & HAT_LEFT))
        Append(left_, eventId, SCANCODE_LEFT);
    else
        Remove(left_, eventId, SCANCODE_LEFT);

    if (0 != (position & HAT_RIGHT))
        Append(right_, eventId, SCANCODE_RIGHT);
    else
        Remove(right_, eventId, SCANCODE_RIGHT);

    const float horizontalValue = ((position & HAT_RIGHT) ? 1.0f : 0.0f) + ((position & HAT_LEFT) ? -1.0f : 0.0f);
    const float verticalValue = ((position & HAT_DOWN) ? 1.0f : 0.0f) + ((position & HAT_UP) ? -1.0f : 0.0f);
}

void DirectionalPadAdapter::HandleJoystickDisconnected(StringHash eventType, VariantMap& args)
{
    using namespace JoystickDisconnected;
    auto joystickId = args[P_JOYSTICKID].GetUInt();

    // Cancel Axis states.
    const auto joyEventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis) + joystickId);
    Remove(up_, joyEventId, SCANCODE_UP);
    Remove(down_, joyEventId, SCANCODE_DOWN);
    Remove(left_, joyEventId, SCANCODE_LEFT);
    Remove(right_, joyEventId, SCANCODE_RIGHT);

    // Cancel DPad states.
    const auto dpadEventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId);
    Remove(up_, dpadEventId, SCANCODE_UP);
    Remove(down_, dpadEventId, SCANCODE_DOWN);
    Remove(left_, dpadEventId, SCANCODE_LEFT);
    Remove(right_, dpadEventId, SCANCODE_RIGHT);
}

void DirectionalPadAdapter::Append(InputVector& activeInput, InputType input, Scancode scancode)
{
    for (unsigned i = 0; i < activeInput.size(); ++i)
    {
        if (activeInput[i] == input)
        {
            return;
        }
    }
    activeInput.push_back(input);
    if (activeInput.size() == 1)
    {
        SendKeyDown(scancode);
    }
}

void DirectionalPadAdapter::Remove(InputVector& activeInput, InputType input, Scancode scancode)
{
    for (unsigned i = 0; i < activeInput.size(); ++i)
    {
        if (activeInput[i] == input)
        {
            activeInput.erase_unsorted(activeInput.begin() + i);
            if (activeInput.empty())
            {
                SendKeyUp(scancode);
            }
        }
    }
}

void DirectionalPadAdapter::RemoveIf(InputVector& activeInput, const ea::function<bool(InputType)>& pred, Scancode scancode)
{
    if (activeInput.empty())
        return;

    for (int i = static_cast<int>(activeInput.size())-1; i >=0; --i)
    {
        if (pred(activeInput[i]))
        {
            activeInput.erase_unsorted(activeInput.begin() + i);
            if (activeInput.empty())
            {
                SendKeyUp(scancode);
            }
        }
    }
}

void DirectionalPadAdapter::UpdateSubscriptions(SubscriptionFlags flags)
{
    const auto toSubscribe = flags & ~subscriptionFlags_;
    const auto toUnsubscribe = subscriptionFlags_ & ~flags;
    subscriptionFlags_ = flags;
    if (toSubscribe & SubscriptionMask::Keyboard)
    {
        SubscribeToEvent(input_, E_KEYUP, URHO3D_HANDLER(DirectionalPadAdapter, HandleKeyUp));
        SubscribeToEvent(input_, E_KEYDOWN, URHO3D_HANDLER(DirectionalPadAdapter, HandleKeyDown));
    }
    else if (toUnsubscribe & SubscriptionMask::Keyboard)
    {
        UnsubscribeFromEvent(E_KEYUP);
        UnsubscribeFromEvent(E_KEYDOWN);

        ea::function<bool(InputType)> pred = [](InputType i) { return i == InputType::Keyboard; };
        RemoveIf(up_, pred, SCANCODE_UP);
        RemoveIf(down_, pred, SCANCODE_DOWN);
        RemoveIf(left_, pred, SCANCODE_LEFT);
        RemoveIf(right_, pred, SCANCODE_RIGHT);
    }
    if (toSubscribe & SubscriptionMask::Joystick)
    {
        SubscribeToEvent(input_, E_JOYSTICKAXISMOVE, URHO3D_HANDLER(DirectionalPadAdapter, HandleJoystickAxisMove));
        SubscribeToEvent(input_, E_JOYSTICKHATMOVE, URHO3D_HANDLER(DirectionalPadAdapter, HandleJoystickHatMove));
        SubscribeToEvent(input_, E_JOYSTICKDISCONNECTED, URHO3D_HANDLER(DirectionalPadAdapter, HandleJoystickDisconnected));
    }
    else if (toUnsubscribe & SubscriptionMask::Joystick)
    {
        UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
        UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
        UnsubscribeFromEvent(E_JOYSTICKDISCONNECTED);

        ea::function<bool(InputType)> pred = [](InputType i) { return i >= InputType::JoystickAxis; };
        RemoveIf(up_, pred, SCANCODE_UP);
        RemoveIf(down_, pred, SCANCODE_DOWN);
        RemoveIf(left_, pred, SCANCODE_LEFT);
        RemoveIf(right_, pred, SCANCODE_RIGHT);
    }
}

} // namespace Urho3D
