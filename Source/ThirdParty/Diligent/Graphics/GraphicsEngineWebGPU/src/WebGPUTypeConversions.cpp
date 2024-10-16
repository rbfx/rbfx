/*
 *  Copyright 2023-2024 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include "pch.h"

#include "WebGPUTypeConversions.hpp"
#include "WebGPUStubs.hpp"
#include "DebugUtilities.hpp"
#include "GraphicsAccessories.hpp"

#define WEBGPU_FORMAT_RANGE_SIZE (WGPUTextureFormat_RGBA16Snorm - WGPUTextureFormat_Undefined + 1)

namespace Diligent
{

class TexFormatToWebGPUFormatMapper
{
public:
    TexFormatToWebGPUFormatMapper()
    {
        // clang-format off
        m_FmtToWGPUFmtMap[TEX_FORMAT_UNKNOWN] = WGPUTextureFormat_Undefined;

        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA32_TYPELESS] = WGPUTextureFormat_RGBA32Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA32_FLOAT]    = WGPUTextureFormat_RGBA32Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA32_UINT]     = WGPUTextureFormat_RGBA32Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA32_SINT]     = WGPUTextureFormat_RGBA32Sint;

        m_FmtToWGPUFmtMap[TEX_FORMAT_RGB32_TYPELESS] = WGPUTextureFormat_Undefined;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGB32_FLOAT]    = WGPUTextureFormat_Undefined;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGB32_UINT]     = WGPUTextureFormat_Undefined;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGB32_SINT]     = WGPUTextureFormat_Undefined;

        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA16_TYPELESS] = WGPUTextureFormat_RGBA16Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA16_FLOAT]    = WGPUTextureFormat_RGBA16Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA16_UNORM]    = WGPUTextureFormat_RGBA16Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA16_UINT]     = WGPUTextureFormat_RGBA16Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA16_SNORM]    = WGPUTextureFormat_RGBA16Snorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA16_SINT]     = WGPUTextureFormat_RGBA16Sint;

        m_FmtToWGPUFmtMap[TEX_FORMAT_RG32_TYPELESS] = WGPUTextureFormat_RG32Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG32_FLOAT]    = WGPUTextureFormat_RG32Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG32_UINT]     = WGPUTextureFormat_RG32Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG32_SINT]     = WGPUTextureFormat_RG32Sint;

        m_FmtToWGPUFmtMap[TEX_FORMAT_R32G8X24_TYPELESS]        = WGPUTextureFormat_Depth32FloatStencil8;
        m_FmtToWGPUFmtMap[TEX_FORMAT_D32_FLOAT_S8X24_UINT]     = WGPUTextureFormat_Depth32FloatStencil8;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS] = WGPUTextureFormat_Depth32FloatStencil8;
        m_FmtToWGPUFmtMap[TEX_FORMAT_X32_TYPELESS_G8X24_UINT]  = WGPUTextureFormat_Depth32FloatStencil8;

        m_FmtToWGPUFmtMap[TEX_FORMAT_RGB10A2_TYPELESS] = WGPUTextureFormat_RGB10A2Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGB10A2_UNORM]    = WGPUTextureFormat_RGB10A2Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGB10A2_UINT]     = WGPUTextureFormat_RGB10A2Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R11G11B10_FLOAT]  = WGPUTextureFormat_RG11B10Ufloat;

        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA8_TYPELESS]   = WGPUTextureFormat_RGBA8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA8_UNORM]      = WGPUTextureFormat_RGBA8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA8_UNORM_SRGB] = WGPUTextureFormat_RGBA8UnormSrgb;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA8_UINT]       = WGPUTextureFormat_RGBA8Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA8_SNORM]      = WGPUTextureFormat_RGBA8Snorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RGBA8_SINT]       = WGPUTextureFormat_RGBA8Sint;

        m_FmtToWGPUFmtMap[TEX_FORMAT_RG16_TYPELESS] = WGPUTextureFormat_RG16Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG16_FLOAT]    = WGPUTextureFormat_RG16Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG16_UNORM]    = WGPUTextureFormat_RG16Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG16_UINT]     = WGPUTextureFormat_RG16Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG16_SNORM]    = WGPUTextureFormat_RG16Snorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG16_SINT]     = WGPUTextureFormat_RG16Sint;

        m_FmtToWGPUFmtMap[TEX_FORMAT_R32_TYPELESS] = WGPUTextureFormat_R32Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_D32_FLOAT]    = WGPUTextureFormat_Depth32Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R32_FLOAT]    = WGPUTextureFormat_R32Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R32_UINT]     = WGPUTextureFormat_R32Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R32_SINT]     = WGPUTextureFormat_R32Sint;

        m_FmtToWGPUFmtMap[TEX_FORMAT_R24G8_TYPELESS]        = WGPUTextureFormat_Depth24PlusStencil8;
        m_FmtToWGPUFmtMap[TEX_FORMAT_D24_UNORM_S8_UINT]     = WGPUTextureFormat_Depth24PlusStencil8;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R24_UNORM_X8_TYPELESS] = WGPUTextureFormat_Depth24PlusStencil8;
        m_FmtToWGPUFmtMap[TEX_FORMAT_X24_TYPELESS_G8_UINT]  = WGPUTextureFormat_Depth24PlusStencil8;

        m_FmtToWGPUFmtMap[TEX_FORMAT_RG8_TYPELESS] = WGPUTextureFormat_RG8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG8_UNORM]    = WGPUTextureFormat_RG8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG8_UINT]     = WGPUTextureFormat_RG8Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG8_SNORM]    = WGPUTextureFormat_RG8Snorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG8_SINT]     = WGPUTextureFormat_RG8Sint;

        m_FmtToWGPUFmtMap[TEX_FORMAT_R16_TYPELESS] = WGPUTextureFormat_R16Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R16_FLOAT]    = WGPUTextureFormat_R16Float;
        m_FmtToWGPUFmtMap[TEX_FORMAT_D16_UNORM]    = WGPUTextureFormat_Depth16Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R16_UNORM]    = WGPUTextureFormat_R16Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R16_UINT]     = WGPUTextureFormat_R16Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R16_SNORM]    = WGPUTextureFormat_R16Snorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R16_SINT]     = WGPUTextureFormat_R16Sint;

        m_FmtToWGPUFmtMap[TEX_FORMAT_R8_TYPELESS] = WGPUTextureFormat_R8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R8_UNORM]    = WGPUTextureFormat_R8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R8_UINT]     = WGPUTextureFormat_R8Uint;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R8_SNORM]    = WGPUTextureFormat_R8Snorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R8_SINT]     = WGPUTextureFormat_R8Sint;

        m_FmtToWGPUFmtMap[TEX_FORMAT_A8_UNORM] = WGPUTextureFormat_R8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_R1_UNORM] = WGPUTextureFormat_Undefined;

        m_FmtToWGPUFmtMap[TEX_FORMAT_RGB9E5_SHAREDEXP] = WGPUTextureFormat_RGB9E5Ufloat;
        m_FmtToWGPUFmtMap[TEX_FORMAT_RG8_B8G8_UNORM]   = WGPUTextureFormat_Undefined;
        m_FmtToWGPUFmtMap[TEX_FORMAT_G8R8_G8B8_UNORM]  = WGPUTextureFormat_Undefined;

        m_FmtToWGPUFmtMap[TEX_FORMAT_BC1_TYPELESS]   = WGPUTextureFormat_BC1RGBAUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC1_UNORM]      = WGPUTextureFormat_BC1RGBAUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC1_UNORM_SRGB] = WGPUTextureFormat_BC1RGBAUnormSrgb;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC2_TYPELESS]   = WGPUTextureFormat_BC2RGBAUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC2_UNORM]      = WGPUTextureFormat_BC2RGBAUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC2_UNORM_SRGB] = WGPUTextureFormat_BC2RGBAUnormSrgb;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC3_TYPELESS]   = WGPUTextureFormat_BC3RGBAUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC3_UNORM]      = WGPUTextureFormat_BC3RGBAUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC3_UNORM_SRGB] = WGPUTextureFormat_BC3RGBAUnormSrgb;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC4_TYPELESS]   = WGPUTextureFormat_BC4RUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC4_UNORM]      = WGPUTextureFormat_BC4RUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC4_SNORM]      = WGPUTextureFormat_BC4RSnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC5_TYPELESS]   = WGPUTextureFormat_BC5RGUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC5_UNORM]      = WGPUTextureFormat_BC5RGUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC5_SNORM]      = WGPUTextureFormat_BC5RGSnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_B5G6R5_UNORM]   = WGPUTextureFormat_Undefined;
        m_FmtToWGPUFmtMap[TEX_FORMAT_B5G5R5A1_UNORM] = WGPUTextureFormat_Undefined;

        m_FmtToWGPUFmtMap[TEX_FORMAT_BGRA8_UNORM] = WGPUTextureFormat_BGRA8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BGRX8_UNORM] = WGPUTextureFormat_BGRA8Unorm;
     
        m_FmtToWGPUFmtMap[TEX_FORMAT_BGRA8_TYPELESS]   = WGPUTextureFormat_BGRA8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BGRA8_UNORM_SRGB] = WGPUTextureFormat_BGRA8UnormSrgb;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BGRX8_TYPELESS]   = WGPUTextureFormat_BGRA8Unorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BGRX8_UNORM_SRGB] = WGPUTextureFormat_BGRA8UnormSrgb;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC6H_TYPELESS]    = WGPUTextureFormat_BC6HRGBUfloat;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC6H_UF16]        = WGPUTextureFormat_BC6HRGBUfloat;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC6H_SF16]        = WGPUTextureFormat_BC6HRGBFloat;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC7_TYPELESS]     = WGPUTextureFormat_BC7RGBAUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC7_UNORM]        = WGPUTextureFormat_BC7RGBAUnorm;
        m_FmtToWGPUFmtMap[TEX_FORMAT_BC7_UNORM_SRGB]   = WGPUTextureFormat_BC7RGBAUnormSrgb;

        m_FmtToWGPUFmtMap[TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM] = WGPUTextureFormat_Undefined;
        // clang-format on
    }

    WGPUTextureFormat operator[](TEXTURE_FORMAT TexFmt) const
    {
        VERIFY_EXPR(TexFmt < _countof(m_FmtToWGPUFmtMap));
        return m_FmtToWGPUFmtMap[TexFmt];
    }

private:
    WGPUTextureFormat m_FmtToWGPUFmtMap[TEX_FORMAT_NUM_FORMATS] = {};
};

WGPUTextureFormat TextureFormatToWGPUFormat(TEXTURE_FORMAT TexFmt)
{
    const static TexFormatToWebGPUFormatMapper FmtMapper;
    return FmtMapper[TexFmt];
}

class WGPUFormatToTexFormatMapper
{
public:
    WGPUFormatToTexFormatMapper()
    {

        // clang-format off
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_Undefined] = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R8Unorm]   = TEX_FORMAT_R8_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R8Snorm]   = TEX_FORMAT_R8_SNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R8Uint]    = TEX_FORMAT_R8_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R8Sint]    = TEX_FORMAT_R8_SINT;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R16Uint]  = TEX_FORMAT_R16_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R16Sint]  = TEX_FORMAT_R16_SINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R16Float] = TEX_FORMAT_R16_FLOAT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R16Unorm] = TEX_FORMAT_R16_UNORM; 
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R16Snorm] = TEX_FORMAT_R16_SNORM;
        
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG8Unorm] = TEX_FORMAT_RG8_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG8Snorm] = TEX_FORMAT_RG8_SNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG8Uint]  = TEX_FORMAT_RG8_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG8Sint]  = TEX_FORMAT_RG8_SINT;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R32Float] = TEX_FORMAT_R32_FLOAT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R32Uint]  = TEX_FORMAT_R32_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_R32Sint]  = TEX_FORMAT_R32_SINT;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG16Uint]  = TEX_FORMAT_RG16_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG16Sint]  = TEX_FORMAT_RG16_SINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG16Float] = TEX_FORMAT_RG16_FLOAT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG16Unorm] = TEX_FORMAT_RG16_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG16Snorm] = TEX_FORMAT_RG16_SNORM;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA8Unorm]     = TEX_FORMAT_RGBA8_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA8UnormSrgb] = TEX_FORMAT_RGBA8_UNORM_SRGB;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA8Snorm]     = TEX_FORMAT_RGBA8_SNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA8Uint]      = TEX_FORMAT_RGBA8_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA8Sint]      = TEX_FORMAT_RGBA8_SINT;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BGRA8Unorm]     = TEX_FORMAT_BGRA8_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BGRA8UnormSrgb] = TEX_FORMAT_BGRA8_UNORM_SRGB;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGB10A2Uint]   = TEX_FORMAT_RGB10A2_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGB10A2Unorm]  = TEX_FORMAT_RGB10A2_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG11B10Ufloat] = TEX_FORMAT_R11G11B10_FLOAT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGB9E5Ufloat]  = TEX_FORMAT_RGB9E5_SHAREDEXP;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG32Float] = TEX_FORMAT_RG32_FLOAT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG32Uint]  = TEX_FORMAT_RG32_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RG32Sint]  = TEX_FORMAT_RG32_SINT;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA16Uint]  = TEX_FORMAT_RGBA16_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA16Sint]  = TEX_FORMAT_RGBA16_SINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA16Float] = TEX_FORMAT_RGBA16_FLOAT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA16Unorm] = TEX_FORMAT_RGBA16_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA16Snorm] = TEX_FORMAT_RGBA16_SNORM;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA32Float] = TEX_FORMAT_RGBA32_FLOAT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA32Uint]  = TEX_FORMAT_RGBA32_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_RGBA32Sint]  = TEX_FORMAT_RGBA32_SINT;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_Stencil8]             = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_Depth16Unorm]         = TEX_FORMAT_D16_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_Depth24Plus]          = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_Depth24PlusStencil8]  = TEX_FORMAT_D24_UNORM_S8_UINT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_Depth32Float]         = TEX_FORMAT_D32_FLOAT;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_Depth32FloatStencil8] = TEX_FORMAT_D32_FLOAT_S8X24_UINT;
    
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC1RGBAUnorm]     = TEX_FORMAT_BC1_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC1RGBAUnormSrgb] = TEX_FORMAT_BC1_UNORM_SRGB;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC2RGBAUnorm]     = TEX_FORMAT_BC2_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC2RGBAUnormSrgb] = TEX_FORMAT_BC2_UNORM_SRGB;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC3RGBAUnorm]     = TEX_FORMAT_BC3_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC3RGBAUnormSrgb] = TEX_FORMAT_BC3_UNORM_SRGB;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC4RUnorm]        = TEX_FORMAT_BC4_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC4RSnorm]        = TEX_FORMAT_BC4_SNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC5RGUnorm]       = TEX_FORMAT_BC5_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC5RGSnorm]       = TEX_FORMAT_BC5_SNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC6HRGBUfloat]    = TEX_FORMAT_BC6H_UF16;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC6HRGBFloat]     = TEX_FORMAT_BC6H_SF16;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC7RGBAUnorm]     = TEX_FORMAT_BC7_UNORM;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_BC7RGBAUnormSrgb] = TEX_FORMAT_BC7_UNORM_SRGB;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ETC2RGB8Unorm]       = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ETC2RGB8UnormSrgb]   = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ETC2RGB8A1Unorm]     = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ETC2RGB8A1UnormSrgb] = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ETC2RGBA8Unorm]      = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ETC2RGBA8UnormSrgb]  = TEX_FORMAT_UNKNOWN;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_EACR11Unorm]  = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_EACR11Snorm]  = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_EACRG11Unorm] = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_EACRG11Snorm] = TEX_FORMAT_UNKNOWN;

        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC4x4Unorm]       = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC4x4UnormSrgb]   = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC5x4Unorm]       = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC5x4UnormSrgb]   = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC5x5Unorm]       = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC5x5UnormSrgb]   = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC6x5Unorm]       = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC6x5UnormSrgb]   = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC6x6Unorm]       = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC6x6UnormSrgb]   = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC8x5Unorm]       = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC8x5UnormSrgb]   = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC8x6Unorm]       = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC8x6UnormSrgb]   = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC8x8Unorm]       = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC8x8UnormSrgb]   = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC10x5Unorm]      = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC10x5UnormSrgb]  = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC10x6Unorm]      = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC10x6UnormSrgb]  = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC10x8Unorm]      = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC10x8UnormSrgb]  = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC10x10Unorm]     = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC10x10UnormSrgb] = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC12x10Unorm]     = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC12x10UnormSrgb] = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC12x12Unorm]     = TEX_FORMAT_UNKNOWN;
        m_WGPUFmtToTexFmtMap[WGPUTextureFormat_ASTC12x12UnormSrgb] = TEX_FORMAT_UNKNOWN;
        // clang-format on
    }

    TEXTURE_FORMAT operator[](WGPUTextureFormat WGPUFmt) const
    {
        if (WGPUFmt < WEBGPU_FORMAT_RANGE_SIZE)
        {
            return m_WGPUFmtToTexFmtMap[WGPUFmt];
        }
        else
        {
            auto Iter = m_WGPUFmtToTexFmtMapExt.find(WGPUFmt);
            return Iter != m_WGPUFmtToTexFmtMapExt.end() ? Iter->second : TEX_FORMAT_UNKNOWN;
        }
    }

private:
    TEXTURE_FORMAT                                        m_WGPUFmtToTexFmtMap[WEBGPU_FORMAT_RANGE_SIZE] = {};
    std::unordered_map<WGPUTextureFormat, TEXTURE_FORMAT> m_WGPUFmtToTexFmtMapExt;
};

TEXTURE_FORMAT WGPUFormatToTextureFormat(WGPUTextureFormat TexFmt)
{
    const static WGPUFormatToTexFormatMapper FmtMapper;
    return FmtMapper[TexFmt];
}

WGPUTextureViewDimension ResourceDimensionToWGPUTextureViewDimension(RESOURCE_DIMENSION ResourceDim)
{
    switch (ResourceDim)
    {
            // clang-format off
        case RESOURCE_DIM_TEX_1D:         return WGPUTextureViewDimension_1D;
        case RESOURCE_DIM_TEX_2D:         return WGPUTextureViewDimension_2D;
        case RESOURCE_DIM_TEX_2D_ARRAY:   return WGPUTextureViewDimension_2DArray;
        case RESOURCE_DIM_TEX_3D:         return WGPUTextureViewDimension_3D;
        case RESOURCE_DIM_TEX_CUBE:       return WGPUTextureViewDimension_Cube;
        case RESOURCE_DIM_TEX_CUBE_ARRAY: return WGPUTextureViewDimension_CubeArray;
            // clang-format on
        default:
            UNEXPECTED("Unexpected resource dimension");
            return WGPUTextureViewDimension_Undefined;
    }
    static_assert(RESOURCE_DIM_NUM_DIMENSIONS == 9, "Please update the switch above to handle the new resource dimension");
}

WGPUAddressMode TexAddressModeToWGPUAddressMode(TEXTURE_ADDRESS_MODE Mode)
{
    switch (Mode)
    {
        case TEXTURE_ADDRESS_UNKNOWN:
            UNEXPECTED("Unknown address mode");
            return WGPUAddressMode_ClampToEdge;

            // clang-format off
        case TEXTURE_ADDRESS_WRAP:   return WGPUAddressMode_Repeat;
        case TEXTURE_ADDRESS_MIRROR: return WGPUAddressMode_MirrorRepeat;
        case TEXTURE_ADDRESS_CLAMP:  return WGPUAddressMode_ClampToEdge;
            // clang-format on

        case TEXTURE_ADDRESS_BORDER:
            UNSUPPORTED("WebGPU does not support border address mode");
            return WGPUAddressMode_ClampToEdge;

        default:
            UNEXPECTED("Unexpected texture address mode");
            return WGPUAddressMode_ClampToEdge;
    }
}

WGPUFilterMode FilterTypeToWGPUFilterMode(FILTER_TYPE FilterType)
{
    switch (FilterType)
    {
        case FILTER_TYPE_UNKNOWN:
            UNEXPECTED("Unknown filter type");
            return WGPUFilterMode_Nearest;

        case FILTER_TYPE_POINT:
        case FILTER_TYPE_COMPARISON_POINT:
        case FILTER_TYPE_MINIMUM_POINT:
        case FILTER_TYPE_MAXIMUM_POINT:
            return WGPUFilterMode_Nearest;

        case FILTER_TYPE_LINEAR:
        case FILTER_TYPE_ANISOTROPIC:
        case FILTER_TYPE_COMPARISON_LINEAR:
        case FILTER_TYPE_COMPARISON_ANISOTROPIC:
        case FILTER_TYPE_MINIMUM_LINEAR:
        case FILTER_TYPE_MINIMUM_ANISOTROPIC:
        case FILTER_TYPE_MAXIMUM_LINEAR:
        case FILTER_TYPE_MAXIMUM_ANISOTROPIC:
            return WGPUFilterMode_Linear;

        default:
            UNEXPECTED("Unexpected filter type");
            return WGPUFilterMode_Nearest;
    }
}

WGPUMipmapFilterMode FilterTypeToWGPUMipMapMode(FILTER_TYPE FilterType)
{
    switch (FilterType)
    {
        case FILTER_TYPE_UNKNOWN:
            UNEXPECTED("Unknown filter type");
            return WGPUMipmapFilterMode_Nearest;

        case FILTER_TYPE_POINT:
        case FILTER_TYPE_COMPARISON_POINT:
        case FILTER_TYPE_MINIMUM_POINT:
        case FILTER_TYPE_MAXIMUM_POINT:
            return WGPUMipmapFilterMode_Nearest;

        case FILTER_TYPE_LINEAR:
        case FILTER_TYPE_ANISOTROPIC:
        case FILTER_TYPE_COMPARISON_LINEAR:
        case FILTER_TYPE_COMPARISON_ANISOTROPIC:
        case FILTER_TYPE_MINIMUM_LINEAR:
        case FILTER_TYPE_MINIMUM_ANISOTROPIC:
        case FILTER_TYPE_MAXIMUM_LINEAR:
        case FILTER_TYPE_MAXIMUM_ANISOTROPIC:
            return WGPUMipmapFilterMode_Linear;

        default:
            UNEXPECTED("Only point and linear filter types are allowed for mipmap mode");
            return WGPUMipmapFilterMode_Nearest;
    }
}

WGPUCompareFunction ComparisonFuncToWGPUCompareFunction(COMPARISON_FUNCTION CmpFunc)
{
    switch (CmpFunc)
    {
        case COMPARISON_FUNC_UNKNOWN:
            UNEXPECTED("Comparison function is not specified");
            return WGPUCompareFunction_Always;

            // clang-format off
        case COMPARISON_FUNC_NEVER:         return WGPUCompareFunction_Never;
        case COMPARISON_FUNC_LESS:          return WGPUCompareFunction_Less;
        case COMPARISON_FUNC_EQUAL:         return WGPUCompareFunction_Equal;
        case COMPARISON_FUNC_LESS_EQUAL:    return WGPUCompareFunction_LessEqual;
        case COMPARISON_FUNC_GREATER:       return WGPUCompareFunction_Greater;
        case COMPARISON_FUNC_NOT_EQUAL:     return WGPUCompareFunction_NotEqual;
        case COMPARISON_FUNC_GREATER_EQUAL: return WGPUCompareFunction_GreaterEqual;
        case COMPARISON_FUNC_ALWAYS:        return WGPUCompareFunction_Always;
            // clang-format on

        default:
            UNEXPECTED("Unknown comparison function");
            return WGPUCompareFunction_Always;
    }
}

WGPUStencilOperation StencilOpToWGPUStencilOperation(STENCIL_OP StencilOp)
{
    switch (StencilOp)
    {
        case STENCIL_OP_UNDEFINED:
            UNEXPECTED("Undefined stencil operation");
            return WGPUStencilOperation_Keep;

            // clang-format off
        case STENCIL_OP_KEEP:           return WGPUStencilOperation_Keep;
        case STENCIL_OP_ZERO:           return WGPUStencilOperation_Zero;
        case STENCIL_OP_REPLACE:        return WGPUStencilOperation_Replace;
        case STENCIL_OP_INCR_SAT:       return WGPUStencilOperation_IncrementClamp;
        case STENCIL_OP_DECR_SAT:       return WGPUStencilOperation_DecrementClamp;
        case STENCIL_OP_INVERT:         return WGPUStencilOperation_Invert;
        case STENCIL_OP_INCR_WRAP:      return WGPUStencilOperation_IncrementWrap;
        case STENCIL_OP_DECR_WRAP:      return WGPUStencilOperation_DecrementWrap;
            // clang-format on

        default:
            UNEXPECTED("Unknown stencil operation");
            return WGPUStencilOperation_Keep;
    }
}

WGPUVertexFormat VertexFormatAttribsToWGPUVertexFormat(VALUE_TYPE ValueType, Uint32 NumComponents, bool IsNormalized)
{
    switch (ValueType)
    {
        case VT_FLOAT16:
        {
            VERIFY(!IsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 2: return WGPUVertexFormat_Float16x2;
                case 4: return WGPUVertexFormat_Float16x4;
                default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
            }
        }
        case VT_FLOAT32:
        {
            VERIFY(!IsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 1: return WGPUVertexFormat_Float32;
                case 2: return WGPUVertexFormat_Float32x2;
                case 3: return WGPUVertexFormat_Float32x3;
                case 4: return WGPUVertexFormat_Float32x4;
                default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
            }
        }
        case VT_INT32:
        {
            VERIFY(!IsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return WGPUVertexFormat_Sint32;
                case 2: return WGPUVertexFormat_Sint32x2;
                case 3: return WGPUVertexFormat_Sint32x3;
                case 4: return WGPUVertexFormat_Sint32x4;
                default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
            }
        }
        case VT_UINT32:
        {
            VERIFY(!IsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return WGPUVertexFormat_Uint32;
                case 2: return WGPUVertexFormat_Uint32x2;
                case 3: return WGPUVertexFormat_Uint32x3;
                case 4: return WGPUVertexFormat_Uint32x4;
                default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
            }
        }
        case VT_INT16:
        {
            if (IsNormalized)
            {
                switch (NumComponents)
                {
                    case 2: return WGPUVertexFormat_Snorm16x2;
                    case 4: return WGPUVertexFormat_Snorm16x4;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 2: return WGPUVertexFormat_Sint16x2;
                    case 4: return WGPUVertexFormat_Sint16x4;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
                }
            }
        }
        case VT_UINT16:
        {
            if (IsNormalized)
            {
                switch (NumComponents)
                {
                    case 2: return WGPUVertexFormat_Unorm16x2;
                    case 4: return WGPUVertexFormat_Unorm16x4;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 2: return WGPUVertexFormat_Uint16x2;
                    case 4: return WGPUVertexFormat_Uint16x4;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
                }
            }
        }
        case VT_INT8:
        {
            if (IsNormalized)
            {
                switch (NumComponents)
                {
                    case 2: return WGPUVertexFormat_Snorm8x2;
                    case 4: return WGPUVertexFormat_Snorm8x4;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 2: return WGPUVertexFormat_Sint8x2;
                    case 4: return WGPUVertexFormat_Sint8x4;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
                }
            }
        }
        case VT_UINT8:
        {
            if (IsNormalized)
            {
                switch (NumComponents)
                {
                    case 2: return WGPUVertexFormat_Unorm8x2;
                    case 4: return WGPUVertexFormat_Unorm8x4;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 2: return WGPUVertexFormat_Uint8x2;
                    case 4: return WGPUVertexFormat_Uint8x4;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUVertexFormat_Force32;
                }
            }
        }

        default: UNEXPECTED("Unusupported format"); return WGPUVertexFormat_Force32;
    }
}

WGPUIndexFormat IndexTypeToWGPUIndexFormat(VALUE_TYPE ValueType)
{
    switch (ValueType)
    {
        case VT_UINT16: return WGPUIndexFormat_Uint16;
        case VT_UINT32: return WGPUIndexFormat_Uint32;
        default: UNEXPECTED("Unsupported index type"); return WGPUIndexFormat_Undefined;
    }
}

WGPUTextureFormat BufferFormatToWGPUTextureFormat(const BufferFormat& BuffFmt)
{
    switch (BuffFmt.ValueType)
    {
        case VT_FLOAT16:
        {
            VERIFY(!BuffFmt.IsNormalized, "Floating point formats cannot be normalized");
            switch (BuffFmt.NumComponents)
            {
                case 1: return WGPUTextureFormat_R16Float;
                case 2: return WGPUTextureFormat_RG16Float;
                case 4: return WGPUTextureFormat_RGBA16Float;
                default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
            }
        }
        case VT_FLOAT32:
        {
            VERIFY(!BuffFmt.IsNormalized, "Floating point formats cannot be normalized");
            switch (BuffFmt.NumComponents)
            {
                case 1: return WGPUTextureFormat_R32Float;
                case 2: return WGPUTextureFormat_RG32Float;
                case 4: return WGPUTextureFormat_RGBA32Float;
                default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
            }
        }
        case VT_INT32:
        {
            VERIFY(!BuffFmt.IsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (BuffFmt.NumComponents)
            {
                case 1: return WGPUTextureFormat_R32Sint;
                case 2: return WGPUTextureFormat_RG32Sint;
                case 4: return WGPUTextureFormat_RGBA32Sint;
                default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
            }
        }
        case VT_UINT32:
        {
            VERIFY(!BuffFmt.IsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (BuffFmt.NumComponents)
            {
                case 1: return WGPUTextureFormat_R32Uint;
                case 2: return WGPUTextureFormat_RG32Uint;
                case 4: return WGPUTextureFormat_RGBA32Uint;
                default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
            }
        }
        case VT_INT16:
        {

            VERIFY(!BuffFmt.IsNormalized, "16-bit UNORM formats are not supported. Use R16_FLOAT instead");
            switch (BuffFmt.NumComponents)
            {
                case 1: return WGPUTextureFormat_R16Sint;
                case 2: return WGPUTextureFormat_RG16Sint;
                case 4: return WGPUTextureFormat_RGBA16Sint;
                default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
            }
        }
        case VT_UINT16:
        {
            VERIFY(!BuffFmt.IsNormalized, "16-bit UNORM formats are not supported. Use R16_FLOAT instead");
            switch (BuffFmt.NumComponents)
            {
                case 1: return WGPUTextureFormat_R16Uint;
                case 2: return WGPUTextureFormat_RG16Uint;
                case 4: return WGPUTextureFormat_RGBA16Uint;
                default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
            }
        }
        case VT_INT8:
        {
            if (BuffFmt.IsNormalized)
            {
                switch (BuffFmt.NumComponents)
                {
                    case 1: return WGPUTextureFormat_R8Snorm;
                    case 2: return WGPUTextureFormat_RG8Snorm;
                    case 4: return WGPUTextureFormat_RGBA8Snorm;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
                }
            }
            else
            {
                switch (BuffFmt.NumComponents)
                {
                    case 1: return WGPUTextureFormat_R8Sint;
                    case 2: return WGPUTextureFormat_RG8Sint;
                    case 4: return WGPUTextureFormat_RGBA8Sint;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
                }
            }
        }
        case VT_UINT8:
        {
            if (BuffFmt.IsNormalized)
            {
                switch (BuffFmt.NumComponents)
                {
                    case 1: return WGPUTextureFormat_R8Unorm;
                    case 2: return WGPUTextureFormat_RG8Unorm;
                    case 4: return WGPUTextureFormat_RGBA8Unorm;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
                }
            }
            else
            {
                switch (BuffFmt.NumComponents)
                {
                    case 1: return WGPUTextureFormat_R8Uint;
                    case 2: return WGPUTextureFormat_RG8Uint;
                    case 4: return WGPUTextureFormat_RGBA8Uint;
                    default: UNEXPECTED("Unusupported number of components"); return WGPUTextureFormat_Undefined;
                }
            }
        }
        default: UNEXPECTED("Unusupported format"); return WGPUTextureFormat_Undefined;
    }
}

WGPUQueryType QueryTypeToWGPUQueryType(QUERY_TYPE QueryType)
{
    static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled");
    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:
            return WGPUQueryType_Occlusion;
        case QUERY_TYPE_BINARY_OCCLUSION:
            return WGPUQueryType_Occlusion;
        case QUERY_TYPE_DURATION:
        case QUERY_TYPE_TIMESTAMP:
            return WGPUQueryType_Timestamp;
        case QUERY_TYPE_PIPELINE_STATISTICS:
            UNEXPECTED("Pipeline statistics queries aren't supported in WebGPU");
            return WGPUQueryType_Occlusion;
        default:
            UNEXPECTED("Unexpected query type");
            return WGPUQueryType_Occlusion;
    }
}

WGPUColorWriteMaskFlags ColorMaskToWGPUColorWriteMask(COLOR_MASK ColorMask)
{
    // clang-format off
    return ((ColorMask & COLOR_MASK_RED)   ? WGPUColorWriteMask_Red   : WGPUColorWriteMask_None) |
           ((ColorMask & COLOR_MASK_GREEN) ? WGPUColorWriteMask_Green : WGPUColorWriteMask_None) |
           ((ColorMask & COLOR_MASK_BLUE)  ? WGPUColorWriteMask_Blue  : WGPUColorWriteMask_None) |
           ((ColorMask & COLOR_MASK_ALPHA) ? WGPUColorWriteMask_Alpha : WGPUColorWriteMask_None);
    // clang-format on
}

WGPULoadOp AttachmentLoadOpToWGPULoadOp(ATTACHMENT_LOAD_OP Operation)
{
    switch (Operation)
    {
        case ATTACHMENT_LOAD_OP_LOAD:
            return WGPULoadOp_Load;
        case ATTACHMENT_LOAD_OP_CLEAR:
            return WGPULoadOp_Clear;
        case ATTACHMENT_LOAD_OP_DISCARD:
            return WGPULoadOp_Clear; // https://www.w3.org/TR/webgpu/ 17.1.1.3. Load & Store Operations
        default:
            UNEXPECTED("Unexpected attachment load operation");
            return WGPULoadOp_Load;
    }
}

WGPUStoreOp AttachmentStoreOpToWGPUStoreOp(ATTACHMENT_STORE_OP Operation)
{
    switch (Operation)
    {
        case ATTACHMENT_STORE_OP_STORE:
            return WGPUStoreOp_Store;
        case ATTACHMENT_STORE_OP_DISCARD:
            return WGPUStoreOp_Discard;
        default:
            UNEXPECTED("Unexpected attachment store operation");
            return WGPUStoreOp_Discard;
    }
}

WGPUBlendFactor BlendFactorToWGPUBlendFactor(BLEND_FACTOR BlendFactor)
{
    switch (BlendFactor)
    {
            // clang-format off
        case BLEND_FACTOR_ZERO:             return WGPUBlendFactor_Zero;
        case BLEND_FACTOR_ONE:              return WGPUBlendFactor_One;
        case BLEND_FACTOR_SRC_COLOR:        return WGPUBlendFactor_Src;
        case BLEND_FACTOR_INV_SRC_COLOR:    return WGPUBlendFactor_OneMinusSrc;
        case BLEND_FACTOR_SRC_ALPHA:        return WGPUBlendFactor_SrcAlpha;
        case BLEND_FACTOR_INV_SRC_ALPHA:    return WGPUBlendFactor_OneMinusSrcAlpha;
        case BLEND_FACTOR_DEST_ALPHA:       return WGPUBlendFactor_DstAlpha;
        case BLEND_FACTOR_INV_DEST_ALPHA:   return WGPUBlendFactor_OneMinusDstAlpha;
        case BLEND_FACTOR_DEST_COLOR:       return WGPUBlendFactor_Dst;
        case BLEND_FACTOR_INV_DEST_COLOR:   return WGPUBlendFactor_OneMinusSrc;
        case BLEND_FACTOR_SRC_ALPHA_SAT:    return WGPUBlendFactor_SrcAlphaSaturated;
        case BLEND_FACTOR_BLEND_FACTOR:     return WGPUBlendFactor_Constant;
        case BLEND_FACTOR_INV_BLEND_FACTOR: return WGPUBlendFactor_OneMinusConstant;
            // clang-format on
        case BLEND_FACTOR_SRC1_COLOR:
        case BLEND_FACTOR_INV_SRC1_COLOR:
        case BLEND_FACTOR_SRC1_ALPHA:
        case BLEND_FACTOR_INV_SRC1_ALPHA:
            UNEXPECTED("Dual-source blending WebGPU is not supported in WebGPU");
            return WGPUBlendFactor_Zero;
        default:
            UNEXPECTED("Unexpected blend factor");
            return WGPUBlendFactor_Zero;
    }
}

WGPUBlendOperation BlendOpToWGPUBlendOperation(BLEND_OPERATION BlendOp)
{
    switch (BlendOp)
    {
            // clang-format off
        case BLEND_OPERATION_ADD:          return WGPUBlendOperation_Add;
        case BLEND_OPERATION_SUBTRACT:     return WGPUBlendOperation_Subtract;
        case BLEND_OPERATION_REV_SUBTRACT: return WGPUBlendOperation_ReverseSubtract;
        case BLEND_OPERATION_MIN:          return WGPUBlendOperation_Min;
        case BLEND_OPERATION_MAX:          return WGPUBlendOperation_Max;
            // clang-format on
        default:
            UNEXPECTED("Unexpected blend operation");
            return WGPUBlendOperation_Add;
    }
}

WGPUPrimitiveTopology PrimitiveTopologyWGPUPrimitiveType(PRIMITIVE_TOPOLOGY PrimitiveType)
{
    switch (PrimitiveType)
    {
        case PRIMITIVE_TOPOLOGY_UNDEFINED:
            UNEXPECTED("Undefined primitive topology");
            return WGPUPrimitiveTopology_PointList;
            // clang-format off
        case PRIMITIVE_TOPOLOGY_POINT_LIST:     return WGPUPrimitiveTopology_PointList;
        case PRIMITIVE_TOPOLOGY_LINE_LIST:      return WGPUPrimitiveTopology_LineList;
        case PRIMITIVE_TOPOLOGY_LINE_STRIP:     return WGPUPrimitiveTopology_LineStrip;
        case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:  return WGPUPrimitiveTopology_TriangleList;
        case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return WGPUPrimitiveTopology_TriangleStrip;
            // clang-format on
        case PRIMITIVE_TOPOLOGY_LINE_LIST_ADJ:
        case PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJ:
        case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_ADJ:
        case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJ:
            UNEXPECTED("Primitive topologies with adjacency are not supported in WebGPU");
            return WGPUPrimitiveTopology_PointList;
        default:
            UNEXPECTED("Unexpected primitive topology");
            return WGPUPrimitiveTopology_PointList;
    }
}

WGPUCullMode CullModeToWGPUCullMode(CULL_MODE CullMode)
{
    switch (CullMode)
    {
        case CULL_MODE_NONE:
            return WGPUCullMode_None;
        case CULL_MODE_FRONT:
            return WGPUCullMode_Front;
        case CULL_MODE_BACK:
            return WGPUCullMode_Back;
        default:
            UNEXPECTED("Unexpected cull mode");
            return WGPUCullMode_None;
    }
}

WGPUShaderStageFlags ShaderTypeToToWGPUShaderStageFlag(SHADER_TYPE Type)
{
    switch (Type)
    {
        case SHADER_TYPE_VERTEX: return WGPUShaderStage_Vertex;
        case SHADER_TYPE_PIXEL: return WGPUShaderStage_Fragment;
        case SHADER_TYPE_COMPUTE: return WGPUShaderStage_Compute;

        default:
            UNSUPPORTED("Unsupported shader type");
            return WGPUShaderStage_None;
    }
}

WGPUShaderStageFlags ShaderStagesToWGPUShaderStageFlags(SHADER_TYPE Stages)
{
    WGPUShaderStageFlags Flags = WGPUShaderStage_None;
    while (Stages != 0)
    {
        SHADER_TYPE ShaderType = ExtractLSB(Stages);
        Flags |= ShaderTypeToToWGPUShaderStageFlag(ShaderType);
        Stages &= ~ShaderType;
    }
    return Flags;
}

WGPUVertexStepMode InputElementFrequencyToWGPUVertexStepMode(INPUT_ELEMENT_FREQUENCY StepRate)
{
    switch (StepRate)
    {
        case INPUT_ELEMENT_FREQUENCY_PER_VERTEX:
            return WGPUVertexStepMode_Vertex;
        case INPUT_ELEMENT_FREQUENCY_PER_INSTANCE:
            return WGPUVertexStepMode_Instance;
        default:
            UNEXPECTED("Unexpected input element frequency");
            return WGPUVertexStepMode_Vertex;
    }
}

} // namespace Diligent
