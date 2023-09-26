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

#include "SceneWidget.h"
#include "../Scene/Scene.h"
#include "../Graphics/Octree.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Skybox.h"
#include "../Graphics/Zone.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/Model.h"
#include "../Graphics/Material.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../Resource/ResourceCache.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Light.h"

namespace Urho3D
{

SceneWidget::SceneWidget(Context* context): BaseClassName(context)
{
}

SceneWidget::~SceneWidget()
{
}

void SceneWidget::RenderContent()
{
    if (!scene_)
        return;

    auto* renderer = GetRenderer();

    if (!scene_->HasComponent<DebugRenderer>())
    {
        DebugRenderer* debug = scene_->GetOrCreateComponent<DebugRenderer>();
        debug->SetTemporary(true);
        debug->SetLineAntiAlias(true);
    }
    const ImVec2 contentPosition = ui::GetCursorPos();

    if (lightPivotNode_)
    {
        const auto cameraNode = renderer->GetCameraNode();
        lightPivotNode_->SetRotation(cameraNode->GetWorldRotation());
    }

    const auto contentSize = ui::GetContentRegionAvail();
    renderer->SetTextureSize(ToIntVector2(contentSize));
    renderer->Update();

    Texture2D* sceneTexture = renderer->GetTexture();
    ui::SetCursorPos(contentPosition);
    Widgets::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));
}

Scene* SceneWidget::CreateDefaultScene()
{
    ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
    renderer_ = nullptr;
    scene_ = MakeShared<Scene>(context_);
    scene_->CreateComponent<Octree>();
    auto* zone = scene_->CreateComponent<Zone>();
    auto* skybox = scene_->CreateComponent<Skybox>();
    zone->SetBoundingBox(BoundingBox(Vector3::ONE * -1000.0f, Vector3::ONE * 1000.0f));
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/DefaultSkybox.xml"));
    zone->SetZoneTexture(cache->GetResource<TextureCube>("Textures/DefaultSkybox.xml"));
    lightPivotNode_ = scene_->CreateChild("DirectionalLightPivot");
    lightNode_ = lightPivotNode_->CreateChild("DirectionalLight");
    auto* light = lightNode_->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    lightNode_->LookAt(Vector3::FORWARD - Vector3::UP);
    return scene_;
}

SceneRendererToTexture* SceneWidget::GetRenderer()
{
    if (!scene_)
        return nullptr;

    if (!renderer_)
    {
        renderer_ = MakeShared<SceneRendererToTexture>(scene_);
        renderer_->SetActive(true);
    }
    return renderer_;
}

void SceneWidget::LookAt(const BoundingBox& box)
{
    auto* camera = GetCamera();
    if (!camera)
        return;

    auto node = camera->GetNode();
    node->SetPosition(box.Center() + box.Size().Length() * Vector3(1, 1, 1));
    node->LookAt(box.Center());
}


void SceneWidget::SetSkyboxTexture(Texture* texture)
{
    Scene* scene = GetScene();
    if (!scene)
        return;

    auto* zone = scene->GetComponent<Zone>(true);
    if (zone)
    {
        zone->SetZoneTexture(texture);
    }
    auto* skybox = scene->GetComponent<Skybox>(true);
    if (skybox)
    {
        auto material = MakeShared<Material>(context_);
        auto* cache = context_->GetSubsystem<ResourceCache>();
        material->SetTechnique(0, cache->GetResource<Technique>("Techniques/DiffSkybox.xml"));
        material->SetTexture(ShaderResources::Albedo, texture);
        material->SetCullMode(CULL_NONE);
        skybox->SetMaterial(material);
    }
}

} // namespace Urho3D
