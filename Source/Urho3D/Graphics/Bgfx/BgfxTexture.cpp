//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "../../Core/StringUtils.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Material.h"
#include "../../IO/FileSystem.h"
#include "../../Resource/ResourceCache.h"
#include "../../Resource/XMLFile.h"

#include "../../DebugNew.h"

#include <bimg/bimg.h>

namespace Urho3D
{

// Texture flag conversion
static const uint32_t bgfxWrapU[] =
{
    BGFX_TEXTURE_NONE,
    BGFX_TEXTURE_U_MIRROR,
    BGFX_TEXTURE_U_CLAMP,
    BGFX_TEXTURE_U_BORDER
};

static const uint32_t bgfxWrapV[] =
{
    BGFX_TEXTURE_NONE,
    BGFX_TEXTURE_V_MIRROR,
    BGFX_TEXTURE_V_CLAMP,
    BGFX_TEXTURE_V_BORDER
};

static const uint32_t bgfxWrapW[] =
{
    BGFX_TEXTURE_NONE,
    BGFX_TEXTURE_W_MIRROR,
    BGFX_TEXTURE_W_CLAMP,
    BGFX_TEXTURE_W_BORDER
};

static const uint32_t bgfxFilterMode[] =
{
    BGFX_TEXTURE_MIN_POINT | BGFX_TEXTURE_MAG_POINT | BGFX_TEXTURE_MIP_POINT, // FILTER_NEAREST
    BGFX_TEXTURE_MIP_POINT, // FILTER_BILINEAR
    BGFX_TEXTURE_NONE, // FILTER_TRILINEAR
    BGFX_TEXTURE_MIN_ANISOTROPIC | BGFX_TEXTURE_MAG_ANISOTROPIC, // FILTER_ANISOTROPIC
    BGFX_TEXTURE_MIN_POINT | BGFX_TEXTURE_MAG_POINT, // FILTER_NEAREST_ANISOTROPIC
    BGFX_TEXTURE_NONE // FILTER_DEFAULT
};

void Texture::SetSRGB(bool enable)
{
    if (graphics_)
        enable &= graphics_->GetSRGBSupport();

    if (enable != sRGB_)
    {
        sRGB_ = enable;
        parametersDirty_ = true;
        // If texture had already been created, must recreate it to set the sRGB texture format
        if (object_.idx_ != bgfx::kInvalidHandle)
            Create();
    }
}

void Texture::UpdateParameters()
{
}

bool Texture::GetParametersDirty() const
{
    return false;
}

bool Texture::IsCompressed() const
{
    return bimg::isCompressed((bimg::TextureFormat::Enum)format_);
}

unsigned Texture::GetRowDataSize(int width) const
{
    return 0;
}

unsigned Texture::GetSRGBFormat(unsigned format)
{
    return format;
}

void Texture::RegenerateLevels()
{
}

unsigned Texture::GetSRVFormat(unsigned format)
{
    return 0;
}

unsigned Texture::GetDSVFormat(unsigned format)
{
    return 0;
}

unsigned Texture::GetBGFXFlags()
{
    unsigned flags = 0;
    flags |= bgfxWrapU[addressMode_[0]] | bgfxWrapV[addressMode_[1]] | bgfxWrapW[addressMode_[2]] | bgfxFilterMode[filterMode_];
    if (sRGB_)
        flags |= BGFX_TEXTURE_SRGB;
    if ((usage_ == TEXTURE_RENDERTARGET) || (usage_ == TEXTURE_DEPTHSTENCIL))
        flags |= BGFX_TEXTURE_RT;
    if (shadowCompare_)
        flags |= BGFX_TEXTURE_COMPARE_LEQUAL;
    if (multiSample_ > 1)
    {
        //flags |= BGFX_TEXTURE_MSAA_SAMPLE;
        switch (multiSample_)
        {
        case 2:
            flags |= BGFX_TEXTURE_RT_MSAA_X2;
            break;
        case 4:
            flags |= BGFX_TEXTURE_RT_MSAA_X4;
            break;
        case 8:
            flags |= BGFX_TEXTURE_RT_MSAA_X8;
            break;
        case 16:
            flags |= BGFX_TEXTURE_RT_MSAA_X16;
            break;
        default:
            break;
        }
    }

    return flags;
}

}
