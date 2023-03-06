/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
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

#include "VulkanTypeConversions.hpp"

#include <unordered_map>
#include <array>

#include "PlatformMisc.hpp"
#include "Align.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

class TexFormatToVkFormatMapper
{
public:
    TexFormatToVkFormatMapper()
    {
        // clang-format off
        m_FmtToVkFmtMap[TEX_FORMAT_UNKNOWN] = VK_FORMAT_UNDEFINED;

        m_FmtToVkFmtMap[TEX_FORMAT_RGBA32_TYPELESS] = VK_FORMAT_R32G32B32A32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA32_FLOAT]    = VK_FORMAT_R32G32B32A32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA32_UINT]     = VK_FORMAT_R32G32B32A32_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA32_SINT]     = VK_FORMAT_R32G32B32A32_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_RGB32_TYPELESS] = VK_FORMAT_R32G32B32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB32_FLOAT]    = VK_FORMAT_R32G32B32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB32_UINT]     = VK_FORMAT_R32G32B32_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB32_SINT]     = VK_FORMAT_R32G32B32_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_TYPELESS] = VK_FORMAT_R16G16B16A16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_FLOAT]    = VK_FORMAT_R16G16B16A16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_UNORM]    = VK_FORMAT_R16G16B16A16_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_UINT]     = VK_FORMAT_R16G16B16A16_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_SNORM]    = VK_FORMAT_R16G16B16A16_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA16_SINT]     = VK_FORMAT_R16G16B16A16_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_RG32_TYPELESS] = VK_FORMAT_R32G32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG32_FLOAT]    = VK_FORMAT_R32G32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG32_UINT]     = VK_FORMAT_R32G32_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG32_SINT]     = VK_FORMAT_R32G32_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R32G8X24_TYPELESS]        = VK_FORMAT_D32_SFLOAT_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_D32_FLOAT_S8X24_UINT]     = VK_FORMAT_D32_SFLOAT_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS] = VK_FORMAT_D32_SFLOAT_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_X32_TYPELESS_G8X24_UINT]  = VK_FORMAT_UNDEFINED;

        m_FmtToVkFmtMap[TEX_FORMAT_RGB10A2_TYPELESS] = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB10A2_UNORM]    = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        m_FmtToVkFmtMap[TEX_FORMAT_RGB10A2_UINT]     = VK_FORMAT_A2R10G10B10_UINT_PACK32;
        m_FmtToVkFmtMap[TEX_FORMAT_R11G11B10_FLOAT]  = VK_FORMAT_B10G11R11_UFLOAT_PACK32;

        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_TYPELESS]   = VK_FORMAT_R8G8B8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_UNORM]      = VK_FORMAT_R8G8B8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_UNORM_SRGB] = VK_FORMAT_R8G8B8A8_SRGB;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_UINT]       = VK_FORMAT_R8G8B8A8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_SNORM]      = VK_FORMAT_R8G8B8A8_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RGBA8_SINT]       = VK_FORMAT_R8G8B8A8_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_RG16_TYPELESS] = VK_FORMAT_R16G16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_FLOAT]    = VK_FORMAT_R16G16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_UNORM]    = VK_FORMAT_R16G16_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_UINT]     = VK_FORMAT_R16G16_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_SNORM]    = VK_FORMAT_R16G16_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG16_SINT]     = VK_FORMAT_R16G16_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R32_TYPELESS] = VK_FORMAT_R32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_D32_FLOAT]    = VK_FORMAT_D32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_R32_FLOAT]    = VK_FORMAT_R32_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_R32_UINT]     = VK_FORMAT_R32_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R32_SINT]     = VK_FORMAT_R32_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R24G8_TYPELESS]        = VK_FORMAT_D24_UNORM_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_D24_UNORM_S8_UINT]     = VK_FORMAT_D24_UNORM_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R24_UNORM_X8_TYPELESS] = VK_FORMAT_D24_UNORM_S8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_X24_TYPELESS_G8_UINT]  = VK_FORMAT_UNDEFINED;

        m_FmtToVkFmtMap[TEX_FORMAT_RG8_TYPELESS] = VK_FORMAT_R8G8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_UNORM]    = VK_FORMAT_R8G8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_UINT]     = VK_FORMAT_R8G8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_SNORM]    = VK_FORMAT_R8G8_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_SINT]     = VK_FORMAT_R8G8_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R16_TYPELESS] = VK_FORMAT_R16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_FLOAT]    = VK_FORMAT_R16_SFLOAT;
        m_FmtToVkFmtMap[TEX_FORMAT_D16_UNORM]    = VK_FORMAT_D16_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_UNORM]    = VK_FORMAT_R16_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_UINT]     = VK_FORMAT_R16_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_SNORM]    = VK_FORMAT_R16_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R16_SINT]     = VK_FORMAT_R16_SINT;

        m_FmtToVkFmtMap[TEX_FORMAT_R8_TYPELESS] = VK_FORMAT_R8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R8_UNORM]    = VK_FORMAT_R8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R8_UINT]     = VK_FORMAT_R8_UINT;
        m_FmtToVkFmtMap[TEX_FORMAT_R8_SNORM]    = VK_FORMAT_R8_SNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R8_SINT]     = VK_FORMAT_R8_SINT;
        // To get the same behaviour as expected for TEX_FORMAT_A8_UNORM we
        // swizzle the components appropriately using the VkImageViewCreateInfo struct
        m_FmtToVkFmtMap[TEX_FORMAT_A8_UNORM]    = VK_FORMAT_R8_UNORM;

        m_FmtToVkFmtMap[TEX_FORMAT_R1_UNORM]    = VK_FORMAT_UNDEFINED;

        m_FmtToVkFmtMap[TEX_FORMAT_RGB9E5_SHAREDEXP] = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
        m_FmtToVkFmtMap[TEX_FORMAT_RG8_B8G8_UNORM]   = VK_FORMAT_UNDEFINED;
        m_FmtToVkFmtMap[TEX_FORMAT_G8R8_G8B8_UNORM]  = VK_FORMAT_UNDEFINED;

        // http://www.g-truc.net/post-0335.html
        // http://renderingpipeline.com/2012/07/texture-compression/
        m_FmtToVkFmtMap[TEX_FORMAT_BC1_TYPELESS]   = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC1_UNORM]      = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC1_UNORM_SRGB] = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC2_TYPELESS]   = VK_FORMAT_BC2_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC2_UNORM]      = VK_FORMAT_BC2_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC2_UNORM_SRGB] = VK_FORMAT_BC2_SRGB_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC3_TYPELESS]   = VK_FORMAT_BC3_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC3_UNORM]      = VK_FORMAT_BC3_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC3_UNORM_SRGB] = VK_FORMAT_BC3_SRGB_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC4_TYPELESS]   = VK_FORMAT_BC4_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC4_UNORM]      = VK_FORMAT_BC4_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC4_SNORM]      = VK_FORMAT_BC4_SNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC5_TYPELESS]   = VK_FORMAT_BC5_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC5_UNORM]      = VK_FORMAT_BC5_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC5_SNORM]      = VK_FORMAT_BC5_SNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_B5G6R5_UNORM]   = VK_FORMAT_B5G6R5_UNORM_PACK16;
        m_FmtToVkFmtMap[TEX_FORMAT_B5G5R5A1_UNORM] = VK_FORMAT_B5G5R5A1_UNORM_PACK16;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRA8_UNORM]    = VK_FORMAT_B8G8R8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRX8_UNORM]    = VK_FORMAT_B8G8R8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM] = VK_FORMAT_UNDEFINED;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRA8_TYPELESS]   = VK_FORMAT_B8G8R8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRA8_UNORM_SRGB] = VK_FORMAT_B8G8R8A8_SRGB;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRX8_TYPELESS]   = VK_FORMAT_B8G8R8A8_UNORM;
        m_FmtToVkFmtMap[TEX_FORMAT_BGRX8_UNORM_SRGB] = VK_FORMAT_B8G8R8A8_SRGB;
        m_FmtToVkFmtMap[TEX_FORMAT_BC6H_TYPELESS] = VK_FORMAT_BC6H_UFLOAT_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC6H_UF16]     = VK_FORMAT_BC6H_UFLOAT_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC6H_SF16]     = VK_FORMAT_BC6H_SFLOAT_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC7_TYPELESS]   = VK_FORMAT_BC7_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC7_UNORM]      = VK_FORMAT_BC7_UNORM_BLOCK;
        m_FmtToVkFmtMap[TEX_FORMAT_BC7_UNORM_SRGB] = VK_FORMAT_BC7_SRGB_BLOCK;
        // clang-format on
    }

    VkFormat operator[](TEXTURE_FORMAT TexFmt) const
    {
        VERIFY_EXPR(TexFmt < _countof(m_FmtToVkFmtMap));
        return m_FmtToVkFmtMap[TexFmt];
    }

private:
    VkFormat m_FmtToVkFmtMap[TEX_FORMAT_NUM_FORMATS] = {};
};

VkFormat TexFormatToVkFormat(TEXTURE_FORMAT TexFmt)
{
    static const TexFormatToVkFormatMapper FmtMapper;
    return FmtMapper[TexFmt];
}



