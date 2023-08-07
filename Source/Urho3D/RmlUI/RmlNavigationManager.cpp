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

#include "../Precompiled.h"

#include "../RmlUI/RmlNavigationManager.h"

#include "../Input/Input.h"
#include "../Input/InputEvents.h"
#include "../RmlUI/RmlUIComponent.h"

#include <RmlUi/Core/ElementDocument.h>
#include <Core/Elements/ElementFormControlInput.h>
#include <Core/Elements/ElementFormControlSelect.h>
#include <Core/Elements/ElementFormControlTextArea.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

Vector2 KeyToDirection(Key key)
{
    switch (key)
    {
    case KEY_UP:
        return {0.0f, -1.0f};

    case KEY_DOWN:
        return {0.0f, 1.0f};

    case KEY_LEFT:
        return {-1.0f, 0.0f};

    case KEY_RIGHT:
        return {1.0f, 0.0f};

    default:
        return Vector2::ZERO;
    }
}

}

RmlNavigationManager::RmlNavigationManager(RmlUIComponent* owner)
    : Object(owner->GetContext())
    , owner_(owner)
    , directionInput_(MakeShared<DirectionalPadAdapter>(context_))
{
    SetInputEnabled(true);

    SubscribeToEvent(directionInput_, E_KEYUP, URHO3D_HANDLER(RmlNavigationManager, HandleDirectionKeyEvent));
    SubscribeToEvent(directionInput_, E_KEYDOWN, URHO3D_HANDLER(RmlNavigationManager, HandleDirectionKeyEvent));

    auto input = GetSubsystem<Input>();
    SubscribeToEvent(input, E_MOUSEBUTTONDOWN, URHO3D_HANDLER(RmlNavigationManager, HandleMouseButtonEvent));
    SubscribeToEvent(input, E_MOUSEBUTTONUP, URHO3D_HANDLER(RmlNavigationManager, HandleMouseButtonEvent));
    SubscribeToEvent(input, E_KEYUP, URHO3D_HANDLER(RmlNavigationManager, HandleKeyboardButtonEvent));
    SubscribeToEvent(input, E_KEYDOWN, URHO3D_HANDLER(RmlNavigationManager, HandleKeyboardButtonEvent));
    SubscribeToEvent(input, E_JOYSTICKBUTTONUP, URHO3D_HANDLER(RmlNavigationManager, HandleJoystickButtonEvent));
    SubscribeToEvent(input, E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(RmlNavigationManager, HandleJoystickButtonEvent));
}

RmlNavigationManager::~RmlNavigationManager()
{
}

void RmlNavigationManager::HandleDirectionKeyEvent(StringHash eventType, VariantMap& eventData)
{
    // Let control in focus handle directional input.
    auto elementInFocus = owner_->GetDocument()->GetFocusLeafNode();
    if (auto formControl = dynamic_cast<Rml::ElementFormControl*>(elementInFocus))
    {
        if (auto inputControl = dynamic_cast<Rml::ElementFormControlInput*>(elementInFocus))
        {
            auto& typeName = inputControl->GetTypeName();
            if (typeName == "range" || typeName == "text" || typeName == "password")
                return;
        }
        else if (dynamic_cast<Rml::ElementFormControlSelect*>(elementInFocus))
        {
            return;
        }
        else if (dynamic_cast<Rml::ElementFormControlTextArea*>(elementInFocus))
        {
            return;
        }
    }

    const bool pressed = eventType == E_KEYDOWN;
    const auto key = static_cast<Key>(eventData[KeyDown::P_KEY].GetUInt());
    const Vector2 direction = KeyToDirection(key);

    if (pressed && direction != Vector2::ZERO)
        MoveCursor(direction);
}

