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
