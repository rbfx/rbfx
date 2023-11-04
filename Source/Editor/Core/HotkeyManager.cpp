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

namespace
{

bool IsPrintableKey(Key key)
{
    return (key >= static_cast<Key>(' ') && key < static_cast<Key>(127)) || key == KEY_TAB || key == KEY_RETURN;
}

bool IsTextEditKey(Key key)
{
    return key == KEY_BACKSPACE || key == KEY_DELETE || key == KEY_LEFT || key == KEY_RIGHT || key == KEY_HOME
        || key == KEY_END || key == KEY_UP || key == KEY_DOWN || key == KEY_PAGEUP || key == KEY_PAGEDOWN;
}

bool IsInputUnavailable(Key key)
{
    return IsPrintableKey(key) || IsTextEditKey(key);
}

bool IsInputUnavailable(Scancode scancode)
{
    const Key key = Input::GetKeyFromScancode(scancode);
    return IsPrintableKey(key) || IsTextEditKey(key);
}

}

bool EditorHotkey::IsValid() const
{
    return qualifiersDown_ != QUAL_NONE
        || mouseButtonPressed_ != MOUSEB_NONE
        || mouseButtonsDown_ != MOUSEB_NONE
        || keyPressed_ != KEY_UNKNOWN
        || keyDown_ != KEY_UNKNOWN
        || scancodePressed_ != SCANCODE_UNKNOWN
        || scancodeDown_ != SCANCODE_UNKNOWN;
}

bool EditorHotkey::IsTextInputFriendly() const
{
    // Ctrl and Alt hotkeys are always text-friendly, unless it is one of fixed text editor hotkeys.
    if (qualifiersDown_.Test(QUAL_CTRL) || qualifiersDown_.Test(QUAL_ALT))
    {
        static const ea::vector<EditorHotkey> textHotkeys = {
            EditorHotkey{}.Press(SCANCODE_X).Ctrl(),
            EditorHotkey{}.Press(SCANCODE_C).Ctrl(),
            EditorHotkey{}.Press(SCANCODE_V).Ctrl(),
            EditorHotkey{}.Press(SCANCODE_A).Ctrl(),
            EditorHotkey{}.Press(SCANCODE_Z).Ctrl(),
            EditorHotkey{}.Press(SCANCODE_Y).Ctrl(),
        };
        for (const EditorHotkey& hotkey : textHotkeys)
        {
            const bool sameQualifiers = hotkey.qualifiersDown_ == qualifiersDown_;
            const bool sameScancodePressed = hotkey.scancodePressed_ == scancodePressed_;
            const bool sameScancodeDown = hotkey.scancodePressed_ == scancodeDown_;
            const bool sameKeyPressed = hotkey.scancodePressed_ == Input::GetScancodeFromKey(keyPressed_);
            const bool sameKeyDown = hotkey.scancodePressed_ == Input::GetScancodeFromKey(keyDown_);

            if (sameQualifiers && (sameScancodePressed || sameScancodeDown || sameKeyPressed || sameKeyDown))
                return false;
        }
        return true;
    }

    // All printable characters and some special keys are not text-friendly.
    if (scancodePressed_ != SCANCODE_UNKNOWN && IsInputUnavailable(scancodePressed_))
        return false;
    if (scancodeDown_ != SCANCODE_UNKNOWN && IsInputUnavailable(scancodeDown_))
        return false;
    if (keyPressed_ != KEY_UNKNOWN && IsInputUnavailable(keyPressed_))
        return false;
    if (keyDown_ != KEY_UNKNOWN && IsInputUnavailable(keyDown_))
        return false;

    // All other special keys are text-friendly.
    return true;
}

