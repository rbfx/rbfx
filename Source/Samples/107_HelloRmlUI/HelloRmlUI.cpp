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
#include <Urho3D/RenderPipeline/ShaderConsts.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/RmlUI/RmlUI.h>
#include <Urho3D/RmlUI/RmlCanvasComponent.h>
#include <RmlUi/Debugger.h>

#include "HelloRmlUI.h"

#include <Urho3D/DebugNew.h>

SimpleWindow::SimpleWindow(Context* context)
    : RmlUIComponent(context)
{
    SetResource("UI/HelloRmlUI.rml");
    variant_ = ResourceRef("Model", "Models/Box.mdl");
    variantVector_.push_back(42);
    variantVector_.push_back("vector text");
    variantMap_["int"] = 24;
    variantMap_["str"] = "map text";
}

void SimpleWindow::OnDataModelInitialized()
{
    Rml::DataModelConstructor* constructor = GetDataModelConstructor();
    // Create a data model for connecting UI with state kept in this class.
    // Important: there can only be one data model with given name per unique RmlUI subsystem!
    constructor->BindFunc("font_size", [this](Rml::Variant& v) { v = this->GetEmSize(); },
        [this](const Rml::Variant& v) { this->SetEmSize(v.Get<float>()); });
    constructor->Bind("slider_value", &sliderValue_);
    constructor->Bind("counter", &counter_);
    constructor->Bind("progress", &progress_);
    BindDataModelVariant("variant", &variant_);
    BindDataModelVariantVector("variant_vector", &variantVector_);
    BindDataModelVariantMap("variant_map", &variantMap_);

    constructor->BindEventCallback("count", &SimpleWindow::CountClicks, this);

    // Act on pressing window close button.
    RmlUI* ui = GetUI();
    SubscribeToEvent(ui, "CloseWindow", &SimpleWindow::OnCloseWindow);
}

void SimpleWindow::CountClicks(Rml::DataModelHandle modelHandle, Rml::Event& ev, const Rml::VariantList& arguments)
{
    // Increase counter and notify model of it's update.
    counter_++;
    modelHandle.DirtyVariable("counter");
}

void SimpleWindow::Update(float timeStep)
{
    BaseClassName::Update(timeStep);

    // Animate progressbars
    progress_ = (Sin(GetSubsystem<Time>()->GetElapsedTime() * 50) + 1) / 2;
    DirtyVariable("progress");
}

void SimpleWindow::OnCloseWindow(StringHash, VariantMap& args)
{
    Rml::Element* element = static_cast<Rml::Element*>(args["_Element"].GetVoidPtr());
    if (element->GetOwnerDocument() == GetDocument())
    {
        Remove();
    }
}


HelloRmlUI::HelloRmlUI(Context* context)
    : Sample(context)
{
}

void HelloRmlUI::Start()
{
    // Register custom components.
    if (!context_->IsReflected<SimpleWindow>())
        context_->AddFactoryReflection<SimpleWindow>();

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

void HelloRmlUI::InitWindow()
{
    auto* ui = context_->GetSubsystem<RmlUI>();

    // Create a window rendered into backbuffer.
    window_ = scene_->CreateComponent<SimpleWindow>();

    // Node that will get UI rendered on it.
    Node* boxNode = scene_->GetChild("Box");

    // Create a texture we will render to.
    texture_ = MakeShared<Texture2D>(context_);

    // Create a material that will display UI texture on a cube.
    material_ = MakeShared<Material>(context_);
    material_->SetTechnique(0, GetSubsystem<ResourceCache>()->GetResource<Technique>("Techniques/DiffUnlit.xml"));
    material_->SetTexture(ShaderResources::Albedo, texture_);

    // Create a component that sets up UI rendering.
    auto* renderer = boxNode->CreateComponent<RmlCanvasComponent>();
    renderer->SetTexture(texture_);
    renderer->SetUISize({512, 512});
    renderer->SetRemapMousePos(true);

    // Create a window that will be rendered on a cube.
    windowOnCube_ = boxNode->CreateComponent<SimpleWindow>();

    // Create a StaticModel for our cube.
    auto* model = boxNode->GetComponent<StaticModel>();

    // And set a material to it so UI would be rendered on to cube.
    model->SetMaterial(material_);

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
    SetViewport(0, viewport);
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

    if (input->GetKeyPress(KEY_F9))
    {
        auto* ui = context_->GetSubsystem<RmlUI>();
        ui->SetDebuggerVisible(!Rml::Debugger::IsVisible());
    }
}
