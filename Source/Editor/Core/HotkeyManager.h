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

#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Container/ConstString.h>
#include <Urho3D/Input/InputConstants.h>

#include <EASTL/shared_ptr.h>
#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

/// Mouse and keyboard combination that can be used as Editor hotkey.
class EditorHotkey
{
public:
    EditorHotkey() = default;
    explicit EditorHotkey(const ea::string& command) : command_(command) {}

    /// Check hotkey state
    /// @{
    bool IsValid() const;
    bool IsTextInputFriendly() const;

    bool CheckKeyboardQualifiers() const;
    bool CheckMouseQualifiers() const;
    bool CheckKeyboardPress() const;
    bool CheckMousePress() const;
    bool Check() const;
    /// @}

    /// Return hotkey text
    /// @{
    ea::string GetQualifiersString() const;
    ea::string GetPressString() const;
    ea::string GetHoldString() const;
    ea::string ToString() const;
    /// @}

    /// Hotkey builder
    /// @{
    EditorHotkey& Press(Key key) { keyPressed_ = key; return *this; }
    EditorHotkey& Press(Scancode scancode) { scancodePressed_ = scancode; return *this; }
    EditorHotkey& Press(MouseButton button) { mouseButtonPressed_ = button; mouseButtonsUp_.Set(button, false); return *this; }

    EditorHotkey& Hold(Key key) { keyDown_ = key; return *this; }
    EditorHotkey& Hold(Scancode scancode) { scancodeDown_ = scancode; return *this; }
    EditorHotkey& Hold(MouseButton button) { mouseButtonsDown_.Set(button, true); mouseButtonsUp_.Set(button, false); return *this; }

    EditorHotkey& Shift() { qualifiersDown_.Set(QUAL_SHIFT, true); qualifiersUp_.Set(QUAL_SHIFT, false); return *this; }
    EditorHotkey& Ctrl() { qualifiersDown_.Set(QUAL_CTRL, true); qualifiersUp_.Set(QUAL_CTRL, false); return *this; }
    EditorHotkey& Alt() { qualifiersDown_.Set(QUAL_ALT, true); qualifiersUp_.Set(QUAL_ALT, false); return *this; }

    EditorHotkey& MaybeShift() { qualifiersUp_.Set(QUAL_SHIFT, false); return *this; }
    EditorHotkey& MaybeCtrl() { qualifiersUp_.Set(QUAL_CTRL, false); return *this; }
    EditorHotkey& MaybeAlt() { qualifiersUp_.Set(QUAL_ALT, false); return *this; }

    EditorHotkey& MaybeMouse() { mouseButtonsUp_ = MOUSEB_NONE; return *this; }
    /// @}

public:
    ea::string command_;

    QualifierFlags qualifiersDown_{};
    QualifierFlags qualifiersUp_{QUAL_SHIFT | QUAL_CTRL | QUAL_ALT};

    MouseButton mouseButtonPressed_{};
    MouseButtonFlags mouseButtonsDown_{};
    MouseButtonFlags mouseButtonsUp_{MOUSEB_ANY};

    Key keyPressed_{};
    Key keyDown_{};
    Scancode scancodePressed_{};
    Scancode scancodeDown_{};
};

/// Class used to manage and dispatch hotkeys.
class HotkeyManager : public Object
{
    URHO3D_OBJECT(HotkeyManager, Object);

public:
    using HotkeyCallback = ea::function<void()>;
    template <class T> using HotkeyMemberCallback = void(T::*)();

    struct HotkeyBinding
    {
        HotkeyBinding(Object* owner, const EditorHotkey& hotkey, HotkeyCallback callback);
        explicit HotkeyBinding(const EditorHotkey& hotkey);

        WeakPtr<Object> owner_;
        EditorHotkey hotkey_;
        HotkeyCallback callback_;
        bool isPassive_{};
        bool isTextInputFriendly_{};
    };
    using HotkeyBindingPtr = ea::shared_ptr<HotkeyBinding>;
    using HotkeyBindingMap = ea::map<ea::string, ea::vector<HotkeyBindingPtr>>;

    explicit HotkeyManager(Context* context);

    /// Bind new hotkeys. Hotkeys for expired objects will be automatically removed.
    /// @{
    void BindPassiveHotkey(const EditorHotkey& hotkey);
    void BindHotkey(Object* owner, const EditorHotkey& hotkey, HotkeyCallback callback);
    template <class T> void BindHotkey(T* owner, const EditorHotkey& hotkey, HotkeyMemberCallback<T> callback);
    /// @}

    /// Return currently bound hotkey combination for given hotkey.
    const EditorHotkey& GetHotkey(const ea::string& command) const;
    const EditorHotkey& GetHotkey(const EditorHotkey& hotkey) const;
    ea::string GetHotkeyLabel(const EditorHotkey& hotkey) const;
    bool IsHotkeyActive(const EditorHotkey& hotkey) const;

    const HotkeyBindingMap& GetBindings() const { return hotkeyByCommand_; }

    /// Routine maintenance, no user logic is performed
    /// @{
    void RemoveExpired();
    void Update();
    /// @}

    /// Check and invoke all hotkeys corresponding to the owner.
    void InvokeFor(Object* owner);

private:
    static bool IsBindingExpired(const HotkeyBindingPtr& ptr);
    HotkeyBindingPtr FindByCommand(const ea::string& command) const;

    unsigned cleanupMs_{1000};
    Timer cleanupTimer_;

    ea::unordered_map<WeakPtr<Object>, ea::vector<HotkeyBindingPtr>> hotkeyByOwner_;
    HotkeyBindingMap hotkeyByCommand_;

    ea::unordered_set<ea::string> invokedCommands_;
    bool isTextInputConsumed_{};
};


template <class T>
void HotkeyManager::BindHotkey(T* owner, const EditorHotkey& hotkey, HotkeyMemberCallback<T> callback)
{
    BindHotkey(owner, hotkey, [owner, callback]() { (owner->*callback)(); });
}

}