bool EditorHotkey::CheckKeyboardQualifiers() const
{
    const bool ctrlDown = ui::IsKeyDown(KEY_LCTRL) || ui::IsKeyDown(KEY_RCTRL);
    const bool altDown = ui::IsKeyDown(KEY_LALT);
    const bool shiftDown = ui::IsKeyDown(KEY_LSHIFT) || ui::IsKeyDown(KEY_RSHIFT);

    if (qualifiersDown_.Test(QUAL_CTRL) && !ctrlDown)
        return false;
    if (qualifiersDown_.Test(QUAL_ALT) && !altDown)
        return false;
    if (qualifiersDown_.Test(QUAL_SHIFT) && !shiftDown)
        return false;

    if (qualifiersUp_.Test(QUAL_CTRL) && ctrlDown)
        return false;
    if (qualifiersUp_.Test(QUAL_ALT) && altDown)
        return false;
    if (qualifiersUp_.Test(QUAL_SHIFT) && shiftDown)
        return false;

    return true;
}

bool EditorHotkey::CheckMouseQualifiers() const
{
    for (const MouseButton mouseButton : {MOUSEB_LEFT, MOUSEB_RIGHT, MOUSEB_MIDDLE, MOUSEB_X1, MOUSEB_X2})
    {
        if (mouseButtonsDown_.Test(mouseButton) && !ui::IsMouseDown(mouseButton))
            return false;
        if (mouseButtonsUp_.Test(mouseButton) && ui::IsMouseDown(mouseButton))
            return false;
    }
    return true;
}

bool EditorHotkey::CheckKeyboardPress() const
{
    if (keyPressed_ != KEY_UNKNOWN && !ui::IsKeyPressed(keyPressed_))
        return false;
    if (scancodePressed_ != SCANCODE_UNKNOWN && !ui::IsKeyPressed(static_cast<int>(scancodePressed_)))
        return false;
    if (keyDown_ != KEY_UNKNOWN && !ui::IsKeyDown(keyDown_))
        return false;
    if (scancodeDown_ != SCANCODE_UNKNOWN && !ui::IsKeyDown(static_cast<int>(scancodeDown_)))
        return false;
    return true;
}

bool EditorHotkey::CheckMousePress() const
{
    if (mouseButtonPressed_ != MOUSEB_NONE && !ui::IsMouseClicked(mouseButtonPressed_))
        return false;
    return true;
}

bool EditorHotkey::Check() const
{
    if (!IsValid())
        return false;

    if (!CheckKeyboardQualifiers())
        return false;
    if (!CheckMouseQualifiers())
        return false;
    if (!CheckKeyboardPress())
        return false;
    if (!CheckMousePress())
        return false;

    return true;
}

ea::string EditorHotkey::GetQualifiersString() const
{
    ea::string str;

    if (qualifiersDown_.Test(QUAL_CTRL))
        str += "Ctrl+";
    else if (!qualifiersUp_.Test(QUAL_CTRL))
        str += "[Ctrl?]+";

    if (qualifiersDown_.Test(QUAL_ALT))
        str += "Alt+";
    else if (!qualifiersUp_.Test(QUAL_ALT))
        str += "[Alt?]+";

    if (qualifiersDown_.Test(QUAL_SHIFT))
        str += "Shift+";
    else if (!qualifiersUp_.Test(QUAL_SHIFT))
        str += "[Shift?]+";

    return str;
}

ea::string EditorHotkey::GetPressString() const
{
    if (mouseButtonPressed_ != MOUSEB_NONE)
        return Input::GetMouseButtonName(mouseButtonPressed_);
    else if (keyPressed_ != KEY_UNKNOWN)
        return Input::GetKeyName(keyPressed_);
    else if (scancodePressed_ != SCANCODE_UNKNOWN)
        return Input::GetScancodeName(scancodePressed_);
    else
        return "";
}

