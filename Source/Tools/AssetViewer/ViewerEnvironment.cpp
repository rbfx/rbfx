// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "ViewerEnvironment.h"

namespace Urho3D
{

using namespace AssetViewerDetail;

namespace
{

const Color rigAmbientColor{0.15f, 0.15f, 0.15f};
const Color sceneAmbientColor{0.05f, 0.05f, 0.05f};
const Color backgroundColor{0.16f, 0.19f, 0.24f};
const float zoneHalfSize = 10000.0f;
const int gridHalfLines = 10;

} // namespace

void ViewerEnvironment::Attach(Scene* scene, Camera* camera)
{
    // The old scene owns the old nodes. Only forget them here.
    rigNode_.Reset();
    keyLightNode_.Reset();
    fillLightNode_.Reset();
    zoneNode_.Reset();
    skyboxNode_.Reset();
    groundNode_.Reset();
    contentBounds_.Clear();
    scene_ = scene;
    camera_ = camera;
    if (camera_)
        camera_->SetFillMode(wireframe_ ? FILL_WIREFRAME : FILL_SOLID);
    Update(BoundingBox());
}

void ViewerEnvironment::SetLightingMode(LightingMode mode)
{
    lightingMode_ = mode;
    UpdateLights();
    UpdateZone();
}

void ViewerEnvironment::SetSkybox(bool enabled)
{
    skybox_ = enabled;
    UpdateSkybox();
    UpdateZone();
}

void ViewerEnvironment::SetGround(bool enabled)
{
    ground_ = enabled;
    UpdateGround();
}

void ViewerEnvironment::SetWireframe(bool enabled)
{
    wireframe_ = enabled;
    if (camera_)
        camera_->SetFillMode(wireframe_ ? FILL_WIREFRAME : FILL_SOLID);
}

const char* ViewerEnvironment::GetLightingModeName(LightingMode mode)
{
    switch (mode)
    {
    case LightingMode::Auto: return "Auto";
    case LightingMode::SceneLights: return "Scene lights";
    case LightingMode::ViewerRig: return "Viewer rig";
    }
    return "Auto";
}

bool ViewerEnvironment::IsEnvironmentNode(const Node* node) const
{
    const Node* environmentNodes[] = {rigNode_, zoneNode_, skyboxNode_, groundNode_};
    for (; node; node = node->GetParent())
    {
        for (const Node* environmentNode : environmentNodes)
        {
            if (environmentNode && node == environmentNode)
                return true;
        }
    }
    return false;
}

bool ViewerEnvironment::SceneHasOwnLights() const
{
    if (!scene_)
        return false;
    ea::vector<Light*> lights;
    scene_->FindComponents<Light>(lights, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    for (Light* light : lights)
    {
        if (!IsEnvironmentNode(light->GetNode()))
            return true;
    }
    return false;
}

bool ViewerEnvironment::IsRigActive() const
{
    return lightingMode_ == LightingMode::ViewerRig || (lightingMode_ == LightingMode::Auto && !SceneHasOwnLights());
}

Node* ViewerEnvironment::CreateTemporaryNode(const char* name)
{
    Node* node = scene_->CreateChild(name);
    node->SetTemporary(true);
    return node;
}

void ViewerEnvironment::Update(const BoundingBox& contentBounds)
{
    if (!scene_)
        return;
    if (contentBounds.Defined() && IsFiniteBoundingBox(contentBounds))
        contentBounds_ = contentBounds;
    UpdateLights();
    UpdateZone();
    UpdateSkybox();
    UpdateGround();
    UpdateRigDirection();
}

void ViewerEnvironment::UpdateLights()
{
    if (!scene_)
        return;
    if (!rigNode_)
    {
        rigNode_ = CreateTemporaryNode("Viewer Rig");

        keyLightNode_ = rigNode_->CreateChild("Key Light");
        keyLightNode_->SetTemporary(true);
        Light* key = keyLightNode_->CreateComponent<Light>();
        key->SetLightType(LIGHT_DIRECTIONAL);
        key->SetBrightness(0.7f);
        key->SetCastShadows(true);

        fillLightNode_ = rigNode_->CreateChild("Fill Light");
        fillLightNode_->SetTemporary(true);
        Light* fill = fillLightNode_->CreateComponent<Light>();
        fill->SetLightType(LIGHT_DIRECTIONAL);
        fill->SetBrightness(0.25f);
        fill->SetCastShadows(false);
        UpdateRigDirection();
    }
    // The rig lights are not counted as scene lights. Compute the state before the toggle.
    const bool active = IsRigActive();
    for (const WeakPtr<Node>* node : {&keyLightNode_, &fillLightNode_})
    {
        if (Light* light = *node ? (*node)->GetComponent<Light>() : nullptr)
            light->SetEnabled(active);
    }
}

void ViewerEnvironment::UpdateRigDirection()
{
    if (!keyLightNode_ || !fillLightNode_)
        return;
    const Quaternion cameraRotation = camera_ ? camera_->GetNode()->GetWorldRotation() : Quaternion::IDENTITY;
    // The key light comes from the upper front-left of the camera. The fill light comes from the other side.
    keyLightNode_->SetWorldDirection(cameraRotation * Vector3(0.5f, -0.65f, 1.0f).Normalized());
    fillLightNode_->SetWorldDirection(cameraRotation * Vector3(-0.8f, 0.15f, 1.0f).Normalized());
}

void ViewerEnvironment::UpdateZone()
{
    if (!scene_)
        return;
    ea::vector<Zone*> zones;
    scene_->FindComponents<Zone>(zones, ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    bool hasOwnZone = false;
    for (Zone* zone : zones)
        hasOwnZone |= !IsEnvironmentNode(zone->GetNode());

    if (hasOwnZone)
    {
        if (zoneNode_)
            zoneNode_->Remove();
        zoneNode_.Reset();
        return;
    }

    if (!zoneNode_)
    {
        zoneNode_ = CreateTemporaryNode("Viewer Zone");
        Zone* zone = zoneNode_->CreateComponent<Zone>();
        zone->SetBoundingBox(BoundingBox(-zoneHalfSize, zoneHalfSize));
    }
    Zone* zone = zoneNode_->GetComponent<Zone>();
    zone->SetAmbientColor(IsRigActive() ? rigAmbientColor : sceneAmbientColor);
    zone->SetFogColor(backgroundColor);
    auto* cache = scene_->GetSubsystem<ResourceCache>();
    zone->SetZoneTexture(skybox_ ? cache->GetResource<TextureCube>("Textures/DefaultSkybox.xml") : nullptr);
    if (!contentBounds_.Defined())
        return;
    ExpandZone(contentBounds_);
}

void ViewerEnvironment::ExpandZone(const BoundingBox& box)
{
    Zone* zone = zoneNode_ ? zoneNode_->GetComponent<Zone>() : nullptr;
    if (!zone || !box.Defined() || !IsFiniteBoundingBox(box))
        return;
    BoundingBox zoneBounds = zone->GetBoundingBox();
    if (zoneBounds.IsInside(box) == INSIDE)
        return;
    zoneBounds.Merge(box);
    zone->SetBoundingBox(zoneBounds);
}

void ViewerEnvironment::UpdateSkybox()
{
    if (!scene_)
        return;
    if (!skybox_)
    {
        if (skyboxNode_)
            skyboxNode_->Remove();
        skyboxNode_.Reset();
        return;
    }
    if (skyboxNode_)
        return;
    auto* cache = scene_->GetSubsystem<ResourceCache>();
    skyboxNode_ = CreateTemporaryNode("Viewer Skybox");
    auto* skybox = skyboxNode_->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/DefaultSkybox.xml"));
    skybox->SetCastShadows(false);
}

void ViewerEnvironment::UpdateGround()
{
    if (!scene_)
        return;
    if (!ground_ || !contentBounds_.Defined())
    {
        if (groundNode_)
            groundNode_->Remove();
        groundNode_.Reset();
        return;
    }
    if (!groundNode_)
    {
        auto* cache = scene_->GetSubsystem<ResourceCache>();
        groundNode_ = CreateTemporaryNode("Viewer Ground");
        auto* model = groundNode_->CreateComponent<StaticModel>();
        model->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
        model->SetMaterial(cache->GetResource<Material>("Materials/DefaultGrey.xml"));
        model->SetCastShadows(false);
    }
    const Vector3 size = contentBounds_.Size();
    const float planeSize = Max(Max(size.x_, size.z_), 0.1f) * 4.0f;
    const float epsilon = Max(size.x_, Max(size.y_, size.z_)) * 0.001f;
    const Vector3 center = contentBounds_.Center();
    groundNode_->SetPosition(Vector3(center.x_, contentBounds_.min_.y_ - epsilon, center.z_));
    groundNode_->SetScale(Vector3(planeSize, 1.0f, planeSize));
}

void ViewerEnvironment::DrawDebug(DebugRenderer* debug)
{
    if (!debug || !contentBounds_.Defined())
        return;

    if (bounds_)
        debug->AddBoundingBox(contentBounds_, Color(1.0f, 0.85f, 0.2f));

    if (grid_)
    {
        const Vector3 size = contentBounds_.Size();
        const float extent = Max(Max(size.x_, size.z_), 0.01f);
        const float step = Pow(10.0f, Round(std::log10(extent / 10.0f)));
        const Vector3 center = contentBounds_.Center();
        const float centerX = Round(center.x_ / step) * step;
        const float centerZ = Round(center.z_ / step) * step;
        const float y = contentBounds_.min_.y_;
        const float half = gridHalfLines * step;
        const Color lineColor(0.45f, 0.5f, 0.58f);
        for (int i = -gridHalfLines; i <= gridHalfLines; ++i)
        {
            const float x = centerX + i * step;
            const float z = centerZ + i * step;
            const bool isAxisX = Abs(z) < step * 0.01f;
            const bool isAxisZ = Abs(x) < step * 0.01f;
            debug->AddLine(Vector3(centerX - half, y, z), Vector3(centerX + half, y, z),
                isAxisX ? Color(0.85f, 0.25f, 0.25f) : lineColor);
            debug->AddLine(Vector3(x, y, centerZ - half), Vector3(x, y, centerZ + half),
                isAxisZ ? Color(0.3f, 0.45f, 0.9f) : lineColor);
        }
    }
}

} // namespace Urho3D
