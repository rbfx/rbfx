// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"

namespace Urho3D
{

void Foundation_SceneDragAndDropMaterial(Context* context, SceneViewTab* sceneViewTab);

/// Addon to update materials via drag&drop.
class SceneDragAndDropMaterial : public SceneViewAddon
{
    URHO3D_OBJECT(SceneDragAndDropMaterial, SceneViewAddon);

public:
    explicit SceneDragAndDropMaterial(SceneViewTab* owner);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "DragAndDropMaterial"; }
    bool IsDragDropPayloadSupported(SceneViewPage& page, DragDropPayload* payload) const override;
    void BeginDragDrop(SceneViewPage& page, DragDropPayload* payload) override;
    void UpdateDragDrop(DragDropPayload* payload) override;
    void CompleteDragDrop(DragDropPayload* payload) override;
    void CancelDragDrop() override;
    /// @}

private:
    struct MaterialAssignment
    {
        WeakPtr<Drawable> drawable_;
        unsigned materialIndex_{};
        Variant oldMaterial_;
        Variant newMaterial_;
    };

    void ClearAssignment();
    void CreateAssignment(Drawable* drawable, unsigned materialIndex);

    ea::pair<Drawable*, unsigned> QueryHoveredGeometry(Scene* scene, const Ray& cameraRay);

    WeakPtr<SceneViewPage> currentPage_;

    SharedPtr<Material> material_;
    MaterialAssignment temporaryAssignment_;
};

} // namespace Urho3D
