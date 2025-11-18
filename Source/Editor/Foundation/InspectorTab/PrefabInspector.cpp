// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/PrefabInspector.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/SystemUI/ModelInspectorWidget.h>
#include <Urho3D/SystemUI/SceneWidget.h>
#include <Urho3D/Input/MoveAndOrbitComponent.h>

namespace Urho3D
{

namespace
{

float CalculateCameraDistance(const BoundingBox& bbox, Camera* camera, float margin)
{
    const float objectRadius = bbox.Size().Length() * 0.5f;
    const float verticalFov = camera->GetFov();
    const float horizontalFov = 2.0f * Atan(Tan(verticalFov * 0.5f) * camera->GetAspectRatio());
    const float minFov = Min(verticalFov, horizontalFov);
    const float distance = objectRadius / Tan(minFov * 0.5f) * (1.0f + margin);
    return distance;
}

Vector3 CalculateOptimalCameraDirection(const BoundingBox& bbox, float topDominanceMargin)
{
    // Calculate visible area for each primary viewing direction
    const Vector3 size = bbox.Size();
    const float areaYZ = size.y_ * size.z_; // Looking along X axis
    const float areaXZ = size.x_ * size.z_; // Looking along Y axis (top/bottom)
    const float areaXY = size.x_ * size.y_; // Looking along Z axis

    // Heuristic:
    // - Prefer top-down if object is very thin vertically vs its widest horizontal dimension
    // - Otherwise, pick the side (RIGHT or FORWARD) that shows the largest visible area
    // - If top area clearly dominates best side by margin, also pick top
    const float bestSideArea = Max(areaYZ, areaXY);

    if (areaXZ >= bestSideArea * topDominanceMargin)
        return Vector3::UP;      // Very thin or clearly best from top: look from top
    else if (areaYZ >= areaXY)
        return Vector3::RIGHT;   // Look from the right (Y-Z plane is larger)
    else
        return Vector3::FORWARD; // Look from the front (X-Y plane is larger)
}

} // namespace

void Foundation_PrefabInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<PrefabInspector>(inspectorTab->GetProject());
}

PrefabInspector::PrefabInspector(Project* project)
    : BaseClassName(project)
{
}

StringHash PrefabInspector::GetResourceType() const
{
    return PrefabResource::GetTypeStatic();
}

SharedPtr<ResourceInspectorWidget> PrefabInspector::MakeInspectorWidget(const ResourceVector& resources)
{
    return MakeShared<ModelInspectorWidget>(context_, resources);
}

SharedPtr<BaseWidget> PrefabInspector::MakePreviewWidget(Resource* resource)
{
    auto sceneWidget = MakeShared<SceneWidget>(context_);
    auto scene = sceneWidget->CreateDefaultScene();
    auto prefabNode = scene->InstantiatePrefab(resource->Cast<PrefabResource>());
    prefabNode->SetName("Prefab");

    // Calculate total bounding box of the prefab
    ea::vector<Drawable*> drawables;
    prefabNode->FindComponents<Drawable>(drawables,
        ComponentSearchFlag::SelfOrChildrenRecursive | ComponentSearchFlag::Derived);
    BoundingBox bbox;
    for (auto* drawable : drawables)
        bbox.Merge(drawable->GetWorldBoundingBox());

    // Scale up small objects
    const float radius = bbox.Size().Length() * 0.5f;
    const float minRadius = 10.0f;
    if (radius > 0.0f && radius < minRadius)
    {
        const float scale = minRadius / radius;
        prefabNode->SetScale(scale);
        bbox.Transform(Matrix3x4::FromScale(scale));
    }

    // Prefab bbox center is at origin
    prefabNode->SetPosition(-bbox.Center());
    bbox.Transform(Matrix3x4::FromTranslation(-bbox.Center()));

    // Add a slight upward/sideways angle for side/front views to see the object better
    Vector3 cameraDirection = CalculateOptimalCameraDirection(bbox, 10.0f);
    if (cameraDirection != Vector3::UP)
    {
        const float elevationAngle = 20.0f;
        const float verticalRotationAngle = -30.0f;
        Vector3 horizontalAxis = cameraDirection.CrossProduct(Vector3::UP);
        cameraDirection = Quaternion(elevationAngle, horizontalAxis) * cameraDirection;
        cameraDirection = Quaternion(verticalRotationAngle, Vector3::UP) * cameraDirection;
    }

    // Position camera to see the prefab.
    auto cameraNode = sceneWidget->GetCamera()->GetNode();
    auto camera = sceneWidget->GetCamera();
    const float distance = CalculateCameraDistance(bbox, camera, +0.1f);
    cameraNode->SetPosition(cameraDirection * distance);
    cameraNode->LookAt(Vector3::ZERO);

    // Apply limits to the MoveAndOrbitComponent
    const float minDistance = CalculateCameraDistance(bbox, camera, -0.9f);
    const float maxDistance = CalculateCameraDistance(bbox, camera, +0.9f);
    auto* moveAndOrbit = cameraNode->GetOrCreateComponent<MoveAndOrbitComponent>();
    moveAndOrbit->SetDistanceLimits(minDistance, maxDistance);

    return sceneWidget;
}

} // namespace Urho3D
