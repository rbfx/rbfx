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

#include "../Input/InputManager.h"

#include "../Engine/Engine.h"
#include "../Graphics/Graphics.h"
#include "../IO/Archive.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
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

InputManager::~InputManager()
{
    inputMap_.clear();
}

bool InputManager::CreateGroup(const char* groupName)
{
    if (inputMap_.find(groupName) != inputMap_.end())
    {
        URHO3D_LOGINFO("Input group '{}' already exists", groupName);
        return false;
    }

    inputMap_.insert(groupName);
    URHO3D_LOGINFO("Created new input group '{}'", groupName);
    return true;
}

bool InputManager::RemoveGroup(const char* groupName)
{
    if (inputMap_.find(groupName) != inputMap_.end())
    {
        inputMap_.erase(groupName);
        URHO3D_LOGINFO("Removed and erased input group '{}'", groupName);
        return true;
    }

    URHO3D_LOGINFO("Input group '{}' was not found", groupName);
    return false;
}

bool InputManager::AddInputMapping(const char* groupName, InputLayoutDesc& inputLayout)
{
    if (inputMap_.find(groupName) != inputMap_.end())
    {
        // Input group already exists, insert at key into the group.
        auto& layoutDescArray = inputMap_.at(groupName);
        layoutDescArray.push_back(inputLayout);

        URHO3D_LOGINFO("Added input event '{0}' to group '{1}'", groupName, inputLayout.eventName_);
        return true;
    }

    // Create a group then add input layout to it.
    CreateGroup(groupName);
    auto& layoutDescArray = inputMap_.at(groupName);
    layoutDescArray.push_back(inputLayout);
    URHO3D_LOGINFO("Added input event '{0}' to group '{1}'", groupName, inputLayout.eventName_);

    return true;
}

bool InputManager::RemoveInputMapping(const char* groupName, const char* eventName, bool clearIfEmpty /*= false*/)
{
    if (inputMap_.find(groupName) != inputMap_.end())
    {
        // Find event name
        if (input_ && inputMap_.find(groupName) != inputMap_.end())
        {
            auto& mappedLayout = inputMap_.at(groupName);

            if (mappedLayout.empty())
                return false;

            for (int i = 0; i < mappedLayout.size(); ++i)
            {
                if (mappedLayout[i].eventName_.c_str() == eventName)
                {
                    mappedLayout.erase_first(mappedLayout[i]);
                    URHO3D_LOGINFO("Erased event '{0}' from input group '{1}'", eventName, groupName);
                }
            }
        }

        if (clearIfEmpty)
        {
            bool isEmpty = inputMap_.at(groupName).empty();
            if (isEmpty)
                inputMap_.erase(groupName);
        }

        return true;
    }

    URHO3D_LOGINFO("Input group '{}' was not found", groupName);
    return false;
}

