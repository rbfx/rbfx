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

#include "CameraOperator.h"

#include "Urho3D/UI/CheckBox.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/CameraOperator.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Input/FreeFlyController.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include <Urho3D/DebugNew.h>

CameraOperatorSample::CameraOperatorSample(Context* context)
    : Sample(context)
{
}

void CameraOperatorSample::Start()
{
    // Execute base class startup
    Sample::Start();

    // Load XML file containing default UI style sheet
    auto* cache = GetSubsystem<ResourceCache>();
    auto* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

    // Set the loaded style as default style
    GetUIRoot()->SetDefaultStyle(style);

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();
    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);

    // Create a CheckBox
    IntVector2 pos = IntVector2(100, 10);
    {
        auto* checkBox = new CheckBox(context_);
        checkBox->SetName("Track Cube A");
        checkBox->SetPosition(pos);
        GetUIRoot()->AddChild(checkBox);
        checkBox->SetStyleAuto();
        SubscribeToEvent(checkBox, E_TOGGLED, &CameraOperatorSample::TrackCubeAToggled);
    }
    {
        pos.y_ += 32;
        auto* checkBox = new CheckBox(context_);
        checkBox->SetName("Track Cube B");
        checkBox->SetPosition(pos);
        GetUIRoot()->AddChild(checkBox);
        checkBox->SetStyleAuto();
        SubscribeToEvent(checkBox, E_TOGGLED, &CameraOperatorSample::TrackCubeBToggled);
    }
    {
        pos.y_ += 32;
        auto* checkBox = new CheckBox(context_);
        checkBox->SetName("Orthographic");
        checkBox->SetPosition(pos);
        GetUIRoot()->AddChild(checkBox);
        checkBox->SetStyleAuto();
        SubscribeToEvent(checkBox, E_TOGGLED, &CameraOperatorSample::OrthographicToggled);
    }
}

void CameraOperatorSample::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->LoadFile("Scenes/CameraOperatorScene.scene");

    cameraNode_ = scene_->GetChild("MainCamera");
    cameraOperator_ = cameraNode_->FindComponent<CameraOperator>();

    cameraNode_->CreateComponent<FreeFlyController>();

    cubeA_ = scene_->FindChild("CubeA", true);
    cubeB_ = scene_->FindChild("CubeB", true);
}

void CameraOperatorSample::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = GetUIRoot()->CreateChild<Text>();
    instructionText->SetText("Right click to rotate camera");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, GetUIRoot()->GetHeight() / 4);
}

void CameraOperatorSample::SetupViewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the
    // camera at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward /
    // deferred) to use, but now we just use full screen and default render path configured in the engine command line
    // options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->FindComponent<Camera>()));
    SetViewport(0, viewport);
}

void CameraOperatorSample::Update(float timeStep)
{
    const auto* input = GetSubsystem<Input>();

    angle_ += timeStep * 45.0f;
    cubeA_->SetPosition(Vector3(0, 4 * Urho3D::Cos(angle_), 0));
    cubeB_->SetPosition(Vector3(0, 0, 4 * Urho3D::Cos(angle_ + 90)));
}

void CameraOperatorSample::TrackCubeAToggled(VariantMap& args)
{
    using namespace Toggled;

    const auto state = args[P_STATE].GetBool();
    if (state)
    {
        cameraOperator_->TrackNode(cubeA_);
    }
    else
    {
        cameraOperator_->RemoveTrackedNode(cubeA_);
    }
}

void CameraOperatorSample::TrackCubeBToggled(VariantMap& args)
{
    using namespace Toggled;

    const auto state = args[P_STATE].GetBool();
    if (state)
    {
        cameraOperator_->TrackNode(cubeB_);
    }
    else
    {
        cameraOperator_->RemoveTrackedNode(cubeB_);
    }
}

void CameraOperatorSample::OrthographicToggled(VariantMap& args)
{
    using namespace Toggled;

    const auto state = args[P_STATE].GetBool();

    cameraOperator_->GetComponent<Camera>()->SetOrthographic(state);
}
