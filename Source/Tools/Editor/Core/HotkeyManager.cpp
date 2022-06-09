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

    if (mouseButton_ == MOUSEB_LEFT)
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

HotkeyManager::HotkeyManager(Context* context)
    : Object(context)
{
}

void HotkeyManager::BindHotkey(Object* owner, const HotkeyInfo& info, ea::function<void()> callback)
{
    const WeakPtr<Object> weakOwner{owner};
    const auto binding = ea::make_shared<HotkeyBinding>(HotkeyBinding{
        weakOwner, info, info.defaultHotkey_, callback});
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
}

void HotkeyManager::InvokeFor(Object* owner)
{
    const WeakPtr<Object> weakOwner{owner};
    const auto iter = hotkeyByOwner_.find(weakOwner);
    if (iter == hotkeyByOwner_.end())
        return;

    for (const HotkeyBindingPtr& bindingPtr : iter->second)
    {
        if (IsInvoked(bindingPtr->hotkey_))
            bindingPtr->callback_();
    }
}

bool HotkeyManager::IsInvoked(const HotkeyCombination& hotkey) const
{
    if (!hotkey.IsValid())
        return false;

    if (hotkey.key_ != SCANCODE_UNKNOWN)
    {
        if (!ui::IsKeyPressed(hotkey.key_, false))
            return false;
    }
    if (hotkey.mouseButton_ != MOUSEB_NONE)
    {
        if (!ui::IsMouseClicked(hotkey.mouseButton_))
            return false;
    }

    const bool ctrlDown = ui::IsKeyDown(KEY_LCTRL) || ui::IsKeyDown(KEY_RCTRL);
    const bool altDown = ui::IsKeyDown(KEY_LALT);
    const bool shiftDown = ui::IsKeyDown(KEY_LSHIFT) || ui::IsKeyDown(KEY_RSHIFT);

    if (hotkey.qualifiers_.Test(QUAL_CTRL) != ctrlDown)
        return false;
    if (hotkey.qualifiers_.Test(QUAL_ALT) != altDown)
        return false;
    if (hotkey.qualifiers_.Test(QUAL_SHIFT) != shiftDown)
        return false;

    return true;
}

HotkeyManager::HotkeyBindingPtr HotkeyManager::FindByCommand(const ea::string& command) const
{
    const auto iter = hotkeyByCommand_.find(command);
    if (iter != hotkeyByCommand_.end() && !iter->second.empty())
        return iter->second[0];
    return nullptr;
}

}
