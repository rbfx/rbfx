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

public:
    /// Construct.
    explicit DirectionalPadAdapter(Context* context);

    /// Set enabled flag. The object subscribes for events when enabled.
    /// Input messages could be passed to the object manually when disabled.
    void SetEnabled(bool enabled);

    bool IsEnabled() const { return enabled_; }

private:
    void SubscribeToEvents();

    void UnsubscribeFromEvents();

    void HandleKeyDown(StringHash eventType, VariantMap& args);
    void HandleKeyUp(StringHash eventType, VariantMap& args);
    void HandleJoystickAxisMove(StringHash eventType, VariantMap& args);
    void HandleJoystickHatMove(StringHash eventType, VariantMap& args);
    void HandleJoystickDisconnected(StringHash eventType, VariantMap& args);

    bool Append(ea::vector<InputType>& activeInput, InputType input);
    bool Remove(ea::vector<InputType>& activeInput, InputType input);

    void SendKeyUp(bool send, Scancode key);
    void SendKeyDown(bool send, Scancode key);

    bool enabled_{false};
    Input* input_{};
    ea::vector<InputType> forward_;
    ea::vector<InputType> backward_;
    ea::vector<InputType> left_;
    ea::vector<InputType> right_;
};

} // namespace Urho3D