void RmlNavigationManager::HandleKeyboardButtonEvent(StringHash eventType, VariantMap& eventData)
{
    const bool pressed = eventType == E_KEYDOWN;
    const auto scancode = static_cast<Scancode>(eventData[KeyDown::P_SCANCODE].GetUInt());
    const bool isRepeat = pressed && eventData[KeyDown::P_REPEAT].GetBool();

    RmlNavigable* cursorNavigable = GetTopCursorNavigable();
    if (pressed && !isRepeat && cursorNavigable && IsPressButton(scancode))
    {
        const NavigablePressMode mode = cursorNavigable->GetKeyboardPressMode();

        if (cursorNavigable != pressedNavigable_.get())
            SetPressedNavigable(cursorNavigable, NavigableInputSource::Keyboard);
        else if (mode == NavigablePressMode::Toggle)
            ResetPressedNavigable();
    }
    else if (!pressed && pressedNavigable_ && pressEventSource_ == NavigableInputSource::Keyboard && IsPressButton(scancode))
    {
        const NavigablePressMode mode = pressedNavigable_->GetKeyboardPressMode();

        if (mode == NavigablePressMode::Trigger)
            ResetPressedNavigable();
    }
    else if (pressed && IsBackButton(scancode))
    {
        PopCursorGroup();
    }
}

void RmlNavigationManager::HandleJoystickButtonEvent(StringHash eventType, VariantMap& eventData)
{
    const bool pressed = eventType == E_JOYSTICKBUTTONDOWN;
    const auto button = static_cast<ControllerButton>(eventData[JoystickButtonDown::P_BUTTON].GetUInt());

    RmlNavigable* cursorNavigable = GetTopCursorNavigable();
    if (pressed && cursorNavigable && IsPressButton(button))
    {
        const NavigablePressMode mode = cursorNavigable->GetKeyboardPressMode();

        if (cursorNavigable != pressedNavigable_.get())
            SetPressedNavigable(cursorNavigable, NavigableInputSource::Joystick);
        else if (mode == NavigablePressMode::Toggle)
            ResetPressedNavigable();
    }
    else if (!pressed && pressedNavigable_ && pressEventSource_ == NavigableInputSource::Joystick && IsPressButton(button))
    {
        const NavigablePressMode mode = pressedNavigable_->GetKeyboardPressMode();

        if (mode == NavigablePressMode::Trigger)
            ResetPressedNavigable();
    }
    else if (pressed && IsBackButton(button))
    {
        PopCursorGroup();
    }
}

void RmlNavigationManager::HandleMouseButtonEvent(StringHash eventType, VariantMap& eventData)
{
    const bool pressed = eventType == E_MOUSEBUTTONDOWN;

    RmlNavigable* cursorNavigable = GetTopCursorNavigable();
    if (pressed && cursorNavigable && cursorNavigable->IsHovered())
    {
        const NavigablePressMode mode = cursorNavigable->GetMousePressMode();

        if (cursorNavigable != pressedNavigable_.get())
            SetPressedNavigable(cursorNavigable, NavigableInputSource::Mouse);
        else if (mode == NavigablePressMode::Toggle)
            ResetPressedNavigable();
    }
    else if (!pressed && pressedNavigable_ && pressEventSource_ == NavigableInputSource::Mouse)
    {
        const NavigablePressMode mode = pressedNavigable_->GetMousePressMode();

        if (mode == NavigablePressMode::Trigger)
            ResetPressedNavigable();
    }
}

bool RmlNavigationManager::IsPressButton(Scancode scancode) const
{
    return scancode == SCANCODE_RETURN || scancode == SCANCODE_SPACE;
}

bool RmlNavigationManager::IsBackButton(Scancode scancode) const
{
    return scancode == SCANCODE_BACKSPACE || scancode == SCANCODE_ESCAPE;
}

bool RmlNavigationManager::IsPressButton(ControllerButton button) const
{
    return button == CONTROLLER_BUTTON_A;
}

bool RmlNavigationManager::IsBackButton(ControllerButton button) const
{
    return button == CONTROLLER_BUTTON_B;
}

