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

#include "HelloVR.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Input/FreeFlyController.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/XR/VREvents.h>
#include <Urho3D/XR/VRRig.h>
#include <Urho3D/XR/VRUtils.h>
#include <Urho3D/XR/VirtualReality.h>

#include <Urho3D/DebugNew.h>

ButtonCommand turnLeft = {4};
ButtonCommand turnRight = {2};

HelloVR::HelloVR(Context* context)
    : Sample(context)
{
}

void HelloVR::Start()
{
    auto virtualReality = GetSubsystem<VirtualReality>();
    if (!virtualReality)
    {
        CloseSample();
        return;
    }

    // Execute base class startup
    Sample::Start();

    // Initialize VR session
    virtualReality->InitializeSession(VRSessionParameters{"XR/DefaultManifest.xml"});

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Set the mouse mode to use in the sample
    SetMouseMode(MM_RELATIVE);
    SetMouseVisible(false);

    // Subscribe to necessary events
    SubscribeToEvent(E_UPDATE, &HelloVR::Update);
}

void HelloVR::Stop()
{
    if (auto virtualReality = GetSubsystem<VirtualReality>())
        virtualReality->ShutdownSession();

    Sample::Stop();
}

void HelloVR::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Load prepared scene
    const auto xmlFile = cache->GetResource<XMLFile>("Scenes/HelloVR.scene");
    scene_->LoadXML(xmlFile->GetRoot());

    // Get the dynamic objects
    dynamicObjects_ = scene_->GetChild("Dynamic Objects");

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
    cameraNode_->CreateComponent<Camera>();
    cameraNode_->CreateComponent<FreeFlyController>();

    // Load the rig from the scene
    Node* rigNode = scene_->GetChild("VRRig");
    VRRig* rig = rigNode->GetComponent<VRRig>();
    rig->Activate();

    // Create kinematic bodies for hands
    SetupHandComponents(rig->GetLeftHandPose(), rig->GetLeftHandAim());
    SetupHandComponents(rig->GetRightHandPose(), rig->GetRightHandAim());
}

void HelloVR::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    statusText_ = GetUIRoot()->CreateChild<Text>("VR Status");
    statusText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    statusText_->SetHorizontalAlignment(HA_CENTER);
    statusText_->SetVerticalAlignment(VA_CENTER);
    statusText_->SetPosition(0, GetUIRoot()->GetHeight() / 4);
}

void HelloVR::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the
    // camera at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward /
    // deferred) to use, but now we just use full screen and default render path configured in the engine command line
    // options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    SetViewport(0, viewport);
}

void HelloVR::SetupHandComponents(Node* handPoseNode, Node* handAimNode)
{
    const float handSize = 0.08f;
    const float aimSize = 0.02f;
    const float aimLength = 0.08f;

    auto cache = GetSubsystem<ResourceCache>();

    // Create visible shape for hands
    Node* displayNode = handPoseNode->CreateChild("Display");
    displayNode->SetScale(handSize);

    auto handModel = displayNode->CreateComponent<StaticModel>();
    handModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    handModel->SetMaterial(cache->GetResource<Material>("Materials/Constant/MattTransparent.xml"));

    // Create kinematic body for hand
    const auto shape = handPoseNode->CreateComponent<CollisionShape>();
    shape->SetBox(handSize * Vector3::ONE);

    const auto body = handPoseNode->CreateComponent<RigidBody>();
    body->SetKinematic(true);

    // Create aim node
    Node* aimNode = handAimNode->CreateChild("Aim");
    aimNode->SetScale({aimSize, aimSize, aimLength});
    aimNode->SetPosition({0.0f, 0.0f, aimLength / 2});

    auto aimModel = aimNode->CreateComponent<StaticModel>();
    aimModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    aimModel->SetMaterial(cache->GetResource<Material>("Materials/Constant/MattTransparent.xml"));
}

void HelloVR::GrabDynamicObject(Node* handNode, VRHand hand)
{
    // Find closest dynamic object
    Node* closestObject = nullptr;
    float closestDistance = M_INFINITY;
    for (Node* object : dynamicObjects_->GetChildren())
    {
        const float distance = (object->GetWorldPosition() - handNode->GetWorldPosition()).Length();
        if (!closestObject || distance < closestDistance)
        {
            closestObject = object;
            closestDistance = distance;
        }
    }

    // Do nothing if too far
    const float grabDistance = 0.25f;
    if (!closestObject || closestDistance > grabDistance)
        return;

    // Activate constraint
    auto constraint = closestObject->GetComponent<Constraint>();
    constraint->SetOtherBody(handNode->GetComponent<RigidBody>());
    constraint->SetOtherPosition(handNode->WorldToLocal(closestObject->GetWorldPosition()));
    constraint->SetOtherRotation(handNode->GetWorldRotation().Inverse() * closestObject->GetWorldRotation());
    constraint->SetEnabled(true);

    // Trigger feedback
    auto virtualReality = GetSubsystem<VirtualReality>();
    virtualReality->TriggerHaptic(hand, 0.1f, 0.0f, 0.5f);
}

