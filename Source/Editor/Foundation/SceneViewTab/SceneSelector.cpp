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
