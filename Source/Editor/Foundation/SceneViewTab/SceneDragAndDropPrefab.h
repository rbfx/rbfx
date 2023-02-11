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