void HelloVR::ReleaseDynamicObject(Node* handNode)
{
    // Deactivate constraint
    for (Node* object : dynamicObjects_->GetChildren())
    {
        auto constraint = object->GetComponent<Constraint>();
        if (constraint->GetOtherBody() == handNode->GetComponent<RigidBody>())
        {
            constraint->SetEnabled(false);
            constraint->SetOtherBody(nullptr);
        }
    }
}

void HelloVR::Update()
{
    statusText_->SetText(GetStatus());

    auto virtualReality = GetSubsystem<VirtualReality>();
    if (!virtualReality)
        return;

    // Get and check the rig
    const VRRigDesc& rigDesc = virtualReality->GetRig();
    auto rigNode = scene_->GetChild("VRRig");
    if (!virtualReality->IsVisible() || !rigNode || !rigDesc.IsValid())
        return;

    // Use left and right grab buttons to grab and release objects
    XRBinding* rightGrab = virtualReality->GetInputBinding("grab", VRHand::Right);
    if (rightGrab && rigDesc.rightHandPose_)
    {
        const bool isPressed = rightGrab->GetFloat() == 1.0f;
        if (rightGrab->IsChanged())
        {
            if (isPressed)
                GrabDynamicObject(rigDesc.rightHandPose_, VRHand::Right);
            else
                ReleaseDynamicObject(rigDesc.rightHandPose_);
        }
    }

    XRBinding* leftGrab = virtualReality->GetInputBinding("grab", VRHand::Left);
    if (leftGrab && rigDesc.leftHandPose_)
    {
        const bool isPressed = leftGrab->GetFloat() == 1.0f;
        if (leftGrab->IsChanged())
        {
            if (isPressed)
                GrabDynamicObject(rigDesc.leftHandPose_, VRHand::Left);
            else
                ReleaseDynamicObject(rigDesc.leftHandPose_);
        }
    }

    // Use left stick to move based on where the user is looking
    if (XRBinding* leftStick = virtualReality->GetInputBinding("stick", VRHand::Left))
    {
        const Vector3 delta = SmoothLocomotionHead(rigNode, leftStick, 0.3f);
        rigNode->Translate(delta * 0.025f, TS_WORLD);
    }

    // Use right stick for left/right snap turning
    if (XRBinding* rightStick = virtualReality->GetInputBinding("stick", VRHand::Right))
    {
        const auto cmd = JoystickAsDPad(rightStick, 0.3f);

        const Vector3 curHeadPos = rigDesc.head_->GetWorldPosition();
        const Vector3 rigPos = rigNode->GetWorldPosition();
        if (turnLeft.CheckStrict(cmd))
        {
            rigNode->RotateAround(
                Vector3(curHeadPos.x_, rigPos.y_, curHeadPos.z_), Quaternion(-45, Vector3::UP), TS_WORLD);
        }
        if (turnRight.CheckStrict(cmd))
        {
            rigNode->RotateAround(
                Vector3(curHeadPos.x_, rigPos.y_, curHeadPos.z_), Quaternion(45, Vector3::UP), TS_WORLD);
        }
    }

    // Draw debug geometry
    auto debug = scene_->GetOrCreateComponent<DebugRenderer>();

    debug->AddNode(rigNode, 1.0f, false);

    for (Node* handPose : {rigDesc.leftHandPose_, rigDesc.rightHandPose_})
        debug->AddNode(handPose, 0.15f, false);

    for (Node* handAim : {rigDesc.leftHandAim_, rigDesc.rightHandAim_})
    {
        const Vector3 position = handAim->GetWorldPosition();
        const Vector3 direction = handAim->GetWorldDirection();
        debug->AddLine(position, position + direction * 2, Color::WHITE, false);
    }
}

ea::string HelloVR::GetStatus() const
{
    auto virtualReality = GetSubsystem<VirtualReality>();
    if (!virtualReality || !virtualReality->IsConnected())
        return "VR is not initialized";
    else if (virtualReality->IsFocused())
        return "VR is visible and receives input";
    else if (virtualReality->IsVisible())
        return "VR is visible but the input is disabled";
    else if (virtualReality->IsRunning())
        return "VR is running but is not rendered";
    else
        return "VR is idle";
}
