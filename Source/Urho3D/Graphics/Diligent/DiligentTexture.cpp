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

#include "../../Precompiled.h"

#include "../../Core/Profiler.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Material.h"
#include "../../IO/FileSystem.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"
#include "../../Resource/XMLFile.h"

#include "../../DebugNew.h"
#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
namespace Urho3D
{
    using namespace Diligent;
    static const FILTER_TYPE sMinMagFilterModes[] = {
        FILTER_TYPE_POINT,
        FILTER_TYPE_LINEAR,
        FILTER_TYPE_LINEAR,
        FILTER_TYPE_ANISOTROPIC,
        FILTER_TYPE_POINT,
        FILTER_TYPE_COMPARISON_POINT,
        FILTER_TYPE_COMPARISON_LINEAR,
        FILTER_TYPE_COMPARISON_LINEAR,
        FILTER_TYPE_COMPARISON_ANISOTROPIC,
        FILTER_TYPE_COMPARISON_POINT
    };
    static const FILTER_TYPE sMipFilterModes[] = {
        FILTER_TYPE_POINT,
        FILTER_TYPE_POINT,
        FILTER_TYPE_LINEAR,
        FILTER_TYPE_ANISOTROPIC,
        FILTER_TYPE_LINEAR,
        FILTER_TYPE_COMPARISON_POINT,
        FILTER_TYPE_COMPARISON_POINT,
        FILTER_TYPE_COMPARISON_LINEAR,
        FILTER_TYPE_COMPARISON_ANISOTROPIC,
        FILTER_TYPE_COMPARISON_LINEAR
    };
    static const Diligent::TEXTURE_ADDRESS_MODE sAddressModes[] = {
        TEXTURE_ADDRESS_WRAP,
        TEXTURE_ADDRESS_MIRROR,
        TEXTURE_ADDRESS_CLAMP,
        TEXTURE_ADDRESS_BORDER
    };

void Texture::SetSRGB(bool enable)
{
    if (graphics_)
        enable &= graphics_->GetSRGBSupport();

    if (enable != sRGB_)
    {
        sRGB_ = enable;
        // If texture had already been created, must recreate it to set the sRGB texture format
        if (object_)
            Create();
    }
}

bool Texture::GetParametersDirty() const
{
    return parametersDirty_ || !sampler_;
}

bool Texture::IsCompressed() const
{
    return format_ == TEX_FORMAT_BC1_UNORM || format_ == TEX_FORMAT_BC2_UNORM || format_ == TEX_FORMAT_BC3_UNORM;
}

unsigned Texture::GetDataSize(int width, int height) const
{
    if (IsCompressed())
        return GetRowDataSize(width) * ((height + 3u) >> 2u);
    else
        return GetRowDataSize(width) * height;
}

unsigned Texture::GetRowDataSize(int width) const
{
    switch (format_)
    {
    case TEX_FORMAT_R8_UNORM:
    case TEX_FORMAT_A8_UNORM:
        return (unsigned)width;

    case TEX_FORMAT_RG8_UNORM:
    case TEX_FORMAT_R16_UNORM:
    case TEX_FORMAT_R16_FLOAT:
    case TEX_FORMAT_R16_TYPELESS:
        return (unsigned)(width * 2);

    case TEX_FORMAT_RGBA8_UNORM:
    case TEX_FORMAT_BGRX8_UNORM:
    case TEX_FORMAT_RG16_UNORM:
    case TEX_FORMAT_RG16_FLOAT:
    case TEX_FORMAT_R32_FLOAT:
    case TEX_FORMAT_R24G8_TYPELESS:
    case TEX_FORMAT_R32_TYPELESS:
        return (unsigned)(width * 4);

    case TEX_FORMAT_RGBA16_UNORM:
    case TEX_FORMAT_RGBA16_FLOAT:
        return (unsigned)(width * 8);

    case TEX_FORMAT_RGBA32_FLOAT:
        return (unsigned)(width * 16);

    case TEX_FORMAT_BC1_UNORM:
        return (unsigned)(((width + 3) >> 2) * 8);

    case TEX_FORMAT_BC2_UNORM:
    case TEX_FORMAT_BC3_UNORM:
        return (unsigned)(((width + 3) >> 2) * 16);

    default:
        return 0;
    }
}

void Texture::UpdateParameters()
{
    if ((!parametersDirty_ && sampler_) || !object_)
        return;

    unsigned filterModeIndex = filterMode_ != FILTER_DEFAULT ? filterMode_ : graphics_->GetDefaultTextureFilterMode();
    if (shadowCompare_)
        filterModeIndex += 5;

    SamplerDesc samplerDesc;
    samplerDesc.Name = GetName().c_str();
    samplerDesc.MinFilter = samplerDesc.MagFilter = sMinMagFilterModes[filterModeIndex];
    samplerDesc.MipFilter = sMipFilterModes[filterModeIndex];
    samplerDesc.AddressU = sAddressModes[addressModes_[0]];
    samplerDesc.AddressV = sAddressModes[addressModes_[1]];
    samplerDesc.AddressW = sAddressModes[addressModes_[2]];
    samplerDesc.MaxAnisotropy = anisotropy_ ? anisotropy_ : graphics_->GetDefaultTextureAnisotropy();
    samplerDesc.ComparisonFunc = COMPARISON_FUNC_LESS_EQUAL;
    samplerDesc.MinLOD = -M_INFINITY;
    samplerDesc.MaxLOD = M_INFINITY;
    memcpy(&samplerDesc.BorderColor, borderColor_.Data(), 4 * sizeof(float));

    graphics_->GetImpl()->GetDevice()->CreateSampler(samplerDesc, (ISampler**)&sampler_);
    if (sampler_ == nullptr) {
        URHO3D_LOGERROR("Failed to create sampler state");
    }

    parametersDirty_ = false;
}

unsigned Texture::GetSRVFormat(unsigned format)
{
    switch (format) {
    case TEX_FORMAT_R24G8_TYPELESS:
        format = TEX_FORMAT_R24_UNORM_X8_TYPELESS;
        break;
    case TEX_FORMAT_R16_TYPELESS:
        format = TEX_FORMAT_R16_UNORM;
        break;
    case TEX_FORMAT_R32_TYPELESS:
        format = TEX_FORMAT_R32_FLOAT;
        break;
    }
    return format;
}

unsigned Texture::GetDSVFormat(unsigned format)
{
    switch (format) {
    case TEX_FORMAT_R24G8_TYPELESS:
        format = TEX_FORMAT_D24_UNORM_S8_UINT;
        break;
    case TEX_FORMAT_R16_TYPELESS:
        format = TEX_FORMAT_D16_UNORM;
        break;
    case TEX_FORMAT_R32_TYPELESS:
        format = TEX_FORMAT_D32_FLOAT;
        break;
    }
    return format;
}

unsigned Texture::GetSRGBFormat(unsigned format)
{
    switch(format) {
        case TEX_FORMAT_RGBA8_UNORM:
            return TEX_FORMAT_RGBA8_UNORM_SRGB;
        case TEX_FORMAT_BC1_UNORM:
            return TEX_FORMAT_BC1_UNORM_SRGB;
        case TEX_FORMAT_BC2_UNORM:
            return TEX_FORMAT_BC2_UNORM;
        case TEX_FORMAT_BC3_UNORM:
            return TEX_FORMAT_BC3_UNORM_SRGB;
    }
    return format;
}

void Texture::RegenerateLevels()
{
    if (!shaderResourceView_)
        return;

    graphics_->GetImpl()->GetDeviceContext()->GenerateMips(static_cast<ITextureView*>(shaderResourceView_));
    levelsDirty_ = false;
}

unsigned Texture::GetExternalFormat(unsigned format)
{
    return 0;
}

unsigned Texture::GetDataType(unsigned format)
{
    return 0;
}

bool Texture::IsComputeWriteable(unsigned format)
{
    switch (format) {
    case TEX_FORMAT_RGBA8_UNORM:
    case TEX_FORMAT_RGBA8_SNORM:
    case TEX_FORMAT_RGBA8_UINT:
        return true;
    case TEX_FORMAT_RGBA16_FLOAT:
    case TEX_FORMAT_RGBA32_FLOAT:
        return true;
    case TEX_FORMAT_R32_FLOAT:
        return true;
    }
    return false;
}

}
