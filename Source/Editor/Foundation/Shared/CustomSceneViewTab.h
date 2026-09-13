// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/CommonEditorActions.h"
#include "../../Project/Project.h"
#include "../../Project/ResourceEditorTab.h"
#include "../../Foundation/Shared/CameraController.h"

#include <Urho3D/SystemUI/SceneWidget.h>
#include <Urho3D/Graphics/Animation.h>

namespace Urho3D
{

/// Tab that renders custom Scene.
class CustomSceneViewTab : public ResourceEditorTab
{
    URHO3D_OBJECT(CustomSceneViewTab, ResourceEditorTab)

public:
    CustomSceneViewTab(Context* context, const ea::string& title, const ea::string& guid, EditorTabFlags flags,
        EditorTabPlacement placement);
    ~CustomSceneViewTab() override;

    /// ResourceEditorTab implementation
    /// @{
    void RenderContent() override;
    /// @}

    virtual void ResetCamera();

    Scene* GetScene() const { return preview_ ? preview_->GetScene() : nullptr; }
    Camera* GetCamera() const { return preview_ ? preview_->GetCamera() : nullptr; }

protected:
    virtual void RenderTitle();

    const SharedPtr<SceneWidget> preview_;
    SharedPtr<CameraController> cameraController_;
    CameraController::PageState state_;
};

} // namespace Urho3D
