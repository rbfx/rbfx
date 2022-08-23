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

#include "../Core/Object.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementInstancer.h>

namespace Urho3D
{

class RmlUIComponent;

/// Input source that caused change of RmlNavigable state.
enum class NavigableInputSource
{
    /// Artificial call.
    Artificial,
    /// Keyboard input.
    Keyboard,
    /// Joystick or gamepad input.
    Joystick,
    /// Mouse input.
    Mouse
};

/// RmlNavigable event mode.
enum class NavigableEventMode
{
    Never,
    OnActivation,
    Always,
};

/// Press/depress behavior of the RmlNavigable.
enum class NavigablePressMode
{
    /// Navigable is pressed when key or mouse button is pressed, and depressed when key or mouse button is released.
    Trigger,
    /// Navigable is pressed when mouse button or key pressed for the first time, and depressed on second press.
    Toggle,
    /// Navigable is pressed when mouse button or key pressed, and depressed only when another element is pressed.
    StickyToggle,
};

/// UI element that can be navigated with arrows.
class URHO3D_API RmlNavigable
    : protected Rml::Element
    , public Rml::EnableObserverPtr<RmlNavigable>
{
public:
    using Rml::EnableObserverPtr<RmlNavigable>::GetObserverPtr;

    RmlNavigable(const Rml::String& tag, const Rml::String& group);
    ~RmlNavigable() override;

    static void Register();

    /// Update caches if dirty.
    void Refresh();

    /// Modify element style.
    /// @{
    void SetNavigated(bool navigated, NavigableEventMode eventMode);
    void SetPressed(bool pressed, NavigableInputSource inputSource, NavigableEventMode eventMode);
    /// @}

    /// Return internal pointers.
    /// @{
    Rml::Element* AsElement() { return this; }
    const Rml::Element* AsElement() const { return this; }
    RmlUIComponent* GetOwner() const { return owner_; }
    /// @}

    /// Return properties of the navigable.
    /// @{
    const ea::string& GetGroup() const { return group_; }
    NavigablePressMode GetMousePressMode() const { return cache_.mousePressMode_; }
    NavigablePressMode GetKeyboardPressMode() const { return cache_.keyboardPressMode_; }
    NavigablePressMode GetJoystickPressMode() const { return cache_.joystickPressMode_; }
    /// @}

    /// Return state of the navigable. May be slightly outdated.
    /// @{
    const Vector2& GetPosition() const { return position_; }
    bool IsDisabled() const { return disabled_; }
    bool IsHovered() const { return cache_.hovered_; }
    bool IsVisible() const { return cache_.visible_; }

    bool IsNavigable() const { return cache_.visible_ && !disabled_; }
    bool IsNavigableInGroup(const ea::string& group) const { return IsNavigable() && group_ == group; }
    /// @}

private:
    /// Implement Rml::Element
    /// @{
    void OnUpdate() override;
    void Release() override;

    void OnResize() override;
    void OnLayout() override;
    void OnDpRatioChange() override;
    void OnStyleSheetChange() override;
    void OnAttributeChange(const Rml::ElementAttributes& changedAttributes) override;
    void OnPropertyChange(const Rml::PropertyIdSet& changedProperties) override;
    void OnPseudoClassChange(const Rml::String& pseudoClass, bool activate) override;
    /// @}

    bool EnsureInitialized();
    void MarkCachesDirty() { cachesDirty_ = true; }

    void UpdatePosition();
    void UpdateCaches();

    void UpdateDisabled();
    void UpdateHovered();
    void UpdateStyle();
    void UpdateVisible();

    template <class T> void ForEach(Rml::Element* element, const T& func);
    template <class T> void ForEachChild(Rml::Element* element, const T& func);

    const Rml::String group_;
    WeakPtr<RmlUIComponent> owner_;

    bool cachesDirty_{true};
    bool firstUpdate_{true};

    Vector2 position_;
    bool disabled_{};
    struct Cache
    {
        bool hovered_{};
        bool visible_{};

        NavigablePressMode mousePressMode_{};
        NavigablePressMode keyboardPressMode_{};
        NavigablePressMode joystickPressMode_{};
    } cache_;
};

namespace Detail
{

/// Implementation of RmlUi decorator instancer for RmlNavigable.
class URHO3D_API NavigableInstancer : public Rml::ElementInstancer
{
public:
    /// Implement ElementInstancer
    /// @{
    Rml::ElementPtr InstanceElement(Rml::Element* parent, const Rml::String& tag, const Rml::XMLAttributes& attributes) override;
    void ReleaseElement(Rml::Element* element) override;
    /// @}
};

}

}
