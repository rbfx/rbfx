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


#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/PluginApplication.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/LogicComponent.h>


namespace Urho3D
{

/// A custom component provided by the plugin.
class FPSCameraController : public LogicComponent
{
    URHO3D_OBJECT(FPSCameraController, LogicComponent);
public:
    FPSCameraController(Context* context) : LogicComponent(context)
    {
        SetUpdateEventMask(USE_UPDATE);
    }

    void Update(float timeStep) override
    {
        Input* input = context_->GetSubsystem<Input>();
        IntVector2 delta = input->GetMouseMove();

        auto yaw = GetNode()->GetRotation().EulerAngles().x_;
        if ((yaw > -90.f && yaw < 90.f) || (yaw <= -90.f && delta.y_ > 0) || (yaw >= 90.f && delta.y_ < 0))
            GetNode()->RotateAround(Vector3::ZERO, Quaternion(0.1f * delta.y_, Vector3::RIGHT), TS_LOCAL);
        GetNode()->RotateAround(GetNode()->GetPosition(), Quaternion(0.1f * delta.x_, Vector3::UP), TS_WORLD);

        // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
        if (input->GetKeyDown(KEY_W))
            GetNode()->Translate(Vector3::FORWARD * timeStep);
        if (input->GetKeyDown(KEY_S))
            GetNode()->Translate(Vector3::BACK * timeStep);
        if (input->GetKeyDown(KEY_A))
            GetNode()->Translate(Vector3::LEFT * timeStep);
        if (input->GetKeyDown(KEY_D))
            GetNode()->Translate(Vector3::RIGHT * timeStep);
    }

    static void RegisterObject(Context* context, PluginApplication* plugin)
    {
        plugin->RegisterFactory<FPSCameraController>("User Components");
    }
};

}
