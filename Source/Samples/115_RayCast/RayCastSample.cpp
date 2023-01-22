//
// Copyright (c) 2023-2023 the rbfx project.
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

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Input/FreeFlyController.h>
#include <Urho3D/UI/DropDownList.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Scene/PrefabReference.h>
#include <Urho3D/Scene/PrefabResource.h>

#include "RayCastSample.h"


#include <Urho3D/DebugNew.h>

RayCastSample::RayCastSample(Context* context) :
    Sample(context)
{
}

void RayCastSample::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();
    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);


}

void RayCastSample::Update(float timeStep)
{
    auto input = GetSubsystem<Input>();
    auto pos = input->GetMousePosition();
    auto ray = GetViewport(0)->GetScreenRay(pos.x_, pos.y_);

    switch (typeOfRayCast_->GetSelection())
    {
        case 0: PhysicalRayCast(ray); break;
        case 1: DrawableRayCast(ray, RAY_TRIANGLE); break;
        case 2: DrawableRayCast(ray, RAY_TRIANGLE_UV); break;
        case 3: DrawableRayCast(ray, RAY_OBB); break;
        case 4: DrawableRayCast(ray, RAY_AABB); break;
    }
}

void RayCastSample::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    hitMarkerNode_ = MakeShared<Node>(context_);
    //hitMarkerNode_->SetScale(0.2f);
    hitMarker_ = hitMarkerNode_->CreateComponent<StaticModel>();
    hitMarker_->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    hitMarker_->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
    hitMarker_->SetViewMask(2);

    scene_ = new Scene(context_);

    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();
    SetDefaultSkybox(scene_);

    // Create a directional light to the world so that we can see something. The light scene node's orientation controls the
    // light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
    // The light will use default settings (white light, no shadows)
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);

    {
        auto* mushroomPrefab = cache->GetResource<PrefabResource>("Prefabs/Mushroom.prefab");
        Node* objectNode = scene_->CreateChild("Mushroom");
        objectNode->SetPosition(Vector3(0, 0, 10));
        objectNode->SetRotation(Quaternion(30, 50, 20));
        objectNode->SetScale(2.0f + Random(5.0f));
        auto* prefabReference = objectNode->CreateComponent<PrefabReference>();
        prefabReference->SetPrefab(mushroomPrefab);
    }

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();
    cameraNode_->CreateComponent<FreeFlyController>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
    cameraNode_->LookAt(Vector3(0, 0, 10));
}

void RayCastSample::CreateInstructions()
{
    UIElement* root = GetUIRoot();
    auto* cache = GetSubsystem<ResourceCache>();
    auto* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    // Construct new Text object, set string to display and font to use
    auto* instructionText = root->CreateChild<Text>();
    instructionText->SetText("Use WASD keys and mouse/touch to move");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, GetUIRoot()->GetHeight() / 4);

    typeOfRayCast_ = root->CreateChild<DropDownList>();
    typeOfRayCast_->SetStyleAuto();
    ea::array<const char*, 5> items{
        "Physics", "Drawable Triangle", "Drawable Triangle+UV", "Drawable OBB", "Drawable AABB"};

    int minWidth = 10;
    for (unsigned i = 0; i < items.size(); ++i)
    {
        SharedPtr<Text> item(MakeShared<Text>(context_));
        typeOfRayCast_->AddItem(item);
        item->SetText(items[i]);
        item->SetStyleAuto();
        item->SetMinWidth(item->GetRowWidth(0) + 10);
        minWidth = Max(minWidth, item->GetMinWidth());
    }
    typeOfRayCast_->SetMinSize(IntVector2(minWidth, 16));
    typeOfRayCast_->UpdateLayout();
    typeOfRayCast_->SetAlignment(HA_LEFT, VA_TOP);
    typeOfRayCast_->SetPosition(150, 10);
}

void RayCastSample::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    SetViewport(0, viewport);
}

void RayCastSample::PlaceHitMarker(const Vector3& position, const Vector3& normal)
{
    hitMarkerNode_->SetPosition(position);
    Quaternion rot;
    rot.FromRotationTo(Vector3::UP, normal);
    hitMarkerNode_->SetRotation(rot);
    hitMarkerNode_->SetEnabled(true);

    if (!isVisible_)
    {
        auto octree = scene_->GetComponent<Octree>();
        octree->AddManualDrawable(hitMarker_);
        isVisible_ = true;
    }
}

void RayCastSample::RemoveHitMarker()
{
    if (isVisible_)
    {
        auto octree = scene_->GetComponent<Octree>();
        octree->RemoveManualDrawable(hitMarker_);
        isVisible_ = false;
    }
}

void RayCastSample::PhysicalRayCast(const Ray& ray)
{
    auto physics = scene_->GetComponent<PhysicsWorld>();

    PhysicsRaycastResult res;
    physics->RaycastSingle(res, ray, 100.0f);
    if (res.body_ != nullptr)
    {
        PlaceHitMarker(res.position_, res.normal_);
    }
    else
    {
        RemoveHitMarker();
    }
}

void RayCastSample::DrawableRayCast(const Ray& ray, RayQueryLevel level)
{
    auto octree = scene_->GetComponent<Octree>();
    RayOctreeQuery query{ray, level, 100.0f, DRAWABLE_GEOMETRY, 1};
    octree->RaycastSingle(query);
    if (!query.result_.empty())
    {
        PlaceHitMarker(query.result_[0].position_, query.result_[0].normal_);
    }
    else
    {
        RemoveHitMarker();
    }
}


