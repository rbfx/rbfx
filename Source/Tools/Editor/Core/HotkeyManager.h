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

namespace Urho3D
{

/// Mouse and keyboard combination that can be used as Editor hotkey.
struct HotkeyCombination
{
    QualifierFlags qualifiers_;
    MouseButton mouseButton_;
    Key key_;

    HotkeyCombination() = default;
    HotkeyCombination(QualifierFlags qualifiers, MouseButton mouseButton)
        : qualifiers_{qualifiers}
        , mouseButton_{mouseButton}
        , key_{KEY_UNKNOWN}
    {
    }
    HotkeyCombination(QualifierFlags qualifiers, Key key)
        : qualifiers_{qualifiers}
        , mouseButton_{MOUSEB_NONE}
        , key_{key}
    {
    }

    bool IsValid() const { return key_ != KEY_UNKNOWN || mouseButton_ != MOUSEB_NONE; }
    ea::string ToString() const;
};

/// Editor hotkey description.
struct HotkeyInfo
{
    ea::string command_;
    ea::string scope_;
    HotkeyCombination defaultHotkey_;

    HotkeyInfo() = default;
    HotkeyInfo(const ea::string& command, const HotkeyCombination& hotkey)
        : command_(command)
        , defaultHotkey_{hotkey}
    {
    }
    HotkeyInfo(const ea::string& command, const ea::string& scope, const HotkeyCombination& hotkey)
        : command_(command)
        , scope_(scope)
        , defaultHotkey_{hotkey}
    {
    }
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
    void BindHotkey(Object* owner, const HotkeyInfo& info, HotkeyCallback callback);
    template <class T> void BindHotkey(T* owner, const HotkeyInfo& info, HotkeyMemberCallback<T> callback);
    /// @}

    /// Return currently bound hotkey combination for given hotkey.
    HotkeyCombination GetHotkey(const HotkeyInfo& info) const;
    ea::string GetHotkeyLabel(const HotkeyInfo& info) const;

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
        WeakPtr<Object> owner_;
        HotkeyInfo info_;
        HotkeyCombination hotkey_;
        HotkeyCallback callback_;
    };
    using HotkeyBindingPtr = ea::shared_ptr<HotkeyBinding>;

    static bool IsBindingExpired(const HotkeyBindingPtr& ptr) { return !ptr->owner_; }
    bool IsInvoked(const HotkeyCombination& hotkey) const;
    HotkeyBindingPtr FindByCommand(const ea::string& command) const;

    unsigned cleanupMs_{1000};
    Timer cleanupTimer_;

    ea::unordered_map<WeakPtr<Object>, ea::vector<HotkeyBindingPtr>> hotkeyByOwner_;
    ea::unordered_map<ea::string, ea::vector<HotkeyBindingPtr>> hotkeyByCommand_;
};


template <class T>
void HotkeyManager::BindHotkey(T* owner, const HotkeyInfo& info, HotkeyMemberCallback<T> callback)
{
    BindHotkey(owner, info, [owner, callback]() { (owner->*callback)(); });
}

}

/// Define hotkey.
#define URHO3D_EDITOR_HOTKEY(name, command, qual, key) \
    URHO3D_GLOBAL_CONSTANT(Urho3D::HotkeyInfo name(command, Urho3D::HotkeyCombination(qual, key)))
