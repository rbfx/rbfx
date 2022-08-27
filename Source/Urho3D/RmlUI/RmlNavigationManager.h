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
#include "../Core/Signal.h"
#include "../Input/DirectionalPadAdapter.h"
#include "../RmlUI/RmlNavigable.h"

#include <RmlUi/Core/Element.h>

namespace Urho3D
{

class RmlUIComponent;

/// Navigation manager that tracks all RmlNavigable in the RmlUIComponent.
class URHO3D_API RmlNavigationManager : public Object
{
    URHO3D_OBJECT(RmlNavigationManager, Object);

public:
    Signal<void()> OnGroupChanged;

    explicit RmlNavigationManager(RmlUIComponent* owner);
    ~RmlNavigationManager() override;

    /// Reset current navigation state to default.
    void Reset(Rml::ElementDocument* document);
    /// Periodical update of navigation state.
    void Update();

    /// Cursor operations.
    /// @{
    const ea::string& GetTopCursorGroup() const;
    RmlNavigable* GetTopCursorNavigable() const;
    bool IsGroupInStack(const ea::string& group) const;

    void MoveCursor(const Vector2& direction);
    void PushCursorGroup(const ea::string& group);
    void PopCursorGroup();
    /// @}

    /// Properties.
    /// @{
    void SetInputEnabled(bool enabled);
    bool GetInputEnabled() const { return inputEnabled_; }
    /// @}

    /// Internal API.
    /// @{
    void AddNavigable(RmlNavigable* navigable);
    void RemoveNavigable(RmlNavigable* navigable);
    /// @}

private:
    struct NavigationStackFrame
    {
        ea::string group_;
        Rml::ObserverPtr<RmlNavigable> navigable_;
    };

    void RepairNavigation();
    void UpdateMouseMove();

    bool IsPressButton(Scancode scancode) const;
    bool IsBackButton(Scancode scancode) const;
    bool IsPressButton(ControllerButton button) const;
    bool IsBackButton(ControllerButton button) const;

    void HandleDirectionKeyEvent(StringHash eventType, VariantMap& eventData);
    void HandleKeyboardButtonEvent(StringHash eventType, VariantMap& eventData);
    void HandleJoystickButtonEvent(StringHash eventType, VariantMap& eventData);
    void HandleMouseButtonEvent(StringHash eventType, VariantMap& eventData);

    void SetCursorNavigable(RmlNavigable* navigable, NavigableEventMode eventMode);
    void ResetPressedNavigable();
    void SetPressedNavigable(RmlNavigable* navigable, NavigableInputSource eventSource);
    void ScrollNavigableIntoView(RmlNavigable* navigable);

    template <class T>
    RmlNavigable* FindBestNavigable(const T& penaltyFunction) const;

    const WeakPtr<RmlUIComponent> owner_;

    SharedPtr<DirectionalPadAdapter> directionInput_;

    /// Properties
    /// @{
    const unsigned maxBadFrames_{1};
    bool inputEnabled_{};
    /// @}

    ea::unordered_map<Rml::Element*, Rml::ObserverPtr<RmlNavigable>> navigables_;
    ea::vector<NavigationStackFrame> navigationStack_;

    unsigned numBadFrames_{};

    NavigableInputSource pressEventSource_{};
    Rml::ObserverPtr<RmlNavigable> pressedNavigable_;
};

}
