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

#include <Urho3D/Input/DirectionAggregator.h>

using namespace Tests;

TEST_CASE("DirectionAggregator tests")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto* input = context->GetSubsystem<Input>();

    DirectionAggregator adapter(context);
    adapter.SetEnabled(true);
    adapter.SetDeadZone(0.1f);

    SECTION("Press S and Down at the same time")
    {
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_DOWN, KEY_DOWN);
        CHECK(Vector2(0.0f, 1.0f).Equals(adapter.GetDirection()));
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_S, KEY_S);
        CHECK(Vector2(0.0f, 1.0f).Equals(adapter.GetDirection()));
        // Release one key but the other key still considered as pressed.
        SendKeyEvent(input, E_KEYUP, SCANCODE_DOWN, KEY_DOWN);
        CHECK(Vector2(0.0f, 1.0f).Equals(adapter.GetDirection()));
        SendKeyEvent(input, E_KEYUP, SCANCODE_S, KEY_S);
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
    }
    SECTION("Press Left and Right at the same time")
    {
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_LEFT, KEY_LEFT);
        CHECK(Vector2(-1.0f, 0.0f).Equals(adapter.GetDirection()));
        // Press Right. The average value becomes 0
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_RIGHT, KEY_RIGHT);
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
        // Release left. The average becomes positive 1
        SendKeyEvent(input, E_KEYUP, SCANCODE_LEFT, KEY_LEFT);
        CHECK(Vector2(1.0f, 0.0f).Equals(adapter.GetDirection()));
        // Release Right. No buttons left.
        SendKeyEvent(input, E_KEYUP, SCANCODE_RIGHT, KEY_RIGHT);
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
    }
    SECTION("Press Left on keyboard and Right on DPad at the same time")
    {
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_LEFT, KEY_LEFT);
        CHECK(Vector2(-1.0f, 0.0f).Equals(adapter.GetDirection()));
        // Press Right on hat.
        SendDPadEvent(input, HAT_RIGHT);
        CHECK(Vector2(0.0f, 0.0f).Equals(adapter.GetDirection()));
        // Release left on keyboard.
        SendKeyEvent(input, E_KEYUP, SCANCODE_LEFT, KEY_LEFT);
        CHECK(Vector2(1.0f, 0.0f).Equals(adapter.GetDirection()));
        SendDPadEvent(input, HAT_CENTER);
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
    }
    SECTION("Axis input")
    {
        SendAxisEvent(input, 0, 0.8f);
        CHECK(Vector2(0.7f / 0.9f, 0.0f).Equals(adapter.GetDirection()));
        SendAxisEvent(input, 1, 0.9f);
        CHECK(Vector2(0.7f / 0.9f, 0.8f / 0.9f).Equals(adapter.GetDirection()));
        SendJoystickDisconnected(input, 0);
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
    }
    SECTION("Test disabling source")
    {
        SendAxisEvent(input, 0, 1.0f);
        CHECK(Vector2(1.0f, 0.0f).Equals(adapter.GetDirection()));
        adapter.SetSubscriptionMask(adapter.GetSubscriptionMask() & ~DirectionAggregatorMask::Joystick);
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
        SendAxisEvent(input, 0, 1.0f);
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
        adapter.SetSubscriptionMask(adapter.GetSubscriptionMask() | ~DirectionAggregatorMask::Joystick);
        CHECK(Vector2::ZERO.Equals(adapter.GetDirection()));
    }
}
