//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../Core/CommonEditorActions.h"
#include "../Core/IniHelpers.h"
#include "../Foundation/CustomSceneViewTab.h"

#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Widgets.h>

namespace Urho3D
{

CustomSceneViewTab::CustomSceneViewTab(Context* context, const ea::string& title, const ea::string& guid,
    EditorTabFlags flags, EditorTabPlacement placement)
    : ResourceEditorTab(context, title, guid, flags, placement)
    , scene_(MakeShared<Scene>(context))
    , renderer_(MakeShared<SceneRendererToTexture>(scene_))
{
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();
    scene_->CreateComponent<Octree>();
    auto* zone = scene_->CreateComponent<Zone>();
    auto* skybox = scene_->CreateComponent<Skybox>();
    zone->SetBoundingBox(BoundingBox(Vector3::ONE * -1000.0f, Vector3::ONE * 1000.0f));
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/DefaultSkybox.xml"));
    zone->SetZoneTexture(cache->GetResource<TextureCube>("Textures/DefaultSkybox.xml"));
    lightNode_ = scene_->CreateChild("DirectionalLight");
    auto* light = lightNode_->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    lightNode_->LookAt(Vector3::FORWARD - Vector3::UP);
    renderer_->SetActive(true);
}

CustomSceneViewTab::~CustomSceneViewTab()
{
}

void CustomSceneViewTab::RenderContent()
{
    if (!scene_->HasComponent<DebugRenderer>())
    {
        DebugRenderer* debug = scene_->GetOrCreateComponent<DebugRenderer>();
        debug->SetTemporary(true);
        debug->SetLineAntiAlias(true);
    }

    renderer_->SetTextureSize(GetContentSize());
    renderer_->Update();

    const ImVec2 basePosition = ui::GetCursorPos();

    Texture2D* sceneTexture = renderer_->GetTexture();
    ui::SetCursorPos(basePosition);
    Widgets::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));
}

} // namespace Urho3D
