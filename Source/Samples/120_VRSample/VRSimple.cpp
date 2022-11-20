//
// Copyright (c) 2008-2022 the Urho3D project.
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
#include <Urho3D/Graphics/DebugRenderer.h>
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
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Zone.h>

#include <Urho3D/XR/XR.h>
#include <Urho3D/XR/VREvents.h>
#include <Urho3D/XR/VRUtils.h>

#include "VRSimple.h"

#include <Urho3D/DebugNew.h>

static const auto TextBoxName = "XR_INFO";
static const auto MSG_XR_INITIALIZING = "XR is initializing";
static const auto MSG_XR_FAILED = "XR failed to initialize";
static const auto MSG_XR_RUNNING = "XR is running, put on your headset";
static const auto MSG_XR_SLEEPING = "XR is running but not updating";

ButtonCommand turnLeft = { 4 };
ButtonCommand turnRight = { 2 };

VRSimple::VRSimple(Context* context) :
    Sample(context)
{
}

void VRSimple::Start()
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
    SetMouseMode(MM_RELATIVE);
    SetMouseVisible(false);

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(VRSimple, Update));
    SubscribeToEvent(E_VRCONTROLLERCHANGE, URHO3D_HANDLER(VRSimple, HandleControllerChange));

    if (GetSubsystem<OpenXR>() == nullptr)
        GetContext()->RegisterSubsystem<OpenXR>();

    GetSubsystem<OpenXR>()->Initialize("xr_manifest.xml");

    SetupXRScene();
}

void VRSimple::Stop()
{
    if (auto xr = GetSubsystem<OpenXR>())
        xr->Shutdown();

    Sample::Stop();
}

void VRSimple::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create the Octree component to the scene. This is required before adding any drawable components, or else nothing will
    // show up. The default octree volume will be from (-1000, -1000, -1000) to (1000, 1000, 1000) in world coordinates; it
    // is also legal to place objects outside the volume but their visibility can then not be checked in a hierarchically
    // optimizing manner
    scene_->CreateComponent<Octree>();

    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.55f, 0.55f, 0.55f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(300.0f);
    zone->SetFogEnd(500.0f);
    zone->SetBoundingBox(BoundingBox(-2000.0f, 2000.0f));

    // Create a child scene node (at world origin) and a StaticModel component into it. Set the StaticModel to show a simple
    // plane mesh with a "stone" material. Note that naming the scene nodes is optional. Scale the scene node larger
    // (100 x 100 world units)
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    auto* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a directional light to the world so that we can see something. The light scene node's orientation controls the
    // light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
    // The light will use default settings (white light, no shadows)
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);


    // Create more StaticModel objects to the scene, randomly positioned, rotated and scaled. For rotation, we construct a
    // quaternion from Euler angles where the Y angle (rotation about the Y axis) is randomized. The mushroom model contains
    // LOD levels, so the StaticModel component will automatically select the LOD level according to the view distance (you'll
    // see the model get simpler as it moves further away). Finally, rendering a large number of the same object with the
    // same material allows instancing to be used, if the GPU supports it. This reduces the amount of CPU work in rendering the
    // scene.
    const unsigned NUM_OBJECTS = 200;
    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* mushroomNode = scene_->CreateChild("Mushroom");
        mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
        mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        mushroomNode->SetScale(0.5f + Random(2.0f));
        auto* mushroomObject = mushroomNode->CreateComponent<StaticModel>();
        mushroomObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
        mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
        mushroomObject->SetCastShadows(true);
    }

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();
    cameraNode_->CreateComponent<FreeFlyController>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void VRSimple::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = GetUIRoot()->CreateChild<Text>(TextBoxName);
    instructionText->SetText(MSG_XR_INITIALIZING);
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, GetUIRoot()->GetHeight() / 4);
}

void VRSimple::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    SetViewport(0, viewport);
}

void VRSimple::SetupXRScene()
{
    auto xr = GetSubsystem<OpenXR>();

    auto rig = scene_->GetChild("VRRig");
    if (rig == nullptr)
    {
        rig = scene_->CreateChild("VRRig");
        xr->PrepareRig(rig);
        rig->SetWorldPosition(Vector3(0, 0, 0));
    }

    xr->UpdateRig(rig, 0.01f, 150.0f, true);
    xr->UpdateHands(scene_, rig, rig->GetChild("Left_Hand"), rig->GetChild("Right_Hand"));
}

