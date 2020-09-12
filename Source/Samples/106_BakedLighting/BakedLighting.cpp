//
// Copyright (c) 2017-2020 the rbfx project.
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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Navigation/CrowdAgent.h>
#include <Urho3D/Navigation/CrowdManager.h>
#include <Urho3D/Navigation/NavigationMesh.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "BakedLighting.h"

#include <Urho3D/DebugNew.h>


BakedLighting::BakedLighting(Context* context) : Sample(context) {}

BakedLighting::~BakedLighting() = default;

void BakedLighting::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Subscribe to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void BakedLighting::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Load scene
    scene_ = new Scene(context_);

    SharedPtr<File> file = cache->GetFile("Scenes/BakedLightingExample.xml");
    scene_->LoadXML(*file);

    auto camera = scene_->GetComponent<Camera>(true);
    cameraNode_ = camera->GetNode();
    GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

    auto navMesh = scene_->GetComponent<NavigationMesh>(true);
    navMesh->Build();

    agent_ = scene_->GetComponent<CrowdAgent>(true);
    agent_->SetUpdateNodePosition(false);

    auto animController = agent_->GetNode()->GetComponent<AnimationController>(true);
    animController->PlayExclusive("Models/Mutant/Mutant_Idle0.ani", 0, true);

    auto crowdManager = scene_->GetComponent<CrowdManager>();
    CrowdObstacleAvoidanceParams params = crowdManager->GetObstacleAvoidanceParams(0);
    params.weightToi = 0.0001f;
    crowdManager->SetObstacleAvoidanceParams(0, params);

    // Initialize yaw and pitch angles
    Node* cameraRotationPitch = cameraNode_->GetParent();
    Node* cameraRotationYaw = cameraRotationPitch->GetParent();
    yaw_ = cameraRotationYaw->GetWorldRotation().YawAngle();
    pitch_ = cameraRotationPitch->GetWorldRotation().PitchAngle();
}

void BakedLighting::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText(
        "Use WASD keys and mouse/touch to move\n"
        "Shift to sprint, Tab to toggle character textures"
    );
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void BakedLighting::SubscribeToEvents()
{
    // Subscribe to Update event for setting the character controls before physics simulation
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(BakedLighting, HandleUpdate));

    // Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void BakedLighting::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    auto* input = GetSubsystem<Input>();

    // Update camera rotation
    const float rotationSensitivity = 0.1f;
    const IntVector2 mouseMove = input->GetMouseMove();
    yaw_ = Mod(yaw_ + mouseMove.x_ * rotationSensitivity, 360.0f);
    pitch_ = Clamp(pitch_ + mouseMove.y_ * rotationSensitivity, -89.0f, 89.0f);

    Node* cameraRotationPitch = cameraNode_->GetParent();
    Node* cameraRotationYaw = cameraRotationPitch->GetParent();
    cameraRotationPitch->SetRotation(Quaternion(pitch_, 0.0f, 0.0f));
    cameraRotationYaw->SetRotation(Quaternion(0.0f, yaw_, 0.0f));

    // Apply movement
    const Quaternion& rotation = cameraRotationYaw->GetWorldRotation();
    Vector3 controlDirection = Vector3::ZERO;
    if (input->GetKeyDown(KEY_W))
        controlDirection += Vector3::FORWARD;
    if (input->GetKeyDown(KEY_S))
        controlDirection += Vector3::BACK;
    if (input->GetKeyDown(KEY_A))
        controlDirection += Vector3::LEFT;
    if (input->GetKeyDown(KEY_D))
        controlDirection += Vector3::RIGHT;

    const Vector3 movementDirection = rotation * controlDirection;
    const float speed = input->GetKeyDown(KEY_SHIFT) ? 5.0f : 2.0f;
    agent_->SetTargetVelocity(speed * movementDirection);

    // Animate model
    auto animController = agent_->GetNode()->GetComponent<AnimationController>(true);
    auto rotationNode = animController->GetNode()->GetParent();
    const Vector3 actualVelocityFlat = (agent_->GetActualVelocity() * Vector3(1, 0, 1));
    if (actualVelocityFlat.Length() > M_LARGE_EPSILON)
    {
        rotationNode->SetWorldDirection(actualVelocityFlat);
        animController->PlayExclusive("Models/Mutant/Mutant_Run.ani", 0, true, 0.2f);
        animController->SetSpeed("Models/Mutant/Mutant_Run.ani", actualVelocityFlat.Length() * 0.3f);
    }
    else
    {
        animController->PlayExclusive("Models/Mutant/Mutant_Idle0.ani", 0, true, 0.2f);
    }

    // Snap position to ground
    agent_->GetNode()->SetWorldPosition(agent_->GetPosition() * Vector3(1, 0, 1));

    // Toggle textures
    if (input->GetKeyPress(KEY_TAB))
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        auto animModel = animController->GetNode()->GetComponent<AnimatedModel>();

        texturesEnabled_ = !texturesEnabled_;
        if (texturesEnabled_)
            animModel->SetMaterial(cache->GetResource<Material>("Models/Mutant/Materials/mutant_M.xml"));
        else
            animModel->SetMaterial(cache->GetResource<Material>("Materials/DefaultWhite.xml"));
    }

    // Draw debug geometry
    //auto navmesh = scene_->GetComponent<NavigationMesh>();
    //navmesh->DrawDebugGeometry(true);
}
