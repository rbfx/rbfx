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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "../CommonUtils.h"
#include <Urho3D/Input/Input.h>

#include <Urho3D/Input/DirectionalPadAdapter.h>

namespace
{
void SendKeyEvent(Input* input, StringHash eventId, Scancode scancode, Key key)
{
    using namespace KeyDown;

    VariantMap args;
    args[P_BUTTONS] = 0;
    args[P_QUALIFIERS] = 0;
    args[P_KEY] = key;
    args[P_SCANCODE] = scancode;
    args[P_REPEAT] = false;
    input->SendEvent(eventId, args);
}

void SendDPadEvent(Input* input, HatPosition position)
{
    using namespace JoystickHatMove;

    VariantMap args;
    args[P_JOYSTICKID] = 0;
    args[P_HAT] = 0;
    args[P_POSITION] = (int)position;
    input->SendEvent(E_JOYSTICKHATMOVE, args);
}

void SendJoystickDisconnected(Input* input, int joystickId)
{
    using namespace JoystickDisconnected;

    VariantMap args;
    args[P_JOYSTICKID] = joystickId;
    input->SendEvent(E_JOYSTICKDISCONNECTED, args);
}

void SendAxisEvent(Input* input, int axis, float value)
{
    using namespace JoystickAxisMove;

    VariantMap args;
    args[P_JOYSTICKID] = 0;
    args[P_AXIS] = axis;
    args[P_POSITION] = value;
    input->SendEvent(E_JOYSTICKAXISMOVE, args);
}
} // namespace

TEST_CASE("DirectionalPadAdapter tests")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto* input = context->GetSubsystem<Input>();

    DirectionalPadAdapter adapter(context);
    adapter.SetEnabled(true);

    SECTION("Press S and Down at the same time")
    {
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
        CHECK(!adapter.GetScancodeDown(SCANCODE_DOWN));
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_DOWN, KEY_DOWN);
        CHECK(adapter.GetScancodeDown(SCANCODE_DOWN));
        CHECK(Vector2(0.0f, 1.0f).Equals(adapter.GetDirection()));
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_S, KEY_S);
        CHECK(adapter.GetScancodeDown(SCANCODE_DOWN));
        CHECK(Vector2(0.0f, 1.0f).Equals(adapter.GetDirection()));
        // Release Down. It releases keyboard "down" state.
        SendKeyEvent(input, E_KEYUP, SCANCODE_DOWN, KEY_DOWN);
        CHECK(!adapter.GetScancodeDown(SCANCODE_DOWN));
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
        SendKeyEvent(input, E_KEYUP, SCANCODE_S, KEY_S);
        CHECK(!adapter.GetScancodeDown(SCANCODE_DOWN));
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
    }
    SECTION("Press Left and Right at the same time")
    {
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_LEFT, KEY_LEFT);
        CHECK(adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(Vector2(-1.0f, 0.0f).Equals(adapter.GetDirection()));
        // Press Right. It overrides horizontal axis
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_RIGHT, KEY_RIGHT);
        CHECK(adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(Vector2(1.0f, 0.0f).Equals(adapter.GetDirection()));
        // Release left. It overrides horizontal axis
        SendKeyEvent(input, E_KEYUP, SCANCODE_LEFT, KEY_LEFT);
        CHECK(!adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(Vector2(0.0f, 0.0f).Equals(adapter.GetDirection()));
        SendKeyEvent(input, E_KEYUP, SCANCODE_RIGHT, KEY_RIGHT);
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
    }
    SECTION("Press Left on keyboard and Right on DPad at the same time")
    {
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_LEFT, KEY_LEFT);
        CHECK(adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(Vector2(-1.0f, 0.0f).Equals(adapter.GetDirection()));
        // Press Right.
        SendDPadEvent(input, HAT_RIGHT);
        CHECK(adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(Vector2(0.0f, 0.0f).Equals(adapter.GetDirection()));
        // Release left.
        SendKeyEvent(input, E_KEYUP, SCANCODE_LEFT, KEY_LEFT);
        CHECK(!adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(Vector2(1.0f, 0.0f).Equals(adapter.GetDirection()));
        SendDPadEvent(input, HAT_CENTER);
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
    }
    SECTION("Axis to dpad translation")
    {
        SendAxisEvent(input, 0, 0.8f);
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(Vector2(0.8f, 0.0f).Equals(adapter.GetDirection()));
        SendAxisEvent(input, 1, 0.9f);
        CHECK(adapter.GetScancodeDown(SCANCODE_DOWN));
        CHECK(Vector2(0.8f, 0.9f).Equals(adapter.GetDirection()));
        SendJoystickDisconnected(input, 0);
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(!adapter.GetScancodeDown(SCANCODE_DOWN));
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
    }
}
