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
namespace Urho3D
{

class URHO3D_API MoveAndOrbitComponent : public LogicComponent
{
    URHO3D_OBJECT(MoveAndOrbitComponent, LogicComponent);

public:
    /// Construct.
    explicit MoveAndOrbitComponent(Context* context);

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Set movement velocity in node's local space.
    virtual void SetVelocity(const Vector3& velocity);
    /// Set yaw angle in degrees.
    virtual void SetYaw(float yaw);
    /// Set pitch angle in degrees.
    virtual void SetPitch(float pitch);

    /// Get movement velocity in node's local space.
    const Vector3& GetVelocity() const { return velocity_; }
    /// Get yaw angle in degrees.
    float GetYaw() const { return yaw_; }
    /// Get pitch angle in degrees.
    float GetPitch() const { return pitch_; }

    /// Get yaw and pitch rotation.
    Quaternion GetYawPitchRotation() const { return Quaternion(pitch_, yaw_, 0.0f); }

private:
    Vector3 velocity_{};
    float yaw_{};
    float pitch_{};
};

} // namespace Urho3D
