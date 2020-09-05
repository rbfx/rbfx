//
// Copyright (c) 2008-2020 the Urho3D project.
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
#include <Urho3D/Engine/Engine.h>
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
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>

#include "AdvancedShaders.h"

#include <Urho3D/DebugNew.h>


AdvancedShaders::AdvancedShaders(Context* context)
    : Sample(context)
    , uiRoot_(GetSubsystem<UI>()->GetRoot())
{
}

void AdvancedShaders::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateSettings();
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();
}

void AdvancedShaders::CreateScene()
{
    auto cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();

    // Create ground plane
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(20.0f, 1.0f, 20.0f));
    auto planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a directional light
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);

    // Create tesselated model
    Node* tessNode = scene_->CreateChild("Tesselated Model");
    tessNode->SetRotation(Quaternion(0.0f, 180.0f, 0.0f));
    auto tessObject = tessNode->CreateComponent<StaticModel>();
    tessObject->SetModel(cache->GetResource<Model>("Models/Kachujin/Kachujin.mdl"));

    auto originalTessMaterial = cache->GetResource<Material>("Materials/Demo/Tess_DistanceLevel_Kachujin.xml");
    tessMaterial_ = originalTessMaterial->Clone();
    tessObject->SetMaterial(tessMaterial_);

    // Create camera
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();
    cameraNode_->SetPosition(Vector3(0.0f, 2.0f, -3.0f));
}

void AdvancedShaders::CreateSettings()
{
    // Create the Window and add it to the UI's root node
    auto window = uiRoot_->CreateChild<Window>("Window");

    // Set Window size and layout settings
    window->SetPosition(128, 128);
    window->SetMinWidth(300);
    window->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window->SetMovable(true);
    window->SetStyleAuto();

    // Create the Window title Text
    auto* windowTitle = window->CreateChild<Text>("WindowTitle");
    windowTitle->SetText("Render Settings");
    windowTitle->SetStyleAuto();

    // Create wireframe controller
    auto* wireframeFrame = window->CreateChild<UIElement>("Wireframe Frame");
    wireframeFrame->SetMinHeight(24);
    wireframeFrame->SetLayout(LM_HORIZONTAL, 6);

    wireframeControl_ = wireframeFrame->CreateChild<CheckBox>("Wireframe Control");
    wireframeControl_->SetStyleAuto();

    auto* wireframeText = wireframeFrame->CreateChild<Text>("Wireframe Label");
    wireframeText->SetText("Wireframe");
    wireframeText->SetMinWidth(CeilToInt(wireframeText->GetRowWidth(0) + 10));
    wireframeText->SetStyleAuto();
}

void AdvancedShaders::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys and mouse/touch to move");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void AdvancedShaders::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void AdvancedShaders::MoveCamera(float timeStep)
{
    auto* input = GetSubsystem<Input>();

    // Rotate camera
    const float MOUSE_SENSITIVITY = 0.1f;
    if (input->GetMouseButtonDown(MOUSEB_RIGHT))
    {
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

    // Move camera
    const float MOVE_SPEED = 5.0f;
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void AdvancedShaders::SubscribeToEvents()
{
    SubscribeToEvent(E_MOUSEBUTTONDOWN, [=](StringHash eventType, VariantMap& eventData)
    {
        if (eventData[MouseButtonDown::P_BUTTON].GetInt() == MOUSEB_RIGHT)
        {
            auto input = context_->GetInput();
            input->SetMouseVisible(false);
            input->SetMouseMode(MM_RELATIVE);
        }
    });

    SubscribeToEvent(E_MOUSEBUTTONUP, [=](StringHash eventType, VariantMap& eventData)
    {
        if (eventData[MouseButtonUp::P_BUTTON].GetInt() == MOUSEB_RIGHT)
        {
            auto input = context_->GetInput();
            input->SetMouseVisible(true);
            input->SetMouseMode(MM_ABSOLUTE);
        }
    });

    SubscribeToEvent(E_UPDATE, [=](StringHash eventType, VariantMap& eventData)
    {
        float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
        MoveCamera(timeStep);
    });

    SubscribeToEvent(wireframeControl_, E_TOGGLED, [=](StringHash eventType, VariantMap& eventData)
    {
        const bool wireframe = eventData[Toggled::P_STATE].GetBool();
        tessMaterial_->SetFillMode(wireframe ? FILL_WIREFRAME : FILL_SOLID);
    });
}
