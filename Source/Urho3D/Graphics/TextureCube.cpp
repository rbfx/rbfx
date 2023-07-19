//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextureCube.h"
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

static const char* cubeMapLayoutNames[] = {
    "horizontal",
    "horizontalnvidia",
    "horizontalcross",
    "verticalcross",
    "blender",
    nullptr
};

static SharedPtr<Image> GetTileImage(Image* src, int tileX, int tileY, int tileWidth, int tileHeight)
{
    return SharedPtr<Image>(
        src->GetSubimage(IntRect(tileX * tileWidth, tileY * tileHeight, (tileX + 1) * tileWidth, (tileY + 1) * tileHeight)));
}

TextureCube::TextureCube(Context* context)
    : Texture(context)
{
    SetSamplerStateDesc(SamplerStateDesc::Bilinear(ADDRESS_CLAMP));
}

TextureCube::~TextureCube()
{
}

void TextureCube::RegisterObject(Context* context)
{
    context->AddFactoryReflection<TextureCube>();
}

bool TextureCube::BeginLoad(Deserializer& source)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto graphics = GetSubsystem<Graphics>();

    // In headless mode, do not actually load the texture, just return success
    if (!graphics)
        return true;

    cache->ResetDependencies(this);

    // Load resource
    loadImageCube_ = cache->GetTempResource<ImageCube>(GetName());
    if (!loadImageCube_)
        return false;

    // Update dependencies
    for (Image* image : loadImageCube_->GetImages())
    {
        if (image && !image->GetName().empty())
            cache->StoreResourceDependency(this, image->GetName());
    }

    return true;
}

bool TextureCube::EndLoad()
{
    // In headless mode, do not actually load the texture, just return success
    if (!renderDevice_)
        return true;

    // If over the texture budget, see if materials can be freed to allow textures to be freed
    CheckTextureBudget(GetTypeStatic());

    SetParameters(loadImageCube_->GetParametersXML());

    const auto& images = loadImageCube_->GetImages();
    for (unsigned i = 0; i < images.size() && i < MAX_CUBEMAP_FACES; ++i)
        SetData((CubeMapFace)i, images[i]);

    loadImageCube_ = nullptr;

    return true;
}

bool TextureCube::SetSize(int size, TextureFormat format, TextureFlags flags, int multiSample)
{
    RawTextureParams params;
    params.type_ = TextureType::TextureCube;
    params.format_ = format;
    params.size_ = {size, size, 1};
    params.numLevels_ = requestedLevels_;
    params.flags_ = flags;
    params.multiSample_ = multiSample;
    if (requestedSRGB_)
        params.format_ = SetTextureFormatSRGB(params.format_);

    return Create(params);
}

bool TextureCube::SetData(CubeMapFace face, unsigned level, int x, int y, int width, int height, const void* data)
{
    Update(level, {x, y, 0}, {width, height, 1}, face, data);
    return true;
}

bool TextureCube::SetData(CubeMapFace face, Deserializer& source)
{
    SharedPtr<Image> image(MakeShared<Image>(context_));
    if (!image->Load(source))
        return false;

    return SetData(face, image);
}

bool TextureCube::SetData(CubeMapFace face, Image* image)
{
    if (!face)
    {
        RawTextureParams params;
        params.type_ = TextureType::TextureCube;
        params.numLevels_ = requestedLevels_;
        if (!CreateForImage(params, image))
            return false;
    }

    return UpdateFromImage(face, image);
}

bool TextureCube::GetData(CubeMapFace face, unsigned level, void* dest)
{
    return Read(face, level, dest, M_MAX_UNSIGNED);
}

SharedPtr<Image> TextureCube::GetImage(CubeMapFace face)
{
    auto image = MakeShared<Image>(context_);
    if (ReadToImage(face, 0, image))
        return image;
    return nullptr;
}

}
