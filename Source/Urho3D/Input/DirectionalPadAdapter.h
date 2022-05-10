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
#pragma once

#include "../Core/Object.h"
#include "InputConstants.h"
#include <EASTL/fixed_vector.h>

namespace Urho3D
{

/// Adapter to translate gamepad axis and dpad messages along with keyboard (WASD and arrows) and externally provided directions into keyboard arrow messages.
class URHO3D_API DirectionalPadAdapter : public Object
{
    URHO3D_OBJECT(DirectionalPadAdapter, Object)

private:
    enum class InputType
    {
        External = 0,
        Keyboard = 1,
        JoystickAxis = 100,
        JoystickDPad = 200,
    };

    typedef ea::fixed_vector<InputType, 4> InputVector;

    enum class SubscriptionMask
    {
        None = 0,
        Keyboard = 1 << 0,
        Joystick = 1 << 1,
        All = Keyboard | Joystick,
    };

    URHO3D_NESTED_FLAGSET(SubscriptionMask, SubscriptionFlags);

public:
    /// Construct.
    explicit DirectionalPadAdapter(Context* context);

    /// Set enabled flag. The object subscribes for events when enabled.
    void SetEnabled(bool enabled);
    /// Set keyboard enabled flag.
    void SetKeyboardEnabled(bool enabled);
    /// Set joystick enabled flag.
    void SetJoystickEnabled(bool enabled);

    /// Get enabled flag.
    bool IsEnabled() const { return enabled_; }
    /// Get keyboard enabled flag.
    bool IsKeyboardEnabled() const { return enabledSubscriptions_ & SubscriptionMask::Keyboard; }
    /// Set joystick enabled flag.
    bool IsJoystickEnabled(bool enabled) const { return enabledSubscriptions_ & SubscriptionMask::Joystick; }

    /// Check if a key is held down by Key code. Only Up, Down, Left and Right keys are supported.
    bool GetKeyDown(Key key) const;

    /// Check if a key is held down by scancode. Only Up, Down, Left and Right scancodes are supported.
    bool GetScancodeDown(Scancode scancode) const;

private:
    void UpdateSubscriptions(SubscriptionFlags flags);

    void HandleKeyDown(StringHash eventType, VariantMap& args);
    void HandleKeyUp(StringHash eventType, VariantMap& args);
    void HandleJoystickAxisMove(StringHash eventType, VariantMap& args);
    void HandleJoystickHatMove(StringHash eventType, VariantMap& args);
    void HandleJoystickDisconnected(StringHash eventType, VariantMap& args);


    // Append input to set.
    void Append(InputVector& activeInput, InputType input, Scancode scancode);
    // Remove input from set.
    void Remove(InputVector& activeInput, InputType input, Scancode scancode);
    // Remove input from set.
    void RemoveIf(InputVector& activeInput, const ea::function<bool(InputType)>& pred, Scancode scancode);

    void SendKeyUp(Scancode key);
    void SendKeyDown(Scancode key);

    /// Is adapter enabled
    bool enabled_{false};
    /// Enabled subscriptions
    SubscriptionFlags enabledSubscriptions_{SubscriptionMask::All};
    /// Active subscriptions bitmask
    SubscriptionFlags subscriptionFlags_{SubscriptionMask::None};
    Input* input_{};
    InputVector up_;
    InputVector down_;
    InputVector left_;
    InputVector right_;
    int ignoreJoystickId_{ea::numeric_limits<int>::lowest()};
};

} // namespace Urho3D
