//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../Scene/Scene.h"
#include "../Scene/Node.h"
#include "../Graphics/Camera.h"

namespace Urho3D
{

class URHO3D_API CameraController: public Object
{
    URHO3D_OBJECT(CameraController, Object);

public:
    /// Construct.
    explicit CameraController(Context* context);
    /// Destruct.
    ~CameraController();

    /// Set viewport camera.
    /// @property
    void SetCamera(Camera* camera);
    /// Set whether reacts to input.
    /// @property
    void SetEnabled(bool enable);
    /// Set normal camera speed.
    /// @property
    void SetSpeed(float speed);
    /// Set acceleated camera speed.
    /// @property
    void SetAcceleratedSpeed(float speed);

    /// Return viewport camera.
    /// @property
    Camera* GetCamera() const;
    /// Return whether reacts to input.
    /// @property
    bool IsEnabled() const { return enabled_; }

private:
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    /// Move camera node.
    void MoveCamera(float timeStep, Camera* camera);

private:
    /// Enabled flag.
    bool enabled_{false};
    /// Camera pointer.
    WeakPtr<Camera> camera_;
    /// Camera speed.
    float speed_{20.0f};
    /// Camera accelerated speed.
    float acceleratedSpeed_{100.0f};
    /// Mouse sensitivity
    float mouseSensitivity_{0.1f};
};

}
