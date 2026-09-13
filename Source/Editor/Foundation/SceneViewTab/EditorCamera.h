// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"
#include "Editor/Foundation/Shared/CameraController.h"

namespace Urho3D
{

void Foundation_EditorCamera(Context* context, SceneViewTab* sceneViewTab);

/// Camera controller used by Scene View.
class EditorCamera : public SceneViewAddon
{
    URHO3D_OBJECT(EditorCamera, SceneViewAddon);

public:
    using SettingsPage = SimpleSettingsPage<CameraController::Settings>;

    EditorCamera(SceneViewTab* owner, CameraController::SettingsPage* settings);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "Camera"; }
    int GetInputPriority() const override { return M_MAX_INT; }
    void ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed) override;
    void SerializePageState(Archive& archive, const char* name, ea::any& stateWrapped) const override;
    /// @}

private:
    CameraController::PageState& GetOrInitializeState(SceneViewPage& scenePage) const;
    void LookAtPosition(SceneViewPage& scenePage, const Vector3& position) const;

    const WeakPtr<CameraController::SettingsPage> settings_;
    SharedPtr<CameraController> cameraController_;
    bool isActive_{};
};

}
