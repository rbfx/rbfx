//
// Copyright (c) 2017 the Urho3D project.
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


#include "ToolboxAPI.h"
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/SystemUI/SystemUI.h>

namespace Urho3D
{

class URHO3D_TOOLBOX_API DebugCameraController : public LogicComponent
{
    URHO3D_OBJECT(DebugCameraController, LogicComponent);
public:
    /// Construct.
    explicit DebugCameraController(Context* context);
    /// Process inputs and update camera.
    virtual void RunFrame(float timeStep) = 0;
    /// Control camera.
    void Update(float timeStep) override;
    /// Get current camera speed in units per second.
    float GetSpeed() const { return speed_; }
    /// Set current camera speed in units per second.
    void SetSpeed(float speed) { speed_ = speed; }

protected:
    /// Current camera speed.
    float speed_ = 2.f;
};

class URHO3D_TOOLBOX_API DebugCameraController3D : public DebugCameraController
{
    URHO3D_OBJECT(DebugCameraController3D, DebugCameraController);
public:
    /// Construct.
    explicit DebugCameraController3D(Context* context);
    /// Process inputs and update camera.
    void RunFrame(float timeStep) override;
    /// Tell this camera which where is the rotation center.
    void SetRotationCenter(const Vector3& center);
    /// Disable the rotation center
    void ClearRotationCenter() { isRotationCenterValid_ = false; }

protected:
    /// Current mouse sensitivity.
    float mouseSensitivity_ = 0.1f;
    /// Where the camera should rotate around
    Vector3 rotationCenter_ = Vector3::ZERO;
    /// If the rotationCenter_ is valid.
    bool isRotationCenterValid_ = false;
};

class URHO3D_TOOLBOX_API DebugCameraController2D : public DebugCameraController
{
URHO3D_OBJECT(DebugCameraController2D, DebugCameraController);
public:
    /// Construct.
    explicit DebugCameraController2D(Context* context);
    /// Process inputs and update camera.
    void RunFrame(float timeStep) override;
};

};
