// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "GamePlugin.h"
#include "RotateObject.h"
#include "PixelArtGenerator.h"

#include <Urho3D/Input/FreeFlyController.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Math/RandomEngine.h>
#include <Urho3D/Resource/ResourceCache.h>

URHO3D_DEFINE_PLUGIN_MAIN(Urho3D::GamePlugin);

namespace Urho3D
{

GamePlugin::GamePlugin(Context* context)
    : MainPluginApplication(context)
{
}

void GamePlugin::Load()
{
    RegisterObject<RotateObject>();
    RegisterObject<PixelArtGenerator>();
}

void GamePlugin::Unload()
{
}

void GamePlugin::Start(bool isMain)
{
    if (!isMain)
        return;

    auto cache = GetSubsystem<ResourceCache>();
    auto input = GetSubsystem<Input>();
    auto renderer = GetSubsystem<Renderer>();

    // Get materials for the sample
    const ea::string materialFolder = "Materials/Constant/";
    StringVector materialList;
    cache->Scan(materialList, materialFolder, "*.xml", SCAN_FILES | SCAN_RECURSIVE);

    // Get models for the sample
    const ea::string modelFolder = "Models/";
    const StringVector modelList = {
        "Box.mdl",
        "Cone.mdl",
        "Cylinder.mdl",
        "Pyramid.mdl",
        "Sphere.mdl",
        "TeaPot.mdl",
        "Torus.mdl",
    };

    // Create scene
    scene_ = MakeShared<Scene>(context_);
    scene_->CreateComponent<Octree>();

    // Create camera
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<FreeFlyController>();
    auto camera = cameraNode_->CreateComponent<Camera>();

    // Create skybox
    Node* skyboxNode = scene_->CreateChild("Skybox");
    auto skybox = skyboxNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create light
    auto light = cameraNode_->CreateComponent<Light>();
    light->SetLightType(LIGHT_POINT);
    light->SetRange(30.0f);

    // Create zone
    Node* zoneNode = scene_->CreateChild("Zone");
    auto zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color::BLACK);
    zone->SetBackgroundBrightness(0.5f);
    zone->SetZoneTexture(cache->GetResource<TextureCube>("Textures/Skybox.xml"));

    // Create objects
    RandomEngine random{0u};
    const unsigned numObjects = 3000;
    for (unsigned i = 0; i < numObjects; ++i)
    {
        const ea::string materialName = materialFolder + materialList[random.GetUInt(0, materialList.size() - 1)];
        const ea::string modelName = modelFolder + modelList[random.GetUInt(0, modelList.size() - 1)];

        Node* node = scene_->CreateChild("Box");
        node->SetPosition(random.GetVector3(-40.0f * Vector3::ONE, 40.0f * Vector3::ONE));
        node->SetRotation(random.GetQuaternion());
        node->SetScale(random.GetFloat(1.0f, 2.0f));

        auto drawable = node->CreateComponent<StaticModel>();
        drawable->SetModel(cache->GetResource<Model>(modelName));
        drawable->SetMaterial(cache->GetResource<Material>(materialName));

        auto rotator = node->CreateComponent<RotateObject>();
    }

    // Setup engine state
    viewport_ = MakeShared<Viewport>(context_, scene_, camera);
    renderer->SetNumViewports(1);
    renderer->SetViewport(0, viewport_);
    input->SetMouseVisible(false);
    input->SetMouseMode(MM_WRAP);
}

void GamePlugin::Stop()
{
    auto renderer = GetSubsystem<Renderer>();
    renderer->SetNumViewports(0);

    viewport_ = nullptr;
    cameraNode_ = nullptr;
    scene_ = nullptr;
}

}
