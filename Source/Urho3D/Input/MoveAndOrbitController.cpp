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

#include "Urho3D/Input/MoveAndOrbitController.h"

#include "Urho3D/Input/MoveAndOrbitComponent.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/IO/VirtualFileSystem.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

MoveAndOrbitController::MoveAndOrbitController(Context* context)
    : BaseClassName(context)
{
    UpdateEventSubscription();
}

MoveAndOrbitController::~MoveAndOrbitController()
{
}

void MoveAndOrbitController::RegisterObject(Context* context)
{
    context->AddFactoryReflection<MoveAndOrbitController>();
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE(
        "Input Map", GetInputMapAttr, SetInputMapAttr, ResourceRef, ResourceRef(InputMap::GetTypeStatic()), AM_DEFAULT);
}

void MoveAndOrbitController::ApplyAttributes()
{
    BaseClassName::ApplyAttributes();
}

void MoveAndOrbitController::OnSetEnabled()
{
    BaseClassName::OnSetEnabled();
}

void MoveAndOrbitController::LoadInputMap(const ea::string& name)
{
    SetInputMapAttr(ResourceRef(InputMap::GetTypeStatic(), "Input/MoveAndOrbit.inputmap"));
}

void MoveAndOrbitController::SetInputMap(InputMap* inputMap)
{
    inputMap_ = inputMap;
}

InputMap* MoveAndOrbitController::GetInputMap() const
{
    return inputMap_;
}

void MoveAndOrbitController::SetInputMapAttr(const ResourceRef& value)
{
    SetInputMap(InputMap::Load(context_, value.name_));
}

ResourceRef MoveAndOrbitController::GetInputMapAttr() const
{
    return GetResourceRef(GetInputMap(), InputMap::GetTypeStatic());
}

void MoveAndOrbitController::SetMovementUIElement(UIElement* element)
{
    movementUIElement_ = element;
}

void MoveAndOrbitController::SetRotationUIElement(UIElement* element)
{
    rotationUIElement_ = element;
}

void MoveAndOrbitController::OnNodeSet(Node* previousNode, Node* currentNode)
{
    Component::OnNodeSet(previousNode, currentNode);

    connectToComponentCalled_ = false;
    UpdateEventSubscription();
}

void MoveAndOrbitController::OnSceneSet(Scene* scene)
{
    Component::OnSceneSet(scene);

    UpdateEventSubscription();
}

void MoveAndOrbitController::HandleInputEnd(StringHash eventName, VariantMap& eventData)
{
    if (!connectToComponentCalled_)
    {
        ConnectToComponent();
    }

    if (!component_)
        return;

    const auto input = GetSubsystem<Input>();
    float yaw = component_->GetYaw();
    float pitch = component_->GetPitch();
    float originalYaw = yaw;
    float originalPitch = pitch;
    Vector3 originalVelocity = component_->GetVelocity();

    float forward = 0.0f;
    float right = 0.0f;
    float turnRight = 0.0f;
    float lookDown = 0.0f;

    if (inputMap_)
    {
        forward = inputMap_->Evaluate(ACTION_FORWARD) - inputMap_->Evaluate(ACTION_BACK);
        right = inputMap_->Evaluate(ACTION_RIGHT) - inputMap_->Evaluate(ACTION_LEFT);
        turnRight = inputMap_->Evaluate(ACTION_TURNRIGHT) - inputMap_->Evaluate(ACTION_TURNLEFT);
        lookDown = inputMap_->Evaluate(ACTION_LOOKDOWN) - inputMap_->Evaluate(ACTION_LOOKUP);
    }

    {
        auto sensitivity = GetSensitivity(AXIS_ROTATION_SENSITIVITY, DEFAULT_AXIS_ROTATION_SENSITIVITY);
        const float timeStep = context_->GetSubsystem<Time>()->GetTimeStep();
        yaw += turnRight * sensitivity * timeStep;
        pitch += lookDown * sensitivity * timeStep;
    }

    IntRect rotationRect, movementRect;
    EvaluateTouchRects(movementRect, rotationRect);
    TouchState* movementTouch = nullptr;
    TouchState* rotationTouch = nullptr;
    FindTouchStates(movementRect, rotationRect, movementTouch, rotationTouch);

    auto dpi = GetSubsystem<Graphics>()->GetDisplayDPI().z_;
    dpi = dpi ? dpi : 96;

    if (movementTouch)
    {
        const auto sensitivity = GetSensitivity(TOUCH_MOVEMENT_SENSITIVITY, DEFAULT_TOUCH_MOVEMENT_SENSITIVITY);
        const float halfAreaSize = Min(movementRect.Width(),movementRect.Height())*0.45f;
        const float fullMotion = Min(dpi/sensitivity,halfAreaSize);
        const auto delta = movementTouch->position_ - movementTouchOrigin_;
        right += Clamp(delta.x_ / fullMotion, -1.0f, 1.0f);
        forward -= Clamp(delta.y_ / fullMotion, -1.0f, 1.0f);
    }

    if (rotationTouch)
    {
        const auto sensitivity = GetSensitivity(TOUCH_ROTATION_SENSITIVITY, DEFAULT_TOUCH_ROTATION_SENSITIVITY);
        const float halfAreaSize = Min(rotationRect.Width(),rotationRect.Height())*0.45f;
        const float halfPiDistance = Min(dpi/sensitivity,halfAreaSize);
        yaw += static_cast<float>(rotationTouch->delta_.x_)/halfPiDistance*90.0f;
        pitch += static_cast<float>(rotationTouch->delta_.y_)/halfPiDistance*90.0f;
    }

    if (input->GetMouseMode() != MM_FREE || input->GetMouseButtonDown(MOUSEB_RIGHT))
    {
        const auto sensitivity = GetSensitivity(MOUSE_SENSITIVITY, DEFAULT_MOUSE_SENSITIVITY);
        auto move = input->GetMouseMove();
        yaw += static_cast<float>(move.x_) * sensitivity;
        pitch += static_cast<float>(move.y_) * sensitivity;
    }
    pitch = Clamp(pitch, -90.0f, 90.0f);
    if (!Equals(originalPitch, pitch))
        component_->SetPitch(pitch);
    if (!Equals(originalYaw, yaw))
        component_->SetYaw(yaw);
    const auto velocity = Vector3(Clamp(right, -1.0f, 1.0f), 0, Clamp(forward, -1.0f, 1.0f));
    if (!originalVelocity.Equals(velocity))
        component_->SetVelocity(velocity);
}


