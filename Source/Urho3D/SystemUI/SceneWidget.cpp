// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/SystemUI/SceneWidget.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/RenderPipeline/ShaderConsts.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Input/MoveAndOrbitComponent.h>

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
    const ImVec2 imageBegin = ui::GetCursorPos();
    Widgets::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));
    const ImVec2 imageEnd = ui::GetCursorPos();

    // Enable basic orbit controls similar to MaterialInspectorWidget
    if (auto* camera = renderer->GetCamera())
    {
        Node* cameraNode = camera->GetNode();
        auto* moveAndOrbit = cameraNode->GetOrCreateComponent<MoveAndOrbitComponent>();

        float distance = cameraNode->GetPosition().Length();
        // Interact only when the image is hovered
        if (ui::IsItemHovered())
        {
            if (ui::IsMouseDown(MOUSEB_RIGHT))
            {
                const Vector2 mouseDelta = ToVector2(ui::GetIO().MouseDelta);
                moveAndOrbit->SetYaw(moveAndOrbit->GetYaw() + mouseDelta.x_ * 0.9f);
                moveAndOrbit->SetPitch(moveAndOrbit->GetPitch() + mouseDelta.y_ * 0.9f);
            }

            if (Abs(ui::GetMouseWheel()) > 0.05f)
            {
                if (ui::GetMouseWheel() > 0.0f)
                    distance *= 0.8f;
                else
                    distance *= 1.3f;
            }
        }
        distance = Clamp(distance, moveAndOrbit->GetMinDistance(), moveAndOrbit->GetMaxDistance());

        cameraNode->SetRotation(moveAndOrbit->GetYawPitchRotation());
        cameraNode->SetPosition(distance * (cameraNode->GetRotation() * Vector3::BACK));
    }
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

    auto* zone = scene->FindComponent<Zone>();
    if (zone)
    {
        zone->SetZoneTexture(texture);
    }
    auto* skybox = scene->FindComponent<Skybox>();
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
