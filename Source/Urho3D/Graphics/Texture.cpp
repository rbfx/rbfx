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

#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/GraphicsImpl.h"
#include "../Graphics/Material.h"
#include "../Graphics/Renderer.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp>

#include "../DebugNew.h"

namespace Urho3D
{

static const char* addressModeNames[] =
{
    "wrap",
    "mirror",
    "clamp",
    "border",
    nullptr
};

static const char* filterModeNames[] =
{
    "nearest",
    "bilinear",
    "trilinear",
    "anisotropic",
    "nearestanisotropic",
    "default",
    nullptr
};

Texture::Texture(Context* context)
    : ResourceWithMetadata(context)
    , RawTexture(context)
{
}

Texture::~Texture() = default;

void Texture::SetNumLevels(unsigned levels)
{
    requestedLevels_ = levels;
}

void Texture::SetFilterMode(TextureFilterMode mode)
{
    auto desc = GetSamplerStateDesc();
    desc.filterMode_ = mode;
    SetSamplerStateDesc(desc);
}

void Texture::SetAddressMode(TextureCoordinate coord, TextureAddressMode mode)
{
    auto desc = GetSamplerStateDesc();
    desc.addressMode_[coord] = mode;
    SetSamplerStateDesc(desc);
}

void Texture::SetAnisotropy(unsigned level)
{
    auto desc = GetSamplerStateDesc();
    desc.anisotropy_ = level;
    SetSamplerStateDesc(desc);
}

void Texture::SetShadowCompare(bool enable)
{
    auto desc = GetSamplerStateDesc();
    desc.shadowCompare_ = enable;
    SetSamplerStateDesc(desc);
}

void Texture::SetBorderColor(const Color& color)
{
    auto desc = GetSamplerStateDesc();
    desc.borderColor_ = color;
    SetSamplerStateDesc(desc);
}

void Texture::SetLinear(bool linear)
{
    linear_ = linear;
}

void Texture::SetSRGB(bool enable)
{
    sRGB_ = enable;
}

void Texture::SetBackupTexture(Texture* texture)
{
    backupTexture_ = texture;
}

void Texture::SetMipsToSkip(MaterialQuality quality, int toSkip)
{
    if (quality >= QUALITY_LOW && quality < MAX_TEXTURE_QUALITY_LEVELS)
    {
        mipsToSkip_[quality] = (unsigned)toSkip;

        // Make sure a higher quality level does not actually skip more mips
        for (int i = 1; i < MAX_TEXTURE_QUALITY_LEVELS; ++i)
        {
            if (mipsToSkip_[i] > mipsToSkip_[i - 1])
                mipsToSkip_[i] = mipsToSkip_[i - 1];
        }
    }
}

int Texture::GetMipsToSkip(MaterialQuality quality) const
{
    return (quality >= QUALITY_LOW && quality < MAX_TEXTURE_QUALITY_LEVELS) ? mipsToSkip_[quality] : 0;
}

int Texture::GetLevelWidth(unsigned level) const
{
    if (level > GetLevels())
        return 0;
    return Max(GetWidth() >> level, 1);
}

int Texture::GetLevelHeight(unsigned level) const
{
    if (level > GetLevels())
        return 0;
    return Max(GetHeight() >> level, 1);
}

int Texture::GetLevelDepth(unsigned level) const
{
    if (level > GetLevels())
        return 0;
    return Max(GetDepth() >> level, 1);
}

unsigned Texture::GetDataSize(int width, int height, int depth) const
{
    return depth * GetDataSize(width, height);
}

unsigned Texture::GetComponents() const
{
    if (!GetWidth() || IsCompressed())
        return 0;
    else
        return GetRowDataSize(GetWidth()) / GetWidth();
}

void Texture::SetParameters(XMLFile* file)
{
    if (!file)
        return;

    XMLElement rootElem = file->GetRoot();
    SetParameters(rootElem);
}

void Texture::SetParameters(const XMLElement& element)
{
    LoadMetadataFromXML(element);
    for (XMLElement paramElem = element.GetChild(); paramElem; paramElem = paramElem.GetNext())
    {
        ea::string name = paramElem.GetName();

        if (name == "address")
        {
            ea::string coord = paramElem.GetAttributeLower("coord");
            if (coord.length() >= 1)
            {
                auto coordIndex = (TextureCoordinate)(coord[0] - 'u');
                ea::string mode = paramElem.GetAttributeLower("mode");
                SetAddressMode(coordIndex, (TextureAddressMode)GetStringListIndex(mode.c_str(), addressModeNames, ADDRESS_WRAP));
            }
        }

        if (name == "border")
            SetBorderColor(paramElem.GetColor("color"));

        if (name == "filter")
        {
            ea::string mode = paramElem.GetAttributeLower("mode");
            SetFilterMode((TextureFilterMode)GetStringListIndex(mode.c_str(), filterModeNames, FILTER_DEFAULT));
            if (paramElem.HasAttribute("anisotropy"))
                SetAnisotropy(paramElem.GetUInt("anisotropy"));
        }

        if (name == "mipmap")
            SetNumLevels(paramElem.GetBool("enable") ? 0 : 1);

        if (name == "quality")
        {
            if (paramElem.HasAttribute("low"))
                SetMipsToSkip(QUALITY_LOW, paramElem.GetInt("low"));
            if (paramElem.HasAttribute("med"))
                SetMipsToSkip(QUALITY_MEDIUM, paramElem.GetInt("med"));
            if (paramElem.HasAttribute("medium"))
                SetMipsToSkip(QUALITY_MEDIUM, paramElem.GetInt("medium"));
            if (paramElem.HasAttribute("high"))
                SetMipsToSkip(QUALITY_HIGH, paramElem.GetInt("high"));
        }

        if (name == "srgb")
            SetSRGB(paramElem.GetBool("enable"));

        if (name == "linear")
            SetLinear(paramElem.GetBool("enable"));
    }
}

bool Texture::CreateGPU()
{
    if (!RawTexture::CreateGPU())
        return false;

    const bool isRTV = GetParams().flags_.Test(TextureFlag::BindRenderTarget);
    const bool isDSV = GetParams().flags_.Test(TextureFlag::BindDepthStencil);

    if (isRTV)
        SubscribeToEvent(E_RENDERSURFACEUPDATE, &Texture::HandleRenderSurfaceUpdate);
    else
        UnsubscribeFromEvent(E_RENDERSURFACEUPDATE);

    if (isRTV || isDSV)
    {
        auto renderSurfaceHandles = GetHandles().renderSurfaces_;
        const unsigned numRenderSurfaces = renderSurfaceHandles.size();

        if (renderSurfaces_.size() != numRenderSurfaces)
        {
            renderSurfaces_.clear();
            for (unsigned i = 0; i < numRenderSurfaces; ++i)
                renderSurfaces_.push_back(MakeShared<RenderSurface>(this));
        }

        for (unsigned i = 0; i < numRenderSurfaces; ++i)
            renderSurfaces_[i]->Restore(renderSurfaceHandles[i]);
    }

    return true;
}

void Texture::DestroyGPU()
{
    for (RenderSurface* renderSurface : renderSurfaces_)
        renderSurface->Invalidate();

    RawTexture::DestroyGPU();
}

bool Texture::TryRestore()
{
    auto* cache = GetSubsystem<ResourceCache>();
    if (cache->Exists(GetName()))
        return cache->ReloadResource(this);
    return false;
}

void Texture::CheckTextureBudget(StringHash type)
{
    auto* cache = GetSubsystem<ResourceCache>();
    unsigned long long textureBudget = cache->GetMemoryBudget(type);
    unsigned long long textureUse = cache->GetMemoryUse(type);
    if (!textureBudget)
        return;

    // If textures are over the budget, they likely can not be freed directly as materials still refer to them.
    // Therefore free unused materials first
    if (textureUse > textureBudget)
        cache->ReleaseResources(Material::GetTypeStatic());
}

void Texture::HandleRenderSurfaceUpdate()
{
    auto* renderer = GetSubsystem<Renderer>();

    for (RenderSurface* renderSurface : renderSurfaces_)
    {
        if (renderSurface->GetUpdateMode() == SURFACE_UPDATEALWAYS || renderSurface->IsUpdateQueued())
        {
            if (renderer)
                renderer->QueueRenderSurface(renderSurface);
            renderSurface->ResetUpdateQueued();
        }
    }
}

bool Texture::IsCompressed() const
{
    const auto& formatInfo = Diligent::GetTextureFormatAttribs(GetParams().format_);
    return formatInfo.ComponentType == Diligent::COMPONENT_TYPE_COMPRESSED;
}

unsigned Texture::GetDataSize(int width, int height) const
{
    const auto& formatInfo = Diligent::GetTextureFormatAttribs(GetParams().format_);
    return GetRowDataSize(width) * ((height + formatInfo.BlockHeight - 1) / formatInfo.BlockHeight);
}

unsigned Texture::GetRowDataSize(int width) const
{
    const auto& formatInfo = Diligent::GetTextureFormatAttribs(GetParams().format_);
    return formatInfo.GetElementSize() * ((width + formatInfo.BlockWidth - 1) / formatInfo.BlockWidth);
}

TextureFormat Texture::GetSRGBFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::TEX_FORMAT_RGBA8_UNORM: return TextureFormat::TEX_FORMAT_RGBA8_UNORM_SRGB;
    case TextureFormat::TEX_FORMAT_BGRA8_UNORM: return TextureFormat::TEX_FORMAT_BGRA8_UNORM_SRGB;
    case TextureFormat::TEX_FORMAT_BGRX8_UNORM: return TextureFormat::TEX_FORMAT_BGRX8_UNORM_SRGB;
    case TextureFormat::TEX_FORMAT_BC1_UNORM: return TextureFormat::TEX_FORMAT_BC1_UNORM_SRGB;
    case TextureFormat::TEX_FORMAT_BC2_UNORM: return TextureFormat::TEX_FORMAT_BC2_UNORM_SRGB;
    case TextureFormat::TEX_FORMAT_BC3_UNORM: return TextureFormat::TEX_FORMAT_BC3_UNORM_SRGB;
    case TextureFormat::TEX_FORMAT_BC7_UNORM: return TextureFormat::TEX_FORMAT_BC7_UNORM_SRGB;
    }
    return format;
}

}
