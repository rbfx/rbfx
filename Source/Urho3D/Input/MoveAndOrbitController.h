//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "Urho3D/Scene/LogicComponent.h"
#include "Urho3D/Input/InputMap.h"

namespace Urho3D
{
class MoveAndOrbitComponent;
class InputTranslator;

class URHO3D_API MoveAndOrbitController : public LogicComponent
{
    URHO3D_OBJECT(MoveAndOrbitController, LogicComponent);

public:
    static inline constexpr float DEFAULT_MOUSE_SENSITIVITY{0.1f};
    static inline constexpr float DEFAULT_TOUCH_MOVEMENT_SENSITIVITY{0.2f};
    static inline constexpr float DEFAULT_TOUCH_ROTATION_SENSITIVITY{0.2f};
    static inline constexpr float DEFAULT_AXIS_ROTATION_SENSITIVITY{100.0f};

    static inline const ea::string MOUSE_SENSITIVITY{"MouseSensitivity"};
    static inline const ea::string AXIS_ROTATION_SENSITIVITY{"AxisRotationSensitivity"};
    static inline const ea::string TOUCH_MOVEMENT_SENSITIVITY{"TouchMovementSensitivity"};
    static inline const ea::string TOUCH_ROTATION_SENSITIVITY{"TouchRotationSensitivity"};
    
    static inline const ea::string ACTION_FORWARD{"Forward"};
    static inline const ea::string ACTION_BACK{"Back"};
    static inline const ea::string ACTION_LEFT{"Left"};
    static inline const ea::string ACTION_RIGHT{"Right"};
    static inline const ea::string ACTION_TURNLEFT{"TurnLeft"};
    static inline const ea::string ACTION_TURNRIGHT{"TurnRight"};
    static inline const ea::string ACTION_LOOKUP{"LookUp"};
    static inline const ea::string ACTION_LOOKDOWN{"LookDown"};

    /// Construct.
    explicit MoveAndOrbitController(Context* context);

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    void ApplyAttributes() override;
    /// Handle enabled/disabled state change. Changes update event subscription.
    void OnSetEnabled() override;

    /// Load input map from config file or resource.
    void LoadInputMap(const ea::string& name);
    /// Set input map.
    void SetInputMap(InputMap* inputMap);
    /// Return input map.
    InputMap* GetInputMap() const;
    /// Set input map attribute.
    void SetInputMapAttr(const ResourceRef& value);
    /// Return input map attribute.
    ResourceRef GetInputMapAttr() const;

    /// Set UI element to filter touch events for movement.
    /// Only touch events originated in the element going to be handled.
    void SetMovementUIElement(UIElement* element);
    /// Set UI element to filter touch events for rotation.
    /// Only touch events originated in the element going to be  handled.
    void SetRotationUIElement(UIElement* element);
    /// Get UI element to filter touch events for movement.
    UIElement* GetMovementUIElement() const { return movementUIElement_; }
    /// Get UI element to filter touch events for rotation.
    UIElement* GetRotationUIElement() const { return rotationUIElement_; }


    /// Called before the first update. At this point all other components of the node should exist. Will also be called
    /// if update events are not wanted; in that case the event is immediately unsubscribed afterward.
    void DelayedStart() override;
    /// Called on scene update, variable timestep.
    void Update(float timeStep) override;

private:
    /// Connect to MoveAndOrbitComponent if possible.
    void ConnectToComponent();
    /// Evaluate active touch areas.
    void EvaluateTouchRects(IntRect& movementRect, IntRect& rotationRect) const;
    /// Find touch states for movement and rotation.
    void FindTouchStates(const IntRect& movementRect, const IntRect& rotationRect, TouchState*& movementTouch,
        TouchState*& rotationTouch);
    /// Input map.
    SharedPtr<InputMap> inputMap_;
    /// Component that provides animation states for the model.
    WeakPtr<MoveAndOrbitComponent> component_;

    /// UI element to filter touch events
    WeakPtr<UIElement> movementUIElement_{};
    /// UI element to filter touch events
    WeakPtr<UIElement> rotationUIElement_{};

    int movementTouchId_{-1};
    int rotationTouchId_{-1};
    IntVector2 movementTouchOrigin_{};
};


} // namespace Urho3D
