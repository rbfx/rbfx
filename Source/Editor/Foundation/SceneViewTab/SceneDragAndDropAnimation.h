// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"

#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationController.h>

namespace Urho3D
{

void Foundation_SceneDragAndDropAnimation(Context* context, SceneViewTab* sceneViewTab);

/// Addon to update materials via drag&drop.
class SceneDragAndDropAnimation : public SceneViewAddon
{
    URHO3D_OBJECT(SceneDragAndDropAnimation, SceneViewAddon);

public:
    explicit SceneDragAndDropAnimation(SceneViewTab* owner);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "DragAndDropAnimation"; }
    bool IsDragDropPayloadSupported(SceneViewPage& page, DragDropPayload* payload) const override;
    void BeginDragDrop(SceneViewPage& page, DragDropPayload* payload) override;
    void UpdateDragDrop(DragDropPayload* payload) override;
    void CompleteDragDrop(DragDropPayload* payload) override;
    void CancelDragDrop() override;
    /// @}

private:
    ea::pair<AnimationController*, Drawable*> QueryHoveredController(Scene* scene, const Ray& cameraRay);

    WeakPtr<SceneViewPage> currentPage_;

    SharedPtr<Animation> animation_;
    WeakPtr<AnimationController> hoveredController_;
    WeakPtr<Drawable> hoveredDrawable_;
};

} // namespace Urho3D