void VRSimple::Update(StringHash eventID, VariantMap& eventData)
{
    auto* ui = GetSubsystem<UI>();

    auto input = GetSubsystem<Input>();
    auto fs = GetSubsystem<FileSystem>();
    if (input->GetKeyDown(KEY_S))
    {
        XMLFile file(GetContext());
        auto rootElem = file.GetOrCreateRoot("scene");
        scene_->SaveXML(rootElem);
        
        file.SaveFile(AddTrailingSlash(fs->GetProgramDir()) + "vrsimple_scene.xml");
    }

    if (auto xr = GetSubsystem<OpenXR>())
    {
        if (auto elem = GetUIRoot()->GetChild(TextBoxName, false))
        {
            auto txt = elem->Cast<Text>();
            if (xr->IsLive())
                txt->SetText(MSG_XR_RUNNING);
            else
                txt->SetText(MSG_XR_SLEEPING);
        }

        if (xr->IsLive())
        {
            if (auto rig = scene_->GetChild("VRRig"))
            {
                auto head = rig->GetChild("Head");
                xr->UpdateRig(rig, 0.001f, 150.0f, true);
                xr->UpdateHands(scene_, rig, rig->GetChild("Left_Hand"), rig->GetChild("Right_Hand"));

                auto dbg = scene_->GetOrCreateComponent<DebugRenderer>();
                
                // this should show where the tracking volume centroid is
                dbg->AddNode(rig, 1.0f, false);

                if (auto child = rig->GetChild("Left_Hand", true))
                {
                    xr->UpdateControllerModel(VR_HAND_LEFT, SharedPtr<Node>(child->GetChild(0u)));
                    // draw hand axis so we can see it even if we have no model
                    dbg->AddNode(child, 0.15f, false);
                }
                if (auto child = rig->GetChild("Right_Hand", true))
                {
                    xr->UpdateControllerModel(VR_HAND_RIGHT, SharedPtr<Node>(child->GetChild(0u)));
                    // draw hand axis so we can see it even if we have no model
                    dbg->AddNode(child, 0.15f, false);

                    // draw a white line going off 2 meters along the aim axis.
                    auto aimMatrix = xr->GetHandAimTransform(VR_HAND_RIGHT);
                    dbg->AddLine(aimMatrix * Vector3(0, 0, 0), aimMatrix * Vector3(0, 0, 2), Color::WHITE, false);
                }

                // use left stick to move based on where the user is looking
                if (auto leftStick = xr->GetInputBinding("stick", VR_HAND_LEFT))
                {
                    auto delta = SmoothLocomotionHead(rig, leftStick, 0.3f);
                    rig->Translate(delta * 0.025f, TS_WORLD);
                }

                // use right stick for left/right snap turning
                if (auto rightStick = xr->GetInputBinding("stick", VR_HAND_RIGHT))
                {
                    auto cmd = JoystickAsDPad(rightStick, 0.3f);

                    auto curHeadPos = head->GetWorldPosition();
                    auto rigPos = rig->GetWorldPosition();
                    if (turnLeft.CheckStrict(cmd))
                        rig->RotateAround(Vector3(curHeadPos.x_, rigPos.y_, curHeadPos.z_), Quaternion(-45, Vector3::UP), TS_WORLD);
                    if (turnRight.CheckStrict(cmd))
                        rig->RotateAround(Vector3(curHeadPos.x_, rigPos.y_, curHeadPos.z_), Quaternion(45, Vector3::UP), TS_WORLD);
                }

                if (auto rightTrigger = xr->GetInputBinding("trigger", VR_HAND_RIGHT))
                {
                    if (rightTrigger->IsChanged()) // would work that anyways because of power * rightHandTrigger
                        xr->SetVignette(rightTrigger->GetFloat() > 0.5f, Color(25.0f, 0.0f, 0.0f, 0.0f), Color(0.25f, 0.0f, 0.0f, 1.0f), rightTrigger->GetFloat());
                }
            }            
        }
    }
}

void VRSimple::HandleControllerChange(StringHash eventType, VariantMap& eventData)
{
    // User could turn on/off their controller while we're responding to.
    int hand = eventData[VRControllerChange::P_HAND].GetInt();
    auto rig = scene_->GetChild("VRRig");
    auto child = rig->GetChild(hand == 0 ? "Left_Hand" : "Right_Hand", true);

    if (child)
    {
        child->RemoveAllChildren();
        if (auto handModel = GetSubsystem<OpenXR>()->GetControllerModel(hand == 0 ? VR_HAND_LEFT : VR_HAND_RIGHT))
            child->AddChild(handModel);
    }
}