void RmlNavigationManager::Reset(Rml::ElementDocument* document)
{
    navigables_.clear();
    navigationStack_.clear();

    const ea::string defaultNavigationGroup = document->GetAttribute<Rml::String>("navigation-group", "default");
    PushCursorGroup(defaultNavigationGroup);
}

void RmlNavigationManager::AddNavigable(RmlNavigable* navigable)
{
    navigables_.emplace(navigable->AsElement(), navigable->GetObserverPtr());
}

void RmlNavigationManager::RemoveNavigable(RmlNavigable* navigable)
{
    navigables_.erase(navigable->AsElement());
}

void RmlNavigationManager::Update()
{
    if (navigationStack_.empty())
        return;

    RepairNavigation();
    UpdateMouseMove();
}

void RmlNavigationManager::UpdateMouseMove()
{
    auto input = GetSubsystem<Input>();
    const bool hasMouseMovement = input->GetMouseMove() != IntVector2::ZERO;

    if (!hasMouseMovement)
        return;

    // Navigate on mouse hover
    for (const auto& [_, navigable] : navigables_)
    {
        if (navigable->IsHovered() && navigable->IsNavigableInGroup(GetTopCursorGroup()))
            SetCursorNavigable(navigable.get(), NavigableEventMode::OnActivation);
    }

    // Reset navigation if another navigable is pressed
    RmlNavigable* cursorNavigable = GetTopCursorNavigable();
    RmlNavigable* pressedNavigable = pressedNavigable_.get();
    if (cursorNavigable && pressedNavigable && !cursorNavigable->IsHovered() && cursorNavigable != pressedNavigable)
        SetCursorNavigable(pressedNavigable, NavigableEventMode::Never);
}

void RmlNavigationManager::RepairNavigation()
{
    URHO3D_ASSERT(!navigationStack_.empty());
    if (navigables_.empty())
        return;

    const NavigationStackFrame& currentFrame = navigationStack_.back();
    const bool canNavigate = currentFrame.navigable_ && currentFrame.navigable_->IsNavigableInGroup(currentFrame.group_);

    if (!canNavigate)
        ++numBadFrames_;
    else
        numBadFrames_ = 0;

    if (numBadFrames_ > maxBadFrames_)
    {
        RmlNavigable* bestNavigable = FindBestNavigable([&](RmlNavigable* navigable) -> ea::optional<float>
        {
            navigable->Refresh();
            if (!navigable->IsNavigableInGroup(currentFrame.group_))
                return ea::nullopt;

            const Vector2& position = navigable->GetPosition();
            return position.x_ + position.y_;
        });

        SetCursorNavigable(bestNavigable, NavigableEventMode::OnActivation);
        ScrollNavigableIntoView(bestNavigable);
    }
}

template <class T>
RmlNavigable* RmlNavigationManager::FindBestNavigable(const T& penaltyFunction) const
{
    RmlNavigable* bestNavigable = nullptr;
    float bestPenalty = 0.0f;
    for (const auto& [_, navigable] : navigables_)
    {
        const auto penalty = penaltyFunction(navigable.get());
        if (penalty && (!bestNavigable || bestPenalty > *penalty))
        {
            bestNavigable = navigable.get();
            bestPenalty = *penalty;
        }
    }
    return bestNavigable;
}

const ea::string& RmlNavigationManager::GetTopCursorGroup() const
{
    return !navigationStack_.empty() ? navigationStack_.back().group_ : EMPTY_STRING;
}

RmlNavigable* RmlNavigationManager::GetTopCursorNavigable() const
{
    return !navigationStack_.empty() ? navigationStack_.back().navigable_.get() : nullptr;
}

bool RmlNavigationManager::IsGroupInStack(const ea::string& group) const
{
    const auto isFound = [&](const NavigationStackFrame& frame) { return frame.group_ == group; };
    return ea::find_if(navigationStack_.begin(), navigationStack_.end(), isFound) != navigationStack_.end();
}