class VkFormatToTexFormatMapper
{
public:
    VkFormatToTexFormatMapper()
    {
        // clang-format off
        m_VkFmtToTexFmtMap[VK_FORMAT_UNDEFINED]                 = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R4G4_UNORM_PACK8]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R4G4B4A4_UNORM_PACK16]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B4G4R4A4_UNORM_PACK16]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R5G6B5_UNORM_PACK16]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B5G6R5_UNORM_PACK16]       = TEX_FORMAT_B5G6R5_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R5G5B5A1_UNORM_PACK16]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B5G5R5A1_UNORM_PACK16]     = TEX_FORMAT_B5G5R5A1_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_A1R5G5B5_UNORM_PACK16]     = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R8_UNORM]                  = TEX_FORMAT_R8_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_SNORM]                  = TEX_FORMAT_R8_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_USCALED]                = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_SSCALED]                = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_UINT]                   = TEX_FORMAT_R8_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_SINT]                   = TEX_FORMAT_R8_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8_SRGB]                   = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_UNORM]                = TEX_FORMAT_RG8_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_SNORM]                = TEX_FORMAT_RG8_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_USCALED]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_SSCALED]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_UINT]                 = TEX_FORMAT_RG8_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_SINT]                 = TEX_FORMAT_RG8_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8_SRGB]                 = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_UNORM]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_SNORM]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_USCALED]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_SSCALED]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_UINT]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_SINT]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8_SRGB]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_UNORM]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_SNORM]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_USCALED]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_SSCALED]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_UINT]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_SINT]               = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8_SRGB]               = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_UNORM]            = TEX_FORMAT_RGBA8_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_SNORM]            = TEX_FORMAT_RGBA8_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_USCALED]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_SSCALED]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_UINT]             = TEX_FORMAT_RGBA8_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_SINT]             = TEX_FORMAT_RGBA8_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R8G8B8A8_SRGB]             = TEX_FORMAT_RGBA8_UNORM_SRGB;

        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_UNORM]            = TEX_FORMAT_BGRA8_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_SNORM]            = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_USCALED]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_SSCALED]          = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_UINT]             = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_SINT]             = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_B8G8R8A8_SRGB]             = TEX_FORMAT_BGRA8_UNORM_SRGB;

        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_UNORM_PACK32]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_SNORM_PACK32]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_USCALED_PACK32]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_SSCALED_PACK32]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_UINT_PACK32]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_SINT_PACK32]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A8B8G8R8_SRGB_PACK32]      = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_UNORM_PACK32]  = TEX_FORMAT_RGB10A2_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_SNORM_PACK32]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_USCALED_PACK32]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_SSCALED_PACK32]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_UINT_PACK32]       = TEX_FORMAT_RGB10A2_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2R10G10B10_SINT_PACK32]       = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_UNORM_PACK32]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_SNORM_PACK32]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_USCALED_PACK32]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_SSCALED_PACK32]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_UINT_PACK32]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_A2B10G10R10_SINT_PACK32]       = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R16_UNORM]             = TEX_FORMAT_R16_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_SNORM]             = TEX_FORMAT_R16_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_USCALED]           = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_SSCALED]           = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_UINT]              = TEX_FORMAT_R16_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_SINT]              = TEX_FORMAT_R16_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16_SFLOAT]            = TEX_FORMAT_R16_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_UNORM]          = TEX_FORMAT_RG16_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_SNORM]          = TEX_FORMAT_RG16_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_USCALED]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_SSCALED]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_UINT]           = TEX_FORMAT_RG16_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_SINT]           = TEX_FORMAT_RG16_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16_SFLOAT]         = TEX_FORMAT_RG16_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_UNORM]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_SNORM]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_USCALED]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_SSCALED]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_UINT]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_SINT]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16_SFLOAT]      = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_UNORM]    = TEX_FORMAT_RGBA16_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_SNORM]    = TEX_FORMAT_RGBA16_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_USCALED]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_SSCALED]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_UINT]     = TEX_FORMAT_RGBA16_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_SINT]     = TEX_FORMAT_RGBA16_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R16G16B16A16_SFLOAT]   = TEX_FORMAT_RGBA16_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R32_UINT]              = TEX_FORMAT_R32_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32_SINT]              = TEX_FORMAT_R32_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32_SFLOAT]            = TEX_FORMAT_R32_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32_UINT]           = TEX_FORMAT_RG32_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32_SINT]           = TEX_FORMAT_RG32_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32_SFLOAT]         = TEX_FORMAT_RG32_FLOAT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32_UINT]        = TEX_FORMAT_RGB32_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32_SINT]        = TEX_FORMAT_RGB32_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32_SFLOAT]      = TEX_FORMAT_RGB32_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32A32_UINT]     = TEX_FORMAT_RGBA32_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32A32_SINT]     = TEX_FORMAT_RGBA32_SINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_R32G32B32A32_SFLOAT]   = TEX_FORMAT_RGBA32_FLOAT;

        m_VkFmtToTexFmtMap[VK_FORMAT_R64_UINT]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64_SINT]              = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64_SFLOAT]            = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64_UINT]           = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64_SINT]           = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64_SFLOAT]         = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64_UINT]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64_SINT]        = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64_SFLOAT]      = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64A64_UINT]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64A64_SINT]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_R64G64B64A64_SFLOAT]   = TEX_FORMAT_UNKNOWN;

        m_VkFmtToTexFmtMap[VK_FORMAT_B10G11R11_UFLOAT_PACK32]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_E5B9G9R9_UFLOAT_PACK32]    = TEX_FORMAT_RGB9E5_SHAREDEXP;
        m_VkFmtToTexFmtMap[VK_FORMAT_D16_UNORM]                 = TEX_FORMAT_D16_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_X8_D24_UNORM_PACK32]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_D32_SFLOAT]                = TEX_FORMAT_D32_FLOAT;
        m_VkFmtToTexFmtMap[VK_FORMAT_S8_UINT]                   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_D16_UNORM_S8_UINT]         = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_D24_UNORM_S8_UINT]         = TEX_FORMAT_D24_UNORM_S8_UINT;
        m_VkFmtToTexFmtMap[VK_FORMAT_D32_SFLOAT_S8_UINT]        = TEX_FORMAT_D32_FLOAT_S8X24_UINT;

        m_VkFmtToTexFmtMap[VK_FORMAT_BC1_RGB_UNORM_BLOCK]       = TEX_FORMAT_BC1_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC1_RGB_SRGB_BLOCK]        = TEX_FORMAT_BC1_UNORM_SRGB;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC1_RGBA_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC1_RGBA_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC2_UNORM_BLOCK]           = TEX_FORMAT_BC2_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC2_SRGB_BLOCK]            = TEX_FORMAT_BC2_UNORM_SRGB;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC3_UNORM_BLOCK]           = TEX_FORMAT_BC3_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC3_SRGB_BLOCK]            = TEX_FORMAT_BC3_UNORM_SRGB;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC4_UNORM_BLOCK]           = TEX_FORMAT_BC4_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC4_SNORM_BLOCK]           = TEX_FORMAT_BC4_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC5_UNORM_BLOCK]           = TEX_FORMAT_BC5_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC5_SNORM_BLOCK]           = TEX_FORMAT_BC5_SNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC6H_UFLOAT_BLOCK]         = TEX_FORMAT_BC6H_UF16;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC6H_SFLOAT_BLOCK]         = TEX_FORMAT_BC6H_SF16;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC7_UNORM_BLOCK]           = TEX_FORMAT_BC7_UNORM;
        m_VkFmtToTexFmtMap[VK_FORMAT_BC7_SRGB_BLOCK]            = TEX_FORMAT_BC7_UNORM_SRGB;

        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_EAC_R11_UNORM_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_EAC_R11_SNORM_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_EAC_R11G11_UNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_EAC_R11G11_SNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_4x4_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_4x4_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_5x4_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_5x4_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_5x5_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_5x5_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_6x5_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_6x5_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_6x6_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_6x6_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x5_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x5_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x6_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x6_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x8_UNORM_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_8x8_SRGB_BLOCK]       = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x5_UNORM_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x5_SRGB_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x6_UNORM_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x6_SRGB_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x8_UNORM_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x8_SRGB_BLOCK]      = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x10_UNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_10x10_SRGB_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_12x10_UNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_12x10_SRGB_BLOCK]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_12x12_UNORM_BLOCK]    = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMap[VK_FORMAT_ASTC_12x12_SRGB_BLOCK]     = TEX_FORMAT_UNKNOWN;



        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8B8G8R8_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_B8G8R8G8_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8R8_2PLANE_420_UNORM]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8R8_2PLANE_422_UNORM]   = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R10X6_UNORM_PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R10X6G10X6_UNORM_2PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16]     = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R12X4_UNORM_PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R12X4G12X4_UNORM_2PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16B16G16R16_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_B16G16R16G16_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16R16_2PLANE_420_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16R16_2PLANE_422_UNORM]  = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        m_VkFmtToTexFmtMapExt[VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG] = TEX_FORMAT_UNKNOWN;
        // clang-format on
    }

    TEXTURE_FORMAT operator[](VkFormat VkFmt) const
    {
        if (VkFmt < VK_FORMAT_RANGE_SIZE)
        {
            return m_VkFmtToTexFmtMap[VkFmt];
        }
        else
        {
            auto it = m_VkFmtToTexFmtMapExt.find(VkFmt);
            return it != m_VkFmtToTexFmtMapExt.end() ? it->second : TEX_FORMAT_UNKNOWN;
        }
    }

private:
    TEXTURE_FORMAT                               m_VkFmtToTexFmtMap[VK_FORMAT_RANGE_SIZE] = {};
    std::unordered_map<VkFormat, TEXTURE_FORMAT> m_VkFmtToTexFmtMapExt;
};

TEXTURE_FORMAT VkFormatToTexFormat(VkFormat VkFmt)
{
    static const VkFormatToTexFormatMapper FmtMapper;
    return FmtMapper[VkFmt];
}



VkFormat TypeToVkFormat(VALUE_TYPE ValType, Uint32 NumComponents, Bool bIsNormalized)
{
    switch (ValType)
    {
        case VT_FLOAT16:
        {
            VERIFY(!bIsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 1: return VK_FORMAT_R16_SFLOAT;
                case 2: return VK_FORMAT_R16G16_SFLOAT;
                case 3: return VK_FORMAT_R16G16B16_SFLOAT;
                case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
                default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
            }
        }

        case VT_FLOAT32:
        {
            VERIFY(!bIsNormalized, "Floating point formats cannot be normalized");
            switch (NumComponents)
            {
                case 1: return VK_FORMAT_R32_SFLOAT;
                case 2: return VK_FORMAT_R32G32_SFLOAT;
                case 3: return VK_FORMAT_R32G32B32_SFLOAT;
                case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
                default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
            }
        }

        case VT_INT32:
        {
            VERIFY(!bIsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return VK_FORMAT_R32_SINT;
                case 2: return VK_FORMAT_R32G32_SINT;
                case 3: return VK_FORMAT_R32G32B32_SINT;
                case 4: return VK_FORMAT_R32G32B32A32_SINT;
                default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
            }
        }

        case VT_UINT32:
        {
            VERIFY(!bIsNormalized, "32-bit UNORM formats are not supported. Use R32_FLOAT instead");
            switch (NumComponents)
            {
                case 1: return VK_FORMAT_R32_UINT;
                case 2: return VK_FORMAT_R32G32_UINT;
                case 3: return VK_FORMAT_R32G32B32_UINT;
                case 4: return VK_FORMAT_R32G32B32A32_UINT;
                default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
            }
        }

        case VT_INT16:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R16_SNORM;
                    case 2: return VK_FORMAT_R16G16_SNORM;
                    case 3: return VK_FORMAT_R16G16B16_SNORM;
                    case 4: return VK_FORMAT_R16G16B16A16_SNORM;
                    default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R16_SINT;
                    case 2: return VK_FORMAT_R16G16_SINT;
                    case 3: return VK_FORMAT_R16G16B16_SINT;
                    case 4: return VK_FORMAT_R16G16B16A16_SINT;
                    default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
        }

        case VT_UINT16:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R16_UNORM;
                    case 2: return VK_FORMAT_R16G16_UNORM;
                    case 3: return VK_FORMAT_R16G16B16_UNORM;
                    case 4: return VK_FORMAT_R16G16B16A16_UNORM;
                    default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R16_UINT;
                    case 2: return VK_FORMAT_R16G16_UINT;
                    case 3: return VK_FORMAT_R16G16B16_UINT;
                    case 4: return VK_FORMAT_R16G16B16A16_UINT;
                    default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
        }

        case VT_INT8:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R8_SNORM;
                    case 2: return VK_FORMAT_R8G8_SNORM;
                    case 3: return VK_FORMAT_R8G8B8_SNORM;
                    case 4: return VK_FORMAT_R8G8B8A8_SNORM;
                    default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R8_SINT;
                    case 2: return VK_FORMAT_R8G8_SINT;
                    case 3: return VK_FORMAT_R8G8B8_SINT;
                    case 4: return VK_FORMAT_R8G8B8A8_SINT;
                    default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
        }

        case VT_UINT8:
        {
            if (bIsNormalized)
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R8_UNORM;
                    case 2: return VK_FORMAT_R8G8_UNORM;
                    case 3: return VK_FORMAT_R8G8B8_UNORM;
                    case 4: return VK_FORMAT_R8G8B8A8_UNORM;
                    default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                switch (NumComponents)
                {
                    case 1: return VK_FORMAT_R8_UINT;
                    case 2: return VK_FORMAT_R8G8_UINT;
                    case 3: return VK_FORMAT_R8G8B8_UINT;
                    case 4: return VK_FORMAT_R8G8B8A8_UINT;
                    default: UNEXPECTED("Unsupported number of components"); return VK_FORMAT_UNDEFINED;
                }
            }
        }

        default: UNEXPECTED("Unsupported format"); return VK_FORMAT_UNDEFINED;
    }
}

VkIndexType TypeToVkIndexType(VALUE_TYPE IndexType)
{
    switch (IndexType)
    {
        // clang-format off
        case VT_UNDEFINED: return VK_INDEX_TYPE_NONE_KHR; // only for ray tracing
        case VT_UINT16:    return VK_INDEX_TYPE_UINT16;
        case VT_UINT32:    return VK_INDEX_TYPE_UINT32;
        // clang-format on
        default:
            UNEXPECTED("Unexpected index type");
            return VK_INDEX_TYPE_UINT32;
    }
}

VkPolygonMode FillModeToVkPolygonMode(FILL_MODE FillMode)
{
    switch (FillMode)
    {
        case FILL_MODE_UNDEFINED:
            UNEXPECTED("Undefined fill mode");
            return VK_POLYGON_MODE_FILL;

        // clang-format off
        case FILL_MODE_SOLID:     return VK_POLYGON_MODE_FILL;
        case FILL_MODE_WIREFRAME: return VK_POLYGON_MODE_LINE;
            // clang-format on

        default:
            UNEXPECTED("Unexpected fill mode");
            return VK_POLYGON_MODE_FILL;
    }
}

VkCullModeFlagBits CullModeToVkCullMode(CULL_MODE CullMode)
{
    switch (CullMode)
    {
        case CULL_MODE_UNDEFINED:
            UNEXPECTED("Undefined cull mode");
            return VK_CULL_MODE_NONE;

        // clang-format off
        case CULL_MODE_NONE:  return VK_CULL_MODE_NONE;
        case CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case CULL_MODE_BACK:  return VK_CULL_MODE_BACK_BIT;
            // clang-format on

        default:
            UNEXPECTED("Unexpected cull mode");
            return VK_CULL_MODE_NONE;
    }
}

