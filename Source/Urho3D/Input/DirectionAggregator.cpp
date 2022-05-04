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

#include "../Core/Context.h"
#include "../Input/DirectionAggregator.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"

namespace Urho3D
{
/// Construct.
DirectionAggregator::DirectionAggregator(Context* context)
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
/// Input messages could be passed to the object manually when disabled.
void DirectionAggregator::SetEnabled(bool enabled)
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

void DirectionAggregator::SubscribeToEvents()
{
    SubscribeToEvent(input_, E_KEYUP, URHO3D_HANDLER(DirectionAggregator, HandleKeyUp));
    SubscribeToEvent(input_, E_KEYDOWN, URHO3D_HANDLER(DirectionAggregator, HandleKeyDown));
    SubscribeToEvent(input_, E_JOYSTICKAXISMOVE, URHO3D_HANDLER(DirectionAggregator, HandleJoystickAxisMove));
    SubscribeToEvent(input_, E_JOYSTICKHATMOVE, URHO3D_HANDLER(DirectionAggregator, HandleJoystickHatMove));
    SubscribeToEvent(input_, E_JOYSTICKDISCONNECTED, URHO3D_HANDLER(DirectionAggregator, HandleJoystickDisconnected));
}

/// Get aggregated direction vector.
Vector2 DirectionAggregator::GetDirection() const
{
    Vector2 res = Vector2::ZERO;

    if (!horizontalAxis_.empty())
    {
        for (const AxisState& activeState : horizontalAxis_)
            res.x_ += activeState.value_;
        res.x_ *= 1.0f / horizontalAxis_.size();
    }
    if (!verticalAxis_.empty())
    {
        for (const AxisState& activeState : verticalAxis_)
            res.y_ += activeState.value_;
        res.x_ *= 1.0f / verticalAxis_.size();
    }

    return res;
}

void DirectionAggregator::HandleKeyDown(StringHash eventType, VariantMap& args)
{
    using namespace KeyDown;
    switch (args[P_SCANCODE].GetUInt())
    {
    case SCANCODE_W:
    case SCANCODE_UP:
        UpdateAxis(verticalAxis_, {InputType::Keyboard, -1});
        break;
    case SCANCODE_S:
    case SCANCODE_DOWN:
        UpdateAxis(verticalAxis_, {InputType::Keyboard, 1});
        break;
    case SCANCODE_A:
    case SCANCODE_LEFT:
        UpdateAxis(horizontalAxis_, {InputType::Keyboard, -1});
        break;
    case SCANCODE_D:
    case SCANCODE_RIGHT:
        UpdateAxis(horizontalAxis_, {InputType::Keyboard, 1});
        break;
    }
}

void DirectionAggregator::HandleKeyUp(StringHash eventType, VariantMap& args)
{
    using namespace KeyUp;
    switch (args[P_SCANCODE].GetUInt())
    {
    case SCANCODE_W:
    case SCANCODE_UP:
        UpdateAxis(verticalAxis_, {InputType::Keyboard, 0.0f});
        break;
    case SCANCODE_S:
    case SCANCODE_DOWN:
        UpdateAxis(verticalAxis_, {InputType::Keyboard, 0.0f});
        break;
    case SCANCODE_A:
    case SCANCODE_LEFT:
        UpdateAxis(horizontalAxis_, {InputType::Keyboard, 0.0f});
        break;
    case SCANCODE_D:
    case SCANCODE_RIGHT:
        UpdateAxis(horizontalAxis_, {InputType::Keyboard, 0.0f});
        break;
    }
}

void DirectionAggregator::HandleJoystickAxisMove(StringHash eventType, VariantMap& args)
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
        UpdateAxis(verticalAxis_, {eventId, value});
    }
    // Left-Right
    if (axisIndex == 0)
    {
        UpdateAxis(horizontalAxis_, {eventId, value});
    }
}

void DirectionAggregator::HandleJoystickHatMove(StringHash eventType, VariantMap& args)
{
    using namespace JoystickHatMove;
    const auto hatIndex = args[P_HAT].GetUInt();
    if (hatIndex != 0)
        return;

    const auto joystickId = args[P_JOYSTICKID].GetUInt();
    const auto eventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId);

    const auto position = args[P_POSITION].GetUInt();

    const float horizontalValue = ((position & HAT_RIGHT) ? 1.0f : 0.0f) + ((position & HAT_LEFT) ? -1.0f : 0.0f);
    UpdateAxis(horizontalAxis_, {eventId, horizontalValue});
    const float verticalValue = ((position & HAT_DOWN) ? 1.0f : 0.0f) + ((position & HAT_UP) ? -1.0f : 0.0f);
    UpdateAxis(verticalAxis_, {eventId, verticalValue});
}

void DirectionAggregator::HandleJoystickDisconnected(StringHash eventType, VariantMap& args)
{
    using namespace JoystickDisconnected;
    auto joystickId = args[P_JOYSTICKID].GetUInt();

    // Cancel Axis states.
    const auto joyEventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickAxis) + joystickId);
    UpdateAxis(verticalAxis_, {joyEventId, 0.0f});
    UpdateAxis(horizontalAxis_, {joyEventId, 0.0f});

    // Cancel DPad states.
    const auto dpadEventId = static_cast<InputType>(static_cast<unsigned>(InputType::JoystickDPad) + joystickId);
    UpdateAxis(verticalAxis_, {dpadEventId, 0.0f});
    UpdateAxis(horizontalAxis_, {dpadEventId, 0.0f});
}

void DirectionAggregator::UpdateAxis(ea::fixed_vector<AxisState, 4>& activeStates, AxisState state)
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

void DirectionAggregator::UnsubscribeFromEvents()
{
    UnsubscribeFromEvent(E_KEYUP);
    UnsubscribeFromEvent(E_KEYDOWN);
    UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
    UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
    UnsubscribeFromEvent(E_JOYSTICKDISCONNECTED);
}

} // namespace Urho3D
