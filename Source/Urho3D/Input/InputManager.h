//
// Copyright (c) 2022 the rbfx project.
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
#include "../Input/Input.h"

namespace Urho3D
{

/// Input device types.
enum class URHO3D_API Device
{
    None = 0,
    Mouse,
    Keyboard,
    Joystick,
};

/// Controller index specification. A maximum of 3 is supported.
enum class URHO3D_API JoystickIndex
{
    JoystickOne = 0,
    JoystickTwo,
    JoystickThree
};

/// Structure that defines all input event key data.
struct URHO3D_API InputLayout
{
    Device DeviceType = Device::Keyboard;

    Key KeyCode = Key::KEY_UNKNOWN;
    Scancode ScanCode = Scancode::SCANCODE_UNKNOWN;
    MouseButton MouseButton = MouseButton::MOUSEB_NONE;
    ControllerButton ControllerButton;
    ControllerAxis ControllerAxis;
    HatPosition HatPosition;

    float AxisScale = 1.0f;
    float DeadZone = 0.01f;

    Qualifier InputQualifier = Qualifier::QUAL_NONE;
    JoystickIndex ControllerIndex = JoystickIndex::JoystickOne;
};

/// Input Layout to store bound input keys to an event.
struct URHO3D_API InputLayoutDesc
{
    bool Enabled = true;

    ea::vector<InputLayout> Layout;
    ea::string InputEventName = "InputEvent";
};

class URHO3D_API InputManager : public Object
{
    URHO3D_OBJECT(InputManager, Object);

public:
    /// Construct.
    explicit InputManager(Context* context);
    /// Destruct.
    ~InputManager() override;

    /// Creates an Input group for input events.
    bool CreateGroup(const char* groupName);
    /// Removes an Input group.
    bool RemoveGroup(const char* groupName);

    /// Adds an input event to an input group. A new group will be created if it does not exist.
    bool AddInputMapping(const char* groupName, InputLayoutDesc& inputLayout);
    /// Removes an input event form an input group. The group will be cleared when empty if enabled.
    bool RemoveInputMapping(const char* groupName, const char* eventName, bool clearIfEmpty = false);

    /// Checks if an input event was pressed.
    bool WasPressed(const char* groupName, const char* eventName);
    /// Checks if an input event was held.
    bool WasDown(const char* groupName, const char* eventName);
    /// Returns the current value state of an input event.
    float GetAxis(const char* groupName, const char* eventName);
    /// Returns the Hat Position of a Joystick input event.
    float GetHatPosition(const char* groupName, const char* eventName);

    /// Save Input map to JSON file.
    bool SaveInputMapToFile(ea::string filePath);
    /// Load Input map from JSON file.
    bool LoadInputMapFromFile(ea::string filePath);

private:
    // Input map containing all input mappings, as well as their mapping groups.
    ea::unordered_map<const char*, ea::vector<InputLayoutDesc>> m_InputMap;
    /// Reference to the Input object.
    WeakPtr<Input> m_Input;
};
} // namespace Urho3D
