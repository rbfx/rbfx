// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "TextureFormats.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Input/FreeFlyController.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/RenderPipeline/ShaderConsts.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/UI.h>

#include <Urho3D/DebugNew.h>

TextureFormatsSample::TextureFormatsSample(Context* context)
    : Sample(context)
{
}

void TextureFormatsSample::Start()
{
    Sample::Start();

    CreateScene();
    CreateInstructions();
    SetupViewport();

    SetMouseMode(MM_RELATIVE);
    SetMouseVisible(false);
}

void TextureFormatsSample::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();

    // Create ground plane
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    auto* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create directional light
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);

    // Create camera.
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();
    cameraNode_->CreateComponent<FreeFlyController>();
    cameraNode_->SetPosition(Vector3(0.0f, 3.0f, -6.0f));

    // Create labels for textured objects.
    Node* softwareLabel = CreateLabel("Supported\non Software");
    softwareLabel->SetPosition({0.0f, 0.5f, 0.0f});
    softwareLabel->SetRotation(Quaternion{45.0f, Vector3::RIGHT});

    Node* hardwareLabel = CreateLabel("Supported\non Hardware");
    hardwareLabel->SetPosition({2.0f, 0.5f, 0.0f});
    hardwareLabel->SetRotation(Quaternion{45.0f, Vector3::RIGHT});

    // Create textured objects.
    static const StringVector textures = {
        "Textures/Formats/RGBA.dds",
        "Textures/Formats/DXT1.dds",
        "Textures/Formats/DXT3.dds",
        "Textures/Formats/DXT5.dds",
        "Textures/Formats/ETC1.dds",
        "Textures/Formats/ETC2.dds",
        "Textures/Formats/PTC2.dds",
        "Textures/Formats/PTC4.dds",
    };

    int index = 0;
    for (const ea::string& textureName : textures)
    {
        const auto image = cache->GetResource<Image>(textureName);
        const auto texture = cache->GetResource<Texture2D>(textureName);
        const bool isHardwareSupported = image->GetGPUFormat() == texture->GetFormat();

        const auto decompressedImage = image->GetDecompressedImage();
        const auto decompressedTexture = MakeShared<Texture2D>(context_);
        decompressedTexture->SetData(decompressedImage);

        Node* softwareBox = CreateTexturedBox(decompressedTexture);
        softwareBox->SetPosition({0.0f, 0.5f, 2.0f + 2.0f * index});

        if (isHardwareSupported)
        {
            Node* hardwareBox = CreateTexturedBox(texture);
            hardwareBox->SetPosition({2.0f, 0.5f, 2.0f + 2.0f * index});
        }

        ++index;
    }
}

Node* TextureFormatsSample::CreateTexturedBox(Texture2D* texture)
{
    auto* cache = GetSubsystem<ResourceCache>();

    auto material = MakeShared<Material>(context_);
    material->SetTechnique(0, cache->GetResource<Technique>("Techniques/LitOpaque.xml"));
    material->SetTexture(ShaderResources::Albedo, texture);

    Node* node = scene_->CreateChild("Box");
    auto staticModel = node->CreateComponent<StaticModel>();
    staticModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    staticModel->SetMaterial(material);
    staticModel->SetCastShadows(true);

    return node;
}

Node* TextureFormatsSample::CreateLabel(const ea::string& text)
{
    auto* cache = GetSubsystem<ResourceCache>();

    Node* node = scene_->CreateChild("Text3D");
    auto* text3D = node->CreateComponent<Text3D>();

    text3D->SetText(text);
    text3D->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.sdf"), 24);
    text3D->SetColor(Color::WHITE);
    text3D->SetAlignment(HA_CENTER, VA_CENTER);
    text3D->SetFaceCameraMode(FC_NONE);

    return node;
}

void TextureFormatsSample::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = GetUIRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys and mouse/touch to move");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, GetUIRoot()->GetHeight() / 4);
}

void TextureFormatsSample::SetupViewport()
{
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    SetViewport(0, viewport);
}
