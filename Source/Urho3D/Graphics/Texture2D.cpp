//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rightsR
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
#include "../Graphics/Texture2D.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#include "../DebugNew.h"

namespace Urho3D
{

Texture2D::Texture2D(Context* context)
    : Texture(context)
{
}

Texture2D::~Texture2D()
{
}

void Texture2D::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Texture2D>();
}

bool Texture2D::BeginLoad(Deserializer& source)
{
    auto graphics = GetSubsystem<Graphics>();

    // In headless mode, do not actually load the texture, just return success
    if (!graphics)
        return true;

    // Load the image data for EndLoad()
    loadImage_ = MakeShared<Image>(context_);
    if (!loadImage_->Load(source))
    {
        loadImage_.Reset();
        return false;
    }

    // Precalculate mip levels if async loading
    if (GetAsyncLoadState() == ASYNC_LOADING)
        loadImage_->PrecalculateLevels();

    // Load the optional parameters file
    auto* cache = GetSubsystem<ResourceCache>();
    ea::string xmlName = ReplaceExtension(GetName(), ".xml");
    loadParameters_ = cache->GetTempResource<XMLFile>(xmlName, false);

    return true;
}

bool Texture2D::EndLoad()
{
    // In headless mode, do not actually load the texture, just return success
    if (!renderDevice_)
        return true;

    // If over the texture budget, see if materials can be freed to allow textures to be freed
    CheckTextureBudget(GetTypeStatic());

    SetParameters(loadParameters_);
    bool success = SetData(loadImage_);

    loadImage_.Reset();
    loadParameters_.Reset();

    return success;
}

bool Texture2D::SetSize(int width, int height, TextureFormat format, TextureFlags flags, int multiSample)
{
    RawTextureParams params;
    params.type_ = TextureType::Texture2D;
    params.format_ = format;
    params.size_ = {width, height, 1};
    params.numLevels_ = requestedLevels_;
    params.flags_ = flags;
    params.multiSample_ = multiSample;
    if (requestedSRGB_)
        params.format_ = SetTextureFormatSRGB(params.format_);

    return Create(params);
}

bool Texture2D::SetData(unsigned level, int x, int y, int width, int height, const void* data)
{
    Update(level, {x, y, 0}, {width, height, 1}, 0, data);
    return true;
}

bool Texture2D::SetData(Image* image)
{
    RawTextureParams params;
    params.type_ = TextureType::Texture2D;
    params.numLevels_ = requestedLevels_;
    if (!CreateForImage(params, image))
        return false;

    return UpdateFromImage(0, image);
}

bool Texture2D::GetData(unsigned level, void* dest)
{
    return Read(0, level, dest, M_MAX_UNSIGNED);
}

bool Texture2D::GetImage(Image& image)
{
    return ReadToImage(0, 0, &image);
}

SharedPtr<Image> Texture2D::GetImage()
{
    auto rawImage = MakeShared<Image>(context_);
    if (!GetImage(*rawImage))
        return nullptr;
    return rawImage;
}

}
