//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Core/HotkeyManager.h"

#include <Urho3D/Input/Input.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <EASTL/unordered_set.h>

namespace Urho3D
{

bool HotkeyCombination::IsValid() const
{
    return qualifiers_ != QUAL_NONE
        || key_ != KEY_UNKNOWN
        || mouseButton_ != MOUSEB_NONE
        || scancode_ != SCANCODE_UNKNOWN;
}

bool HotkeyCombination::IsInvoked() const
{
    if (!IsValid())
        return false;

    if (key_ != KEY_UNKNOWN)
    {
        const bool keyActive = holdKey_
            ? ui::IsKeyDown(key_)
            : ui::IsKeyPressed(key_);

        if (!keyActive)
            return false;
    }
    if (scancode_ != SCANCODE_UNKNOWN)
    {
        const bool scancodeActive = holdKey_
            ? ui::IsKeyDown(static_cast<int>(scancode_))
            : ui::IsKeyPressed(static_cast<int>(scancode_));

        if (!scancodeActive)
            return false;
    }
    if (mouseButton_ != MOUSEB_NONE)
    {
        const bool mouseActive = holdMouseButton_
            ? ui::IsMouseDown(mouseButton_)
            : ui::IsMouseClicked(mouseButton_);

        if (!mouseActive)
            return false;
    }

    const bool ctrlDown = ui::IsKeyDown(KEY_LCTRL) || ui::IsKeyDown(KEY_RCTRL);
    const bool altDown = ui::IsKeyDown(KEY_LALT);
    const bool shiftDown = ui::IsKeyDown(KEY_LSHIFT) || ui::IsKeyDown(KEY_RSHIFT);

    if (!ignoredQualifiers_.Test(QUAL_CTRL) && qualifiers_.Test(QUAL_CTRL) != ctrlDown)
        return false;
    if (!ignoredQualifiers_.Test(QUAL_ALT) && qualifiers_.Test(QUAL_ALT) != altDown)
        return false;
    if (!ignoredQualifiers_.Test(QUAL_SHIFT) && qualifiers_.Test(QUAL_SHIFT) != shiftDown)
        return false;

    if (qualifiers_ == QUAL_NONE && mouseButton_ == MOUSEB_NONE && ui::IsMouseDown(MOUSEB_ANY))
        return false;

    return true;
}

ea::string HotkeyCombination::ToString() const
{
    if (!IsValid())
        return "";

    ea::string result;
    if (qualifiers_.Test(QUAL_CTRL))
        result += "Ctrl+";
    if (qualifiers_.Test(QUAL_ALT))
        result += "Alt+";
    if (qualifiers_.Test(QUAL_SHIFT))
        result += "Shift+";

    if (key_ != KEY_UNKNOWN)
        result += Input::GetKeyName(key_);
    else if (scancode_ != SCANCODE_UNKNOWN)
        result += Input::GetScancodeName(scancode_);
    else if (mouseButton_ == MOUSEB_LEFT)
        result += "Mouse L";
    else if (mouseButton_ == MOUSEB_RIGHT)
        result += "Mouse R";
    else if (mouseButton_ == MOUSEB_MIDDLE)
        result += "Mouse 3";
    else if (mouseButton_ == MOUSEB_X1)
        result += "Mouse 4";
    else if (mouseButton_ == MOUSEB_X2)
        result += "Mouse 5";

    return result;
}

HotkeyManager::HotkeyBinding::HotkeyBinding(Object* owner, const HotkeyInfo& info, HotkeyCallback callback)
    : owner_{owner}
    , info_{info}
    , hotkey_{info.defaultHotkey_}
    , callback_{callback}
{
}

HotkeyManager::HotkeyBinding::HotkeyBinding(const HotkeyInfo& info)
    : info_(info)
    , hotkey_{info.defaultHotkey_}
    , isPassive_{true}
{
}

HotkeyManager::HotkeyManager(Context* context)
    : Object(context)
{
}

void HotkeyManager::BindPassiveHotkey(const HotkeyInfo& info)
{
    const auto binding = ea::make_shared<HotkeyBinding>(info);
    hotkeyByCommand_[info.command_] = {binding};
}

void HotkeyManager::BindHotkey(Object* owner, const HotkeyInfo& info, ea::function<void()> callback)
{
    const WeakPtr<Object> weakOwner{owner};
    const auto binding = ea::make_shared<HotkeyBinding>(weakOwner, info, callback);
    hotkeyByOwner_[weakOwner].push_back(binding);
    hotkeyByCommand_[info.command_].push_back(binding);
}

HotkeyCombination HotkeyManager::GetHotkey(const HotkeyInfo& info) const
{
    const HotkeyBindingPtr ptr = FindByCommand(info.command_);
    return ptr ? ptr->hotkey_ : HotkeyCombination{};
}

ea::string HotkeyManager::GetHotkeyLabel(const HotkeyInfo& info) const
{
    return GetHotkey(info).ToString();
}

bool HotkeyManager::IsHotkeyActive(const HotkeyInfo& info) const
{
    return GetHotkey(info).IsInvoked();
}

void HotkeyManager::RemoveExpired()
{
    ea::unordered_set<ea::string> commandsToCheck;
    ea::erase_if(hotkeyByOwner_, [&](const auto& elem)
    {
        const auto& [owner, bindings] = elem;
        if (!owner && !bindings.empty())
        {
            commandsToCheck.insert(bindings[0]->info_.command_);
            return true;
        }
        return false;
    });

    for (const ea::string& command : commandsToCheck)
    {
        const auto iter = hotkeyByCommand_.find(command);
        if (iter == hotkeyByCommand_.end())
            continue;

        ea::erase_if(iter->second, &IsBindingExpired);
        if (iter->second.empty())
            hotkeyByCommand_.erase(iter);
    }
}

void HotkeyManager::Update()
{
    if (cleanupTimer_.GetMSec(false) >= cleanupMs_)
    {
        cleanupTimer_.Reset();
        RemoveExpired();
    }

    invokedCommands_.clear();
}

void HotkeyManager::InvokeFor(Object* owner)
{
    const WeakPtr<Object> weakOwner{owner};
    const auto iter = hotkeyByOwner_.find(weakOwner);
    if (iter == hotkeyByOwner_.end())
        return;

    for (const HotkeyBindingPtr& bindingPtr : iter->second)
    {
        const ea::string& command = bindingPtr->info_.command_;
        if (!invokedCommands_.contains(command) && bindingPtr->hotkey_.IsInvoked())
        {
            bindingPtr->callback_();
            invokedCommands_.insert(command);
        }
    }
}

bool HotkeyManager::IsBindingExpired(const HotkeyBindingPtr& ptr)
{
    return !ptr->isPassive_ && !ptr->owner_;
}

HotkeyManager::HotkeyBindingPtr HotkeyManager::FindByCommand(const ea::string& command) const
{
    const auto iter = hotkeyByCommand_.find(command);
    if (iter != hotkeyByCommand_.end() && !iter->second.empty())
        return iter->second[0];
    return nullptr;
}

}