bool InputManager::WasPressed(const char* groupName, const char* eventName)
{
    // Find event name
    if (input_ && inputMap_.find(groupName) != inputMap_.end())
    {
        auto& mappedLayout = inputMap_.at(groupName);

        if (mappedLayout.empty())
            return false;

        for (auto& layoutDesc : mappedLayout)
        {
            if (layoutDesc.eventName_.c_str() == eventName)
            {
                // Begin input press logic
                if (!layoutDesc.active_)
                {
                    URHO3D_LOGINFO("The event {0} of group {1} is disabled", eventName, groupName);
                    return false;
                }

                if (layoutDesc.layout_.empty())
                {
                    URHO3D_LOGINFO("The event {0} of group {1} does not contain an Input Layout", eventName, groupName);
                    return false;
                }

                for (auto& layout : layoutDesc.layout_)
                {
                    switch (layout.deviceType_)
                    {
                    case Device::Mouse:
                    {
                        // If no qualifier, return true if mouse button pressed. Else, return mouse button and qualifier
                        // ressed.
                        if (layout.qualifier_ == Qualifier::QUAL_NONE || layout.qualifier_ == Qualifier::QUAL_ANY)
                        {
                            return input_->GetMouseButtonPress(layout.mouseButton_)
                                || input_->GetMouseButtonClick(layout.mouseButton_);
                        }
                        else
                        {
                            return input_->GetQualifierPress(layout.qualifier_)
                                && (input_->GetMouseButtonPress(layout.mouseButton_)
                                    || input_->GetMouseButtonClick(layout.mouseButton_));
                        }

                        return false;
                    }
                    case Device::Keyboard:
                    {
                        if (layout.qualifier_ == Qualifier::QUAL_NONE || layout.qualifier_ == Qualifier::QUAL_ANY)
                        {
                            return input_->GetKeyPress(layout.keyCode_) || input_->GetScancodePress(layout.scanCode_);
                        }
                        else
                        {
                            // Check all registered keys
                            if (input_->GetQualifierPress(layout.qualifier_) && input_->GetKeyPress(layout.keyCode_))
                            {
                                return true;
                            }

                            // Check all registered scancodes
                            if (input_->GetQualifierPress(layout.qualifier_)
                                && input_->GetScancodePress(layout.scanCode_))
                            {
                                return true;
                            }

                            return false;
                        }

                        return false;
                    }
                    case Device::Joystick:
                    {
                        int numJoysticks = input_->GetNumJoysticks();

                        if (numJoysticks == 0)
                            return false;

                        int joystickIndex = static_cast<int>(layout.controllerIndex_);

                        if (joystickIndex < numJoysticks)
                        {
                            // Get joystick by joystick index
                            auto* joystick = input_->GetJoystickByIndex(joystickIndex);
                            if (joystick)
                            {
                                if (joystick->GetButtonPress(layout.controllerButton_))
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
    if (input_ && inputMap_.find(groupName) != inputMap_.end())
    {
        auto& mappedLayout = inputMap_.at(groupName);

        if (mappedLayout.empty())
            return false;

        for (auto& layoutDesc : mappedLayout)
        {
            if (layoutDesc.eventName_.c_str() == eventName)
            {
                // Begin input press logic
                if (!layoutDesc.active_)
                {
                    URHO3D_LOGINFO("The event {0} of group {1} is disabled", eventName, groupName);
                    return false;
                }

                if (layoutDesc.layout_.empty())
                {
                    URHO3D_LOGINFO("The event {0} of group {1} does not contain an Input Layout", eventName, groupName);
                    return false;
                }

                for (auto& layout : layoutDesc.layout_)
                {
                    switch (layout.deviceType_)
                    {
                    case Device::Mouse:
                    {
                        // If no qualifier, return true if mouse button pressed. Else, return mouse button and qualifier
                        // ressed.
                        if (layout.qualifier_ == Qualifier::QUAL_NONE || layout.qualifier_ == Qualifier::QUAL_ANY)
                        {
                            return input_->GetMouseButtonDown(layout.mouseButton_);
                        }
                        else
                        {
                            return input_->GetQualifierDown(layout.qualifier_)
                                && input_->GetMouseButtonDown(layout.mouseButton_);
                        }

                        return false;
                    }
                    case Device::Keyboard:
                    {
                        if (layout.qualifier_ == Qualifier::QUAL_NONE || layout.qualifier_ == Qualifier::QUAL_ANY)
                        {
                            return input_->GetKeyDown(layout.keyCode_) || input_->GetScancodeDown(layout.scanCode_);
                        }
                        else
                        {
                            // Check all registered keys
                            if (input_->GetQualifierDown(layout.qualifier_) && input_->GetKeyDown(layout.keyCode_))
                            {
                                return true;
                            }

                            // Check all registered scancodes
                            if (input_->GetQualifierDown(layout.qualifier_)
                                && input_->GetScancodeDown(layout.scanCode_))
                            {
                                return true;
                            }

                            return false;
                        }

                        return false;
                    }
                    case Device::Joystick:
                    {
                        int numJoysticks = input_->GetNumJoysticks();

                        if (numJoysticks == 0)
                            return false;

                        int joystickIndex = static_cast<int>(layout.controllerIndex_);

                        if (joystickIndex < numJoysticks)
                        {
                            // Get joystick by joystick index
                            auto* joystick = input_->GetJoystickByIndex(joystickIndex);
                            if (joystick)
                            {
                                if (joystick->GetButtonDown(layout.controllerButton_))
                                {
                                    return true;
                                }
                            }
                        }
                        else
                        {
                            URHO3D_LOGERROR("Called event {0} of group {1} with an invalid controller index.",
                                layoutDesc.eventName_, layout.controllerIndex_);
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
    if (input_ && inputMap_.find(groupName) != inputMap_.end())
    {
        auto& mappedLayout = inputMap_.at(groupName);

        if (mappedLayout.empty())
            return false;

        for (auto& layoutDesc : mappedLayout)
        {
            if (layoutDesc.eventName_.c_str() == eventName)
            {
                // Begin input press logic
                if (!layoutDesc.active_)
                {
                    URHO3D_LOGINFO("The event {0} of group {1} is disabled", eventName, groupName);
                    return false;
                }

                if (layoutDesc.layout_.empty())
                {
                    URHO3D_LOGINFO("The event {0} of group {1} does not contain an Input Layout", eventName, groupName);
                    return false;
                }

                for (auto& layout : layoutDesc.layout_)
                {
                    switch (layout.deviceType_)
                    {
                    case Device::Keyboard:
                    {
                        bool pressed = input_->GetKeyDown(layout.keyCode_) || input_->GetScancodeDown(layout.scanCode_);

                        return pressed ? layout.axisScale_ : 0.0f;
                    }
                    case Device::Joystick:
                    {
                        int numJoysticks = input_->GetNumJoysticks();

                        if (numJoysticks == 0)
                            return 0.0f;

                        int joystickIndex = static_cast<int>(layout.controllerIndex_);

                        if (joystickIndex < numJoysticks)
                        {
                            // Get joystick by joystick index
                            auto* joystick = input_->GetJoystickByIndex(joystickIndex);
                            if (joystick)
                            {
                                float axisPosition = joystick->GetAxisPosition(layout.controllerAxis_);
                                if (Abs(axisPosition) >= layout.deadZone_)
                                {
                                    return axisPosition * layout.axisScale_;
                                }
                            }
                        }
                        else
                        {
                            URHO3D_LOGERROR("Called event {0} of group {1} with an invalid controller index.",
                                layoutDesc.eventName_, layout.controllerIndex_);
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
    if (input_ && inputMap_.find(groupName) != inputMap_.end())
    {
        auto& mappedLayout = inputMap_.at(groupName);

        if (mappedLayout.empty())
            return false;

        for (auto& layoutDesc : mappedLayout)
        {
            if (layoutDesc.eventName_.c_str() == eventName)
            {
                // Begin input press logic
                if (!layoutDesc.active_)
                {
                    URHO3D_LOGINFO("The event {0} of group {1} is disabled", eventName, groupName);
                    return false;
                }

                if (layoutDesc.layout_.empty())
                {
                    URHO3D_LOGINFO("The event {0} of group {1} does not contain an Input Layout", eventName, groupName);
                    return false;
                }

                for (auto& layout : layoutDesc.layout_)
                {
                    switch (layout.deviceType_)
                    {
                    case Device::Joystick:
                    {
                        int numJoysticks = input_->GetNumJoysticks();

                        if (numJoysticks == 0)
                            return 0.0f;

                        int joystickIndex = static_cast<int>(layout.controllerIndex_);

                        if (joystickIndex < numJoysticks)
                        {
                            // Get joystick by joystick index
                            auto* joystick = input_->GetJoystickByIndex(joystickIndex);
                            if (joystick)
                            {
                                return joystick->GetHatPosition(layout.hatPosition);
                            }
                        }
                        else
                        {
                            URHO3D_LOGERROR("Called event {0} of group {1} with an invalid controller index.",
                                layoutDesc.eventName_, layout.controllerIndex_);
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

const int VERSION = 1;

bool InputManager::SerializeToArchive(Archive& archive)
{
    if (auto projectBlock = archive.OpenUnorderedBlock("InputRegistry"))
    {
        int archiveVersion = VERSION;
        SerializeValue(archive, "Version", archiveVersion);

        for (auto& inputMapping : inputMap_)
        {
            if (auto projectBlock = archive.OpenUnorderedBlock(inputMapping.first))
            {
                for (auto& inputDesc : inputMapping.second)
                {
                    if (auto projectBlock = archive.OpenUnorderedBlock(inputDesc.eventName_.c_str()))
                    {
                        int size = inputDesc.layout_.size();
                        SerializeValue(archive, "EventName", inputDesc.eventName_);
                        SerializeValue(archive, "Active", inputDesc.active_);
                        SerializeValue(archive, "LayoutSize", size);

                        for (auto& layout : inputDesc.layout_)
                        {
                            if (auto projectBlock = archive.OpenUnorderedBlock(""))
                            {
                                int deviceType = (int)layout.deviceType_;
                                int keyCode = (int)layout.keyCode_;
                                int scanCode = (int)layout.scanCode_;
                                int mouseButton = (int)layout.mouseButton_;
                                int controllerButton = (int)layout.controllerButton_;
                                int controllerAxis = (int)layout.controllerAxis_;
                                int hatPosition = (int)layout.hatPosition;
                                int qualifier = (int)layout.qualifier_;
                                int controllerIndex = (int)layout.controllerIndex_;

                                SerializeValue(archive, "DeviceType", deviceType);
                                SerializeValue(archive, "KeyCode", keyCode);
                                SerializeValue(archive, "ScanCode", scanCode);
                                SerializeValue(archive, "MouseButton", mouseButton);
                                SerializeValue(archive, "ControllerButton", controllerButton);
                                SerializeValue(archive, "ControllerAxis", controllerAxis);
                                SerializeValue(archive, "HatPosition", hatPosition);
                                SerializeValue(archive, "Qualifier", qualifier);
                                SerializeValue(archive, "ControllerIndex", controllerIndex);
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool InputManager::DeserializeFromArchive(Archive& archive)
{
    return false;
}

} // namespace Urho3D
