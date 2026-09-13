// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
