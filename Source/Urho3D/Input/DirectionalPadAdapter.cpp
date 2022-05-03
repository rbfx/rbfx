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
#include "../Input/InputEvents.h"
#include "../Input/Input.h"
#include "../Core/Context.h"

namespace Urho3D
{
/// Construct.
DirectionalPadAdapter::DirectionalPadAdapter(Context* context)
    : Object(context)
    , input_(context->GetSubsystem<Input>())
{
}

/// Set enabled flag. The object subscribes for events when enabled.
/// Input messages could be passed to the object manually when disabled.
void DirectionalPadAdapter::SetEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        if (enabled_)
        {
            SubscribeToEvents();
        }
        else
        {
            UnsubscribeFromEvents();
        }
    }
}

void DirectionalPadAdapter::SubscribeToEvents()
{
    SubscribeToEvent(input_, E_KEYUP, URHO3D_HANDLER(DirectionalPadAdapter, HandleKeyUp));
    SubscribeToEvent(input_, E_KEYDOWN, URHO3D_HANDLER(DirectionalPadAdapter, HandleKeyDown));
    SubscribeToEvent(input_, E_JOYSTICKAXISMOVE, URHO3D_HANDLER(DirectionalPadAdapter, HandleJoystickAxisMove));
    SubscribeToEvent(input_, E_JOYSTICKHATMOVE, URHO3D_HANDLER(DirectionalPadAdapter, HandleJoystickHatMove));
    SubscribeToEvent(input_, E_JOYSTICKDISCONNECTED, URHO3D_HANDLER(DirectionalPadAdapter, HandleJoystickDisconnected));
}

void DirectionalPadAdapter::HandleKeyDown(StringHash eventType, VariantMap& args)
{
    using namespace KeyDown;
    switch (args[P_SCANCODE].GetUInt())
    {
    case SCANCODE_W:
    case SCANCODE_UP: SendKeyDown(Append(forward_, InputType::Keyboard), SCANCODE_UP); break;
    case SCANCODE_S:
    case SCANCODE_DOWN: SendKeyDown(Append(backward_, InputType::Keyboard), SCANCODE_DOWN); break;
    case SCANCODE_A:
    case SCANCODE_LEFT: SendKeyDown(Append(left_, InputType::Keyboard), SCANCODE_LEFT); break;
    case SCANCODE_D:
    case SCANCODE_RIGHT: SendKeyDown(Append(right_, InputType::Keyboard), SCANCODE_RIGHT); break;
    }
}
void DirectionalPadAdapter::SendKeyDown(bool send, Scancode key)
{
    if (!send)
        return;

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

void DirectionalPadAdapter::SendKeyUp(bool send, Scancode key)
{
    if (!send)
        return;

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

void DirectionalPadAdapter::HandleKeyUp(StringHash eventType, VariantMap& args)
{
    using namespace KeyUp;
    switch (args[P_SCANCODE].GetUInt())
    {
    case SCANCODE_W:
    case SCANCODE_UP: SendKeyUp(Remove(forward_, InputType::Keyboard), SCANCODE_UP); break;
    case SCANCODE_S:
    case SCANCODE_DOWN: SendKeyUp(Remove(backward_, InputType::Keyboard), SCANCODE_DOWN); break;
    case SCANCODE_A:
    case SCANCODE_LEFT: SendKeyUp(Remove(left_, InputType::Keyboard), SCANCODE_LEFT); break;
    case SCANCODE_D:
    case SCANCODE_RIGHT: SendKeyUp(Remove(right_, InputType::Keyboard), SCANCODE_RIGHT); break;
    }
}

void DirectionalPadAdapter::HandleJoystickAxisMove(StringHash eventType, VariantMap& args)
{
    using namespace JoystickAxisMove;

    auto joystickId = args[P_JOYSTICKID].GetUInt();
    auto eventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis) + joystickId);

    auto axisIndex = args[P_AXIS].GetUInt();
    auto value = args[P_POSITION].GetFloat();

    // Up-Down
    if (axisIndex == 1)
    {
        if (value > 0.6f)
        {
            SendKeyDown(Append(backward_, eventId), SCANCODE_DOWN);
            SendKeyUp(Remove(forward_, eventId), SCANCODE_UP);
        }
        else if (value <-0.6f)
        {
            SendKeyUp(Remove(backward_, eventId), SCANCODE_DOWN);
            SendKeyDown(Append(forward_, eventId), SCANCODE_UP);
        }
        else if (value > -0.4f && value < 0.4f)
        {
            SendKeyUp(Remove(forward_, eventId), SCANCODE_UP);
            SendKeyUp(Remove(backward_, eventId), SCANCODE_DOWN);
        }
    }
    // Left-Right
    if (axisIndex == 0)
    {
        if (value > 0.6f)
        {
            SendKeyDown(Append(right_, eventId), SCANCODE_RIGHT);
            SendKeyUp(Remove(left_, eventId), SCANCODE_LEFT);
        }
        else if (value < -0.6f)
        {
            SendKeyDown(Append(left_, eventId), SCANCODE_LEFT);
            SendKeyUp(Remove(right_, eventId), SCANCODE_RIGHT);
        }
        else if (value > -0.4f && value < 0.4f)
        {
            SendKeyUp(Remove(right_, eventId), SCANCODE_RIGHT);
            SendKeyUp(Remove(left_, eventId), SCANCODE_LEFT);
        }
    }
}

