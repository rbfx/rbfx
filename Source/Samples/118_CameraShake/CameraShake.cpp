// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Input/FreeFlyController.h>

#include "CameraShake.h"

#include "Urho3D/Scene/ShakeComponent.h"

#include <Urho3D/DebugNew.h>


CameraShake::CameraShake(Context* context) :
    Sample(context)
{
}

void CameraShake::Start()
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
}

void CameraShake::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->LoadFile("Scenes/SimpleScene.scene");
    
    cameraNode_ = scene_->GetChild("MainCamera");
    shakeComponent_ = cameraNode_->FindComponent<ShakeComponent>();
    cameraNode_->CreateComponent<FreeFlyController>();
}

void CameraShake::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = GetUIRoot()->CreateChild<Text>();
    instructionText->SetText("Press SPACE to shake camera");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, GetUIRoot()->GetHeight() / 4);
}

void CameraShake::SetupViewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->FindComponent<Camera>()));
    SetViewport(0, viewport);
}

void CameraShake::Update(float timeStep)
{
    const auto* input = GetSubsystem<Input>();

    if (input->GetKeyPress(KEY_SPACE))
    {
        shakeComponent_->AddTrauma(1.0f);
    }
}
