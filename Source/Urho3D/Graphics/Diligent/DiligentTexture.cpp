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
static const D3D11_FILTER sFilterMode[] =
{
    D3D11_FILTER_MIN_MAG_MIP_POINT,
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
    D3D11_FILTER_MIN_MAG_MIP_LINEAR,
    D3D11_FILTER_ANISOTROPIC,
    D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
    D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
    D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
    D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
    D3D11_FILTER_COMPARISON_ANISOTROPIC,
    D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR
};

static const TEXTURE_ADDRESS_MODE d3dAddressMode[] =
{
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
        if (object_.name_)
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
    assert(0);
    //if ((!parametersDirty_ && sampler_) || !object_.ptr_)
    //    return;

    //// Release old sampler
    //URHO3D_SAFE_RELEASE(sampler_);

    //D3D11_SAMPLER_DESC samplerDesc;
    //memset(&samplerDesc, 0, sizeof samplerDesc);
    //unsigned filterModeIndex = filterMode_ != FILTER_DEFAULT ? filterMode_ : graphics_->GetDefaultTextureFilterMode();
    //if (shadowCompare_)
    //    filterModeIndex += 5;
    //samplerDesc.Filter = sFilterMode[filterModeIndex];
    //samplerDesc.AddressU = d3dAddressMode[addressModes_[0]];
    //samplerDesc.AddressV = d3dAddressMode[addressModes_[1]];
    //samplerDesc.AddressW = d3dAddressMode[addressModes_[2]];
    //samplerDesc.MaxAnisotropy = anisotropy_ ? anisotropy_ : graphics_->GetDefaultTextureAnisotropy();
    //samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    //samplerDesc.MinLOD = -M_INFINITY;
    //samplerDesc.MaxLOD = M_INFINITY;
    //memcpy(&samplerDesc.BorderColor, borderColor_.Data(), 4 * sizeof(float));

    //HRESULT hr = graphics_->GetImpl()->GetDevice()->CreateSamplerState(&samplerDesc, (ID3D11SamplerState**)&sampler_);
    //if (FAILED(hr))
    //{
    //    URHO3D_SAFE_RELEASE(sampler_);
    //    URHO3D_LOGD3DERROR("Failed to create sampler state", hr);
    //}

    //parametersDirty_ = false;
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
    if (format == DXGI_FORMAT_R8G8B8A8_UNORM)
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    else if (format == DXGI_FORMAT_BC1_UNORM)
        return DXGI_FORMAT_BC1_UNORM_SRGB;
    else if (format == DXGI_FORMAT_BC2_UNORM)
        return DXGI_FORMAT_BC2_UNORM_SRGB;
    else if (format == DXGI_FORMAT_BC3_UNORM)
        return DXGI_FORMAT_BC3_UNORM_SRGB;
    else
        return format;
}

void Texture::RegenerateLevels()
{
    if (!shaderResourceView_)
        return;

    assert(0);
    graphics_->GetImpl()->GetDiligentDeviceContext()->GenerateMips(static_cast<ITextureView*>(shaderResourceView_));
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
