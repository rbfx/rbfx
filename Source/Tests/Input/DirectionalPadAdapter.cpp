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

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/DirectionalPadAdapter.h>

using namespace Tests;

TEST_CASE("DirectionalPadAdapter tests")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto* input = context->GetSubsystem<Input>();

    DirectionalPadAdapter adapter(context);
    adapter.SetEnabled(true);

    SECTION("Press S and Down at the same time")
    {
        CHECK(!adapter.GetScancodeDown(SCANCODE_DOWN));
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_DOWN, KEY_DOWN);
        CHECK(adapter.GetScancodeDown(SCANCODE_DOWN));
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_S, KEY_S);
        CHECK(adapter.GetScancodeDown(SCANCODE_DOWN));
        // Release Down. It releases keyboard "down" state.
        SendKeyEvent(input, E_KEYUP, SCANCODE_DOWN, KEY_DOWN);
        CHECK(!adapter.GetScancodeDown(SCANCODE_DOWN));
        SendKeyEvent(input, E_KEYUP, SCANCODE_S, KEY_S);
        CHECK(!adapter.GetScancodeDown(SCANCODE_DOWN));
    }
    SECTION("Press Left and Right at the same time")
    {
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_LEFT, KEY_LEFT);
        CHECK(adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        // Press Right. It overrides horizontal axis
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_RIGHT, KEY_RIGHT);
        CHECK(adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        // Release left. It overrides horizontal axis
        SendKeyEvent(input, E_KEYUP, SCANCODE_LEFT, KEY_LEFT);
        CHECK(!adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        SendKeyEvent(input, E_KEYUP, SCANCODE_RIGHT, KEY_RIGHT);
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
    }
    SECTION("Press Left on keyboard and Right on DPad at the same time")
    {
        SendKeyEvent(input, E_KEYDOWN, SCANCODE_LEFT, KEY_LEFT);
        CHECK(adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        // Press Right.
        SendDPadEvent(input, HAT_RIGHT);
        CHECK(adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        // Release left.
        SendKeyEvent(input, E_KEYUP, SCANCODE_LEFT, KEY_LEFT);
        CHECK(!adapter.GetScancodeDown(SCANCODE_LEFT));
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        SendDPadEvent(input, HAT_CENTER);
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
    }
    SECTION("Axis to dpad translation")
    {
        SendAxisEvent(input, 0, 0.8f);
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        SendAxisEvent(input, 1, 0.9f);
        CHECK(adapter.GetScancodeDown(SCANCODE_DOWN));
        SendJoystickDisconnected(input, 0);
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        CHECK(!adapter.GetScancodeDown(SCANCODE_DOWN));
    }
    SECTION("Test disabling source")
    {
        SendAxisEvent(input, 0, 0.8f);
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        adapter.SetSubscriptionMask(adapter.GetSubscriptionMask() & ~DirectionalPadAdapterMask::Joystick);
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        SendAxisEvent(input, 0, 1.0f);
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
        adapter.SetSubscriptionMask(adapter.GetSubscriptionMask() | DirectionalPadAdapterMask::Joystick);
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
    }
    SECTION("Test repeating")
    {
        adapter.SetSubscriptionMask(adapter.GetSubscriptionMask() | DirectionalPadAdapterMask::KeyRepeat);
        adapter.SetRepeatDelay(1.0f);
        adapter.SetRepeatInterval(0.5f);
        SendAxisEvent(input, 0, 0.8f);
        CHECK(adapter.GetScancodeDown(SCANCODE_RIGHT));
        auto obj = MakeShared<Node>(context);
        unsigned eventCounter = 0;
        obj->SubscribeToEvent(&adapter, E_KEYDOWN, [&] { ++eventCounter; });
        Tests::RunFrame(context, 0.9f, 1.0f);
        CHECK(eventCounter == 0); //Time 0.9, no event yet
        Tests::RunFrame(context, 0.2f, 1.0f);
        CHECK(eventCounter == 1); // Time 1.1, first repeat event arrives
        Tests::RunFrame(context, 0.3f, 1.0f);
        CHECK(eventCounter == 1); // Time 1.4, no new events
        Tests::RunFrame(context, 0.2f, 1.0f);
        CHECK(eventCounter == 2); // Time 1.6, one more repeat event arrives
        Tests::RunFrame(context, 2.4f, 5.0f);
        CHECK(eventCounter == 3); // Time 4.0, only one more repeat event arrives
        SendAxisEvent(input, 0, 0.0f);
        Tests::RunFrame(context, 2.0f, 5.0f);
        CHECK(eventCounter == 3); // Time 6.0, no new events arrive since "key" is released
        CHECK(!adapter.GetScancodeDown(SCANCODE_RIGHT));
    }
}