VkPipelineRasterizationStateCreateInfo RasterizerStateDesc_To_VkRasterizationStateCI(const RasterizerStateDesc& RasterizerDesc)
{
    VkPipelineRasterizationStateCreateInfo RSStateCI = {};

    RSStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RSStateCI.pNext = nullptr;
    RSStateCI.flags = 0; // Reserved for future use.

    // If depth clamping is enabled, before the incoming fragment's zf is compared to za, zf is clamped to
    // [min(n,f), max(n,f)], where n and f are the minDepth and maxDepth depth range values of the viewport
    // used by this fragment, respectively (25.10)
    // This value is the opposite of clip enable
    RSStateCI.depthClampEnable = RasterizerDesc.DepthClipEnable ? VK_FALSE : VK_TRUE;

    RSStateCI.rasterizerDiscardEnable = VK_FALSE;                                                                                         // Whether primitives are discarded immediately before the rasterization stage.
    RSStateCI.polygonMode             = FillModeToVkPolygonMode(RasterizerDesc.FillMode);                                                 // 24.7.2
    RSStateCI.cullMode                = CullModeToVkCullMode(RasterizerDesc.CullMode);                                                    // 24.7.1
    RSStateCI.frontFace               = RasterizerDesc.FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE; // 24.7.1
    // Depth bias (24.7.3)
    RSStateCI.depthBiasEnable = (RasterizerDesc.DepthBias != 0 || RasterizerDesc.SlopeScaledDepthBias != 0.f) ? VK_TRUE : VK_FALSE;
    RSStateCI.depthBiasConstantFactor =
        static_cast<float>(RasterizerDesc.DepthBias);         // a scalar factor applied to an implementation-dependent constant
                                                              // that relates to the usable resolution of the depth buffer
    RSStateCI.depthBiasClamp = RasterizerDesc.DepthBiasClamp; // maximum (or minimum) depth bias of a fragment.
    RSStateCI.depthBiasSlopeFactor =
        RasterizerDesc.SlopeScaledDepthBias; //  a scalar factor applied to a fragment's slope in depth bias calculations.
    RSStateCI.lineWidth = 1.f;               // If the wide lines feature is not enabled, and no element of the pDynamicStates member of
                                             // pDynamicState is VK_DYNAMIC_STATE_LINE_WIDTH, the lineWidth member of
                                             // pRasterizationState must be 1.0 (9.2)

    return RSStateCI;
}

VkCompareOp ComparisonFuncToVkCompareOp(COMPARISON_FUNCTION CmpFunc)
{
    switch (CmpFunc)
    {
        // clang-format off
        case COMPARISON_FUNC_UNKNOWN:
            UNEXPECTED("Comparison function is not specified" );
            return VK_COMPARE_OP_ALWAYS;

        case COMPARISON_FUNC_NEVER:         return VK_COMPARE_OP_NEVER;
        case COMPARISON_FUNC_LESS:          return VK_COMPARE_OP_LESS;
        case COMPARISON_FUNC_EQUAL:         return VK_COMPARE_OP_EQUAL;
        case COMPARISON_FUNC_LESS_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
        case COMPARISON_FUNC_GREATER:       return VK_COMPARE_OP_GREATER;
        case COMPARISON_FUNC_NOT_EQUAL:     return VK_COMPARE_OP_NOT_EQUAL;
        case COMPARISON_FUNC_GREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case COMPARISON_FUNC_ALWAYS:        return VK_COMPARE_OP_ALWAYS;
            // clang-format on

        default:
            UNEXPECTED("Unknown comparison function");
            return VK_COMPARE_OP_ALWAYS;
    }
}

VkStencilOp StencilOpToVkStencilOp(STENCIL_OP StencilOp)
{
    switch (StencilOp)
    {
        // clang-format off
        case STENCIL_OP_UNDEFINED:
            UNEXPECTED("Undefined stencil operation");
            return VK_STENCIL_OP_KEEP;

        case STENCIL_OP_KEEP:       return VK_STENCIL_OP_KEEP;
        case STENCIL_OP_ZERO:       return VK_STENCIL_OP_ZERO;
        case STENCIL_OP_REPLACE:    return VK_STENCIL_OP_REPLACE;
        case STENCIL_OP_INCR_SAT:   return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case STENCIL_OP_DECR_SAT:   return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case STENCIL_OP_INVERT:     return VK_STENCIL_OP_INVERT;
        case STENCIL_OP_INCR_WRAP:  return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case STENCIL_OP_DECR_WRAP:  return VK_STENCIL_OP_DECREMENT_AND_WRAP;
            // clang-format on

        default:
            UNEXPECTED("Unknown stencil operation");
            return VK_STENCIL_OP_KEEP;
    }
}

VkStencilOpState StencilOpDescToVkStencilOpState(const StencilOpDesc& desc, Uint8 StencilReadMask, Uint8 StencilWriteMask)
{
    // Stencil state (25.9)
    VkStencilOpState StencilState = {};
    StencilState.failOp           = StencilOpToVkStencilOp(desc.StencilFailOp);
    StencilState.passOp           = StencilOpToVkStencilOp(desc.StencilPassOp);
    StencilState.depthFailOp      = StencilOpToVkStencilOp(desc.StencilDepthFailOp);
    StencilState.compareOp        = ComparisonFuncToVkCompareOp(desc.StencilFunc);

    // The s least significant bits of compareMask,  where s is the number of bits in the stencil framebuffer attachment,
    // are bitwise ANDed with both the reference and the stored stencil value, and the resulting masked values are those
    // that participate in the comparison controlled by compareOp (25.9)
    StencilState.compareMask = StencilReadMask;

    // The least significant s bits of writeMask, where s is the number of bits in the stencil framebuffer
    // attachment, specify an integer mask. Where a 1 appears in this mask, the corresponding bit in the stencil
    // value in the depth / stencil attachment is written; where a 0 appears, the bit is not written (25.9)
    StencilState.writeMask = StencilWriteMask;

    StencilState.reference = 0; // Set dynamically

    return StencilState;
}


VkPipelineDepthStencilStateCreateInfo DepthStencilStateDesc_To_VkDepthStencilStateCI(const DepthStencilStateDesc& DepthStencilDesc)
{
    // Depth-stencil state (25.7)
    VkPipelineDepthStencilStateCreateInfo DSStateCI = {};

    DSStateCI.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DSStateCI.pNext                 = nullptr;
    DSStateCI.flags                 = 0; // reserved for future use
    DSStateCI.depthTestEnable       = DepthStencilDesc.DepthEnable ? VK_TRUE : VK_FALSE;
    DSStateCI.depthWriteEnable      = DepthStencilDesc.DepthWriteEnable ? VK_TRUE : VK_FALSE;
    DSStateCI.depthCompareOp        = ComparisonFuncToVkCompareOp(DepthStencilDesc.DepthFunc); // 25.10
    DSStateCI.depthBoundsTestEnable = VK_FALSE;                                                // 25.8
    DSStateCI.stencilTestEnable     = DepthStencilDesc.StencilEnable ? VK_TRUE : VK_FALSE;     // 25.9
    DSStateCI.front                 = StencilOpDescToVkStencilOpState(DepthStencilDesc.FrontFace, DepthStencilDesc.StencilReadMask, DepthStencilDesc.StencilWriteMask);
    DSStateCI.back                  = StencilOpDescToVkStencilOpState(DepthStencilDesc.BackFace, DepthStencilDesc.StencilReadMask, DepthStencilDesc.StencilWriteMask);
    // Depth Bounds Test (25.8)
    DSStateCI.minDepthBounds = 0; // must be between 0.0 and 1.0, inclusive
    DSStateCI.maxDepthBounds = 1; // must be between 0.0 and 1.0, inclusive

    return DSStateCI;
}

class BlendFactorToVkBlendFactorMapper
{
public:
    BlendFactorToVkBlendFactorMapper()
    {
        // 26.1.1
        m_Map[BLEND_FACTOR_ZERO]             = VK_BLEND_FACTOR_ZERO;
        m_Map[BLEND_FACTOR_ONE]              = VK_BLEND_FACTOR_ONE;
        m_Map[BLEND_FACTOR_SRC_COLOR]        = VK_BLEND_FACTOR_SRC_COLOR;
        m_Map[BLEND_FACTOR_INV_SRC_COLOR]    = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        m_Map[BLEND_FACTOR_SRC_ALPHA]        = VK_BLEND_FACTOR_SRC_ALPHA;
        m_Map[BLEND_FACTOR_INV_SRC_ALPHA]    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_Map[BLEND_FACTOR_DEST_ALPHA]       = VK_BLEND_FACTOR_DST_ALPHA;
        m_Map[BLEND_FACTOR_INV_DEST_ALPHA]   = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        m_Map[BLEND_FACTOR_DEST_COLOR]       = VK_BLEND_FACTOR_DST_COLOR;
        m_Map[BLEND_FACTOR_INV_DEST_COLOR]   = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        m_Map[BLEND_FACTOR_SRC_ALPHA_SAT]    = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        m_Map[BLEND_FACTOR_BLEND_FACTOR]     = VK_BLEND_FACTOR_CONSTANT_COLOR;
        m_Map[BLEND_FACTOR_INV_BLEND_FACTOR] = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        m_Map[BLEND_FACTOR_SRC1_COLOR]       = VK_BLEND_FACTOR_SRC1_COLOR;
        m_Map[BLEND_FACTOR_INV_SRC1_COLOR]   = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        m_Map[BLEND_FACTOR_SRC1_ALPHA]       = VK_BLEND_FACTOR_SRC1_ALPHA;
        m_Map[BLEND_FACTOR_INV_SRC1_ALPHA]   = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }

    VkBlendFactor operator[](BLEND_FACTOR bf) const
    {
        VERIFY_EXPR(bf > BLEND_FACTOR_UNDEFINED && bf < BLEND_FACTOR_NUM_FACTORS);
        return m_Map[static_cast<int>(bf)];
    }

private:
    std::array<VkBlendFactor, BLEND_FACTOR_NUM_FACTORS> m_Map = {};
};


class LogicOperationToVkLogicOp
{
public:
    LogicOperationToVkLogicOp()
    {
        // 26.2
        m_Map[LOGIC_OP_CLEAR]         = VK_LOGIC_OP_CLEAR;
        m_Map[LOGIC_OP_SET]           = VK_LOGIC_OP_SET;
        m_Map[LOGIC_OP_COPY]          = VK_LOGIC_OP_COPY;
        m_Map[LOGIC_OP_COPY_INVERTED] = VK_LOGIC_OP_COPY_INVERTED;
        m_Map[LOGIC_OP_NOOP]          = VK_LOGIC_OP_NO_OP;
        m_Map[LOGIC_OP_INVERT]        = VK_LOGIC_OP_INVERT;
        m_Map[LOGIC_OP_AND]           = VK_LOGIC_OP_AND;
        m_Map[LOGIC_OP_NAND]          = VK_LOGIC_OP_NAND;
        m_Map[LOGIC_OP_OR]            = VK_LOGIC_OP_OR;
        m_Map[LOGIC_OP_NOR]           = VK_LOGIC_OP_NOR;
        m_Map[LOGIC_OP_XOR]           = VK_LOGIC_OP_XOR;
        m_Map[LOGIC_OP_EQUIV]         = VK_LOGIC_OP_EQUIVALENT;
        m_Map[LOGIC_OP_AND_REVERSE]   = VK_LOGIC_OP_AND_REVERSE;
        m_Map[LOGIC_OP_AND_INVERTED]  = VK_LOGIC_OP_AND_INVERTED;
        m_Map[LOGIC_OP_OR_REVERSE]    = VK_LOGIC_OP_OR_REVERSE;
        m_Map[LOGIC_OP_OR_INVERTED]   = VK_LOGIC_OP_OR_INVERTED;
    }

    VkLogicOp operator[](LOGIC_OPERATION op) const
    {
        VERIFY_EXPR(op >= LOGIC_OP_CLEAR && op < LOGIC_OP_NUM_OPERATIONS);
        return m_Map[static_cast<int>(op)];
    }

private:
    std::array<VkLogicOp, LOGIC_OP_NUM_OPERATIONS> m_Map = {};
};

class BlendOperationToVkBlendOp
{
public:
    BlendOperationToVkBlendOp()
    {
        // 26.1.3
        m_Map[BLEND_OPERATION_ADD]          = VK_BLEND_OP_ADD;
        m_Map[BLEND_OPERATION_SUBTRACT]     = VK_BLEND_OP_SUBTRACT;
        m_Map[BLEND_OPERATION_REV_SUBTRACT] = VK_BLEND_OP_REVERSE_SUBTRACT;
        m_Map[BLEND_OPERATION_MIN]          = VK_BLEND_OP_MIN;
        m_Map[BLEND_OPERATION_MAX]          = VK_BLEND_OP_MAX;
    }

    VkBlendOp operator[](BLEND_OPERATION op) const
    {
        VERIFY_EXPR(op > BLEND_OPERATION_UNDEFINED && op < BLEND_OPERATION_NUM_OPERATIONS);
        return m_Map[static_cast<int>(op)];
    }

private:
    std::array<VkBlendOp, BLEND_OPERATION_NUM_OPERATIONS> m_Map = {};
};

