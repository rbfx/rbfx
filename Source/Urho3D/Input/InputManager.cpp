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

#include "../Precompiled.h"

#include "../Engine/Engine.h"
#include "../Graphics/Graphics.h"
#include "../IO/Archive.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Input/InputManager.h"
#include "../Resource/JSONArchive.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"

namespace Urho3D
{
InputManager::InputManager(Context* context)
    : Object(context)
{
    CreateGroup("Default");
}

InputManager::~InputManager() { m_InputMap.clear(); }

bool InputManager::CreateGroup(const char* groupName)
{
    if (m_InputMap.find(groupName) != m_InputMap.end())
    {
        URHO3D_LOGINFO("Input group '{}' already exists", groupName);
        return false;
    }

    m_InputMap.insert(groupName);
    URHO3D_LOGINFO("Created new input group '{}'", groupName);
    return true;
}

bool InputManager::RemoveGroup(const char* groupName)
{
    if (m_InputMap.find(groupName) != m_InputMap.end())
    {
        m_InputMap.erase(groupName);
        URHO3D_LOGINFO("Removed and erased input group '{}'", groupName);
        return true;
    }

    URHO3D_LOGINFO("Input group '{}' was not found", groupName);
    return false;
}

bool InputManager::AddInputMapping(const char* groupName, InputLayoutDesc& inputLayout)
{
    if (m_InputMap.find(groupName) != m_InputMap.end())
    {
        // Input group already exists, insert at key into the group.
        auto& layoutDescArray = m_InputMap.at(groupName);
        layoutDescArray.push_back(inputLayout);

        URHO3D_LOGINFO("Added input event '{0}' to group '{1}'", groupName, inputLayout.InputEventName);
        return true;
    }

    // Create a group then add input layout to it.
    CreateGroup(groupName);
    auto& layoutDescArray = m_InputMap.at(groupName);
    layoutDescArray.push_back(inputLayout);
    URHO3D_LOGINFO("Added input event '{0}' to group '{1}'", groupName, inputLayout.InputEventName);

    return true;
}

bool InputManager::RemoveInputMapping(const char* groupName, const char* eventName, bool clearIfEmpty /*= false*/)
{
    if (m_InputMap.find(groupName) != m_InputMap.end())
    {
        // Find event name
        if (m_Input && m_InputMap.find(groupName) != m_InputMap.end())
        {
            auto& mappedLayout = m_InputMap.at(groupName);

            if (mappedLayout.empty())
                return false;

            for (int i = 0; i < mappedLayout.size(); ++i)
            {
                if (mappedLayout[i].InputEventName.c_str() == eventName)
                {
                    mappedLayout.erase_first(mappedLayout[i]);
                    URHO3D_LOGINFO("Erased event '{0}' from input group '{1}'", eventName, groupName);
                }
            }
        }

        if (clearIfEmpty)
        {
            bool isEmpty = m_InputMap.at(groupName).empty();
            if (isEmpty)
                m_InputMap.erase(groupName);
        }

        return true;
    }

    URHO3D_LOGINFO("Input group '{}' was not found", groupName);
    return false;
}

bool InputManager::WasPressed(const char* groupName, const char* eventName)
{
    // Find event name
    if (m_Input && m_InputMap.find(groupName) != m_InputMap.end())
    {
        auto& mappedLayout = m_InputMap.at(groupName);

        if (mappedLayout.empty())
            return false;

        for (auto& layoutDesc : mappedLayout)
        {
            if (layoutDesc.InputEventName.c_str() == eventName)
            {
                // Begin input press logic
                if (!layoutDesc.Enabled)
                {
                    URHO3D_LOGINFO("The event {0} of group {1} is disabled", eventName, groupName);
                    return false;
                }

                if (layoutDesc.Layout.empty())
                {
                    URHO3D_LOGINFO("The event {0} of group {1} does not contain an Input Layout", eventName, groupName);
                    return false;
                }

                for (auto& layout : layoutDesc.Layout)
                {
                    switch (layout.DeviceType)
                    {
                    case Device::Mouse:
                    {
                        // If no qualifier, return true if mouse button pressed. Else, return mouse button and qualifier
                        // ressed.
                        if (layout.InputQualifier == Qualifier::QUAL_NONE
                            || layout.InputQualifier == Qualifier::QUAL_ANY)
                        {
                            return m_Input->GetMouseButtonPress(layout.MouseButton)
                                || m_Input->GetMouseButtonClick(layout.MouseButton);
                        }
                        else
                        {
                            return m_Input->GetQualifierPress(layout.InputQualifier)
                                && (m_Input->GetMouseButtonPress(layout.MouseButton)
                                    || m_Input->GetMouseButtonClick(layout.MouseButton));
                        }

                        return false;
                    }
                    case Device::Keyboard:
                    {
                        if (layout.InputQualifier == Qualifier::QUAL_NONE
                            || layout.InputQualifier == Qualifier::QUAL_ANY)
                        {
                            return m_Input->GetKeyPress(layout.KeyCode) || m_Input->GetScancodePress(layout.ScanCode);
                        }
                        else
                        {
                            // Check all registered keys
                            if (m_Input->GetQualifierPress(layout.InputQualifier)
                                && m_Input->GetKeyPress(layout.KeyCode))
                            {
                                return true;
                            }

                            // Check all registered scancodes
                            if (m_Input->GetQualifierPress(layout.InputQualifier)
                                && m_Input->GetScancodePress(layout.ScanCode))
                            {
                                return true;
                            }

                            return false;
                        }

                        return false;
                    }
                    case Device::Joystick:
                    {
                        int numJoysticks = m_Input->GetNumJoysticks();

                        if (numJoysticks == 0)
                            return false;

                        int joystickIndex = static_cast<int>(layout.ControllerIndex);

                        if (joystickIndex < numJoysticks)
                        {
                            // Get joystick by joystick index
                            auto* joystick = m_Input->GetJoystickByIndex(joystickIndex);
                            if (joystick)
                            {
                                if (joystick->GetButtonPress(layout.ControllerButton))
                                {
                                    return true;
                                }
                            }
                        }

                        return false;
                    }
                    default: return false;
                    }
                }
            }
        }
    }

    URHO3D_LOGERROR("Could not find input mapping at {}", eventName);
    return false;
}

bool InputManager::WasDown(const char* groupName, const char* eventName)
{
    // Find event name
    if (m_Input && m_InputMap.find(groupName) != m_InputMap.end())
    {
        auto& mappedLayout = m_InputMap.at(groupName);

        if (mappedLayout.empty())
            return false;

        for (auto& layoutDesc : mappedLayout)
        {
            if (layoutDesc.InputEventName.c_str() == eventName)
            {
                // Begin input press logic
                if (!layoutDesc.Enabled)
                {
                    URHO3D_LOGINFO("The event {0} of group {1} is disabled", eventName, groupName);
                    return false;
                }

                if (layoutDesc.Layout.empty())
                {
                    URHO3D_LOGINFO("The event {0} of group {1} does not contain an Input Layout", eventName, groupName);
                    return false;
                }

                for (auto& layout : layoutDesc.Layout)
                {
                    switch (layout.DeviceType)
                    {
                    case Device::Mouse:
                    {
                        // If no qualifier, return true if mouse button pressed. Else, return mouse button and qualifier
                        // ressed.
                        if (layout.InputQualifier == Qualifier::QUAL_NONE
                            || layout.InputQualifier == Qualifier::QUAL_ANY)
                        {
                            return m_Input->GetMouseButtonDown(layout.MouseButton);
                        }
                        else
                        {
                            return m_Input->GetQualifierDown(layout.InputQualifier)
                                && m_Input->GetMouseButtonDown(layout.MouseButton);
                        }

                        return false;
                    }
                    case Device::Keyboard:
                    {
                        if (layout.InputQualifier == Qualifier::QUAL_NONE
                            || layout.InputQualifier == Qualifier::QUAL_ANY)
                        {
                            return m_Input->GetKeyDown(layout.KeyCode) || m_Input->GetScancodeDown(layout.ScanCode);
                        }
                        else
                        {
                            // Check all registered keys
                            if (m_Input->GetQualifierDown(layout.InputQualifier) && m_Input->GetKeyDown(layout.KeyCode))
                            {
                                return true;
                            }

                            // Check all registered scancodes
                            if (m_Input->GetQualifierDown(layout.InputQualifier)
                                && m_Input->GetScancodeDown(layout.ScanCode))
                            {
                                return true;
                            }

                            return false;
                        }

                        return false;
                    }
                    case Device::Joystick:
                    {
                        int numJoysticks = m_Input->GetNumJoysticks();

                        if (numJoysticks == 0)
                            return false;

                        int joystickIndex = static_cast<int>(layout.ControllerIndex);

                        if (joystickIndex < numJoysticks)
                        {
                            // Get joystick by joystick index
                            auto* joystick = m_Input->GetJoystickByIndex(joystickIndex);
                            if (joystick)
                            {
                                if (joystick->GetButtonDown(layout.ControllerButton))
                                {
                                    return true;
                                }
                            }
                        }
                        else
                        {
                            URHO3D_LOGERROR("Called event {0} of group {1} with an invalid controller index.");
                        }

                        return false;
                    }
                    default: return false;
                    }
                }
            }
        }
    }

    URHO3D_LOGERROR("Could not find input mapping at {}", eventName);
    return false;
}

float InputManager::GetAxis(const char* groupName, const char* eventName)
{
    // Find event name
    if (m_Input && m_InputMap.find(groupName) != m_InputMap.end())
    {
        auto& mappedLayout = m_InputMap.at(groupName);

        if (mappedLayout.empty())
            return false;

        for (auto& layoutDesc : mappedLayout)
        {
            if (layoutDesc.InputEventName.c_str() == eventName)
            {
                // Begin input press logic
                if (!layoutDesc.Enabled)
                {
                    URHO3D_LOGINFO("The event {0} of group {1} is disabled", eventName, groupName);
                    return false;
                }

                if (layoutDesc.Layout.empty())
                {
                    URHO3D_LOGINFO("The event {0} of group {1} does not contain an Input Layout", eventName, groupName);
                    return false;
                }

                for (auto& layout : layoutDesc.Layout)
                {
                    switch (layout.DeviceType)
                    {
                    case Device::Keyboard:
                    {
                        bool pressed = m_Input->GetKeyDown(layout.KeyCode) || m_Input->GetScancodeDown(layout.ScanCode);

                        return pressed ? layout.AxisScale : 0.0f;
                    }
                    case Device::Joystick:
                    {
                        int numJoysticks = m_Input->GetNumJoysticks();

                        if (numJoysticks == 0)
                            return 0.0f;

                        int joystickIndex = static_cast<int>(layout.ControllerIndex);

                        if (joystickIndex < numJoysticks)
                        {
                            // Get joystick by joystick index
                            auto* joystick = m_Input->GetJoystickByIndex(joystickIndex);
                            if (joystick)
                            {
                                float axisPosition = joystick->GetAxisPosition(layout.ControllerAxis);
                                if (Abs(axisPosition) >= layout.DeadZone)
                                {
                                    return axisPosition * layout.AxisScale;
                                }
                            }
                        }
                        else
                        {
                            URHO3D_LOGERROR("Called event {0} of group {1} with an invalid controller index.");
                        }
                    }
                    default: return 0.0f;
                    }
                }
            }
        }
    }

    URHO3D_LOGERROR("Could not find input mapping at {}", eventName);
    return 0.0f;
}

float InputManager::GetHatPosition(const char* groupName, const char* eventName)
{
    // Find event name
    if (m_Input && m_InputMap.find(groupName) != m_InputMap.end())
    {
        auto& mappedLayout = m_InputMap.at(groupName);

        if (mappedLayout.empty())
            return false;

        for (auto& layoutDesc : mappedLayout)
        {
            if (layoutDesc.InputEventName.c_str() == eventName)
            {
                // Begin input press logic
                if (!layoutDesc.Enabled)
                {
                    URHO3D_LOGINFO("The event {0} of group {1} is disabled", eventName, groupName);
                    return false;
                }

                if (layoutDesc.Layout.empty())
                {
                    URHO3D_LOGINFO("The event {0} of group {1} does not contain an Input Layout", eventName, groupName);
                    return false;
                }

                for (auto& layout : layoutDesc.Layout)
                {
                    switch (layout.DeviceType)
                    {
                    case Device::Joystick:
                    {
                        int numJoysticks = m_Input->GetNumJoysticks();

                        if (numJoysticks == 0)
                            return 0.0f;

                        int joystickIndex = static_cast<int>(layout.ControllerIndex);

                        if (joystickIndex < numJoysticks)
                        {
                            // Get joystick by joystick index
                            auto* joystick = m_Input->GetJoystickByIndex(joystickIndex);
                            if (joystick)
                            {
                                return joystick->GetHatPosition(layout.HatPosition);
                            }
                        }
                        else
                        {
                            URHO3D_LOGERROR("Called event {0} of group {1} with an invalid controller index.");
                        }
                    }
                    }
                }
            }
        }
    }

    URHO3D_LOGERROR("Could not find input mapping at {}", eventName);
    return 0.0f;
}

bool InputManager::SaveInputMapToFile(ea::string filePath) { return false; }

bool InputManager::LoadInputMapFromFile(ea::string filePath) { return false; }

} // namespace Urho3D
