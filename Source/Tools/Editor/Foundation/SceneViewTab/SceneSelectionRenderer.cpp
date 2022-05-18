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

#include "../../Foundation/SceneViewTab/SceneSelectionRenderer.h"

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Scene/Node.h>

namespace Urho3D
{

void Foundation_SceneSelectionRenderer(Context* context, SceneViewTab* sceneViewTab)
{
    sceneViewTab->RegisterAddon<SceneSelectionRenderer>();
}

SceneSelectionRenderer::SceneSelectionRenderer(Context* context)
    : SceneViewAddon(context)
{
}

void SceneSelectionRenderer::UpdateAndRender(SceneViewPage& scenePage, bool& mouseConsumed)
{
    Scene* scene = scenePage.scene_;

    for (Node* node : scenePage.selection_.nodes_)
    {
        if (node)
        {
            for (Component* component : node->GetComponents())
                DrawSelection(scene, component);
        }
    }

    for (Component* component : scenePage.selection_.components_)
    {
        if (component)
            DrawSelection(scene, component);
    }
}

void SceneSelectionRenderer::DrawSelection(Scene* scene, Component* component)
{
    auto debugRenderer = scene->GetComponent<DebugRenderer>();

    if (auto light = dynamic_cast<Light*>(component))
        light->DrawDebugGeometry(debugRenderer, true);
    else if (auto drawable = dynamic_cast<Drawable*>(component))
        debugRenderer->AddBoundingBox(drawable->GetWorldBoundingBox(), Color::WHITE);
    else if (auto terrain = dynamic_cast<Terrain*>(component))
        ;
    else
        component->DrawDebugGeometry(debugRenderer, true);
}

}
