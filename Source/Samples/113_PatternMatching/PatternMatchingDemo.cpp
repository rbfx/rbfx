//
// Copyright (c) 2022-2022 the rbfx project.
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

#include <Urho3D/Input/Input.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/PatternMatching/CharacterConfigurator.h>

#include "PatternMatchingDemo.h"

#include <Urho3D/DebugNew.h>

PatternMatchingDemo::PatternMatchingDemo(Context* context) :
    Sample(context)
{
    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);
}

void PatternMatchingDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create static scene content
    CreateScene();

    // Create "Hello World" Text
    CreateText();
}

void PatternMatchingDemo::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create scene subsystem components
    scene_->CreateComponent<Octree>();

    // Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside
    // the scene, so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
    cameraNode_ = new Node(context_);
    auto* camera = cameraNode_->CreateComponent<Camera>();
    cameraNode_->SetPosition(Vector3(0, 1, 5));
    cameraNode_->LookAt(Vector3(0, 1, 0));
    camera->SetFarClip(500.0f);
    SetViewport(0, new Viewport(context_, scene_, camera));

    // Create static scene content. First create a zone for ambient lighting and fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(300.0f);
    zone->SetFogEnd(500.0f);
    zone->SetBoundingBox(BoundingBox(-2000.0f, 2000.0f));

    // Create a directional light with cascaded shadow mapping
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.3f, -0.5f, -0.425f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    light->SetSpecularIntensity(0.5f);

    Node* characterNode = scene_->CreateChild("Character");
    configurator_ = characterNode->CreateComponent<CharacterConfigurator>();
    auto conf = cache->GetResource<CharacterConfiguration>("Models/Mutant/Character.xml");
    configurator_->SetConfiguration(conf);
    pattern_.SetKey("OnGround", onGround_ ? 1 : 0);
    configurator_->Update(pattern_);
}

void PatternMatchingDemo::CreateText()
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Construct new Text object
    SharedPtr<Text> helloText(new Text(context_));

    // Set String to display
    helloText->SetText("Pattern matching demo");

    // Set font and text color
    helloText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    helloText->SetColor(Color(0.0f, 1.0f, 0.0f));

    // Align Text center-screen
    helloText->SetHorizontalAlignment(HA_CENTER);
    helloText->SetVerticalAlignment(VA_TOP);

    // Add Text instance to the UI root element
    GetUIRoot()->AddChild(helloText);
}

void PatternMatchingDemo::Update(float timeStep)
{
    if (ui::Begin("Pattern", 0, ImGuiWindowFlags_NoSavedSettings))
    {
        if (ui::Checkbox("Shield", &shield_))
        {
            if (shield_)
                pattern_.SetKey("Shield");
            else
                pattern_.RemoveKey("Shield");
        }
        if (ui::Checkbox("Sword", &sword_))
        {
            if (sword_)
                pattern_.SetKey("Sword");
            else
                pattern_.RemoveKey("Sword");
        }
        if (ui::Checkbox("Run", &run_))
        {
            if (run_)
                pattern_.SetKey("Run");
            else
                pattern_.RemoveKey("Run");
        }
        if (ui::Checkbox("Left", &left_))
        {
            if (left_)
                pattern_.SetKey("Left");
            else
                pattern_.RemoveKey("Left");
        }
        if (ui::Checkbox("Right", &right_))
        {
            if (right_)
                pattern_.SetKey("Right");
            else
                pattern_.RemoveKey("Right");
        }
        if (ui::Checkbox("OnGround", &onGround_))
        {
            pattern_.SetKey("OnGround", onGround_ ? 1 : 0);
        }
    }
    ui::End();
    if (pattern_.Commit())
    {
        configurator_->Update(pattern_);
    }
}
