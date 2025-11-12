//
// Copyright (c) 2025-2025 the rbfx project.
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

#include "../../Foundation/InspectorTab/PrefabInspector.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/SystemUI/ModelInspectorWidget.h>
#include <Urho3D/SystemUI/SceneWidget.h>

namespace Urho3D
{

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
    ea::vector<Drawable*> drawables;
    auto sceneWidget = MakeShared<SceneWidget>(context_);
    auto scene = sceneWidget->CreateDefaultScene();
    auto prefabNode = scene->InstantiatePrefab(resource->Cast<PrefabResource>());
    prefabNode->SetName("Prefab");
    prefabNode->FindComponents<Drawable>(drawables, ComponentSearchFlag::SelfOrChildrenRecursive | ComponentSearchFlag::Derived);
    BoundingBox bbox;
    for (auto* drawable : drawables)
        bbox.Merge(drawable->GetWorldBoundingBox());

    // Center the prefab at origin and calculate size before moving
    prefabNode->SetPosition(-bbox.Center());

    // Find optimal camera position by evaluating visible area from three primary axes
    // Calculate visible area for each primary viewing direction
    Vector3 size = bbox.Size();
    const float x = size.x_;
    const float y = size.y_;
    const float z = size.z_;
    const float areaYZ = y * z; // Looking along X axis
    const float areaXZ = x * z; // Looking along Y axis (top/bottom)
    const float areaXY = x * y; // Looking along Z axis

    // Heuristic:
    // - Prefer top-down if object is very thin vertically vs its widest horizontal dimension
    // - Otherwise, pick the side (RIGHT or FORWARD) that shows the largest visible area
    // - If top area clearly dominates best side by margin, also pick top
    const float horizMax = Max(x, z);
    const float bestSideArea = Max(areaYZ, areaXY);
    const float topDominanceMargin = 10.0f; // Top area must be at least 10x larger than best side to be chosen

    Vector3 cameraDirection;

    if (areaXZ >= bestSideArea * topDominanceMargin)
        cameraDirection = Vector3::UP;      // Very thin or clearly best from top: look from top
    else if (areaYZ >= areaXY)
        cameraDirection = Vector3::RIGHT;   // Look from the right (Y-Z plane is larger)
    else
        cameraDirection = Vector3::FORWARD; // Look from the front (X-Y plane is larger)

    // Add a slight upward angle for side/front views to see the object better
    if (cameraDirection == Vector3::UP)
    {
        // Rotate camera direction upward by elevation angle
        // Find the horizontal component and rotate around the perpendicular axis
        Vector3 horizontalDir = cameraDirection;
        horizontalDir.y_ = 0.0f;
        horizontalDir.Normalize();

        // Create rotation axis perpendicular to horizontal direction (in XZ plane)
        Vector3 rotationAxis = horizontalDir.CrossProduct(Vector3::UP);

        // Rotate the camera direction upward
        const float elevationAngle = 20.0f; // degrees
        Quaternion elevation(elevationAngle, rotationAxis);
        cameraDirection = elevation * cameraDirection;
    }

    // Calculate distance based on camera FOV to fit object in viewport
    auto cameraNode = sceneWidget->GetCamera()->GetNode();
    auto camera = sceneWidget->GetCamera();
    const float fov = camera->GetFov();
    const float aspectRatio = camera->GetAspectRatio();

    // Calculate the radius of bounding sphere (worst case for fitting)
    const float boundingSphereRadius = size.Length() * 0.5f;

    // Calculate distance needed to fit the bounding sphere in view
    // Use the smaller of vertical or horizontal FOV
    const float verticalFov = fov;
    const float horizontalFov = 2.0f * Atan(Tan(verticalFov * 0.5f) * aspectRatio);
    const float effectiveFov = Min(verticalFov, horizontalFov);

    // Distance = radius / tan(fov/2), with 10% margin
    const float distance = (boundingSphereRadius / Tan(effectiveFov * 0.5f)) * 1.1f;

    // Position camera relative to origin (0,0,0) since prefab is now centered there
    cameraNode->SetPosition(cameraDirection * distance);
    cameraNode->LookAt(Vector3::ZERO);  // Look at the centered prefab

    return sceneWidget;
}

} // namespace Urho3D