VkPipelineColorBlendAttachmentState RenderTargetBlendDescToVkColorBlendAttachmentState(const RenderTargetBlendDesc& RTBlendDesc)
{
    static const BlendFactorToVkBlendFactorMapper BFtoVKBF;
    static const BlendOperationToVkBlendOp        BOtoVKBO;
    VkPipelineColorBlendAttachmentState           AttachmentBlendState = {};

    AttachmentBlendState.blendEnable         = RTBlendDesc.BlendEnable;
    AttachmentBlendState.srcColorBlendFactor = BFtoVKBF[RTBlendDesc.SrcBlend];
    AttachmentBlendState.dstColorBlendFactor = BFtoVKBF[RTBlendDesc.DestBlend];
    AttachmentBlendState.colorBlendOp        = BOtoVKBO[RTBlendDesc.BlendOp];
    AttachmentBlendState.srcAlphaBlendFactor = BFtoVKBF[RTBlendDesc.SrcBlendAlpha];
    AttachmentBlendState.dstAlphaBlendFactor = BFtoVKBF[RTBlendDesc.DestBlendAlpha];
    AttachmentBlendState.alphaBlendOp        = BOtoVKBO[RTBlendDesc.BlendOpAlpha];
    AttachmentBlendState.colorWriteMask =
        ((RTBlendDesc.RenderTargetWriteMask & COLOR_MASK_RED) ? VK_COLOR_COMPONENT_R_BIT : 0) |
        ((RTBlendDesc.RenderTargetWriteMask & COLOR_MASK_GREEN) ? VK_COLOR_COMPONENT_G_BIT : 0) |
        ((RTBlendDesc.RenderTargetWriteMask & COLOR_MASK_BLUE) ? VK_COLOR_COMPONENT_B_BIT : 0) |
        ((RTBlendDesc.RenderTargetWriteMask & COLOR_MASK_ALPHA) ? VK_COLOR_COMPONENT_A_BIT : 0);

    return AttachmentBlendState;
}

void BlendStateDesc_To_VkBlendStateCI(const BlendStateDesc&                             BSDesc,
                                      VkPipelineColorBlendStateCreateInfo&              ColorBlendStateCI,
                                      std::vector<VkPipelineColorBlendAttachmentState>& ColorBlendAttachments)
{
    // Color blend state (26.1)
    static const LogicOperationToVkLogicOp LogicOpToVkLogicOp;
    ColorBlendStateCI.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendStateCI.pNext             = nullptr;
    ColorBlendStateCI.flags             = 0;                                            // reserved for future use
    ColorBlendStateCI.logicOpEnable     = BSDesc.RenderTargets[0].LogicOperationEnable; // 26.2
    ColorBlendStateCI.logicOp           = LogicOpToVkLogicOp[BSDesc.RenderTargets[0].LogicOp];
    ColorBlendStateCI.blendConstants[0] = 0.f; // We use dynamic blend constants
    ColorBlendStateCI.blendConstants[1] = 0.f;
    ColorBlendStateCI.blendConstants[2] = 0.f;
    ColorBlendStateCI.blendConstants[3] = 0.f;
    // attachmentCount must equal the colorAttachmentCount for the subpass in which this pipeline is used.
    for (uint32_t attachment = 0; attachment < ColorBlendStateCI.attachmentCount; ++attachment)
    {
        const auto& RTBlendState          = BSDesc.IndependentBlendEnable ? BSDesc.RenderTargets[attachment] : BSDesc.RenderTargets[0];
        ColorBlendAttachments[attachment] = RenderTargetBlendDescToVkColorBlendAttachmentState(RTBlendState);
    }
}

VkVertexInputRate LayoutElemFrequencyToVkInputRate(INPUT_ELEMENT_FREQUENCY frequency)
{
    switch (frequency)
    {
        case INPUT_ELEMENT_FREQUENCY_UNDEFINED:
            UNEXPECTED("Undefined layout element frequency");
            return VK_VERTEX_INPUT_RATE_VERTEX;

        case INPUT_ELEMENT_FREQUENCY_PER_VERTEX: return VK_VERTEX_INPUT_RATE_VERTEX;
        case INPUT_ELEMENT_FREQUENCY_PER_INSTANCE: return VK_VERTEX_INPUT_RATE_INSTANCE;

        default:
            UNEXPECTED("Unknown layout element frequency");
            return VK_VERTEX_INPUT_RATE_VERTEX;
    }
}

void InputLayoutDesc_To_VkVertexInputStateCI(const InputLayoutDesc&                                                      LayoutDesc,
                                             VkPipelineVertexInputStateCreateInfo&                                       VertexInputStateCI,
                                             VkPipelineVertexInputDivisorStateCreateInfoEXT&                             VertexInputDivisorCI,
                                             std::array<VkVertexInputBindingDescription, MAX_LAYOUT_ELEMENTS>&           BindingDescriptions,
                                             std::array<VkVertexInputAttributeDescription, MAX_LAYOUT_ELEMENTS>&         AttributeDescription,
                                             std::array<VkVertexInputBindingDivisorDescriptionEXT, MAX_LAYOUT_ELEMENTS>& VertexBindingDivisors)
{
    // Vertex input description (20.2)
    VertexInputStateCI.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputStateCI.pNext                           = nullptr;
    VertexInputStateCI.flags                           = 0; // reserved for future use.
    VertexInputStateCI.vertexBindingDescriptionCount   = 0;
    VertexInputStateCI.pVertexBindingDescriptions      = BindingDescriptions.data();
    VertexInputStateCI.vertexAttributeDescriptionCount = LayoutDesc.NumElements;
    VertexInputStateCI.pVertexAttributeDescriptions    = AttributeDescription.data();

    VertexInputDivisorCI.sType                     = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;
    VertexInputDivisorCI.pNext                     = nullptr;
    VertexInputDivisorCI.vertexBindingDivisorCount = 0;
    VertexInputDivisorCI.pVertexBindingDivisors    = VertexBindingDivisors.data();

    std::array<Int32, MAX_LAYOUT_ELEMENTS> BufferSlot2BindingDescInd;
    BufferSlot2BindingDescInd.fill(-1);
    for (Uint32 elem = 0; elem < LayoutDesc.NumElements; ++elem)
    {
        auto& LayoutElem     = LayoutDesc.LayoutElements[elem];
        auto& BindingDescInd = BufferSlot2BindingDescInd[LayoutElem.BufferSlot];
        if (BindingDescInd < 0)
        {
            BindingDescInd        = VertexInputStateCI.vertexBindingDescriptionCount++;
            auto& BindingDesc     = BindingDescriptions[BindingDescInd];
            BindingDesc.binding   = LayoutElem.BufferSlot;
            BindingDesc.stride    = LayoutElem.Stride;
            BindingDesc.inputRate = LayoutElemFrequencyToVkInputRate(LayoutElem.Frequency);
        }

        const auto& BindingDesc = BindingDescriptions[BindingDescInd];
        VERIFY(BindingDesc.binding == LayoutElem.BufferSlot, "Inconsistent buffer slot");
        VERIFY(BindingDesc.stride == LayoutElem.Stride, "Inconsistent strides");
        VERIFY(BindingDesc.inputRate == LayoutElemFrequencyToVkInputRate(LayoutElem.Frequency), "Inconsistent layout element frequency");

        auto& AttribDesc    = AttributeDescription[elem];
        AttribDesc.binding  = BindingDesc.binding;
        AttribDesc.location = LayoutElem.InputIndex;
        AttribDesc.format   = TypeToVkFormat(LayoutElem.ValueType, LayoutElem.NumComponents, LayoutElem.IsNormalized);
        AttribDesc.offset   = LayoutElem.RelativeOffset;

        if (LayoutElem.Frequency == INPUT_ELEMENT_FREQUENCY_PER_INSTANCE && LayoutElem.InstanceDataStepRate != 1)
        {
            auto& AttribDivisor   = VertexBindingDivisors[VertexInputDivisorCI.vertexBindingDivisorCount++];
            AttribDivisor.binding = BindingDesc.binding;
            AttribDivisor.divisor = LayoutElem.InstanceDataStepRate;
        }
    }
}

void PrimitiveTopology_To_VkPrimitiveTopologyAndPatchCPCount(PRIMITIVE_TOPOLOGY   PrimTopology,
                                                             VkPrimitiveTopology& VkPrimTopology,
                                                             uint32_t&            PatchControlPoints)
{
    PatchControlPoints = 0;
    static_assert(PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES == 42, "Did you add a new primitive topology? Please handle it here.");
    switch (PrimTopology)
    {
        case PRIMITIVE_TOPOLOGY_UNDEFINED:
            UNEXPECTED("Undefined primitive topology");
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            return;

        case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            return;

        case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            return;

        case PRIMITIVE_TOPOLOGY_POINT_LIST:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            return;

        case PRIMITIVE_TOPOLOGY_LINE_LIST:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            return;

        case PRIMITIVE_TOPOLOGY_LINE_STRIP:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            return;

        case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_ADJ:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
            return;

        case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJ:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
            return;

        case PRIMITIVE_TOPOLOGY_LINE_LIST_ADJ:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
            return;

        case PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJ:
            VkPrimTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
            return;

        default:
            VERIFY_EXPR(PrimTopology >= PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST && PrimTopology < PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES);
            VkPrimTopology     = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
            PatchControlPoints = static_cast<uint32_t>(PrimTopology) - static_cast<uint32_t>(PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST) + 1;
            return;
    }
}

VkFilter FilterTypeToVkFilter(FILTER_TYPE FilterType)
{
    switch (FilterType)
    {
        case FILTER_TYPE_UNKNOWN:
            UNEXPECTED("Unknown filter type");
            return VK_FILTER_NEAREST;

        case FILTER_TYPE_POINT:
        case FILTER_TYPE_COMPARISON_POINT:
        case FILTER_TYPE_MINIMUM_POINT:
        case FILTER_TYPE_MAXIMUM_POINT:
            return VK_FILTER_NEAREST;

        case FILTER_TYPE_LINEAR:
        case FILTER_TYPE_ANISOTROPIC:
        case FILTER_TYPE_COMPARISON_LINEAR:
        case FILTER_TYPE_COMPARISON_ANISOTROPIC:
        case FILTER_TYPE_MINIMUM_LINEAR:
        case FILTER_TYPE_MINIMUM_ANISOTROPIC:
        case FILTER_TYPE_MAXIMUM_LINEAR:
        case FILTER_TYPE_MAXIMUM_ANISOTROPIC:
            return VK_FILTER_LINEAR;

        default:
            UNEXPECTED("Unexpected filter type");
            return VK_FILTER_NEAREST;
    }
}

