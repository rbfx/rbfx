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

enum class DirectionalPadAdapterMask : unsigned
{
    None = 0,
    Keyboard = 1 << 0,
    Joystick = 1 << 1,
    KeyRepeat = 1 << 2,
    All = Keyboard | Joystick | KeyRepeat,
};
URHO3D_FLAGSET(DirectionalPadAdapterMask, DirectionalPadAdapterFlags);

/// Adapter to translate gamepad axis and DPad messages along with keyboard (WASD and arrows) and externally provided
/// directions into keyboard arrow messages.
///
/// DPadAdapter collects all inputs that it can categorize as a movement into specific direction. When at least one
/// input received it sends a corresponding keyboard message about arrow key been pressed. When last input is
/// released it sends a message about key been released. It can also be used as a substitute for Input as it implements
/// GetScancodeDown and GetKeyDown methods but it only supports arrow key and scan codes.
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

    struct AggregatedState
    {
        AggregatedState(Scancode scancode);

        Scancode scancode_;
        ea::fixed_vector<InputType, 4> activeSources_;
        float timeToRepeat_;

        bool Append(InputType inputType);
        bool Remove(InputType inputType);
        bool RemoveIf(const ea::function<bool(InputType)>& pred);
        bool IsActive() const { return !activeSources_.empty(); }
    };

public:
    /// Construct.
    explicit DirectionalPadAdapter(Context* context);

    /// Set enabled flag. The object subscribes for events when enabled.
    void SetEnabled(bool enabled);
    /// Set input device subscription mask.
    void SetSubscriptionMask(DirectionalPadAdapterFlags mask);
    /// Set axis upper threshold. Axis value greater than threshold is interpreted as key press.
    void SetAxisUpperThreshold(float threshold);
    /// Set axis lower threshold. Axis value lower than threshold is interpreted as key press.
    void SetAxisLowerThreshold(float threshold);
    /// Set repeat delay in seconds.
    void SetRepeatDelay(float delayInSeconds);
    /// Set repeat interval in seconds.
    void SetRepeatInterval(float intervalInSeconds);

    /// Get enabled flag.
    bool IsEnabled() const { return enabled_; }
    /// Get input device subscription mask.
    DirectionalPadAdapterFlags GetSubscriptionMask() const { return enabledSubscriptions_; }

    /// Check if a key is held down by Key code. Only Up, Down, Left and Right keys are supported.
    bool GetKeyDown(Key key) const;

    /// Check if a key is held down by scancode. Only Up, Down, Left and Right scancodes are supported.
    bool GetScancodeDown(Scancode scancode) const;

    /// Get axis upper threshold. Axis value greater than threshold is interpreted as key press.
    float GetAxisUpperThreshold() const { return axisUpperThreshold_; }
    /// Get axis lower threshold. Axis value lower than threshold is interpreted as key press.
    float GetAxisLowerThreshold() const { return axisLowerThreshold_; }
    /// Get repeat delay in seconds.
    float GetRepeatDelay() const { return repeatDelay_; }
    /// Get repeat interval in seconds.
    float GetRepeatInterval() const { return repeatInterval_; }

private:
    void UpdateSubscriptions(DirectionalPadAdapterFlags flags);

    void HandleInputFocus(StringHash eventType, VariantMap& args);
    void HandleUpdate(StringHash eventType, VariantMap& args);
    void HandleKeyDown(StringHash eventType, VariantMap& args);
    void HandleKeyUp(StringHash eventType, VariantMap& args);
    void HandleJoystickAxisMove(StringHash eventType, VariantMap& args);
    void HandleJoystickHatMove(StringHash eventType, VariantMap& args);
    void HandleJoystickDisconnected(StringHash eventType, VariantMap& args);

    // Append input to set.
    void Append(AggregatedState& activeInput, InputType input);
    // Remove input from set.
    void Remove(AggregatedState& activeInput, InputType input);
    // Remove input from set.
    void RemoveIf(AggregatedState& activeInput, const ea::function<bool(InputType)>& pred);

    void SendKeyUp(Scancode key);
    void SendKeyDown(Scancode key, bool repeat);

    /// Is adapter enabled
    bool enabled_{false};
    /// Enabled subscriptions
    DirectionalPadAdapterFlags enabledSubscriptions_{DirectionalPadAdapterMask::All};
    /// Active subscriptions bitmask
    DirectionalPadAdapterFlags subscriptionFlags_{DirectionalPadAdapterMask::None};
    Input* input_{};
    AggregatedState up_;
    AggregatedState down_;
    AggregatedState left_;
    AggregatedState right_;
    int ignoreJoystickId_{ea::numeric_limits<int>::lowest()};
    float axisUpperThreshold_{0.6f};
    float axisLowerThreshold_{0.4f};
    float repeatDelay_{0.5f};
    float repeatInterval_{0.03f};
};

} // namespace Urho3D
