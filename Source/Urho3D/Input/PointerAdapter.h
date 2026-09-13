// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Input/DirectionAggregator.h"

namespace Urho3D
{

enum class PointerAdapterMask : unsigned
{
    None = 0,
    Mouse = 1 << 0,
    Touch = 1 << 1,
    Keyboard = 1 << 2,
    Joystick = 1 << 3,
    All = Touch | Mouse | Keyboard | Joystick,
};

URHO3D_FLAGSET(PointerAdapterMask, PointerAdapterFlags);

class URHO3D_API PointerAdapter : public Object
{
    URHO3D_OBJECT(PointerAdapter, Object)

public:
    /// Construct.
    explicit PointerAdapter(Context* context);

    /// Set enabled flag. The object subscribes for events when enabled.
    void SetEnabled(bool enabled);

    /// Set subscription mask.
    void SetSubscriptionMask(PointerAdapterFlags mask);
    /// Set UI element to filter touch events. Only touch events originated in the element going to be handled.
    void SetUIElement(UIElement* element);
    /// Set maximum cursor velocity.
    void SetCursorSpeed(float cursorSpeed);
    /// Set cursor acceleration factor.
    void SetCursorAcceleration(float cursorAcceleration);

    /// Get enabled flag.
    bool IsEnabled() const { return enabled_; }
    /// Get subscription mask.
    PointerAdapterFlags GetSubscriptionMask() const { return enabledSubscriptions_; }
    /// Get UI element to filter touch events.
    UIElement* GetUIElement() const { return directionAdapter_->GetUIElement(); }
    /// Is button down (left mouse button, touch or gamepad A button).
    bool IsButtonDown() const { return pointerPressed_; }
    /// Set maximum cursor velocity.
    float GetCursorVelocity() const { return maxCursorSpeed_; }
    /// Set cursor acceleration factor.
    float GetCursorAcceleration() const { return cursorAcceleration_; }

    /// Get last known pointer position.
    IntVector2 GetPointerPosition() const { return pointerPosition_.ToIntVector2(); }
    /// Get last known pointer position in UI space.
    IntVector2 GetUIPointerPosition() const;
    

    /// Get DirectionAggregator instance used to handle joystick and keyboard input.
    DirectionAggregator* GetDirectionAggregator() const;

private:
    void UpdateSubscriptions(PointerAdapterFlags flags);

    void HandleUpdate(StringHash eventType, VariantMap& args);

    void HandleMouseMove(StringHash eventType, VariantMap& args);
    void HandleMouseButtonUp(StringHash eventType, VariantMap& args);
    void HandleMouseButtonDown(StringHash eventType, VariantMap& args);

    void HandleTouchBegin(StringHash eventType, VariantMap& args);
    void HandleTouchMove(StringHash eventType, VariantMap& args);
    void HandleTouchEnd(StringHash eventType, VariantMap& args);

    void HandleJoystickButton(StringHash eventType, VariantMap& args);

    void UpdatePointer(const Vector2& position, bool press, bool moveMouse);

    // Keyboard and joystick adapter to move the cursor.
    SharedPtr<DirectionAggregator> directionAdapter_;
    /// Is aggregator enabled
    bool enabled_{false};
    /// Enabled subscriptions
    PointerAdapterFlags enabledSubscriptions_{PointerAdapterMask::All};
    /// Active subscriptions bitmask
    PointerAdapterFlags subscriptionFlags_{PointerAdapterMask::None};
    /// Last known pointer position.
    /// This is required in case of SDL can't set a mouse position on the platform.
    /// Also it is a floating point vector to handle analog axis input correctly.
    Vector2 pointerPosition_;
    /// Is there an active "pressed" event.
    bool pointerPressed_{false};
    /// Identifier of active touch
    ea::optional<int> activeTouchId_{};
    /// Current cursor velocity.
    float cursorSpeed_{};
    /// Max cursor velocity.
    float maxCursorSpeed_{100};
    /// Cursor acceleration.
    float cursorAcceleration_{1};
};

} //Urho3D
