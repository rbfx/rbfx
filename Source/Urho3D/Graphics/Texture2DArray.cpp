// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Texture2DArray.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#include "../DebugNew.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

namespace Urho3D
{

Texture2DArray::Texture2DArray(Context* context)
    : Texture(context)
{
}

Texture2DArray::~Texture2DArray()
{
}

void Texture2DArray::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Texture2DArray>();
}

bool Texture2DArray::BeginLoad(Deserializer& source)
{
    auto graphics = GetSubsystem<Graphics>();
    auto* cache = GetSubsystem<ResourceCache>();

    // In headless mode, do not actually load the texture, just return success
    if (!graphics)
        return true;

    cache->ResetDependencies(this);

    ea::string texPath, texName, texExt;
    SplitPath(GetName(), texPath, texName, texExt);

    loadParameters_ = (MakeShared<XMLFile>(context_));
    if (!loadParameters_->Load(source))
    {
        loadParameters_.Reset();
        return false;
    }

    loadImages_.clear();

    XMLElement textureElem = loadParameters_->GetRoot();
    XMLElement layerElem = textureElem.GetChild("layer");
    while (layerElem)
    {
        ea::string name = layerElem.GetAttribute("name");

        // If path is empty, add the XML file path
        if (GetPath(name).empty())
            name = texPath + name;

        loadImages_.push_back(cache->GetTempResource<Image>(name));
        cache->StoreResourceDependency(this, name);

        layerElem = layerElem.GetNext("layer");
    }

    // Precalculate mip levels if async loading
    if (GetAsyncLoadState() == ASYNC_LOADING)
    {
        for (unsigned i = 0; i < loadImages_.size(); ++i)
        {
            if (loadImages_[i])
                loadImages_[i]->PrecalculateLevels();
        }
    }

    return true;
}

bool Texture2DArray::EndLoad()
{
    // In headless mode, do not actually load the texture, just return success
    if (!renderDevice_)
        return true;

    // If over the texture budget, see if materials can be freed to allow textures to be freed
    CheckTextureBudget(GetTypeStatic());

    SetParameters(loadParameters_);
    SetLayers(loadImages_.size());

    for (unsigned i = 0; i < loadImages_.size(); ++i)
        SetData(i, loadImages_[i]);

    loadImages_.clear();
    loadParameters_.Reset();

    return true;
}

void Texture2DArray::SetLayers(unsigned layers)
{
    layers_ = layers;
}

bool Texture2DArray::SetSize(unsigned layers, int width, int height, TextureFormat format, TextureFlags flags)
{
    layers_ = layers ? layers : layers_;

    RawTextureParams params;
    params.type_ = TextureType::Texture2DArray;
    params.format_ = format;
    params.size_ = {width, height, 1};
    params.arraySize_ = layers_;
    params.numLevels_ = requestedLevels_;
    params.flags_ = flags;
    if (requestedSRGB_)
        params.format_ = SetTextureFormatSRGB(params.format_);

    return Create(params);
}

bool Texture2DArray::SetData(unsigned layer, unsigned level, int x, int y, int width, int height, const void* data)
{
    Update(level, {x, y, 0}, {width, height, 1}, layer, data);
    return true;
}

bool Texture2DArray::SetData(unsigned layer, Deserializer& source)
{
    SharedPtr<Image> image(MakeShared<Image>(context_));
    if (!image->Load(source))
        return false;

    return SetData(layer, image);
}

bool Texture2DArray::SetData(unsigned layer, Image* image)
{
    if (layer == 0)
    {
        RawTextureParams params;
        params.type_ = TextureType::Texture2DArray;
        params.numLevels_ = requestedLevels_;
        params.arraySize_ = layers_;
        if (!CreateForImage(params, image))
            return false;
    }

    return UpdateFromImage(layer, image);
}

bool Texture2DArray::GetData(unsigned layer, unsigned level, void* dest)
{
    return Read(layer, level, dest, M_MAX_UNSIGNED);
}

}
