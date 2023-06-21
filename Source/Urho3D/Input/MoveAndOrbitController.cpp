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

namespace Urho3D
{

MoveAndOrbitController::MoveAndOrbitController(Context* context)
    : BaseClassName(context)
{
    SetUpdateEventMask(USE_UPDATE);
}

void MoveAndOrbitController::RegisterObject(Context* context)
{
    context->AddFactoryReflection<MoveAndOrbitController>();
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Input Map", GetInputMapAttr, SetInputMapAttr, ResourceRef, ResourceRef(InputMap::GetTypeStatic()), AM_DEFAULT);
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

void MoveAndOrbitController::DelayedStart()
{
    LogicComponent::DelayedStart();
    ConnectToComponent();
}

void MoveAndOrbitController::Update(float timeStep)
{
    LogicComponent::Update(timeStep);

    if (!component_ || !inputMap_)
        return;

    const auto input = GetSubsystem<Input>();
    float yaw = component_->GetYaw();
    float pitch = component_->GetPitch();

    auto forward = inputMap_->Evaluate(ACTION_FORWARD) - inputMap_->Evaluate(ACTION_BACK);
    auto right = inputMap_->Evaluate(ACTION_RIGHT) - inputMap_->Evaluate(ACTION_LEFT);
    auto turnRight = inputMap_->Evaluate(ACTION_TURNRIGHT) - inputMap_->Evaluate(ACTION_TURNLEFT);
    auto lookDown = inputMap_->Evaluate(ACTION_LOOKDOWN) - inputMap_->Evaluate(ACTION_LOOKUP);

    {
        auto sensitivity = inputMap_->GetMetadata(AXIS_ROTATION_SENSITIVITY).GetFloat();
        if (sensitivity == 0)
            sensitivity = DEFAULT_AXIS_ROTATION_SENSITIVITY;
        yaw += turnRight * sensitivity * timeStep;
        pitch += lookDown * sensitivity * timeStep;
    }

    IntRect rotationRect, movementRect;
    EvaluateTouchRects(movementRect, rotationRect);
    TouchState* movementTouch = nullptr;
    TouchState* rotationTouch = nullptr;
    FindTouchStates(movementRect, rotationRect, movementTouch, rotationTouch);

    if (movementTouch)
    {
        auto sensitivity = inputMap_->GetMetadata(TOUCH_MOVEMENT_SENSITIVITY).GetFloat();
        if (sensitivity == 0)
            sensitivity = DEFAULT_TOUCH_MOVEMENT_SENSITIVITY;
        const auto delta = movementTouch->position_ - movementTouchOrigin_;
        right += delta.x_ * sensitivity;
        forward -= delta.y_ * sensitivity;
    }

    if (rotationTouch)
    {
        auto sensitivity = inputMap_->GetMetadata(TOUCH_ROTATION_SENSITIVITY).GetFloat();
        if (sensitivity == 0)
            sensitivity = DEFAULT_TOUCH_ROTATION_SENSITIVITY;
        yaw += sensitivity * static_cast<float>(rotationTouch->delta_.x_);
        pitch += sensitivity * static_cast<float>(rotationTouch->delta_.y_);
    }

    {
        auto sensitivity = inputMap_->GetMetadata(MOUSE_SENSITIVITY).GetFloat();
        if (sensitivity == 0)
            sensitivity = DEFAULT_MOUSE_SENSITIVITY;
        yaw += static_cast<float>(input->GetMouseMoveX()) * sensitivity;
        pitch += static_cast<float>(input->GetMouseMoveY()) * sensitivity;
    }
    component_->SetPitch(pitch);
    component_->SetYaw(yaw);
    component_->SetVelocity(Vector3(Clamp(right, -1.0f, 1.0f), 0, Clamp(forward, -1.0f, 1.0f)));
}

void MoveAndOrbitController::ConnectToComponent()
{
    if (node_)
        component_ = node_->GetComponentImplementation<MoveAndOrbitComponent>();
    else
        component_ = nullptr;
}

void MoveAndOrbitController::EvaluateTouchRects(IntRect& movementRect, IntRect& rotationRect) const
{
    IntRect screenRect;

    const auto renderer = GetSubsystem<Renderer>();
    if (renderer)
    {
        const auto viewport = renderer->GetViewportForScene(GetScene(), 0);
        screenRect = viewport->GetRect();
        if (screenRect == IntRect::ZERO)
        {
            const auto graphics = GetSubsystem<Graphics>();
            if (graphics)
            {
                screenRect = graphics->GetViewport();
            }
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
        auto halfSize = IntVector2(movementRect.Width() / 2, movementRect.Height());
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
} // namespace Urho3D
