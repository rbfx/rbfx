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

void DirectionalPadAdapter::SubscribeToEvents()
{
    SubscribeToEvent(input_, E_KEYUP, URHO3D_HANDLER(DirectionalPadAdapter, HandleKeyUp));
    SubscribeToEvent(input_, E_KEYDOWN, URHO3D_HANDLER(DirectionalPadAdapter, HandleKeyDown));
    SubscribeToEvent(input_, E_JOYSTICKAXISMOVE, URHO3D_HANDLER(DirectionalPadAdapter, HandleJoystickAxisMove));
    SubscribeToEvent(input_, E_JOYSTICKHATMOVE, URHO3D_HANDLER(DirectionalPadAdapter, HandleJoystickHatMove));
    SubscribeToEvent(input_, E_JOYSTICKDISCONNECTED, URHO3D_HANDLER(DirectionalPadAdapter, HandleJoystickDisconnected));
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
    SendEvent(E_KEYDOWN, args, false);
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
    SendEvent(E_KEYUP, args, false);
}

/// Get aggregated direction vector.
Vector2 DirectionalPadAdapter::GetDirection() const
{
    Vector2 res = Vector2::ZERO;

    for (const AxisState& activeState : horizontalAxis_)
        res.x_ += activeState.value_;
    for (const AxisState& activeState : verticalAxis_)
        res.y_ += activeState.value_;

    return res;
}

void DirectionalPadAdapter::HandleKeyDown(StringHash eventType, VariantMap& args)
{
    using namespace KeyDown;
    switch (args[P_SCANCODE].GetUInt())
    {
    case SCANCODE_W:
    case SCANCODE_UP:
        Append(up_, InputType::Keyboard, SCANCODE_UP);
        UpdateAxis(verticalAxis_, {InputType::Keyboard, -1});
        break;
    case SCANCODE_S:
    case SCANCODE_DOWN:
        Append(down_, InputType::Keyboard, SCANCODE_DOWN);
        UpdateAxis(verticalAxis_, {InputType::Keyboard, 1});
        break;
    case SCANCODE_A:
    case SCANCODE_LEFT:
        Append(left_, InputType::Keyboard, SCANCODE_LEFT);
        UpdateAxis(horizontalAxis_, {InputType::Keyboard, -1});
        break;
    case SCANCODE_D:
    case SCANCODE_RIGHT:
        Append(right_, InputType::Keyboard, SCANCODE_RIGHT);
        UpdateAxis(horizontalAxis_, {InputType::Keyboard, 1});
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
        UpdateAxis(verticalAxis_, {InputType::Keyboard, 0.0f});
        break;
    case SCANCODE_S:
    case SCANCODE_DOWN:
        Remove(down_, InputType::Keyboard, SCANCODE_DOWN);
        UpdateAxis(verticalAxis_, {InputType::Keyboard, 0.0f});
        break;
    case SCANCODE_A:
    case SCANCODE_LEFT:
        Remove(left_, InputType::Keyboard, SCANCODE_LEFT);
        UpdateAxis(horizontalAxis_, {InputType::Keyboard, 0.0f});
        break;
    case SCANCODE_D:
    case SCANCODE_RIGHT:
        Remove(right_, InputType::Keyboard, SCANCODE_RIGHT);
        UpdateAxis(horizontalAxis_, {InputType::Keyboard, 0.0f});
        break;
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
        UpdateAxis(verticalAxis_, {eventId, value});
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
        UpdateAxis(horizontalAxis_, {eventId, value});
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
    UpdateAxis(horizontalAxis_, {eventId, horizontalValue});
    const float verticalValue = ((position & HAT_DOWN) ? 1.0f : 0.0f) + ((position & HAT_UP) ? -1.0f : 0.0f);
    UpdateAxis(verticalAxis_, {eventId, verticalValue});
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
    UpdateAxis(verticalAxis_, {joyEventId, 0.0f});
    UpdateAxis(horizontalAxis_, {joyEventId, 0.0f});

    // Cancel DPad states.
    const auto dpadEventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId);
    Remove(up_, dpadEventId, SCANCODE_UP);
    Remove(down_, dpadEventId, SCANCODE_DOWN);
    Remove(left_, dpadEventId, SCANCODE_LEFT);
    Remove(right_, dpadEventId, SCANCODE_RIGHT);
    UpdateAxis(verticalAxis_, {dpadEventId, 0.0f});
    UpdateAxis(horizontalAxis_, {dpadEventId, 0.0f});
}

void DirectionalPadAdapter::UpdateAxis(ea::fixed_vector<AxisState, 4>& activeStates, AxisState state)
{
    for (AxisState& activeState : activeStates)
    {
        if (activeState.input_ == state.input_)
        {
            if (Abs(state.value_) < axisDeadZone_)
                activeStates.erase_unsorted(&activeState);
            else
                activeState.value_ = state.value_;
            return;
        }
    }
    activeStates.push_back(state);
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

void DirectionalPadAdapter::UnsubscribeFromEvents()
{
    UnsubscribeFromEvent(E_KEYUP);
    UnsubscribeFromEvent(E_KEYDOWN);
    UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
    UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
    UnsubscribeFromEvent(E_JOYSTICKDISCONNECTED);

    if (!up_.empty())
    {
        SendKeyUp(SCANCODE_UP);
        up_.clear();
    }
    if (!down_.empty())
    {
        SendKeyUp(SCANCODE_DOWN);
        down_.clear();
    }
    if (!left_.empty())
    {
        SendKeyUp(SCANCODE_LEFT);
        left_.clear();
    }
    if (!right_.empty())
    {
        SendKeyUp(SCANCODE_RIGHT);
        right_.clear();
    }
}

} // namespace Urho3D
