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

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/Input/FreeFlyController.h>

#include "RenderingShowcase.h"

#include <Urho3D/DebugNew.h>


RenderingShowcase::RenderingShowcase(Context* context)
    : Sample(context)
{
    // All these scenes correspond to Scenes/RenderingShowcase_*.xml resources
    sceneNames_.push_back({ "0" });
    sceneNames_.push_back({ "2_Dynamic", "2_BakedDirect", "2_BakedIndirect", "2_BakedDirectIndirect" });
    sceneNames_.push_back({ "3_MixedBoxProbes", "3_MixedProbes" });
    // Web doesn't like exceptions
    if (GetPlatform() != PlatformId::Web)
        sceneNames_.push_back({ "6_InvalidShader" });
    // Keep 1 last because it may crash mobile browsers
    sceneNames_.push_back({ "1" });
}

void RenderingShowcase::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create scene content
    CreateScene();
    SetupSelectedScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();
    // Set the mouse mode to use in the sample
    SetMouseMode(MM_RELATIVE);
    SetMouseVisible(false);
}

void RenderingShowcase::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();
    auto* input = GetSubsystem<Input>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = GetUIRoot()->CreateChild<Text>();
    auto rName = input->GetKeyName(input->GetKeyFromScancode(SCANCODE_R));
    auto fName = input->GetKeyName(input->GetKeyFromScancode(SCANCODE_F));
    instructionText->SetText(Format("Press Tab to switch scene. Press {} to switch scene mode. \n"
        "Press {} to toggle probe object. Use WASD keys and mouse to move.", rName, fName));
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, GetUIRoot()->GetHeight() / 4);
}

void RenderingShowcase::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = MakeShared<Scene>(context_);
    cameraScene_ = MakeShared<Scene>(context_);

    // Create the camera (not included in the scene file)
    cameraNode_ = cameraScene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();
    cameraNode_->CreateComponent<FreeFlyController>();

    Node* probeObjectNode = cameraNode_->CreateChild();
    probeObjectNode->SetPosition({ 0.0f, 0.0f, 1.0f });
    probeObjectNode->SetScale(0.5f);

    probeObject_ = probeObjectNode->CreateComponent<StaticModel>();
    probeObject_->SetModel(cache->GetResource<Model>("Models/TeaPot.mdl"));
    probeObject_->SetCastShadows(true);
    probeObject_->SetViewMask(0x1);
    probeObject_->SetGlobalIlluminationType(GlobalIlluminationType::BlendLightProbes);
    probeObject_->SetReflectionMode(ReflectionMode::BlendProbesAndZone);
}

void RenderingShowcase::SetupSelectedScene(bool resetCamera)
{
    auto* cache = GetSubsystem<ResourceCache>();

    const bool isProbeObjectVisible = probeObject_->IsInOctree();

    if (isProbeObjectVisible)
        scene_->GetComponent<Octree>()->RemoveManualDrawable(probeObject_);

    // Load scene content prepared in the editor (XML format). GetFile() returns an open file from the resource system
    // which scene.LoadXML() will read
    const ea::string fileName = Format("Scenes/RenderingShowcase_{}.xml", sceneNames_[sceneIndex_][sceneMode_]);
    const auto xmlFile = cache->GetResource<XMLFile>(fileName);
    scene_->LoadXML(xmlFile->GetRoot());

    if (resetCamera)
    {
        cameraNode_->SetPosition({ 0.0f, 4.0f, 8.0f });
        cameraNode_->LookAt(Vector3::ZERO);

        yaw_ = cameraNode_->GetRotation().YawAngle();
        pitch_ = cameraNode_->GetRotation().PitchAngle();
    }

    if (isProbeObjectVisible)
        scene_->GetComponent<Octree>()->AddManualDrawable(probeObject_);
}

void RenderingShowcase::SetupViewport()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    SetViewport(0, viewport);
}

void RenderingShowcase::Update(float timeStep)
{
    auto* input = GetSubsystem<Input>();
    auto* cache = GetSubsystem<ResourceCache>();

    // Keep probe object orientation
    probeObject_->GetNode()->SetWorldRotation(Quaternion::IDENTITY);

    // Update scene
    if (sceneNames_.size() > 1 && input->GetKeyPress(KEY_TAB))
    {
        sceneIndex_ = (sceneIndex_ + 1) % sceneNames_.size();
        sceneMode_ = 0;
        SetupSelectedScene();
    }

    // Update scene mode
    if (sceneNames_[sceneIndex_].size() > 1 && input->GetScancodePress(SCANCODE_R))
    {
        sceneMode_ = (sceneMode_ + 1) % sceneNames_[sceneIndex_].size();
        SetupSelectedScene(false);
    }

    // Update probe object
    static const ea::string probeMaterials[] = {
        "",
        "Materials/Constant/GlossyWhiteDielectric.xml",
        "Materials/Constant/GlossyWhiteMetal.xml",
        "Materials/CheckboardProperties.xml",
    };
    if (input->GetScancodePress(SCANCODE_F))
    {
        const unsigned numProbeMaterials = sizeof(probeMaterials) / sizeof(probeMaterials[0]);
        probeMaterialIndex_ = (probeMaterialIndex_ + 1) % numProbeMaterials;
        const ea::string& probeMaterialName = probeMaterials[probeMaterialIndex_];

        const bool isProbeObjectVisible = probeObject_->IsInOctree();
        const bool shouldProbeObjectBeVisible = !probeMaterialName.empty();

        auto* octree = scene_->GetComponent<Octree>();
        if (isProbeObjectVisible && !shouldProbeObjectBeVisible)
            octree->RemoveManualDrawable(probeObject_);
        else if (!isProbeObjectVisible && shouldProbeObjectBeVisible)
            octree->AddManualDrawable(probeObject_);

        if (shouldProbeObjectBeVisible)
        {
            auto probeModel = probeObject_->GetComponent<StaticModel>();
            probeModel->SetMaterial(cache->GetResource<Material>(probeMaterialName));
        }
    }
}