void RmlNavigationManager::MoveCursor(const Vector2& direction)
{
    // Don't do anything, should be repaired soon
    RmlNavigable* cursorNavigable = GetTopCursorNavigable();
    if (!cursorNavigable)
        return;

    const ea::string& currentGroup = GetTopCursorGroup();
    const Vector2& currentPosition = cursorNavigable->GetPosition();

    RmlNavigable* bestNavigable = FindBestNavigable([&](RmlNavigable* navigable) -> ea::optional<float>
    {
        if (navigable == cursorNavigable || !navigable->IsNavigableInGroup(currentGroup))
            return ea::nullopt;

        const Vector2 otherPosition = navigable->GetPosition();
        const Vector2 otherDirection = otherPosition - currentPosition;
        if (direction.Angle(otherDirection) > 50.0f)
            return ea::nullopt;

        const float distance = otherDirection.Length();
        return distance;
    });

    if (bestNavigable)
    {
        SetCursorNavigable(bestNavigable, NavigableEventMode::OnActivation);
        ScrollNavigableIntoView(bestNavigable);
    }
}

void RmlNavigationManager::PushCursorGroup(const ea::string& group)
{
    if (IsGroupInStack(group))
    {
        URHO3D_LOGWARNING("Group '{}' is already pushed to the navigation stack", group);
        return;
    }

    navigationStack_.push_back(NavigationStackFrame{group, nullptr});
    OnGroupChanged(this);
}

void RmlNavigationManager::PopCursorGroup()
{
    // Never pop the root frame, don't make it an error though
    if (navigationStack_.size() <= 1)
        return;

    SetCursorNavigable(nullptr, NavigableEventMode::Never);
    navigationStack_.pop_back();
    OnGroupChanged(this);
}

void RmlNavigationManager::SetInputEnabled(bool enabled)
{
    inputEnabled_ = enabled;
    directionInput_->SetEnabled(inputEnabled_);
}

void RmlNavigationManager::SetCursorNavigable(RmlNavigable* navigable, NavigableEventMode eventMode)
{
    if (navigationStack_.empty())
    {
        URHO3D_LOGERROR("Unexpected call to RmlNavigationManager::SetCursorNavigable");
        return;
    }

    if (navigable && !navigable->IsNavigableInGroup(GetTopCursorGroup()))
    {
        URHO3D_LOGERROR("Navigable is not in the top group '{}'", GetTopCursorGroup());
        return;
    }

    NavigationStackFrame& currentFrame = navigationStack_.back();
    if (currentFrame.navigable_ == navigable)
        return;

    if (currentFrame.navigable_)
        currentFrame.navigable_->SetNavigated(false, eventMode);

    currentFrame.navigable_ = navigable ? navigable->GetObserverPtr() : nullptr;

    if (currentFrame.navigable_)
        currentFrame.navigable_->SetNavigated(true, eventMode);
}

void RmlNavigationManager::ResetPressedNavigable()
{
    if (!pressedNavigable_)
        return;

    const bool isStillNavigated = pressedNavigable_ == GetTopCursorNavigable();
    const NavigableEventMode eventMode = isStillNavigated ? NavigableEventMode::Always : NavigableEventMode::Never;
    pressedNavigable_->SetPressed(false, pressEventSource_, eventMode);
    pressedNavigable_ = nullptr;
}

void RmlNavigationManager::SetPressedNavigable(RmlNavigable* navigable, NavigableInputSource eventSource)
{
    if (pressedNavigable_ == navigable)
        return;

    if (pressedNavigable_)
        ResetPressedNavigable();

    pressedNavigable_ = navigable ? navigable->GetObserverPtr() : nullptr;
    pressEventSource_ = eventSource;

    if (pressedNavigable_)
        pressedNavigable_->SetPressed(true, pressEventSource_, NavigableEventMode::Always);
}

void RmlNavigationManager::ScrollNavigableIntoView(RmlNavigable* navigable)
{
    if (navigable)
        navigable->AsElement()->ScrollIntoView(Rml::ScrollIntoViewOptions(Rml::ScrollAlignment::Nearest));
}

}