VkSamplerMipmapMode FilterTypeToVkMipmapMode(FILTER_TYPE FilterType)
{
    switch (FilterType)
    {
        case FILTER_TYPE_UNKNOWN:
            UNEXPECTED("Unknown filter type");
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case FILTER_TYPE_POINT:
        case FILTER_TYPE_COMPARISON_POINT:
        case FILTER_TYPE_MINIMUM_POINT:
        case FILTER_TYPE_MAXIMUM_POINT:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case FILTER_TYPE_LINEAR:
        case FILTER_TYPE_ANISOTROPIC:
        case FILTER_TYPE_COMPARISON_LINEAR:
        case FILTER_TYPE_COMPARISON_ANISOTROPIC:
        case FILTER_TYPE_MINIMUM_LINEAR:
        case FILTER_TYPE_MINIMUM_ANISOTROPIC:
        case FILTER_TYPE_MAXIMUM_LINEAR:
        case FILTER_TYPE_MAXIMUM_ANISOTROPIC:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;

        default:
            UNEXPECTED("Only point and linear filter types are allowed for mipmap mode");
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
}

VkSamplerAddressMode AddressModeToVkAddressMode(TEXTURE_ADDRESS_MODE AddressMode)
{
    switch (AddressMode)
    {
        case TEXTURE_ADDRESS_UNKNOWN:
            UNEXPECTED("Unknown address mode");
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        // clang-format off
        case TEXTURE_ADDRESS_WRAP:   return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TEXTURE_ADDRESS_MIRROR: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case TEXTURE_ADDRESS_CLAMP:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TEXTURE_ADDRESS_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case TEXTURE_ADDRESS_MIRROR_ONCE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            // clang-format on

        default:
            UNEXPECTED("Unexpected address mode");
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
}

VkBorderColor BorderColorToVkBorderColor(const Float32 BorderColor[])
{
    VkBorderColor vkBorderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    if (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 0)
        vkBorderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    else if (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 1)
        vkBorderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    else if (BorderColor[0] == 1 && BorderColor[1] == 1 && BorderColor[2] == 1 && BorderColor[3] == 1)
        vkBorderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    else
    {
        LOG_ERROR_MESSAGE("Vulkan samplers only allow transparent black (0,0,0,0), opaque black (0,0,0,1) or opaque white (1,1,1,1) as border colors.");
    }

    return vkBorderColor;
}


static VkPipelineStageFlags ResourceStateFlagToVkPipelineStage(RESOURCE_STATE StateFlag)
{
    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "This function must be updated to handle new resource state flag");
    VERIFY((StateFlag & (StateFlag - 1)) == 0, "Only single bit must be set");
    switch (StateFlag)
    {
        // clang-format off
        case RESOURCE_STATE_UNDEFINED:         return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case RESOURCE_STATE_VERTEX_BUFFER:     return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        case RESOURCE_STATE_CONSTANT_BUFFER:   return VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
        case RESOURCE_STATE_INDEX_BUFFER:      return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        case RESOURCE_STATE_RENDER_TARGET:     return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case RESOURCE_STATE_UNORDERED_ACCESS:  return VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
        case RESOURCE_STATE_DEPTH_WRITE:       return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case RESOURCE_STATE_DEPTH_READ:        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case RESOURCE_STATE_SHADER_RESOURCE:   return VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
        case RESOURCE_STATE_STREAM_OUT:        return 0;
        case RESOURCE_STATE_INDIRECT_ARGUMENT: return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        case RESOURCE_STATE_COPY_DEST:         return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case RESOURCE_STATE_COPY_SOURCE:       return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case RESOURCE_STATE_RESOLVE_DEST:      return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case RESOURCE_STATE_RESOLVE_SOURCE:    return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case RESOURCE_STATE_INPUT_ATTACHMENT:  return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case RESOURCE_STATE_PRESENT:           return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case RESOURCE_STATE_BUILD_AS_READ:     return VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        case RESOURCE_STATE_BUILD_AS_WRITE:    return VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        case RESOURCE_STATE_RAY_TRACING:       return VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        case RESOURCE_STATE_COMMON:            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT; // resource may be used in multiple states
        case RESOURCE_STATE_SHADING_RATE:      return VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT | VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
            // clang-format on

        default:
            UNEXPECTED("Unexpected resource state flag");
            return 0;
    }
}

VkPipelineStageFlags ResourceStateFlagsToVkPipelineStageFlags(RESOURCE_STATE StateFlags)
{
    VERIFY(Uint32{StateFlags} < (RESOURCE_STATE_MAX_BIT << 1), "Resource state flags are out of range");

    VkPipelineStageFlags vkPipelineStages = 0;
    while (StateFlags != RESOURCE_STATE_UNKNOWN)
    {
        auto StateBit = ExtractLSB(StateFlags);
        vkPipelineStages |= ResourceStateFlagToVkPipelineStage(StateBit);
    }
    return vkPipelineStages;
}


static VkAccessFlags ResourceStateFlagToVkAccessFlags(RESOURCE_STATE StateFlag)
{
    // Currently not used:
    //VK_ACCESS_HOST_READ_BIT
    //VK_ACCESS_HOST_WRITE_BIT
    //VK_ACCESS_MEMORY_READ_BIT
    //VK_ACCESS_MEMORY_WRITE_BIT
    //VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT
    //VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT
    //VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT
    //VK_ACCESS_COMMAND_PROCESS_READ_BIT_NVX
    //VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX
    //VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT
    //VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV

    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "This function must be updated to handle new resource state flag");
    VERIFY((StateFlag & (StateFlag - 1)) == 0, "Only single bit must be set");
    switch (StateFlag)
    {
        // clang-format off
        case RESOURCE_STATE_UNDEFINED:         return 0;
        case RESOURCE_STATE_VERTEX_BUFFER:     return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        case RESOURCE_STATE_CONSTANT_BUFFER:   return VK_ACCESS_UNIFORM_READ_BIT;
        case RESOURCE_STATE_INDEX_BUFFER:      return VK_ACCESS_INDEX_READ_BIT;
        case RESOURCE_STATE_RENDER_TARGET:     return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case RESOURCE_STATE_UNORDERED_ACCESS:  return VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        case RESOURCE_STATE_DEPTH_WRITE:       return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case RESOURCE_STATE_DEPTH_READ:        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case RESOURCE_STATE_SHADER_RESOURCE:   return VK_ACCESS_SHADER_READ_BIT;
        case RESOURCE_STATE_STREAM_OUT:        return VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT;
        case RESOURCE_STATE_INDIRECT_ARGUMENT: return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        case RESOURCE_STATE_COPY_DEST:         return VK_ACCESS_TRANSFER_WRITE_BIT;
        case RESOURCE_STATE_COPY_SOURCE:       return VK_ACCESS_TRANSFER_READ_BIT;
        case RESOURCE_STATE_RESOLVE_DEST:      return VK_ACCESS_TRANSFER_WRITE_BIT;
        case RESOURCE_STATE_RESOLVE_SOURCE:    return VK_ACCESS_TRANSFER_READ_BIT;
        case RESOURCE_STATE_INPUT_ATTACHMENT:  return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        case RESOURCE_STATE_PRESENT:           return 0;
        case RESOURCE_STATE_BUILD_AS_READ:     return VK_ACCESS_SHADER_READ_BIT; // for vertex, index, transform, AABB, instance buffers
        case RESOURCE_STATE_BUILD_AS_WRITE:    return VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR; // for scratch buffer
        case RESOURCE_STATE_RAY_TRACING:       return VK_ACCESS_SHADER_READ_BIT; // for SBT
        case RESOURCE_STATE_COMMON:            return 0; // COMMON state must be used for queue to queue transition (like in D3D12), queue to queue synchronization via semaphore creates a memory dependency
        case RESOURCE_STATE_SHADING_RATE:      return VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT | VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
            // clang-format on

        default:
            UNEXPECTED("Unexpected resource state flag");
            return 0;
    }
}

VkPipelineStageFlags ResourceStateFlagsToVkAccessFlags(RESOURCE_STATE StateFlags)
{
    VERIFY(Uint32{StateFlags} < (RESOURCE_STATE_MAX_BIT << 1), "Resource state flags are out of range");

    VkAccessFlags AccessFlags = 0;
    while (StateFlags != RESOURCE_STATE_UNKNOWN)
    {
        auto StateBit = ExtractLSB(StateFlags);
        AccessFlags |= ResourceStateFlagToVkAccessFlags(StateBit);
    }
    return AccessFlags;
}


VkAccessFlags AccelStructStateFlagsToVkAccessFlags(RESOURCE_STATE StateFlags)
{
    VERIFY(Uint32{StateFlags} < (RESOURCE_STATE_MAX_BIT << 1), "Resource state flags are out of range");
    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "This function must be updated to handle new resource state flag");

    VkAccessFlags AccessFlags = 0;
    Uint32        Bits        = StateFlags;
    while (Bits != 0)
    {
        auto Bit = ExtractLSB(Bits);
        switch (Bit)
        {
            // clang-format off
            case RESOURCE_STATE_BUILD_AS_READ:  AccessFlags |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR; break;
            case RESOURCE_STATE_BUILD_AS_WRITE: AccessFlags |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR; break;
            case RESOURCE_STATE_RAY_TRACING:    AccessFlags |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR; break;
            default:                            UNEXPECTED("Unexpected resource state flag");
                // clang-format on
        }
    }
    return AccessFlags;
}

static RESOURCE_STATE VkAccessFlagToResourceStates(VkAccessFlagBits AccessFlagBit)
{
    VERIFY((AccessFlagBit & (AccessFlagBit - 1)) == 0, "Single access flag bit is expected");

    switch (AccessFlagBit)
    {
        // clang-format off
        case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:                 return RESOURCE_STATE_INDIRECT_ARGUMENT;
        case VK_ACCESS_INDEX_READ_BIT:                            return RESOURCE_STATE_INDEX_BUFFER;
        case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT:                 return RESOURCE_STATE_VERTEX_BUFFER;
        case VK_ACCESS_UNIFORM_READ_BIT:                          return RESOURCE_STATE_CONSTANT_BUFFER;
        case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT:                 return RESOURCE_STATE_INPUT_ATTACHMENT;
        case VK_ACCESS_SHADER_READ_BIT:                           return RESOURCE_STATE_SHADER_RESOURCE; // or RESOURCE_STATE_BUILD_AS_READ
        case VK_ACCESS_SHADER_WRITE_BIT:                          return RESOURCE_STATE_UNORDERED_ACCESS;
        case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT:                 return RESOURCE_STATE_RENDER_TARGET;
        case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:                return RESOURCE_STATE_RENDER_TARGET;
        case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:         return RESOURCE_STATE_DEPTH_READ;
        case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:        return RESOURCE_STATE_DEPTH_WRITE;
        case VK_ACCESS_TRANSFER_READ_BIT:                         return RESOURCE_STATE_COPY_SOURCE;
        case VK_ACCESS_TRANSFER_WRITE_BIT:                        return RESOURCE_STATE_COPY_DEST;
        case VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR:      return RESOURCE_STATE_BUILD_AS_WRITE;
        case VK_ACCESS_HOST_READ_BIT:                             return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_HOST_WRITE_BIT:                            return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_MEMORY_READ_BIT:                           return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_MEMORY_WRITE_BIT:                          return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT:          return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT:   return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT:  return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT:        return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV:            return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV:           return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT: return RESOURCE_STATE_UNKNOWN;
        case VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV:            return RESOURCE_STATE_UNKNOWN;
            // clang-format on
        default:
            UNEXPECTED("Unknown access flag");
            return RESOURCE_STATE_UNKNOWN;
    }
}


class VkAccessFlagBitPosToResourceState
{
public:
    VkAccessFlagBitPosToResourceState()
    {
        for (Uint32 bit = 0; bit < FlagBitPosToResourceState.size(); ++bit)
        {
            FlagBitPosToResourceState[bit] = VkAccessFlagToResourceStates(static_cast<VkAccessFlagBits>(1 << bit));
        }
    }

    RESOURCE_STATE operator()(Uint32 BitPos) const
    {
        VERIFY(BitPos <= MaxFlagBitPos, "Resource state flag bit position (", BitPos, ") exceeds max bit position (", Uint32{MaxFlagBitPos}, ")");
        return FlagBitPosToResourceState[BitPos];
    }

private:
    static constexpr const Uint32                 MaxFlagBitPos = 20;
    std::array<RESOURCE_STATE, MaxFlagBitPos + 1> FlagBitPosToResourceState;
};


RESOURCE_STATE VkAccessFlagsToResourceStates(VkAccessFlags AccessFlags)
{
    static const VkAccessFlagBitPosToResourceState BitPosToState;
    Uint32                                         State = 0;
    while (AccessFlags != 0)
    {
        auto lsb = PlatformMisc::GetLSB(AccessFlags);
        State |= BitPosToState(lsb);
        AccessFlags &= ~(1 << lsb);
    }
    return static_cast<RESOURCE_STATE>(State);
}



