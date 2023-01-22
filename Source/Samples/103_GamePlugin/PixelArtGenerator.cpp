//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "PixelArtGenerator.h"

#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>

namespace Urho3D
{

namespace
{

void CreateCommonObjects(Scene* scene)
{
    auto cache = scene->GetSubsystem<ResourceCache>();

    auto skyboxNode = scene->CreateChild("Skybox");
    auto skybox = skyboxNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/DefaultSkybox.xml"));

    auto zoneNode = scene->CreateChild("Global Zone");
    auto zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox{-1000.0f, 1000.0f});
    zone->SetAmbientColor(Color::BLACK);
    zone->SetBackgroundBrightness(0.5f);
    zone->SetZoneTexture(cache->GetResource<TextureCube>("Textures/DefaultSkybox.xml"));

    auto lightNode = scene->CreateChild("Global Light");
    lightNode->SetDirection({1.0f, -1.0f, 1.0f});
    auto light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color::WHITE);
    light->SetBrightness(0.5f);
}

SharedPtr<Image> GetDownScaledImage(Image* image, unsigned maxSize)
{
    maxSize = ea::max(maxSize, 1u);
    SharedPtr<Image> result{image};
    while (result->GetWidth() > maxSize || result->GetHeight() > maxSize)
        result = result->GetNextLevel();
    return result;
}

Color SnapColor(const Color& color)
{
    const float step = 1.0f / 4.0f;

    return Color(VectorRound(color.ToVector4() / step) * step);
}

ea::vector<SharedPtr<Material>> GenerateSceneFromImage(Scene* scene, Image* sourceImage, const ea::string& materialResourcePath, unsigned maxSize)
{
    const float scale = 0.4f;

    auto cache = scene->GetSubsystem<ResourceCache>();

    CreateCommonObjects(scene);

    auto image = GetDownScaledImage(sourceImage, maxSize);
    const IntVector2 imageSize = image->GetSize().ToIntVector2();

    auto mainNode = scene->CreateChild("Pixel Art");
    mainNode->SetScale(scale);
    mainNode->SetPosition({-imageSize.x_ * 0.5f * scale, -imageSize.y_ * 0.5f * scale, 0.0f});

    ea::unordered_map<Color, SharedPtr<Material>> materials;

    const IntRect imageRect{IntVector2::ZERO, imageSize};
    for (const IntVector2 pixel : imageRect)
    {
        const Color pixelColor = SnapColor(image->GetPixel(pixel.x_, pixel.y_));
        if (pixelColor.a_ <= 0.05f)
            continue;

        auto& material = materials[pixelColor];
        if (!material)
        {
            material = cache->GetResource<Material>("Materials/DefaultWhite.xml")->Clone();
            material->SetName(materialResourcePath + Format("Mat_{:#08x}.xml", pixelColor.ToUInt()));
            if (pixelColor.a_ != 1.0f)
                material->SetTechnique(0, cache->GetResource<Technique>("Techniques/LitTransparent.xml"));
            material->SetShaderParameter("MatDiffColor", pixelColor);
        }

        auto pixelNode = mainNode->CreateChild("Pixel");
        pixelNode->SetPosition({static_cast<float>(pixel.x_), static_cast<float>(imageSize.y_ - pixel.y_ - 1), 0.0f});
        pixelNode->SetScale(0.83f);

        auto pixelModel = pixelNode->CreateComponent<StaticModel>();
        pixelModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        pixelModel->SetMaterial(material);
    }

    return materials.values();
}

}

PixelArtGenerator::PixelArtGenerator(Context* context)
    : AssetTransformer(context)
{
}

void PixelArtGenerator::RegisterObject(Context* context)
{
    context->AddFactoryReflection<PixelArtGenerator>(Category_Transformer);

    URHO3D_ATTRIBUTE("Max Size", unsigned, maxSize_, 32, AM_DEFAULT);
}

bool PixelArtGenerator::IsApplicable(const AssetTransformerInput& input)
{
    return input.resourceName_.ends_with(".png", false);
}

bool PixelArtGenerator::Execute(const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    auto cache = GetSubsystem<ResourceCache>();
    auto image = cache->GetResource<Image>(input.resourceName_);
    if (!image)
        return false;

    const ea::string sceneFileName = input.outputFileName_ + "/PixelArt.xml";
    const ea::string materialResourcePath = input.resourceName_ + "/Materials/";

    auto scene = MakeShared<Scene>(context_);
    scene->CreateComponent<Octree>();
    const auto materials = GenerateSceneFromImage(scene, image, materialResourcePath, maxSize_);

    auto xmlFile = MakeShared<XMLFile>(context_);
    XMLElement xmlRoot = xmlFile->CreateRoot("scene");
    if (!scene->SaveXML(xmlRoot))
        return false;

    xmlFile->SaveFile(sceneFileName);

    for (Material* material : materials)
        material->SaveFile(input.tempPath_ + material->GetName());

    return true;
}

}
