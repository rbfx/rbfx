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
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/RmlUI/RmlUI.h>
#include <RmlUi/Debugger.h>
#include <Urho3D/RmlUI/RmlUIComponent.h>

#include "HelloRmlUI.h"

#include <Urho3D/DebugNew.h>


DemoWindow::DemoWindow(RmlUI* ui)
    : Object(ui->GetContext())
    , ui_(ui)
{
    Rml::DataModelConstructor constructor = ui->GetRmlContext()->CreateDataModel("example_model");
    assert(constructor);
    constructor.Bind("slider_value", &sliderValue_);
    constructor.Bind("counter", &counter_);
    constructor.Bind("progress", &progress_);
    constructor.BindEventCallback("count", &DemoWindow::CountClicks, this);
    model_ = constructor.GetModelHandle();
    document_ = ui->LoadDocument(documentPath_);
    document_->Show();

    SubscribeToEvent(E_UPDATE, &DemoWindow::OnUpdate);
    SubscribeToEvent("CloseWindow", &DemoWindow::OnCloseWindow);
}

DemoWindow::~DemoWindow()
{
    document_->Close();
    ui_->GetRmlContext()->RemoveDataModel("example_model");
}

void DemoWindow::CountClicks(Rml::DataModelHandle modelHandle, Rml::Event& ev, const Rml::VariantList& arguments)
{
    // Increase counter and notify model of it's update.
    counter_++;
    modelHandle.DirtyVariable("counter");
}

void DemoWindow::OnUpdate(StringHash, VariantMap&)
{
    // Animate progressbars
    progress_ = (Sin(GetSubsystem<Time>()->GetElapsedTime() * 50) + 1) / 2;
    model_.DirtyVariable("progress");

    // Update UI model. Called once per frame in E_UPDATE event.
    model_.Update();
}

void DemoWindow::Reload()
{
    // RmlUI instance which owns current document. It is not necessarily same as a result of GetSubsystem<RmlUI>().
    // Save position and size of window document.
    Vector2 pos = document_->GetAbsoluteOffset(Rml::Box::BORDER);
    Vector2 size = document_->GetBox().GetSize(Rml::Box::CONTENT);
    // Close document discarding any state.
    document_->Close();
    // Reset stylesheets and templates so they are reloaded from filesystem/cache.
    Rml::Factory::ClearStyleSheetCache();
    Rml::Factory::ClearTemplateCache();
    // Reopen same document.
    document_ = ui_->LoadDocument(documentPath_);
    // Restore old position and size.
    document_->SetProperty(Rml::PropertyId::Left, Rml::Property(pos.x_, Rml::Property::PX));
    document_->SetProperty(Rml::PropertyId::Top, Rml::Property(pos.y_, Rml::Property::PX));
    document_->SetProperty(Rml::PropertyId::Width, Rml::Property(size.x_, Rml::Property::PX));
    document_->SetProperty(Rml::PropertyId::Height, Rml::Property(size.y_, Rml::Property::PX));
    // Reopen document with old position and size.
    document_->Show();
    // Model does not have to be recreated and old model will be reused. State stored in the model persists across reloads.
}

void DemoWindow::OnCloseWindow(StringHash, VariantMap& args)
{
    Rml::Element* element = static_cast<Rml::Element*>(args["_Element"].GetVoidPtr());
    if (element->GetOwnerDocument() == document_)
    {
        document_->Close();
        document_ = nullptr;
    }
}

HelloRmlUI::HelloRmlUI(Context* context)
    : Sample(context)
{
}

void HelloRmlUI::Start()
{
    // Execute base class startup
    Sample::Start();

    // Initialize Scene
    InitScene();

    // Initialize Window
    InitWindow();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);

    GetSubsystem<FileSystem>()->SetExecuteConsoleCommands(true);
}

void HelloRmlUI::InitWindow()
{
    auto* ui = context_->GetSubsystem<RmlUI>();
    auto* cache = GetSubsystem<ResourceCache>();

    // Node that will get UI rendered on it.
    Node* boxNode = scene_->GetChild("Box");
    // Create a component that sets up UI rendering. It sets material to StaticModel of the node.
    auto* component = boxNode->CreateComponent<RmlUIComponent>();
    // Optionally modify material. Technique is changed so object is visible without any lights.
    component->GetMaterial()->SetTechnique(0, cache->GetResource<Technique>("Techniques/DiffUnlit.xml"));

    // Initialize fonts in backbuffer UI.
    ui->LoadFont("Fonts/NotoSans-Condensed.ttf", false);
    ui->LoadFont("Fonts/NotoSans-CondensedBold.ttf", false);
    ui->LoadFont("Fonts/NotoSans-CondensedBoldItalic.ttf", false);
    ui->LoadFont("Fonts/NotoSans-CondensedItalic.ttf", false);

    // Initialize fonts in 3D UI.
    component->GetUI()->LoadFont("Fonts/NotoSans-Condensed.ttf", false);
    component->GetUI()->LoadFont("Fonts/NotoSans-CondensedBold.ttf", false);
    component->GetUI()->LoadFont("Fonts/NotoSans-CondensedBoldItalic.ttf", false);
    component->GetUI()->LoadFont("Fonts/NotoSans-CondensedItalic.ttf", false);

    // Create a window rendered into backbuffer.
    window_ = new DemoWindow(ui);
    // Create a window rendered onto a cube.
    window3D_ = new DemoWindow(component->GetUI());

    // Subscribe to update event for handling keys and animating cube.
    SubscribeToEvent(E_UPDATE, &HelloRmlUI::OnUpdate);
}

void HelloRmlUI::InitScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    auto* zone = scene_->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetFogColor(Color::GRAY);
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a child scene node (at world origin) and a StaticModel component into it.
    Node* boxNode = scene_->CreateChild("Box");
    boxNode->SetScale(Vector3(5.0f, 5.0f, 5.0f));
    boxNode->SetRotation(Quaternion(90, Vector3::LEFT));

    // Create a box model and hide it initially.
    auto* boxModel = boxNode->CreateComponent<StaticModel>();
    boxModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));

    // Create a camera.
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node.
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    // Set up a viewport so 3D scene can be visible.
    auto* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void HelloRmlUI::OnUpdate(StringHash, VariantMap& eventData)
{
    using namespace Update;
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Animate cube.
    Node* node = scene_->GetChild("Box");
    node->Yaw(6.0f * timeStep * 1.5f);
    node->Roll(-6.0f * timeStep * 1.5f);
    node->Pitch(-6.0f * timeStep * 1.5f);

    Input* input = GetSubsystem<Input>();
    if (input->GetKeyPress(KEY_F5))
    {
        DemoWindow* windows[] = {window_, window3D_};
        for (DemoWindow* window : windows)
            window->Reload();
    }

    if (input->GetKeyPress(KEY_F9))
    {
        auto* ui = context_->GetSubsystem<RmlUI>();
        ui->SetDebuggerVisible(!Rml::Debugger::IsVisible());
    }
}
