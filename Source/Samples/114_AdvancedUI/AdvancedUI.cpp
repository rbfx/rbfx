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

#include "AdvancedUI.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/RmlUI/RmlCanvasComponent.h>
#include <Urho3D/RmlUI/RmlUI.h>
#include <Urho3D/UI/Font.h>

#include <RmlUi/Debugger.h>

#include <Urho3D/DebugNew.h>

AdvancedUIWindow::AdvancedUIWindow(Context* context)
    : RmlUIComponent(context)
{
    SetResource("UI/AdvancedUI.rml");

    // Fill sample data
    const int numSavedGames = 25;
    for (int i = 1; i <= numSavedGames; ++i)
        savedGames_.push_back(Format("Saved Game {}<br/>2022-08-14 16:00", i));
    ea::reverse(savedGames_.begin(), savedGames_.end());
}

void AdvancedUIWindow::OnDataModelInitialized()
{
    Rml::DataModelConstructor* constructor = GetDataModelConstructor();
    constructor->RegisterArray<StringVector>();

    constructor->Bind("saved_games", &savedGames_);
    constructor->Bind("game_to_load", &gameToLoad_);
    constructor->Bind("is_game_played", &isGamePlayed_);

    constructor->BindEventCallback("on_continue", WrapCallback(&AdvancedUIWindow::OnContinue));
    constructor->BindEventCallback("on_new_game", WrapCallback(&AdvancedUIWindow::OnNewGame));
    constructor->BindEventCallback("on_load_game", WrapCallback(&AdvancedUIWindow::OnLoadGame));
    constructor->BindEventCallback("on_settings", WrapCallback(&AdvancedUIWindow::OnSettings));
    constructor->BindEventCallback("on_exit", WrapCallback(&AdvancedUIWindow::OnExit));
}

AdvancedUI* AdvancedUIWindow::GetSample() const
{
    auto stateManager = GetSubsystem<StateManager>();
    return dynamic_cast<AdvancedUI*>(stateManager->GetState());
}

void AdvancedUIWindow::Update(float timeStep)
{
    BaseClassName::Update(timeStep);
}

void AdvancedUIWindow::OnContinue()
{
}

void AdvancedUIWindow::OnNewGame()
{
    isGamePlayed_ = true;
    playedGameName_ = Format("New Game {}<br/>2022-08-14 16:00", newGameIndex_);
    ++newGameIndex_;

    if (auto sample = GetSample())
        sample->InitGame(isGamePlayed_, playedGameName_);

    DirtyAllVariables();
}

void AdvancedUIWindow::OnLoadGame()
{
    isGamePlayed_ = true;
    playedGameName_ = gameToLoad_;

    if (auto sample = GetSample())
        sample->InitGame(isGamePlayed_, playedGameName_);

    DirtyAllVariables();
}

void AdvancedUIWindow::OnSettings()
{
}

void AdvancedUIWindow::OnExit()
{
    if (auto sample = GetSample())
        sample->CloseSample();
}

AdvancedUI::AdvancedUI(Context* context)
    : Sample(context)
{
}

void AdvancedUI::Start()
{
    // Register custom components.
    if (!context_->IsReflected<AdvancedUIWindow>())
        context_->AddFactoryReflection<AdvancedUIWindow>();

    // Execute base class startup
    Sample::Start();

    // Initialize Scene
    InitScene();

    // Initialize Window
    InitWindow();

    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);
}

void AdvancedUI::InitWindow()
{
    auto* ui = context_->GetSubsystem<RmlUI>();

    // Create a window rendered into backbuffer.
    window_ = scene_->CreateComponent<AdvancedUIWindow>();

    // Subscribe to update event for handling keys and animating cube.
    SubscribeToEvent(E_UPDATE, &AdvancedUI::OnUpdate);
}

void AdvancedUI::InitScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    auto* zone = scene_->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetFogColor(Color::GRAY);
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create skybox.
    Node* skyboxNode = scene_->CreateChild("Sky");
    auto skybox = skyboxNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create 3D text.
    auto* textNode = scene_->CreateChild("Text");
    textNode->SetPosition({-2.0f, 1.0f, 0.0f});
    text3D_ = textNode->CreateComponent<Text3D>();
    text3D_->SetFont(cache->GetResource<Font>("Fonts/BlueHighway.sdf"), 24);
    text3D_->SetText("This text should never be visible");
    text3D_->SetColor(0x483d8b_rgb);
    text3D_->SetEnabled(false);

    // Create a camera.
    cameraNode_ = scene_->CreateChild("Camera");
    auto camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFov(90.0f);

    // Set an initial position for the camera scene node.
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -2.0f));

    // Set up a viewport so 3D scene can be visible.
    auto* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, camera));
    SetViewport(0, viewport);
}

void AdvancedUI::InitGame(bool gamePlayed, const ea::string& text)
{
    text3D_->SetEnabled(gamePlayed);
    text3D_->SetText(text.replaced("<br/>", "\n"));
}

void AdvancedUI::OnUpdate(StringHash, VariantMap& eventData)
{
    using namespace Update;
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    Input* input = GetSubsystem<Input>();

    if (input->GetKeyPress(KEY_F9))
    {
        auto* ui = context_->GetSubsystem<RmlUI>();
        ui->SetDebuggerVisible(!Rml::Debugger::IsVisible());
    }
}
