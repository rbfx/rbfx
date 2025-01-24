/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include "SPIRVUtils.hpp"
#include "SPIRVShaderResources.hpp" // required for diligent_spirv_cross

#include "spirv_cross.hpp"

namespace Diligent
{

static spv::ImageFormat TextureFormatToSpvImageFormat(TEXTURE_FORMAT Format)
{
    switch (Format)
    {
            // clang-format off
        case TEX_FORMAT_RGBA8_UNORM: return spv::ImageFormatRgba8;
        case TEX_FORMAT_RG8_UNORM:   return spv::ImageFormatRg8;
        case TEX_FORMAT_R8_UNORM:    return spv::ImageFormatR8;

        case TEX_FORMAT_RGBA8_SNORM: return spv::ImageFormatRgba8Snorm;
        case TEX_FORMAT_RG8_SNORM:   return spv::ImageFormatRg8Snorm;
        case TEX_FORMAT_R8_SNORM:    return spv::ImageFormatR8Snorm;

		case TEX_FORMAT_RGBA8_UINT: return spv::ImageFormatRgba8ui;
        case TEX_FORMAT_RG8_UINT:   return spv::ImageFormatRg8ui;
        case TEX_FORMAT_R8_UINT:    return spv::ImageFormatR8ui;

        case TEX_FORMAT_RGBA8_SINT: return spv::ImageFormatRgba8i;
        case TEX_FORMAT_RG8_SINT:   return spv::ImageFormatRg8i;
        case TEX_FORMAT_R8_SINT:    return spv::ImageFormatR8i;

		case TEX_FORMAT_RGBA16_UNORM: return spv::ImageFormatRgba16;
        case TEX_FORMAT_RG16_UNORM:   return spv::ImageFormatRg16;
		case TEX_FORMAT_R16_UNORM:    return spv::ImageFormatR16;

		case TEX_FORMAT_RGBA16_SNORM: return spv::ImageFormatRgba16Snorm;
		case TEX_FORMAT_RG16_SNORM:   return spv::ImageFormatRg16Snorm;
		case TEX_FORMAT_R16_SNORM:    return spv::ImageFormatR16Snorm;

        case TEX_FORMAT_RGBA16_UINT: return spv::ImageFormatRgba16ui;
		case TEX_FORMAT_RG16_UINT:   return spv::ImageFormatRg16ui;
		case TEX_FORMAT_R16_UINT:    return spv::ImageFormatR16ui;

		case TEX_FORMAT_RGBA16_SINT: return spv::ImageFormatRgba16i;
		case TEX_FORMAT_RG16_SINT:   return spv::ImageFormatRg16i;
		case TEX_FORMAT_R16_SINT:    return spv::ImageFormatR16i;

        case TEX_FORMAT_RGBA32_UINT: return spv::ImageFormatRgba32ui;
        case TEX_FORMAT_RG32_UINT:   return spv::ImageFormatRg32ui;
        case TEX_FORMAT_R32_UINT:    return spv::ImageFormatR32ui;

        case TEX_FORMAT_RGBA32_SINT: return spv::ImageFormatRgba32i;
        case TEX_FORMAT_RG32_SINT:   return spv::ImageFormatRg32i;
        case TEX_FORMAT_R32_SINT:    return spv::ImageFormatR32i;

		case TEX_FORMAT_RGBA32_FLOAT: return spv::ImageFormatRgba32f;
        case TEX_FORMAT_RG32_FLOAT:   return spv::ImageFormatRg32f;
		case TEX_FORMAT_R32_FLOAT:    return spv::ImageFormatR32f;

		case TEX_FORMAT_RGBA16_FLOAT: return spv::ImageFormatRgba16f;
		case TEX_FORMAT_RG16_FLOAT:   return spv::ImageFormatRg16f;
        case TEX_FORMAT_R16_FLOAT:    return spv::ImageFormatR16f;

        case TEX_FORMAT_R11G11B10_FLOAT: return spv::ImageFormatR11fG11fB10f;
        case TEX_FORMAT_RGB10A2_UNORM:   return spv::ImageFormatRgb10A2;
        case TEX_FORMAT_RGB10A2_UINT:    return spv::ImageFormatRgb10a2ui;

        // clang-format on
        default:
            return spv::ImageFormatUnknown;
    }
}

std::vector<uint32_t> PatchImageFormats(const std::vector<uint32_t>&                                SPIRV,
                                        const std::unordered_map<HashMapStringKey, TEXTURE_FORMAT>& ImageFormats)
{
    diligent_spirv_cross::Compiler Compiler{SPIRV};

    diligent_spirv_cross::ShaderResources resources = Compiler.get_shader_resources();

    std::unordered_map<uint32_t, uint32_t> ImageTypeIdToFormat;
    for (uint32_t i = 0; i < SPIRV.size(); ++i)
    {
        // OpTypeImage
        //      0          1          2          3      4        5      6       7           8               9
        // |  OpCode  | Result | Sampled Type | Dim | Depth | Arrayed | MS | Sampled | Image Format | Access Qualifier
        constexpr uint32_t ImageFormatOffset = 8;

        uint32_t OpCode = SPIRV[i] & 0xFFFF;
        if (OpCode == spv::OpTypeImage && i + ImageFormatOffset < SPIRV.size())
        {
            const uint32_t ImageTypeId       = SPIRV[i + 1];
            ImageTypeIdToFormat[ImageTypeId] = i + ImageFormatOffset;
        }
    }

    std::vector<uint32_t> PatchedSPIRV = SPIRV;
    for (const diligent_spirv_cross::Resource& Img : resources.storage_images)
    {
        const diligent_spirv_cross::SPIRType& type = Compiler.get_type(Img.type_id);
        if (type.image.dim == spv::Dim1D ||
            type.image.dim == spv::Dim2D ||
            type.image.dim == spv::Dim3D)
        {
            auto FormatIt = ImageFormats.find(HashMapStringKey{Img.name.c_str()});
            if (FormatIt != ImageFormats.end())
            {
                auto spvFormat = TextureFormatToSpvImageFormat(FormatIt->second);
                if (spvFormat != spv::ImageFormatUnknown)
                {
                    auto FormatOffsetIt = ImageTypeIdToFormat.find(Img.base_type_id);
                    if (FormatOffsetIt != ImageTypeIdToFormat.end())
                    {
                        const uint32_t ImageFormatOffset = FormatOffsetIt->second;
                        uint32_t&      FormatWord        = PatchedSPIRV[ImageFormatOffset];
                        if (FormatWord != static_cast<uint32_t>(type.image.format) &&
                            FormatWord != static_cast<uint32_t>(spvFormat))
                        {
                            LOG_ERROR_MESSAGE("Inconsistent formats encountered while patching format for image '", Img.name,
                                              "'.\nThis likely is the result of the same-format textures using inconsistent format specifiers in HLSL, for example:"
                                              "\n  RWTexture2D<float4/*format=rgba32f>  g_RWTex1;"
                                              "\n  RWTexture2D<float4/*format=rgba32ui> g_RWTex2;");
                        }
                        FormatWord = spvFormat;
                    }
                }
            }
        }
    }

    return PatchedSPIRV;
}

} // namespace Diligent
