// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/CommonEditorActionBuilders.h"
#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"

#include <EASTL/unique_ptr.h>

namespace Urho3D
{

void Foundation_SceneDragAndDropPrefab(Context* context, SceneViewTab* sceneViewTab);

/// Addon to create new nodes via drag&drop.
class SceneDragAndDropPrefab : public SceneViewAddon
{
    URHO3D_OBJECT(SceneDragAndDropPrefab, SceneViewAddon);

public:
    explicit SceneDragAndDropPrefab(SceneViewTab* owner);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "DragAndDropPrefab"; }
    bool IsDragDropPayloadSupported(SceneViewPage& page, DragDropPayload* payload) const override;
    void BeginDragDrop(SceneViewPage& page, DragDropPayload* payload) override;
    void UpdateDragDrop(DragDropPayload* payload) override;
    void CompleteDragDrop(DragDropPayload* payload) override;
    void CancelDragDrop() override;
    /// @}

private:
    static constexpr float DefaultDistance = 10.0f; // TODO: Make configurable

    struct HitResult
    {
        bool hit_{};
        float distance_{};
        Vector3 position_;
        Vector3 normal_;
    };

    void CreateNodeFromPrefab(Scene* scene, const ResourceFileDescriptor& desc);
    void CreateNodeFromModel(Scene* scene, const ResourceFileDescriptor& desc);

    HitResult QueryHoveredGeometry(Scene* scene, const Ray& cameraRay);

    SharedPtr<Node> temporaryNode_;
    WeakPtr<SceneViewPage> currentPage_;
    ea::unique_ptr<CreateNodeActionBuilder> nodeActionBuilder_;
};

} // namespace Urho3D
