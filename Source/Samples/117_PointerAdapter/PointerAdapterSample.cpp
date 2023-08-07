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
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Input/FreeFlyController.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Scene/PrefabReference.h>
#include <Urho3D/Scene/PrefabResource.h>

#include "PointerAdapterSample.h"

#include <Urho3D/DebugNew.h>

PointerAdapterSample::PointerAdapterSample(Context* context) :
    Sample(context)
    , pointerAdapter_(context)
{
}

void PointerAdapterSample::Start()
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

    SubscribeToEvent(&pointerAdapter_, E_MOUSEMOVE, &PointerAdapterSample::HandleMouseMove);
    SubscribeToEvent(&pointerAdapter_, E_MOUSEBUTTONUP, &PointerAdapterSample::HandleMouseButtonUp);
    SubscribeToEvent(&pointerAdapter_, E_MOUSEBUTTONDOWN, &PointerAdapterSample::HandleMouseButtonDown);

    pointerAdapter_.SetEnabled(true);
}

void PointerAdapterSample::Stop()
{
    Sample::Stop();

    pointerAdapter_.SetEnabled(false);
}

void PointerAdapterSample::HandleMouseMove(VariantMap& args)
{
    using namespace MouseMove;
    auto ray = GetViewport(0)->GetScreenRay(args[P_X].GetInt(), args[P_Y].GetInt());

    auto octree = scene_->GetComponent<Octree>();
    RayOctreeQuery query{ray, RAY_TRIANGLE, 100.0f, DRAWABLE_GEOMETRY, 1};
    octree->RaycastSingle(query);

    outlineGroup_->ClearDrawables();

    if (!query.result_.empty())
    {
        outlineGroup_->AddDrawable(query.result_.front().drawable_);
    }
}

void PointerAdapterSample::HandleMouseButtonUp(VariantMap& args)
{
    outlineGroup_->SetColor(Color::WHITE);
}

void PointerAdapterSample::HandleMouseButtonDown(VariantMap& args)
{
    outlineGroup_->SetColor(Color::RED);
}

void PointerAdapterSample::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    octree_ = scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();

    outlineGroup_ = scene_->CreateComponent<OutlineGroup>();
    SetDefaultSkybox(scene_);

    // Create a directional light to the world so that we can see something. The light scene node's orientation controls the
    // light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
    // The light will use default settings (white light, no shadows)
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);

    for (int x=-1; x<=1; ++x)
        for (int y = -1; y <= 1; ++y)
    {
        auto* mushroomPrefab = cache->GetResource<PrefabResource>("Prefabs/Mushroom.prefab");
        Node* objectNode = scene_->CreateChild("Mushroom");
        objectNode->SetPosition(Vector3(x*3, y*3, 10));
        //objectNode->SetRotation(Quaternion(30, 50, 20));
        objectNode->SetScale(2.0f + Random(1.0f));
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

void PointerAdapterSample::CreateInstructions()
{
    UIElement* root = GetUIRoot();
    auto* cache = GetSubsystem<ResourceCache>();
    auto* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    // Construct new Text object, set string to display and font to use
    auto* instructionText = root->CreateChild<Text>();
    instructionText->SetText("Use mouse, touch or gamepad to move cursor");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, GetUIRoot()->GetHeight() / 4);
}

void PointerAdapterSample::SetupViewport()
{
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    SetViewport(0, viewport);
}

