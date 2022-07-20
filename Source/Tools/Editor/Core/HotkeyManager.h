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
#include <EASTL/vector.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

/// Mouse and keyboard combination that can be used as Editor hotkey.
struct HotkeyCombination
{
    QualifierFlags qualifiers_{};
    MouseButton mouseButton_{};
    Key key_{};
    Scancode scancode_{};

    QualifierFlags ignoredQualifiers_{};
    bool holdMouseButton_{};
    bool holdKey_{};

    bool IsValid() const;
    bool IsInvoked() const;
    ea::string ToString() const;
};

/// Editor hotkey description.
struct HotkeyInfo
{
    ea::string command_;
    HotkeyCombination defaultHotkey_;

    HotkeyInfo() = default;
    explicit HotkeyInfo(const ea::string& command) : command_{command} {}

    HotkeyInfo& Shift() { defaultHotkey_.qualifiers_.Set(QUAL_SHIFT); return *this; }
    HotkeyInfo& Ctrl() { defaultHotkey_.qualifiers_.Set(QUAL_CTRL); return *this; }
    HotkeyInfo& Alt() { defaultHotkey_.qualifiers_.Set(QUAL_ALT); return *this; }

    HotkeyInfo& IgnoreShift() { defaultHotkey_.ignoredQualifiers_.Set(QUAL_SHIFT); return *this; }
    HotkeyInfo& IgnoreCtrl() { defaultHotkey_.ignoredQualifiers_.Set(QUAL_CTRL); return *this; }
    HotkeyInfo& IgnoreAlt() { defaultHotkey_.ignoredQualifiers_.Set(QUAL_ALT); return *this; }
    HotkeyInfo& IgnoreQualifiers() { defaultHotkey_.ignoredQualifiers_ = QUAL_SHIFT | QUAL_CTRL | QUAL_ALT; return *this; }

    HotkeyInfo& Press(Key key) { defaultHotkey_.key_ = key; return *this; }
    HotkeyInfo& Press(Scancode scancode) { defaultHotkey_.scancode_ = scancode; return *this; }
    HotkeyInfo& Press(MouseButton button) { defaultHotkey_.mouseButton_ = button; return *this; }

    HotkeyInfo& Hold(Key key) { defaultHotkey_.holdKey_ = true; defaultHotkey_.key_ = key; return *this; }
    HotkeyInfo& Hold(Scancode scancode) { defaultHotkey_.holdKey_ = true; defaultHotkey_.scancode_ = scancode; return *this; }
    HotkeyInfo& Hold(MouseButton button) { defaultHotkey_.holdMouseButton_ = true; defaultHotkey_.mouseButton_ = button; return *this; }
};

/// Class used to manage and dispatch hotkeys.
class HotkeyManager : public Object
{
    URHO3D_OBJECT(HotkeyManager, Object);

public:
    using HotkeyCallback = ea::function<void()>;
    template <class T> using HotkeyMemberCallback = void(T::*)();

    explicit HotkeyManager(Context* context);

    /// Bind new hotkeys. Hotkeys for expired objects will be automatically removed.
    /// @{
    void BindPassiveHotkey(const HotkeyInfo& info);
    void BindHotkey(Object* owner, const HotkeyInfo& info, HotkeyCallback callback);
    template <class T> void BindHotkey(T* owner, const HotkeyInfo& info, HotkeyMemberCallback<T> callback);
    /// @}

    /// Return currently bound hotkey combination for given hotkey.
    HotkeyCombination GetHotkey(const HotkeyInfo& info) const;
    ea::string GetHotkeyLabel(const HotkeyInfo& info) const;
    bool IsHotkeyActive(const HotkeyInfo& info) const;

    /// Routine maintenance, no user logic is performed
    /// @{
    void RemoveExpired();
    void Update();
    /// @}

    /// Check and invoke all hotkeys corresponding to the owner.
    void InvokeFor(Object* owner);

private:
    struct HotkeyBinding
    {
        HotkeyBinding(Object* owner, const HotkeyInfo& info, HotkeyCallback callback);
        explicit HotkeyBinding(const HotkeyInfo& info);

        WeakPtr<Object> owner_;
        HotkeyInfo info_;
        HotkeyCombination hotkey_;
        HotkeyCallback callback_;
        bool isPassive_{};
    };
    using HotkeyBindingPtr = ea::shared_ptr<HotkeyBinding>;

    static bool IsBindingExpired(const HotkeyBindingPtr& ptr);
    HotkeyBindingPtr FindByCommand(const ea::string& command) const;

    unsigned cleanupMs_{1000};
    Timer cleanupTimer_;

    ea::unordered_map<WeakPtr<Object>, ea::vector<HotkeyBindingPtr>> hotkeyByOwner_;
    ea::unordered_map<ea::string, ea::vector<HotkeyBindingPtr>> hotkeyByCommand_;

    ea::unordered_set<ea::string> invokedCommands_;
};


template <class T>
void HotkeyManager::BindHotkey(T* owner, const HotkeyInfo& info, HotkeyMemberCallback<T> callback)
{
    BindHotkey(owner, info, [owner, callback]() { (owner->*callback)(); });
}

}