ea::string EditorHotkey::GetHoldString() const
{
    ea::string result;

    for (const MouseButton mouseButton : {MOUSEB_LEFT, MOUSEB_RIGHT, MOUSEB_MIDDLE, MOUSEB_X1, MOUSEB_X2})
    {
        if (mouseButtonsDown_.Test(mouseButton))
            result += Input::GetMouseButtonName(mouseButton) + "+";
    }

    if (keyDown_ != KEY_UNKNOWN)
        result += Input::GetKeyName(keyDown_) + "+";
    else if (scancodeDown_ != SCANCODE_UNKNOWN)
        result += Input::GetScancodeName(scancodeDown_) + "+";

    if (result.ends_with("+"))
        result.pop_back();

    return result;
}


ea::string EditorHotkey::ToString() const
{
    const ea::string qualifiers = GetQualifiersString();
    const ea::string press = GetPressString();
    const ea::string hold = GetHoldString();
    if (!press.empty())
        return qualifiers + press;
    else if (!hold.empty())
        return qualifiers + hold;
    else
        return "";
}

HotkeyManager::HotkeyBinding::HotkeyBinding(Object* owner, const EditorHotkey& hotkey, HotkeyCallback callback)
    : owner_{owner}
    , hotkey_{hotkey}
    , callback_{callback}
    , isTextInputFriendly_{hotkey.IsTextInputFriendly()}
{
}

HotkeyManager::HotkeyBinding::HotkeyBinding(const EditorHotkey& hotkey)
    : hotkey_{hotkey}
    , isPassive_{true}
    , isTextInputFriendly_{hotkey.IsTextInputFriendly()}
{
}

HotkeyManager::HotkeyManager(Context* context)
    : Object(context)
{
}

void HotkeyManager::BindPassiveHotkey(const EditorHotkey& hotkey)
{
    const auto binding = ea::make_shared<HotkeyBinding>(hotkey);
    hotkeyByCommand_[hotkey.command_] = {binding};
}

void HotkeyManager::BindHotkey(Object* owner, const EditorHotkey& hotkey, ea::function<void()> callback)
{
    const WeakPtr<Object> weakOwner{owner};
    const auto binding = ea::make_shared<HotkeyBinding>(weakOwner, hotkey, callback);
    hotkeyByOwner_[weakOwner].push_back(binding);
    hotkeyByCommand_[hotkey.command_].push_back(binding);
}

const EditorHotkey& HotkeyManager::GetHotkey(const ea::string& command) const
{
    static const EditorHotkey emptyHotkey;
    const HotkeyBindingPtr ptr = FindByCommand(command);
    return ptr ? ptr->hotkey_ : emptyHotkey;
}

const EditorHotkey& HotkeyManager::GetHotkey(const EditorHotkey& hotkey) const
{
    return GetHotkey(hotkey.command_);
}

ea::string HotkeyManager::GetHotkeyLabel(const EditorHotkey& hotkey) const
{
    return GetHotkey(hotkey).ToString();
}

bool HotkeyManager::IsHotkeyActive(const EditorHotkey& hotkey) const
{
    return GetHotkey(hotkey).Check();
}

void HotkeyManager::RemoveExpired()
{
    ea::unordered_set<ea::string> commandsToCheck;
    ea::erase_if(hotkeyByOwner_, [&](const auto& elem)
    {
        const auto& [owner, bindings] = elem;
        if (!owner && !bindings.empty())
        {
            commandsToCheck.insert(bindings[0]->hotkey_.command_);
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

    isTextInputConsumed_ = ui::GetIO().WantTextInput;
}

void HotkeyManager::InvokeFor(Object* owner)
{
    const WeakPtr<Object> weakOwner{owner};
    const auto iter = hotkeyByOwner_.find(weakOwner);
    if (iter == hotkeyByOwner_.end())
        return;

    for (const HotkeyBindingPtr& bindingPtr : iter->second)
    {
        if (isTextInputConsumed_ && !bindingPtr->hotkey_.IsTextInputFriendly())
            continue;

        const ea::string& command = bindingPtr->hotkey_.command_;
        if (!invokedCommands_.contains(command) && bindingPtr->hotkey_.Check())
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
