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

#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"

namespace Urho3D
{

/// Camera controller used by Scene View.
class CameraController : public Object
{
    URHO3D_OBJECT(CameraController, Object)

public:
    struct Settings
    {
        ea::string GetUniqueName() { return "Editor.Scene:Camera"; }

        void SerializeInBlock(Archive& archive);
        void RenderSettings();

        float mouseSensitivity_{0.25f};
        float minSpeed_{2.0f};
        float maxSpeed_{10.0f};
        float scrollSpeed_{3.5f};
        float acceleration_{1.0f};
        float shiftFactor_{4.0f};
        float focusDistance_{10.0f};
        float focusSpeed_{17.0f};
        bool orthographic_{false};
        float orthoSize_{10.0f};
    };
    using SettingsPage = SimpleSettingsPage<Settings>;

    struct PageState
    {
        Vector3 lastCameraPosition_;
        Quaternion lastCameraRotation_;
        float yaw_{};
        float pitch_{};
        float currentMoveSpeed_{};
        Vector3 pendingOffset_;
        ea::optional<Vector3> orbitPosition_;

        PageState();
        void LookAt(const Vector3& position, const Vector3& target);
        void LookAt(const BoundingBox& box);
        void SerializeInBlock(Archive& archive);
    };

    CameraController(Context* context, HotkeyManager* hotkeyManager);

    void ProcessInput(Camera* camera, PageState& state, const Settings* settings = nullptr);

private:
    void UpdateState(const Settings& settings, const Camera* camera, PageState& state) const;
    Vector2 GetMouseMove() const;
    Vector3 GetMoveDirection() const;

    bool isActive_{};
    HotkeyManager* hotkeyManager_{};
};

} // namespace Urho3D