void MoveAndOrbitController::UpdateEventSubscription()
{
    auto subscribe = IsEnabledEffective() && (GetScene() && GetScene()->IsUpdateEnabled());
    if (subscribed_ != subscribe)
    {
        subscribed_ = subscribe;
        if (subscribed_)
        {
            SubscribeToEvent(E_INPUTEND, URHO3D_HANDLER(MoveAndOrbitController, HandleInputEnd));
        }
        else
        {
            UnsubscribeFromEvent(E_INPUTEND);
        }
    }
}

void MoveAndOrbitController::ConnectToComponent()
{
    if (node_)
    {
        component_ = node_->GetDerivedComponent<MoveAndOrbitComponent>();
        if (!component_)
        {
            URHO3D_LOGERROR("MoveAndOrbitComponent not found on the same node as MoveAndOrbitController");
        }
    }
    else
    {
        component_ = nullptr;
    }

    connectToComponentCalled_ = true;
}

void MoveAndOrbitController::EvaluateTouchRects(IntRect& movementRect, IntRect& rotationRect) const
{
    IntRect screenRect{IntRect::ZERO};

    if (const auto renderer = GetSubsystem<Renderer>())
    {
        if (const auto viewport = renderer->GetViewportForScene(GetScene(), 0))
        {
            screenRect = viewport->GetRect();
        }
    }

    if (screenRect == IntRect::ZERO)
    {
        if (const auto graphics = GetSubsystem<Graphics>())
        {
            screenRect = {IntVector2::ZERO, graphics->GetSwapChainSize()};
        }
    }

    if (movementUIElement_)
    {
        movementRect = movementUIElement_->GetCombinedScreenRect();
    }
    else
    {
        movementRect = screenRect;
    }
    if (rotationUIElement_)
    {
        rotationRect = rotationUIElement_->GetCombinedScreenRect();
    }
    else
    {
        rotationRect = screenRect;
    }
    if (movementUIElement_ == rotationUIElement_)
    {
        const auto halfSize = IntVector2(movementRect.Width() / 2, movementRect.Height());
        movementRect = IntRect(movementRect.Min(), movementRect.Min() + halfSize);
        rotationRect = IntRect(rotationRect.Min() + IntVector2(halfSize.x_, 0), rotationRect.Max());
    }
}

void MoveAndOrbitController::FindTouchStates(const IntRect& movementRect, const IntRect& rotationRect,
    TouchState*& movementTouch, TouchState*& rotationTouch)
{
    const auto input = GetSubsystem<Input>();
    for (unsigned touchIndex = 0; touchIndex < input->GetNumTouches(); ++touchIndex)
    {
        auto touch = input->GetTouch(touchIndex);
        if (movementTouchId_ < 0 && touch->touchedElement_ == movementUIElement_
            && movementRect.Contains(touch->position_))
        {
            movementTouchId_ = touch->touchID_;
            movementTouchOrigin_ = touch->position_;
        }
        if (touch->touchID_ == movementTouchId_)
        {
            movementTouch = touch;
        }
        if (rotationTouchId_ < 0 && touch->touchedElement_ == rotationUIElement_
            && rotationRect.Contains(touch->position_))
        {
            rotationTouchId_ = touch->touchID_;
        }
        if (touch->touchID_ == rotationTouchId_)
        {
            rotationTouch = touch;
        }
    }
    if (!movementTouch)
    {
        movementTouchId_ = -1;
    }
    if (!rotationTouch)
    {
        rotationTouchId_ = -1;
    }
}

float MoveAndOrbitController::GetSensitivity(const ea::string& key, float defaultValue) const
{
    if (!inputMap_)
        return defaultValue;
    const auto sensitivity = inputMap_->GetMetadata(key).GetFloat();
    if (sensitivity == 0)
        return  defaultValue;
    return sensitivity;
}

} // namespace Urho3D