void DirectionalPadAdapter::HandleJoystickHatMove(StringHash eventType, VariantMap& args)
{
    using namespace JoystickHatMove;
    auto hatIndex = args[P_HAT].GetUInt();
    if (hatIndex != 0)
        return;

    auto joystickId = args[P_JOYSTICKID].GetUInt();
    auto eventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId);

    auto position = args[P_POSITION].GetUInt();

    if (0 != (position & HAT_UP))
        SendKeyDown(Append(forward_, eventId), SCANCODE_UP);
    else
        SendKeyUp(Remove(forward_, eventId), SCANCODE_UP);

    if (0 != (position & HAT_DOWN))
        SendKeyDown(Append(backward_, eventId), SCANCODE_DOWN);
    else
        SendKeyUp(Remove(backward_, eventId), SCANCODE_DOWN);

    if (0 != (position & HAT_LEFT))
        SendKeyDown(Append(left_, eventId), SCANCODE_LEFT);
    else
        SendKeyUp(Remove(left_, eventId), SCANCODE_LEFT);

    if (0 != (position & HAT_RIGHT))
        SendKeyDown(Append(right_, eventId), SCANCODE_RIGHT);
    else
        SendKeyUp(Remove(right_, eventId), SCANCODE_RIGHT);
}

void DirectionalPadAdapter::HandleJoystickDisconnected(StringHash eventType, VariantMap& args)
{
    using namespace JoystickDisconnected;
    auto joystickId = args[P_JOYSTICKID].GetUInt();

    // Cancel Axis states.
    SendKeyUp(Remove(forward_, static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis) + joystickId)),
        SCANCODE_UP);
    SendKeyUp(Remove(backward_, static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis) + joystickId)),
        SCANCODE_DOWN);
    SendKeyUp(Remove(left_, static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis) + joystickId)),
        SCANCODE_LEFT);
    SendKeyUp(Remove(right_, static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis) + joystickId)),
        SCANCODE_RIGHT);

    // Cancel DPad states.
    SendKeyUp(Remove(forward_, static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId)),
        SCANCODE_UP);
    SendKeyUp(Remove(backward_, static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId)),
        SCANCODE_DOWN);
        SendKeyUp(Remove(left_, static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId)),
        SCANCODE_LEFT);
    SendKeyUp(Remove(right_, static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId)),
        SCANCODE_RIGHT);
}

bool DirectionalPadAdapter::Append(ea::vector<InputType>& activeInput, InputType input)
{
    for (unsigned i = 0; i < activeInput.size(); ++i)
    {
        if (activeInput[i] == input)
        {
            return false;
        }
    }
    activeInput.push_back(input);
    return activeInput.size() == 1;
}

bool DirectionalPadAdapter::Remove(ea::vector<InputType>& activeInput, InputType input)
{
    for (unsigned i = 0; i < activeInput.size(); ++i)
    {
        if (activeInput[i] == input)
        {
            activeInput.erase_unsorted(activeInput.begin() + i);
            return activeInput.empty();
        }
    }
    return false;
}

void DirectionalPadAdapter::UnsubscribeFromEvents()
{
    UnsubscribeFromEvent(E_KEYUP);
    UnsubscribeFromEvent(E_KEYDOWN);
    UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
    UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
    UnsubscribeFromEvent(E_JOYSTICKDISCONNECTED);

    if (!forward_.empty())
    {
        SendKeyUp(true, SCANCODE_UP);
        forward_.clear();
    }
    if (!backward_.empty())
    {
        SendKeyUp(true, SCANCODE_DOWN);
        backward_.clear();
    }
    if (!left_.empty())
    {
        SendKeyUp(true, SCANCODE_LEFT);
        left_.clear();
    }
    if (!right_.empty())
    {
        SendKeyUp(true, SCANCODE_RIGHT);
        right_.clear();
    }
}

} // namespace Urho3D
