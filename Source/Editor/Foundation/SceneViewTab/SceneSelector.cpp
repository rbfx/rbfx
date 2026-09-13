// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/SceneViewTab/SceneSelector.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/SystemUI/SystemUI.h>

namespace Urho3D
{

void Foundation_SceneSelector(Context* context, SceneViewTab* sceneViewTab)
{
    sceneViewTab->RegisterAddon<SceneSelector>();
}

SceneSelector::SceneSelector(SceneViewTab* owner)
    : SceneViewAddon(owner)
{
}

void SceneSelector::ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed)
{
    Scene* scene = scenePage.scene_;

    if (!mouseConsumed)
    {
        if (ui::IsItemHovered() && ui::IsMouseReleased(MOUSEB_LEFT) && !ui::IsMouseDragPastThreshold(MOUSEB_LEFT))
        {
            mouseConsumed = true;
            Node* selectedNode = QuerySelectedNode(scene, scenePage.cameraRay_);

            const bool toggle = ui::IsKeyDown(KEY_LCTRL) || ui::IsKeyDown(KEY_RCTRL);
            const bool append = ui::IsKeyDown(KEY_LSHIFT) || ui::IsKeyDown(KEY_RSHIFT);
            SelectNode(scenePage.selection_, selectedNode, toggle, append);
        }
    }
}

Drawable* SceneSelector::QuerySelectedDrawable(Scene* scene, const Ray& cameraRay, RayQueryLevel level) const
{
    const auto results = QueryGeometriesFromScene(scene, cameraRay);

    for (const RayQueryResult& result : results)
    {
        if (result.drawable_->GetScene() != nullptr)
            return result.drawable_;
    }

    return nullptr;
}

Node* SceneSelector::QuerySelectedNode(Scene* scene, const Ray& cameraRay) const
{
    Drawable* selectedDrawable = QuerySelectedDrawable(scene, cameraRay, RAY_TRIANGLE);
    if (!selectedDrawable)
        selectedDrawable = QuerySelectedDrawable(scene, cameraRay, RAY_OBB);

    Node* selectedNode = selectedDrawable ? selectedDrawable->GetNode() : nullptr;

    while (selectedNode && selectedNode->IsTemporary())
        selectedNode = selectedNode->GetParent();

    return selectedNode;
}

void SceneSelector::SelectNode(SceneSelection& selection, Node* node, bool toggle, bool append) const
{
    selection.ConvertToNodes();

    if (toggle)
        selection.SetSelected(node, !selection.IsSelected(node));
    else if (append)
        selection.SetSelected(node, true);
    else
    {
        selection.Clear();
        selection.SetSelected(node, true);
    }
}

}