VkImageLayout ResourceStateToVkImageLayout(RESOURCE_STATE StateFlag, bool IsInsideRenderPass, bool FragDensityMapInsteadOfShadingRate)
{
    if (StateFlag == RESOURCE_STATE_UNKNOWN)
    {
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
    // Currently not used:
    //VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
    //VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
    //VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV
    //VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
    //VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,

    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "This function must be updated to handle new resource state flag");
    VERIFY((StateFlag & (StateFlag - 1)) == 0, "Only single bit must be set");
    switch (StateFlag)
    {
        // clang-format off
        case RESOURCE_STATE_UNDEFINED:         return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_VERTEX_BUFFER:     UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_CONSTANT_BUFFER:   UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_INDEX_BUFFER:      UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_RENDER_TARGET:     return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case RESOURCE_STATE_UNORDERED_ACCESS:  return VK_IMAGE_LAYOUT_GENERAL;
        case RESOURCE_STATE_DEPTH_WRITE:       return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case RESOURCE_STATE_DEPTH_READ:        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case RESOURCE_STATE_SHADER_RESOURCE:   return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case RESOURCE_STATE_STREAM_OUT:        UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_INDIRECT_ARGUMENT: UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_COPY_DEST:         return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case RESOURCE_STATE_COPY_SOURCE:       return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case RESOURCE_STATE_RESOLVE_DEST:      return IsInsideRenderPass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case RESOURCE_STATE_RESOLVE_SOURCE:    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case RESOURCE_STATE_INPUT_ATTACHMENT:  return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case RESOURCE_STATE_PRESENT:           return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case RESOURCE_STATE_BUILD_AS_READ:     UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_BUILD_AS_WRITE:    UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_RAY_TRACING:       UNEXPECTED("Invalid resource state"); return VK_IMAGE_LAYOUT_UNDEFINED;
        case RESOURCE_STATE_COMMON:            return VK_IMAGE_LAYOUT_GENERAL;
        case RESOURCE_STATE_SHADING_RATE:      return FragDensityMapInsteadOfShadingRate ? VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT : VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
            // clang-format on

        default:
            UNEXPECTED("Unexpected resource state flag (", StateFlag, ")");
            return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

RESOURCE_STATE VkImageLayoutToResourceState(VkImageLayout Layout)
{
    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "This function must be updated to handle new resource state flag");
    switch (Layout)
    {
        // clang-format off
        case VK_IMAGE_LAYOUT_UNDEFINED:                                    return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_GENERAL:                                      return RESOURCE_STATE_UNORDERED_ACCESS;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:                     return RESOURCE_STATE_RENDER_TARGET;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:             return RESOURCE_STATE_DEPTH_WRITE;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:              return RESOURCE_STATE_DEPTH_READ;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:                     return RESOURCE_STATE_SHADER_RESOURCE;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:                         return RESOURCE_STATE_COPY_SOURCE;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:                         return RESOURCE_STATE_COPY_DEST;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:                               UNEXPECTED("This layout is not supported"); return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:   UNEXPECTED("This layout is not supported"); return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:   UNEXPECTED("This layout is not supported"); return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                              return RESOURCE_STATE_PRESENT;
        case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:                           UNEXPECTED("This layout is not supported"); return RESOURCE_STATE_UNDEFINED;
        case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR: return RESOURCE_STATE_SHADING_RATE;
        // clang-format on
        default:
            UNEXPECTED("Unknown image layout (", Layout, ")");
            return RESOURCE_STATE_UNDEFINED;
    }
}

SURFACE_TRANSFORM VkSurfaceTransformFlagToSurfaceTransform(VkSurfaceTransformFlagBitsKHR vkTransformFlag)
{
    VERIFY(IsPowerOfTwo(static_cast<Uint32>(vkTransformFlag)), "Only single transform bit is expected");

    // clang-format off
    switch (vkTransformFlag)
    {
        case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:                     return SURFACE_TRANSFORM_IDENTITY;
        case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:                    return SURFACE_TRANSFORM_ROTATE_90;
        case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:                   return SURFACE_TRANSFORM_ROTATE_180;
        case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:                   return SURFACE_TRANSFORM_ROTATE_270;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:            return SURFACE_TRANSFORM_HORIZONTAL_MIRROR;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:  return SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR: return SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR: return SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270;

        default:
            UNEXPECTED("Unexpected surface transform");
            return SURFACE_TRANSFORM_IDENTITY;
    }
    // clang-format on
}

VkSurfaceTransformFlagBitsKHR SurfaceTransformToVkSurfaceTransformFlag(SURFACE_TRANSFORM SrfTransform)
{
    // clang-format off
    switch (SrfTransform)
    {
        case SURFACE_TRANSFORM_OPTIMAL:
            UNEXPECTED("Optimal transform does not have corresponding Vulkan flag");
            return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

        case SURFACE_TRANSFORM_IDENTITY:                      return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        case SURFACE_TRANSFORM_ROTATE_90:                     return VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;
        case SURFACE_TRANSFORM_ROTATE_180:                    return VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR;
        case SURFACE_TRANSFORM_ROTATE_270:                    return VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR;
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR:             return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR;
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:   return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR;
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:  return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR;
        case SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:  return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR;

        default:
            UNEXPECTED("Unexpected surface transform");
            return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    // clang-format on
}


#define ASSERT_SAME(Val1, Val2) static_assert(static_cast<int>(Val1) == static_cast<int>(Val2), #Val1 " is expected to be equal to " #Val2)

ASSERT_SAME(ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD);
ASSERT_SAME(ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_CLEAR);
ASSERT_SAME(ATTACHMENT_LOAD_OP_DISCARD, VK_ATTACHMENT_LOAD_OP_DONT_CARE);
VkAttachmentLoadOp AttachmentLoadOpToVkAttachmentLoadOp(ATTACHMENT_LOAD_OP LoadOp)
{
    return static_cast<VkAttachmentLoadOp>(LoadOp);
}


ASSERT_SAME(ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_STORE);
ASSERT_SAME(ATTACHMENT_STORE_OP_DISCARD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
VkAttachmentStoreOp AttachmentStoreOpToVkAttachmentStoreOp(ATTACHMENT_STORE_OP StoreOp)
{
    return static_cast<VkAttachmentStoreOp>(StoreOp);
}


// clang-format off
ASSERT_SAME(PIPELINE_STAGE_FLAG_TOP_OF_PIPE,                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_DRAW_INDIRECT,                VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_VERTEX_INPUT,                 VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_VERTEX_SHADER,                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_HULL_SHADER,                  VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_DOMAIN_SHADER,                VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_GEOMETRY_SHADER,              VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_PIXEL_SHADER,                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_EARLY_FRAGMENT_TESTS,         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_LATE_FRAGMENT_TESTS,          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_RENDER_TARGET,                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_COMPUTE_SHADER,               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_TRANSFER,                     VK_PIPELINE_STAGE_TRANSFER_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_BOTTOM_OF_PIPE,               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_HOST,                         VK_PIPELINE_STAGE_HOST_BIT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_CONDITIONAL_RENDERING,        VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT);
ASSERT_SAME(PIPELINE_STAGE_FLAG_SHADING_RATE_TEXTURE,         VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER,           VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_TASK_SHADER,                  VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_MESH_SHADER,                  VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV);
ASSERT_SAME(PIPELINE_STAGE_FLAG_FRAGMENT_DENSITY_PROCESS,     VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT);
// clang-format on
VkPipelineStageFlags PipelineStageFlagsToVkPipelineStageFlags(PIPELINE_STAGE_FLAGS PipelineStageFlags)
{
    return static_cast<VkPipelineStageFlags>(PipelineStageFlags);
}


// clang-format off
static_assert(ACCESS_FLAG_NONE == 0, "");
ASSERT_SAME(ACCESS_FLAG_INDIRECT_COMMAND_READ,        VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_INDEX_READ,                   VK_ACCESS_INDEX_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_VERTEX_READ,                  VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_UNIFORM_READ,                 VK_ACCESS_UNIFORM_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_INPUT_ATTACHMENT_READ,        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_SHADER_READ,                  VK_ACCESS_SHADER_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_SHADER_WRITE,                 VK_ACCESS_SHADER_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_RENDER_TARGET_READ,           VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_RENDER_TARGET_WRITE,          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_DEPTH_STENCIL_READ,           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_DEPTH_STENCIL_WRITE,          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_COPY_SRC,                     VK_ACCESS_TRANSFER_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_COPY_DST,                     VK_ACCESS_TRANSFER_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_HOST_READ,                    VK_ACCESS_HOST_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_HOST_WRITE,                   VK_ACCESS_HOST_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_MEMORY_READ,                  VK_ACCESS_MEMORY_READ_BIT);
ASSERT_SAME(ACCESS_FLAG_MEMORY_WRITE,                 VK_ACCESS_MEMORY_WRITE_BIT);
ASSERT_SAME(ACCESS_FLAG_CONDITIONAL_RENDERING_READ,   VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT);
ASSERT_SAME(ACCESS_FLAG_SHADING_RATE_TEXTURE_READ,    VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV);
ASSERT_SAME(ACCESS_FLAG_ACCELERATION_STRUCTURE_READ,  VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV);
ASSERT_SAME(ACCESS_FLAG_ACCELERATION_STRUCTURE_WRITE, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV);
ASSERT_SAME(ACCESS_FLAG_FRAGMENT_DENSITY_MAP_READ,    VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT);
// clang-format on
VkAccessFlags AccessFlagsToVkAccessFlags(ACCESS_FLAGS AccessFlags)
{
    return static_cast<VkAccessFlags>(AccessFlags);
}

// clang-format off
ASSERT_SAME(SAMPLE_COUNT_1,  VK_SAMPLE_COUNT_1_BIT);
ASSERT_SAME(SAMPLE_COUNT_2,  VK_SAMPLE_COUNT_2_BIT);
ASSERT_SAME(SAMPLE_COUNT_4,  VK_SAMPLE_COUNT_4_BIT);
ASSERT_SAME(SAMPLE_COUNT_8,  VK_SAMPLE_COUNT_8_BIT);
ASSERT_SAME(SAMPLE_COUNT_16, VK_SAMPLE_COUNT_16_BIT);
ASSERT_SAME(SAMPLE_COUNT_32, VK_SAMPLE_COUNT_32_BIT);
ASSERT_SAME(SAMPLE_COUNT_64, VK_SAMPLE_COUNT_64_BIT);
// clang-format on
SAMPLE_COUNT VkSampleCountFlagsToSampleCount(VkSampleCountFlags Flags)
{
    return static_cast<SAMPLE_COUNT>(Flags);
}
#undef ASSERT_SAME


VkShaderStageFlagBits ShaderTypeToVkShaderStageFlagBit(SHADER_TYPE ShaderType)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the switch below to handle the new shader type");
    VERIFY(IsPowerOfTwo(Uint32{ShaderType}), "More than one shader type is specified");
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:           return VK_SHADER_STAGE_VERTEX_BIT;
        case SHADER_TYPE_HULL:             return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case SHADER_TYPE_DOMAIN:           return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case SHADER_TYPE_GEOMETRY:         return VK_SHADER_STAGE_GEOMETRY_BIT;
        case SHADER_TYPE_PIXEL:            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case SHADER_TYPE_COMPUTE:          return VK_SHADER_STAGE_COMPUTE_BIT;
        case SHADER_TYPE_AMPLIFICATION:    return VK_SHADER_STAGE_TASK_BIT_NV;
        case SHADER_TYPE_MESH:             return VK_SHADER_STAGE_MESH_BIT_NV;
        case SHADER_TYPE_RAY_GEN:          return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case SHADER_TYPE_RAY_MISS:         return VK_SHADER_STAGE_MISS_BIT_KHR;
        case SHADER_TYPE_RAY_CLOSEST_HIT:  return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case SHADER_TYPE_RAY_ANY_HIT:      return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case SHADER_TYPE_RAY_INTERSECTION: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        case SHADER_TYPE_CALLABLE:         return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
        // clang-format on
        case SHADER_TYPE_TILE:
            UNEXPECTED("Unsupported shader type");
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        default:
            UNEXPECTED("Unknown shader type");
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }
}

VkShaderStageFlags ShaderTypesToVkShaderStageFlags(SHADER_TYPE ShaderTypes)
{
    VkShaderStageFlags Result = 0;
    while (ShaderTypes != SHADER_TYPE_UNKNOWN)
    {
        auto Type = ExtractLSB(ShaderTypes);
        Result |= ShaderTypeToVkShaderStageFlagBit(Type);
    }
    return Result;
}

SHADER_TYPE VkShaderStageFlagsToShaderTypes(VkShaderStageFlags StageFlags)
{
    if (StageFlags == VK_SHADER_STAGE_ALL_GRAPHICS)
    {
        return SHADER_TYPE_ALL_GRAPHICS;
    }
    else if (StageFlags == VK_SHADER_STAGE_ALL)
    {
        static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the return value below");
        return SHADER_TYPE_ALL_GRAPHICS | SHADER_TYPE_COMPUTE | SHADER_TYPE_ALL_MESH | SHADER_TYPE_ALL_RAY_TRACING;
    }

    SHADER_TYPE Result = SHADER_TYPE_UNKNOWN;
    while (StageFlags != 0)
    {
        auto Type = ExtractLSB(StageFlags);

        static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the switch below to handle the new shader type");
        switch (Type)
        {
            // clang-format off
            case VK_SHADER_STAGE_VERTEX_BIT:                  Result |= SHADER_TYPE_VERTEX;           break;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    Result |= SHADER_TYPE_HULL;             break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: Result |= SHADER_TYPE_DOMAIN;           break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:                Result |= SHADER_TYPE_GEOMETRY;         break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:                Result |= SHADER_TYPE_PIXEL;            break;
            case VK_SHADER_STAGE_COMPUTE_BIT:                 Result |= SHADER_TYPE_COMPUTE;          break;
            case VK_SHADER_STAGE_TASK_BIT_NV:                 Result |= SHADER_TYPE_AMPLIFICATION;    break;
            case VK_SHADER_STAGE_MESH_BIT_NV:                 Result |= SHADER_TYPE_MESH;             break;
            case VK_SHADER_STAGE_RAYGEN_BIT_KHR:              Result |= SHADER_TYPE_RAY_GEN;          break;
            case VK_SHADER_STAGE_MISS_BIT_KHR:                Result |= SHADER_TYPE_RAY_MISS;         break;
            case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:         Result |= SHADER_TYPE_RAY_CLOSEST_HIT;  break;
            case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:             Result |= SHADER_TYPE_RAY_ANY_HIT;      break;
            case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:        Result |= SHADER_TYPE_RAY_INTERSECTION; break;
            case VK_SHADER_STAGE_CALLABLE_BIT_KHR:            Result |= SHADER_TYPE_CALLABLE;         break;
            // clang-format on
            default:
                UNEXPECTED("Unknown shader type");
        }
    }
    return Result;
}

VkBuildAccelerationStructureFlagsKHR BuildASFlagsToVkBuildAccelerationStructureFlags(RAYTRACING_BUILD_AS_FLAGS Flags)
{
    static_assert(RAYTRACING_BUILD_AS_FLAG_LAST == RAYTRACING_BUILD_AS_LOW_MEMORY,
                  "Please update the switch below to handle the new ray tracing build flag");

    VkBuildAccelerationStructureFlagsKHR Result = 0;
    while (Flags != RAYTRACING_BUILD_AS_NONE)
    {
        auto FlagBit = ExtractLSB(Flags);
        switch (FlagBit)
        {
            // clang-format off
            case RAYTRACING_BUILD_AS_ALLOW_UPDATE:      Result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;      break;
            case RAYTRACING_BUILD_AS_ALLOW_COMPACTION:  Result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;  break;
            case RAYTRACING_BUILD_AS_PREFER_FAST_TRACE: Result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; break;
            case RAYTRACING_BUILD_AS_PREFER_FAST_BUILD: Result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR; break;
            case RAYTRACING_BUILD_AS_LOW_MEMORY:        Result |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;        break;
            // clang-format on
            default: UNEXPECTED("unknown build AS flag");
        }
    }
    return Result;
}

VkGeometryFlagsKHR GeometryFlagsToVkGeometryFlags(RAYTRACING_GEOMETRY_FLAGS Flags)
{
    static_assert(RAYTRACING_GEOMETRY_FLAG_LAST == RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANY_HIT_INVOCATION,
                  "Please update the switch below to handle the new ray tracing geometry flag");

    VkGeometryFlagsKHR Result = 0;
    while (Flags != RAYTRACING_GEOMETRY_FLAG_NONE)
    {
        auto FlagBit = ExtractLSB(Flags);
        switch (FlagBit)
        {
            // clang-format off
            case RAYTRACING_GEOMETRY_FLAG_OPAQUE:                          Result |= VK_GEOMETRY_OPAQUE_BIT_KHR;                          break;
            case RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANY_HIT_INVOCATION: Result |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR; break;
            // clang-format on
            default: UNEXPECTED("unknown geometry flag");
        }
    }
    return Result;
}

VkGeometryInstanceFlagsKHR InstanceFlagsToVkGeometryInstanceFlags(RAYTRACING_INSTANCE_FLAGS Flags)
{
    static_assert(RAYTRACING_INSTANCE_FLAG_LAST == RAYTRACING_INSTANCE_FORCE_NO_OPAQUE,
                  "Please update the switch below to handle the new ray tracing instance flag");

    VkGeometryInstanceFlagsKHR Result = 0;
    while (Flags != RAYTRACING_INSTANCE_NONE)
    {
        auto FlagBit = ExtractLSB(Flags);
        switch (FlagBit)
        {
            // clang-format off
            case RAYTRACING_INSTANCE_TRIANGLE_FACING_CULL_DISABLE:    Result |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;    break;
            case RAYTRACING_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE: Result |= VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR; break;
            case RAYTRACING_INSTANCE_FORCE_OPAQUE:                    Result |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;                    break;
            case RAYTRACING_INSTANCE_FORCE_NO_OPAQUE:                 Result |= VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;                 break;
            // clang-format on
            default: UNEXPECTED("unknown instance flag");
        }
    }
    return Result;
}

VkCopyAccelerationStructureModeKHR CopyASModeToVkCopyAccelerationStructureMode(COPY_AS_MODE Mode)
{
    static_assert(COPY_AS_MODE_LAST == COPY_AS_MODE_COMPACT,
                  "Please update the switch below to handle the new copy AS mode");

    switch (Mode)
    {
        // clang-format off
        case COPY_AS_MODE_CLONE:   return VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR;
        case COPY_AS_MODE_COMPACT: return VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
        // clang-format on
        default:
            UNEXPECTED("unknown AS copy mode");
            return VK_COPY_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;
    }
}

WAVE_FEATURE VkSubgroupFeatureFlagsToWaveFeatures(VkSubgroupFeatureFlags FeatureFlags)
{
    WAVE_FEATURE Result = WAVE_FEATURE_UNKNOWN;
    while (FeatureFlags != 0)
    {
        auto Feature = ExtractLSB(FeatureFlags);
        static_assert(WAVE_FEATURE_LAST == WAVE_FEATURE_QUAD,
                      "Please update the switch below to handle the new wave feature");
        switch (Feature)
        {
            // clang-format off
            case VK_SUBGROUP_FEATURE_BASIC_BIT:            Result |= WAVE_FEATURE_BASIC;            break;
            case VK_SUBGROUP_FEATURE_VOTE_BIT:             Result |= WAVE_FEATURE_VOTE;             break;
            case VK_SUBGROUP_FEATURE_ARITHMETIC_BIT:       Result |= WAVE_FEATURE_ARITHMETIC;       break;
            case VK_SUBGROUP_FEATURE_BALLOT_BIT:           Result |= WAVE_FEATURE_BALLOUT;          break;
            case VK_SUBGROUP_FEATURE_SHUFFLE_BIT:          Result |= WAVE_FEATURE_SHUFFLE;          break;
            case VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT: Result |= WAVE_FEATURE_SHUFFLE_RELATIVE; break;
            case VK_SUBGROUP_FEATURE_CLUSTERED_BIT:        Result |= WAVE_FEATURE_CLUSTERED;        break;
            case VK_SUBGROUP_FEATURE_QUAD_BIT:             Result |= WAVE_FEATURE_QUAD;             break;
            // clang-format on
            default:
                // Don't handle unknown features
                break;
        }
    }
    return Result;
}

ADAPTER_TYPE VkPhysicalDeviceTypeToAdapterType(VkPhysicalDeviceType DeviceType)
{
    switch (DeviceType)
    {
        // clang-format off
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return ADAPTER_TYPE_INTEGRATED;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return ADAPTER_TYPE_DISCRETE;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:            return ADAPTER_TYPE_SOFTWARE;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        default:                                     return ADAPTER_TYPE_UNKNOWN;
            // clang-format on
    }
}

COMMAND_QUEUE_TYPE VkQueueFlagsToCmdQueueType(VkQueueFlags QueueFlags)
{
    static_assert(COMMAND_QUEUE_TYPE_MAX_BIT == 0x7, "Please update the code below to handle the new context type");

    COMMAND_QUEUE_TYPE Result = COMMAND_QUEUE_TYPE_UNKNOWN;

    if (QueueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
        Result |= COMMAND_QUEUE_TYPE_SPARSE_BINDING;

    if ((QueueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        return Result | COMMAND_QUEUE_TYPE_GRAPHICS;

    if ((QueueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
        return Result | COMMAND_QUEUE_TYPE_COMPUTE;

    if ((QueueFlags & VK_QUEUE_TRANSFER_BIT) != 0)
        return Result | COMMAND_QUEUE_TYPE_TRANSFER;

    return COMMAND_QUEUE_TYPE_UNKNOWN;
}

VkQueueGlobalPriorityEXT QueuePriorityToVkQueueGlobalPriority(QUEUE_PRIORITY Priority)
{
    static_assert(QUEUE_PRIORITY_LAST == 4, "Please update the switch below to handle the new queue priority");
    switch (Priority)
    {
        // clang-format off
        case QUEUE_PRIORITY_LOW:      return VK_QUEUE_GLOBAL_PRIORITY_LOW_EXT;
        case QUEUE_PRIORITY_MEDIUM:   return VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT;
        case QUEUE_PRIORITY_HIGH:     return VK_QUEUE_GLOBAL_PRIORITY_HIGH_EXT;
        case QUEUE_PRIORITY_REALTIME: return VK_QUEUE_GLOBAL_PRIORITY_REALTIME_EXT;
        // clang-format on
        default:
            UNEXPECTED("Unexpected queue priority");
            return VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT;
    }
}

VkExtent2D ShadingRateToVkFragmentSize(SHADING_RATE Rate)
{
    VkExtent2D Result;
    Result.width  = 1u << ((Uint32{Rate} >> SHADING_RATE_X_SHIFT) & 0x3);
    Result.height = 1u << (Uint32{Rate} & 0x3);
    VERIFY_EXPR(Result.width > 0 && Result.height > 0);
    VERIFY_EXPR(Result.width <= (1u << AXIS_SHADING_RATE_MAX) && Result.height <= (1u << AXIS_SHADING_RATE_MAX));
    VERIFY_EXPR(IsPowerOfTwo(Result.width) && IsPowerOfTwo(Result.height));
    return Result;
}

SHADING_RATE VkFragmentSizeToShadingRate(const VkExtent2D& Size)
{
    VERIFY_EXPR(IsPowerOfTwo(Size.width) && IsPowerOfTwo(Size.height));
    auto X = PlatformMisc::GetMSB(Size.width);
    auto Y = PlatformMisc::GetMSB(Size.height);
    VERIFY_EXPR((1u << X) == Size.width);
    VERIFY_EXPR((1u << Y) == Size.height);
    return static_cast<SHADING_RATE>((X << SHADING_RATE_X_SHIFT) | Y);
}

VkFragmentShadingRateCombinerOpKHR ShadingRateCombinerToVkFragmentShadingRateCombinerOp(SHADING_RATE_COMBINER Combiner)
{
    static_assert(SHADING_RATE_COMBINER_LAST == (1u << 5), "Please update the switch below to handle the new shading rate combiner");
    VERIFY(IsPowerOfTwo(Uint32{Combiner}), "Only single combiner should be provided");
    switch (Combiner)
    {
        // clang-format off
        case SHADING_RATE_COMBINER_PASSTHROUGH: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
        case SHADING_RATE_COMBINER_OVERRIDE:    return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
        case SHADING_RATE_COMBINER_MIN:         return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR;
        case SHADING_RATE_COMBINER_MAX:         return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
        case SHADING_RATE_COMBINER_SUM:
        case SHADING_RATE_COMBINER_MUL:         return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR;
        // clang-format on
        default:
            UNEXPECTED("Unexpected shading rate combiner");
            return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_ENUM_KHR;
    }
}

DeviceFeatures VkFeaturesToDeviceFeatures(uint32_t                                                          vkVersion,
                                          const VkPhysicalDeviceFeatures&                                   vkFeatures,
                                          const VkPhysicalDeviceProperties&                                 vkDeviceProps,
                                          const VulkanUtilities::VulkanPhysicalDevice::ExtensionFeatures&   ExtFeatures,
                                          const VulkanUtilities::VulkanPhysicalDevice::ExtensionProperties& ExtProps,
                                          DEVICE_FEATURE_STATE                                              OptionalState)
{
    VERIFY_EXPR(OptionalState != DEVICE_FEATURE_STATE_DISABLED);

    DeviceFeatures Features;

    // Enable features
#define INIT_FEATURE(FeatureName, Supported) \
    Features.FeatureName = (Supported) ? OptionalState : DEVICE_FEATURE_STATE_DISABLED;

    // The following features are always enabled
    Features.SeparablePrograms             = DEVICE_FEATURE_STATE_ENABLED;
    Features.ShaderResourceQueries         = DEVICE_FEATURE_STATE_ENABLED;
    Features.MultithreadedResourceCreation = DEVICE_FEATURE_STATE_ENABLED;
    Features.ComputeShaders                = DEVICE_FEATURE_STATE_ENABLED;
    Features.BindlessResources             = DEVICE_FEATURE_STATE_ENABLED;
    Features.BinaryOcclusionQueries        = DEVICE_FEATURE_STATE_ENABLED;
    Features.SubpassFramebufferFetch       = DEVICE_FEATURE_STATE_ENABLED;
    Features.TextureComponentSwizzle       = DEVICE_FEATURE_STATE_ENABLED;

    // Timestamps are not a feature and can't be disabled. They are either supported by the device, or not.
    Features.TimestampQueries = vkDeviceProps.limits.timestampComputeAndGraphics ? DEVICE_FEATURE_STATE_ENABLED : DEVICE_FEATURE_STATE_DISABLED;
    Features.DurationQueries  = vkDeviceProps.limits.timestampComputeAndGraphics ? DEVICE_FEATURE_STATE_ENABLED : DEVICE_FEATURE_STATE_DISABLED;

    // clang-format off
    INIT_FEATURE(GeometryShaders,                   vkFeatures.geometryShader);
    INIT_FEATURE(Tessellation,                      vkFeatures.tessellationShader);
    INIT_FEATURE(PipelineStatisticsQueries,         vkFeatures.pipelineStatisticsQuery);
    INIT_FEATURE(OcclusionQueries,                  vkFeatures.occlusionQueryPrecise);
    INIT_FEATURE(WireframeFill,                     vkFeatures.fillModeNonSolid);
    INIT_FEATURE(DepthBiasClamp,                    vkFeatures.depthBiasClamp);
    INIT_FEATURE(DepthClamp,                        vkFeatures.depthClamp);
    INIT_FEATURE(IndependentBlend,                  vkFeatures.independentBlend);
    INIT_FEATURE(DualSourceBlend,                   vkFeatures.dualSrcBlend);
    INIT_FEATURE(MultiViewport,                     vkFeatures.multiViewport);
    INIT_FEATURE(TextureCompressionBC,              vkFeatures.textureCompressionBC);
    INIT_FEATURE(VertexPipelineUAVWritesAndAtomics, vkFeatures.vertexPipelineStoresAndAtomics);
    INIT_FEATURE(PixelUAVWritesAndAtomics,          vkFeatures.fragmentStoresAndAtomics);
    INIT_FEATURE(TextureUAVExtendedFormats,         vkFeatures.shaderStorageImageExtendedFormats);
    INIT_FEATURE(SparseResources,                   vkFeatures.sparseBinding && (vkFeatures.sparseResidencyBuffer || vkFeatures.sparseResidencyImage2D)); // requires support for resident resources
    // clang-format on

    const auto& MeshShaderFeats = ExtFeatures.MeshShader;
    INIT_FEATURE(MeshShaders, MeshShaderFeats.taskShader != VK_FALSE && MeshShaderFeats.meshShader != VK_FALSE);

    const auto& ShaderFloat16Int8Feats = ExtFeatures.ShaderFloat16Int8;
    // clang-format off
    INIT_FEATURE(ShaderFloat16, ShaderFloat16Int8Feats.shaderFloat16 != VK_FALSE);
    INIT_FEATURE(ShaderInt8,    ShaderFloat16Int8Feats.shaderInt8    != VK_FALSE);
    // clang-format on

    const auto& Storage16BitFeats = ExtFeatures.Storage16Bit;
    // clang-format off
    INIT_FEATURE(ResourceBuffer16BitAccess, Storage16BitFeats.storageBuffer16BitAccess           != VK_FALSE && vkFeatures.shaderInt16 != VK_FALSE);
    INIT_FEATURE(UniformBuffer16BitAccess,  Storage16BitFeats.uniformAndStorageBuffer16BitAccess != VK_FALSE && vkFeatures.shaderInt16 != VK_FALSE);
    INIT_FEATURE(ShaderInputOutput16,       Storage16BitFeats.storageInputOutput16               != VK_FALSE && vkFeatures.shaderInt16 != VK_FALSE);
    // clang-format on

    const auto& Storage8BitFeats = ExtFeatures.Storage8Bit;
    // clang-format off
    INIT_FEATURE(ResourceBuffer8BitAccess, Storage8BitFeats.storageBuffer8BitAccess           != VK_FALSE);
    INIT_FEATURE(UniformBuffer8BitAccess,  Storage8BitFeats.uniformAndStorageBuffer8BitAccess != VK_FALSE);
    // clang-format on

    const auto& DescrIndexingFeats = ExtFeatures.DescriptorIndexing;
    INIT_FEATURE(ShaderResourceRuntimeArray, DescrIndexingFeats.runtimeDescriptorArray != VK_FALSE);
    const auto& AccelStructFeats = ExtFeatures.AccelStruct;
    const auto& RayTracingFeats  = ExtFeatures.RayTracingPipeline;
    const auto& RayQueryFeats    = ExtFeatures.RayQuery;
    // clang-format off
    INIT_FEATURE(RayTracing,
                 vkVersion                              >= VK_API_VERSION_1_1 &&
                 AccelStructFeats.accelerationStructure != VK_FALSE           &&
                 (RayTracingFeats.rayTracingPipeline != VK_FALSE || RayQueryFeats.rayQuery != VK_FALSE));
    // clang-format on

    const auto& SubgroupProps          = ExtProps.Subgroup;
    const auto  RequiredSubgroupFeats  = VK_SUBGROUP_FEATURE_BASIC_BIT;
    const auto  RequiredSubgroupStages = VK_SHADER_STAGE_COMPUTE_BIT;
    Features.WaveOp =
        (vkVersion >= VK_API_VERSION_1_1 &&
         (SubgroupProps.supportedOperations & RequiredSubgroupFeats) == RequiredSubgroupFeats &&
         (SubgroupProps.supportedStages & RequiredSubgroupStages) == RequiredSubgroupStages) ?
        DEVICE_FEATURE_STATE_ENABLED :
        DEVICE_FEATURE_STATE_DISABLED;

    const auto& VertexAttribDivisorFeats = ExtFeatures.VertexAttributeDivisor;
    INIT_FEATURE(InstanceDataStepRate,
                 (VertexAttribDivisorFeats.vertexAttributeInstanceRateDivisor != VK_FALSE &&
                  VertexAttribDivisorFeats.vertexAttributeInstanceRateZeroDivisor != VK_FALSE));

    const auto& TimelineSemaphoreFeats = ExtFeatures.TimelineSemaphore;
    INIT_FEATURE(NativeFence,
                 TimelineSemaphoreFeats.timelineSemaphore != VK_FALSE);

    INIT_FEATURE(TileShaders, false); // Not currently supported

    INIT_FEATURE(TransferQueueTimestampQueries,
                 ExtFeatures.HostQueryReset.hostQueryReset != VK_FALSE);

    INIT_FEATURE(VariableRateShading,
                 (ExtFeatures.ShadingRate.pipelineFragmentShadingRate != VK_FALSE ||
                  ExtFeatures.ShadingRate.primitiveFragmentShadingRate != VK_FALSE ||
                  ExtFeatures.ShadingRate.attachmentFragmentShadingRate != VK_FALSE ||
                  ExtFeatures.FragmentDensityMap.fragmentDensityMap != VK_FALSE));

#undef INIT_FEATURE

    // Not supported in Vulkan on top of Metal.
#if PLATFORM_MACOS || PLATFORM_IOS || PLATFORM_TVOS
    Features.BinaryOcclusionQueries = DEVICE_FEATURE_STATE_DISABLED;
    Features.TimestampQueries       = DEVICE_FEATURE_STATE_DISABLED;
    Features.DurationQueries        = DEVICE_FEATURE_STATE_DISABLED;
#endif

    ASSERT_SIZEOF(DeviceFeatures, 41, "Did you add a new feature to DeviceFeatures? Please handle its status here (if necessary).");

    return Features;
}

SPARSE_TEXTURE_FLAGS VkSparseImageFormatFlagsToSparseTextureFlags(VkSparseImageFormatFlags Flags)
{
    SPARSE_TEXTURE_FLAGS Result = SPARSE_TEXTURE_FLAG_NONE;
    while (Flags != 0)
    {
        auto FlagBit = static_cast<VkSparseImageFormatFlagBits>(ExtractLSB(Flags));
        static_assert(SPARSE_TEXTURE_FLAG_LAST == (1u << 2), "This function must be updated to handle new sparse texture flag");
        switch (FlagBit)
        {
            // clang-format off
            case VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT:         Result |= SPARSE_TEXTURE_FLAG_SINGLE_MIPTAIL;         break;
            case VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT:       Result |= SPARSE_TEXTURE_FLAG_ALIGNED_MIP_SIZE;       break;
            case VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT: Result |= SPARSE_TEXTURE_FLAG_NONSTANDARD_BLOCK_SIZE; break;
            // clang-format on
            default:
                UNEXPECTED("Unexpected sparse image format flag");
        }
    }
    return Result;
}

VkImageUsageFlags BindFlagsToVkImageUsage(BIND_FLAGS Flags, bool IsMemoryless, bool FragDensityMapInsteadOfShadingRate)
{
    VkImageUsageFlags Result = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    while (Flags != BIND_NONE)
    {
        auto FlagBit = ExtractLSB(Flags);
        static_assert(BIND_FLAG_LAST == (1u << 11), "This function must be updated to handle new bind flag");
        switch (FlagBit)
        {
            case BIND_RENDER_TARGET:
                // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required for vkCmdClearColorImage
                Result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                break;
            case BIND_DEPTH_STENCIL:
                // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required for vkCmdClearDepthStencilImage()
                Result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                break;
            case BIND_UNORDERED_ACCESS:
                Result |= VK_IMAGE_USAGE_STORAGE_BIT;
                break;
            case BIND_SHADER_RESOURCE:
                Result |= VK_IMAGE_USAGE_SAMPLED_BIT;
                break;
            case BIND_INPUT_ATTACHMENT:
                Result |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                break;
            case BIND_SHADING_RATE:
                Result |= (FragDensityMapInsteadOfShadingRate ? VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT : VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR);
                break;
            default:
                UNEXPECTED("Unexpected bind flag");
        }
    }
    if (IsMemoryless)
    {
        Result &= (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
        Result |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    }
    return Result;
}

void GetAllowedStagesAndAccessMask(BIND_FLAGS Flags, VkPipelineStageFlags& StageMask, VkAccessFlags& AccessMask)
{
    StageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
    AccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    while (Flags != BIND_NONE)
    {
        auto FlagBit = ExtractLSB(Flags);
        static_assert(BIND_FLAG_LAST == (1u << 11), "This function must be updated to handle new bind flag");
        switch (FlagBit)
        {
            case BIND_VERTEX_BUFFER:
                StageMask |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                AccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                break;
            case BIND_INDEX_BUFFER:
                StageMask |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                AccessMask |= VK_ACCESS_INDEX_READ_BIT;
                break;
            case BIND_UNIFORM_BUFFER:
                StageMask |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
                AccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
                break;
            case BIND_SHADER_RESOURCE:
                StageMask |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
                AccessMask |= VK_ACCESS_SHADER_READ_BIT;
                break;
            case BIND_RENDER_TARGET:
                StageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                AccessMask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case BIND_DEPTH_STENCIL:
                StageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                AccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case BIND_UNORDERED_ACCESS:
                StageMask |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_SHADERS;
                AccessMask |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                break;
            case BIND_INDIRECT_DRAW_ARGS:
                StageMask |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
                AccessMask |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                break;
            case BIND_INPUT_ATTACHMENT:
                StageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                AccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                break;
            case BIND_RAY_TRACING:
                StageMask |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
                AccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
                break;
            case BIND_SHADING_RATE:
                StageMask |= VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT | VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
                AccessMask |= VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT | VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
                break;

            case BIND_NONE:
            case BIND_STREAM_OUTPUT:
            default:
                UNEXPECTED("Unexpected bind flag");
                break;
        }
    }
}

VkComponentSwizzle TextureComponentSwizzleToVkComponentSwizzle(TEXTURE_COMPONENT_SWIZZLE Swizzle)
{
    static_assert(TEXTURE_COMPONENT_SWIZZLE_COUNT == 7, "Did you add a new swizzle type? Please handle it here.");
    // clang-format off
    static_assert(static_cast<VkComponentSwizzle>(TEXTURE_COMPONENT_SWIZZLE_IDENTITY) == VK_COMPONENT_SWIZZLE_IDENTITY, "Unexpected value of TEXTURE_COMPONENT_SWIZZLE_IDENTITY enum.");
    static_assert(static_cast<VkComponentSwizzle>(TEXTURE_COMPONENT_SWIZZLE_ZERO)     == VK_COMPONENT_SWIZZLE_ZERO,     "Unexpected value of TEXTURE_COMPONENT_SWIZZLE_ZERO enum.");
    static_assert(static_cast<VkComponentSwizzle>(TEXTURE_COMPONENT_SWIZZLE_ONE)      == VK_COMPONENT_SWIZZLE_ONE,      "Unexpected value of TEXTURE_COMPONENT_SWIZZLE_ONE enum.");
    static_assert(static_cast<VkComponentSwizzle>(TEXTURE_COMPONENT_SWIZZLE_R)        == VK_COMPONENT_SWIZZLE_R,        "Unexpected value of TEXTURE_COMPONENT_SWIZZLE_R enum.");
    static_assert(static_cast<VkComponentSwizzle>(TEXTURE_COMPONENT_SWIZZLE_G)        == VK_COMPONENT_SWIZZLE_G,        "Unexpected value of TEXTURE_COMPONENT_SWIZZLE_G enum.");
    static_assert(static_cast<VkComponentSwizzle>(TEXTURE_COMPONENT_SWIZZLE_B)        == VK_COMPONENT_SWIZZLE_B,        "Unexpected value of TEXTURE_COMPONENT_SWIZZLE_B enum.");
    static_assert(static_cast<VkComponentSwizzle>(TEXTURE_COMPONENT_SWIZZLE_A)        == VK_COMPONENT_SWIZZLE_A,        "Unexpected value of TEXTURE_COMPONENT_SWIZZLE_A enum.");
    // clang-format on
    return static_cast<VkComponentSwizzle>(Swizzle);
}

VkComponentMapping TextureComponentMappingToVkComponentMapping(const TextureComponentMapping& Mapping)
{
    return VkComponentMapping{
        TextureComponentSwizzleToVkComponentSwizzle(Mapping.R),
        TextureComponentSwizzleToVkComponentSwizzle(Mapping.G),
        TextureComponentSwizzleToVkComponentSwizzle(Mapping.B),
        TextureComponentSwizzleToVkComponentSwizzle(Mapping.A) //
    };
}

} // namespace Diligent
