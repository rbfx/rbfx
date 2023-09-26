/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include <algorithm>
#include <array>

#include "GraphicsAccessories.hpp"
#include "DebugUtilities.hpp"
#include "Align.hpp"
#include "BasicMath.hpp"
#include "Cast.hpp"
#include "StringTools.hpp"

namespace Diligent
{

const Char* GetValueTypeString(VALUE_TYPE Val)
{
    struct ValueTypeToStringMap
    {
        ValueTypeToStringMap()
        {
#define INIT_VALUE_TYPE_STR(ValType) ValueTypeStrings[ValType] = #ValType
            INIT_VALUE_TYPE_STR(VT_UNDEFINED);
            INIT_VALUE_TYPE_STR(VT_INT8);
            INIT_VALUE_TYPE_STR(VT_INT16);
            INIT_VALUE_TYPE_STR(VT_INT32);
            INIT_VALUE_TYPE_STR(VT_UINT8);
            INIT_VALUE_TYPE_STR(VT_UINT16);
            INIT_VALUE_TYPE_STR(VT_UINT32);
            INIT_VALUE_TYPE_STR(VT_FLOAT16);
            INIT_VALUE_TYPE_STR(VT_FLOAT32);
            INIT_VALUE_TYPE_STR(VT_FLOAT64);
#undef INIT_VALUE_TYPE_STR
            static_assert(VT_NUM_TYPES == 10, "Not all value type strings are initialized.");
        }

        const Char* operator[](VALUE_TYPE Val) const
        {
            if (Val < ValueTypeStrings.size())
            {
                return ValueTypeStrings[Val];
            }
            else
            {
                UNEXPECTED("Incorrect value type (", static_cast<Uint32>(Val), ")");
                return "unknown value type";
            }
        }

    private:
        std::array<const Char*, VT_NUM_TYPES> ValueTypeStrings{};
    };

    static const ValueTypeToStringMap ValueTypeStrings;
    return ValueTypeStrings[Val];
}

class TexFormatToViewFormatConverter
{
public:
    TexFormatToViewFormatConverter()
    {
        // clang-format off
#define INIT_TEX_VIEW_FORMAT_INFO(TexFmt, SRVFmt, RTVFmt, DSVFmt, UAVFmt)\
        {\
            m_ViewFormats[TexFmt][TEXTURE_VIEW_SHADER_RESOURCE-1]         = TEX_FORMAT_##SRVFmt; \
            m_ViewFormats[TexFmt][TEXTURE_VIEW_RENDER_TARGET-1]           = TEX_FORMAT_##RTVFmt; \
            m_ViewFormats[TexFmt][TEXTURE_VIEW_DEPTH_STENCIL-1]           = TEX_FORMAT_##DSVFmt; \
            m_ViewFormats[TexFmt][TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL-1] = TEX_FORMAT_##DSVFmt; \
            m_ViewFormats[TexFmt][TEXTURE_VIEW_UNORDERED_ACCESS-1]        = TEX_FORMAT_##UAVFmt; \
        }
        static_assert(TEXTURE_VIEW_NUM_VIEWS == 7, "Please handle the new view type above, if necessary");

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_UNKNOWN,                 UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA32_TYPELESS,         RGBA32_FLOAT, RGBA32_FLOAT, UNKNOWN, RGBA32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA32_FLOAT,            RGBA32_FLOAT, RGBA32_FLOAT, UNKNOWN, RGBA32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA32_UINT,             RGBA32_UINT,  RGBA32_UINT,  UNKNOWN, RGBA32_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA32_SINT,             RGBA32_SINT,  RGBA32_SINT,  UNKNOWN, RGBA32_SINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGB32_TYPELESS,          RGB32_FLOAT, RGB32_FLOAT, UNKNOWN, RGB32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGB32_FLOAT,             RGB32_FLOAT, RGB32_FLOAT, UNKNOWN, RGB32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGB32_UINT,              RGB32_UINT,  RGB32_UINT,  UNKNOWN, RGB32_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGB32_SINT,              RGB32_SINT,  RGB32_SINT,  UNKNOWN, RGB32_SINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA16_TYPELESS,         RGBA16_FLOAT, RGBA16_FLOAT, UNKNOWN, RGBA16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA16_FLOAT,            RGBA16_FLOAT, RGBA16_FLOAT, UNKNOWN, RGBA16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA16_UNORM,            RGBA16_UNORM, RGBA16_UNORM, UNKNOWN, RGBA16_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA16_UINT,             RGBA16_UINT,  RGBA16_UINT,  UNKNOWN, RGBA16_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA16_SNORM,            RGBA16_SNORM, RGBA16_SNORM, UNKNOWN, RGBA16_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA16_SINT,             RGBA16_SINT,  RGBA16_SINT,  UNKNOWN, RGBA16_SINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG32_TYPELESS,           RG32_FLOAT, RG32_FLOAT, UNKNOWN, RG32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG32_FLOAT,              RG32_FLOAT, RG32_FLOAT, UNKNOWN, RG32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG32_UINT,               RG32_UINT,  RG32_UINT,  UNKNOWN, RG32_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG32_SINT,               RG32_SINT,  RG32_SINT,  UNKNOWN, RG32_SINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R32G8X24_TYPELESS,       R32_FLOAT_X8X24_TYPELESS, UNKNOWN, D32_FLOAT_S8X24_UINT, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_D32_FLOAT_S8X24_UINT,    R32_FLOAT_X8X24_TYPELESS, UNKNOWN, D32_FLOAT_S8X24_UINT, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,R32_FLOAT_X8X24_TYPELESS, UNKNOWN, D32_FLOAT_S8X24_UINT, R32_FLOAT_X8X24_TYPELESS);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_X32_TYPELESS_G8X24_UINT, X32_TYPELESS_G8X24_UINT,  UNKNOWN, D32_FLOAT_S8X24_UINT, X32_TYPELESS_G8X24_UINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGB10A2_TYPELESS,        RGB10A2_UNORM, RGB10A2_UNORM, UNKNOWN, RGB10A2_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGB10A2_UNORM,           RGB10A2_UNORM, RGB10A2_UNORM, UNKNOWN, RGB10A2_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGB10A2_UINT,            RGB10A2_UINT,  RGB10A2_UINT,  UNKNOWN, RGB10A2_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R11G11B10_FLOAT,         R11G11B10_FLOAT, R11G11B10_FLOAT, UNKNOWN, R11G11B10_FLOAT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA8_TYPELESS,          RGBA8_UNORM_SRGB, RGBA8_UNORM_SRGB, UNKNOWN, RGBA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA8_UNORM,             RGBA8_UNORM,      RGBA8_UNORM,      UNKNOWN, RGBA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA8_UNORM_SRGB,        RGBA8_UNORM_SRGB, RGBA8_UNORM_SRGB, UNKNOWN, RGBA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA8_UINT,              RGBA8_UINT,       RGBA8_UINT,       UNKNOWN, RGBA8_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA8_SNORM,             RGBA8_SNORM,      RGBA8_SNORM,      UNKNOWN, RGBA8_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGBA8_SINT,              RGBA8_SINT,       RGBA8_SINT,       UNKNOWN, RGBA8_SINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG16_TYPELESS,           RG16_FLOAT, RG16_FLOAT, UNKNOWN, RG16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG16_FLOAT,              RG16_FLOAT, RG16_FLOAT, UNKNOWN, RG16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG16_UNORM,              RG16_UNORM, RG16_UNORM, UNKNOWN, RG16_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG16_UINT,               RG16_UINT,  RG16_UINT,  UNKNOWN, RG16_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG16_SNORM,              RG16_SNORM, RG16_SNORM, UNKNOWN, RG16_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG16_SINT,               RG16_SINT,  RG16_SINT,  UNKNOWN, RG16_SINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R32_TYPELESS,            R32_FLOAT, R32_FLOAT, D32_FLOAT, R32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_D32_FLOAT,               R32_FLOAT, R32_FLOAT, D32_FLOAT, R32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R32_FLOAT,               R32_FLOAT, R32_FLOAT, D32_FLOAT, R32_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R32_UINT,                R32_UINT,  R32_UINT,  UNKNOWN,   R32_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R32_SINT,                R32_SINT,  R32_SINT,  UNKNOWN,   R32_SINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R24G8_TYPELESS,          R24_UNORM_X8_TYPELESS, UNKNOWN, D24_UNORM_S8_UINT, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_D24_UNORM_S8_UINT,       R24_UNORM_X8_TYPELESS, UNKNOWN, D24_UNORM_S8_UINT, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R24_UNORM_X8_TYPELESS,   R24_UNORM_X8_TYPELESS, UNKNOWN, D24_UNORM_S8_UINT, R24_UNORM_X8_TYPELESS);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_X24_TYPELESS_G8_UINT,    X24_TYPELESS_G8_UINT,  UNKNOWN, D24_UNORM_S8_UINT, X24_TYPELESS_G8_UINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG8_TYPELESS,            RG8_UNORM, RG8_UNORM, UNKNOWN, RG8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG8_UNORM,               RG8_UNORM, RG8_UNORM, UNKNOWN, RG8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG8_UINT,                RG8_UINT,  RG8_UINT,  UNKNOWN, RG8_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG8_SNORM,               RG8_SNORM, RG8_SNORM, UNKNOWN, RG8_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG8_SINT,                RG8_SINT,  RG8_SINT,  UNKNOWN, RG8_SINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R16_TYPELESS,            R16_FLOAT, R16_FLOAT, UNKNOWN,   R16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R16_FLOAT,               R16_FLOAT, R16_FLOAT, UNKNOWN,   R16_FLOAT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_D16_UNORM,               R16_UNORM, R16_UNORM, D16_UNORM, R16_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R16_UNORM,               R16_UNORM, R16_UNORM, D16_UNORM, R16_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R16_UINT,                R16_UINT,  R16_UINT,  UNKNOWN,   R16_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R16_SNORM,               R16_SNORM, R16_SNORM, UNKNOWN,   R16_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R16_SINT,                R16_SINT,  R16_SINT,  UNKNOWN,   R16_SINT);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R8_TYPELESS,             R8_UNORM, R8_UNORM, UNKNOWN, R8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R8_UNORM,                R8_UNORM, R8_UNORM, UNKNOWN, R8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R8_UINT,                 R8_UINT,  R8_UINT,  UNKNOWN, R8_UINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R8_SNORM,                R8_SNORM, R8_SNORM, UNKNOWN, R8_SNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R8_SINT,                 R8_SINT,  R8_SINT,  UNKNOWN, R8_SINT);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_A8_UNORM,                A8_UNORM, A8_UNORM, UNKNOWN, A8_UNORM);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R1_UNORM,                R1_UNORM, R1_UNORM, UNKNOWN, R1_UNORM);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RGB9E5_SHAREDEXP,        RGB9E5_SHAREDEXP, RGB9E5_SHAREDEXP, UNKNOWN, RGB9E5_SHAREDEXP);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_RG8_B8G8_UNORM,          RG8_B8G8_UNORM,  RG8_B8G8_UNORM,  UNKNOWN, RG8_B8G8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_G8R8_G8B8_UNORM,         G8R8_G8B8_UNORM, G8R8_G8B8_UNORM, UNKNOWN, G8R8_G8B8_UNORM);

        // http://www.g-truc.net/post-0335.html
        // http://renderingpipeline.com/2012/07/texture-compression/
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC1_TYPELESS,            BC1_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC1_UNORM,               BC1_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC1_UNORM_SRGB,          BC1_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC2_TYPELESS,            BC2_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC2_UNORM,               BC2_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC2_UNORM_SRGB,          BC2_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC3_TYPELESS,            BC3_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC3_UNORM,               BC3_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC3_UNORM_SRGB,          BC3_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC4_TYPELESS,            BC4_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC4_UNORM,               BC4_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC4_SNORM,               BC4_SNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC5_TYPELESS,            BC5_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC5_UNORM,               BC5_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC5_SNORM,               BC5_SNORM,      UNKNOWN, UNKNOWN, UNKNOWN);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_B5G6R5_UNORM,            B5G6R5_UNORM,   B5G6R5_UNORM,    UNKNOWN, B5G6R5_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_B5G5R5A1_UNORM,          B5G5R5A1_UNORM, B5G5R5A1_UNORM,  UNKNOWN, B5G5R5A1_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BGRA8_UNORM,             BGRA8_UNORM,    BGRA8_UNORM,     UNKNOWN, BGRA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BGRX8_UNORM,             BGRX8_UNORM,    BGRX8_UNORM,     UNKNOWN, BGRX8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, R10G10B10_XR_BIAS_A2_UNORM, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BGRA8_TYPELESS,          BGRA8_UNORM_SRGB, BGRA8_UNORM_SRGB, UNKNOWN, BGRA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BGRA8_UNORM_SRGB,        BGRA8_UNORM_SRGB, BGRA8_UNORM_SRGB, UNKNOWN, BGRA8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BGRX8_TYPELESS,          BGRX8_UNORM_SRGB, BGRX8_UNORM_SRGB, UNKNOWN, BGRX8_UNORM);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BGRX8_UNORM_SRGB,        BGRX8_UNORM_SRGB, BGRX8_UNORM_SRGB, UNKNOWN, BGRX8_UNORM);

        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC6H_TYPELESS,           BC6H_UF16,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC6H_UF16,               BC6H_UF16,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC6H_SF16,               BC6H_SF16,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC7_TYPELESS,            BC7_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC7_UNORM,               BC7_UNORM,      UNKNOWN, UNKNOWN, UNKNOWN);
        INIT_TEX_VIEW_FORMAT_INFO(TEX_FORMAT_BC7_UNORM_SRGB,          BC7_UNORM_SRGB, UNKNOWN, UNKNOWN, UNKNOWN);
#undef INIT_TVIEW_FORMAT_INFO
        // clang-format on

        m_ViewFormats[TEX_FORMAT_R8_UINT][TEXTURE_VIEW_SHADING_RATE - 1]   = TEX_FORMAT_R8_UINT;
        m_ViewFormats[TEX_FORMAT_RG8_UNORM][TEXTURE_VIEW_SHADING_RATE - 1] = TEX_FORMAT_RG8_UNORM;
    }

    TEXTURE_FORMAT GetViewFormat(TEXTURE_FORMAT Format, TEXTURE_VIEW_TYPE ViewType, Uint32 BindFlags)
    {
        VERIFY(ViewType > TEXTURE_VIEW_UNDEFINED && ViewType < TEXTURE_VIEW_NUM_VIEWS, "Unexpected texture view type");
        VERIFY(Format >= TEX_FORMAT_UNKNOWN && Format < TEX_FORMAT_NUM_FORMATS, "Unknown texture format");
        switch (Format)
        {
            case TEX_FORMAT_R16_TYPELESS:
            {
                if (BindFlags & BIND_DEPTH_STENCIL)
                {
                    switch (ViewType)
                    {
                        case TEXTURE_VIEW_SHADER_RESOURCE:
                        case TEXTURE_VIEW_RENDER_TARGET:
                        case TEXTURE_VIEW_UNORDERED_ACCESS:
                            return TEX_FORMAT_R16_UNORM;

                        case TEXTURE_VIEW_DEPTH_STENCIL:
                        case TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL:
                            return TEX_FORMAT_D16_UNORM;

                        case TEXTURE_VIEW_SHADING_RATE:
                            return TEX_FORMAT_UNKNOWN;

                        default:
                            UNEXPECTED("Unexpected texture view type");
                            return TEX_FORMAT_UNKNOWN;
                    }
                    static_assert(TEXTURE_VIEW_NUM_VIEWS == 7, "Please handle the new view type in the switch above, if necessary");
                }
            }

            default: /*do nothing*/ break;
        }

        return m_ViewFormats[Format][ViewType - 1];
    }

private:
    std::array<std::array<TEXTURE_FORMAT, TEXTURE_VIEW_NUM_VIEWS - 1>, TEX_FORMAT_NUM_FORMATS> m_ViewFormats{};
};

TEXTURE_FORMAT GetDefaultTextureViewFormat(TEXTURE_FORMAT TextureFormat, TEXTURE_VIEW_TYPE ViewType, Uint32 BindFlags)
{
    static TexFormatToViewFormatConverter FmtConverter;
    return FmtConverter.GetViewFormat(TextureFormat, ViewType, BindFlags);
}

const TextureFormatAttribs& GetTextureFormatAttribs(TEXTURE_FORMAT Format)
{
    struct FormatToTextureFormatAttribsMap
    {
        FormatToTextureFormatAttribsMap()
        {
            // clang-format off
#define INIT_TEX_FORMAT_INFO(TexFmt, ComponentSize, NumComponents, ComponentType, IsTypeless, BlockWidth, BlockHeight) \
            FmtAttribs[ TexFmt ] = TextureFormatAttribs{#TexFmt, TexFmt, ComponentSize, NumComponents, ComponentType, IsTypeless, BlockWidth, BlockHeight};

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA32_TYPELESS,         4, 4, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA32_FLOAT,            4, 4, COMPONENT_TYPE_FLOAT,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA32_UINT,             4, 4, COMPONENT_TYPE_UINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA32_SINT,             4, 4, COMPONENT_TYPE_SINT,      false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGB32_TYPELESS,          4, 3, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGB32_FLOAT,             4, 3, COMPONENT_TYPE_FLOAT,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGB32_UINT,              4, 3, COMPONENT_TYPE_UINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGB32_SINT,              4, 3, COMPONENT_TYPE_SINT,      false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA16_TYPELESS,         2, 4, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA16_FLOAT,            2, 4, COMPONENT_TYPE_FLOAT,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA16_UNORM,            2, 4, COMPONENT_TYPE_UNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA16_UINT,             2, 4, COMPONENT_TYPE_UINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA16_SNORM,            2, 4, COMPONENT_TYPE_SNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA16_SINT,             2, 4, COMPONENT_TYPE_SINT,      false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG32_TYPELESS,           4, 2, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG32_FLOAT,              4, 2, COMPONENT_TYPE_FLOAT,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG32_UINT,               4, 2, COMPONENT_TYPE_UINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG32_SINT,               4, 2, COMPONENT_TYPE_SINT,      false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R32G8X24_TYPELESS,       4, 2, COMPONENT_TYPE_DEPTH_STENCIL,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_D32_FLOAT_S8X24_UINT,    4, 2, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,4, 2, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_X32_TYPELESS_G8X24_UINT, 4, 2, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGB10A2_TYPELESS,        4, 1, COMPONENT_TYPE_COMPOUND,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGB10A2_UNORM,           4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGB10A2_UINT,            4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R11G11B10_FLOAT,         4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA8_TYPELESS,          1, 4, COMPONENT_TYPE_UNDEFINED,   true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA8_UNORM,             1, 4, COMPONENT_TYPE_UNORM,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA8_UNORM_SRGB,        1, 4, COMPONENT_TYPE_UNORM_SRGB, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA8_UINT,              1, 4, COMPONENT_TYPE_UINT,       false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA8_SNORM,             1, 4, COMPONENT_TYPE_SNORM,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGBA8_SINT,              1, 4, COMPONENT_TYPE_SINT,       false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG16_TYPELESS,           2, 2, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG16_FLOAT,              2, 2, COMPONENT_TYPE_FLOAT,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG16_UNORM,              2, 2, COMPONENT_TYPE_UNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG16_UINT,               2, 2, COMPONENT_TYPE_UINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG16_SNORM,              2, 2, COMPONENT_TYPE_SNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG16_SINT,               2, 2, COMPONENT_TYPE_SINT,      false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R32_TYPELESS,            4, 1, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_D32_FLOAT,               4, 1, COMPONENT_TYPE_DEPTH,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R32_FLOAT,               4, 1, COMPONENT_TYPE_FLOAT,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R32_UINT,                4, 1, COMPONENT_TYPE_UINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R32_SINT,                4, 1, COMPONENT_TYPE_SINT,      false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R24G8_TYPELESS,          4, 1, COMPONENT_TYPE_DEPTH_STENCIL,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_D24_UNORM_S8_UINT,       4, 1, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R24_UNORM_X8_TYPELESS,   4, 1, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_X24_TYPELESS_G8_UINT,    4, 1, COMPONENT_TYPE_DEPTH_STENCIL, false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG8_TYPELESS,            1, 2, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG8_UNORM,               1, 2, COMPONENT_TYPE_UNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG8_UINT,                1, 2, COMPONENT_TYPE_UINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG8_SNORM,               1, 2, COMPONENT_TYPE_SNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG8_SINT,                1, 2, COMPONENT_TYPE_SINT,      false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R16_TYPELESS,            2, 1, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R16_FLOAT,               2, 1, COMPONENT_TYPE_FLOAT,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_D16_UNORM,               2, 1, COMPONENT_TYPE_DEPTH,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R16_UNORM,               2, 1, COMPONENT_TYPE_UNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R16_UINT,                2, 1, COMPONENT_TYPE_UINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R16_SNORM,               2, 1, COMPONENT_TYPE_SNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R16_SINT,                2, 1, COMPONENT_TYPE_SINT,      false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R8_TYPELESS,             1, 1, COMPONENT_TYPE_UNDEFINED,  true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R8_UNORM,                1, 1, COMPONENT_TYPE_UNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R8_UINT,                 1, 1, COMPONENT_TYPE_UINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R8_SNORM,                1, 1, COMPONENT_TYPE_SNORM,     false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R8_SINT,                 1, 1, COMPONENT_TYPE_SINT,      false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_A8_UNORM,                1, 1, COMPONENT_TYPE_UNORM,     false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R1_UNORM,                1, 1, COMPONENT_TYPE_UNORM,    false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RGB9E5_SHAREDEXP,        4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_RG8_B8G8_UNORM,          1, 4, COMPONENT_TYPE_UNORM,    false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_G8R8_G8B8_UNORM,         1, 4, COMPONENT_TYPE_UNORM,    false, 1,1);

            // http://www.g-truc.net/post-0335.html
            // http://renderingpipeline.com/2012/07/texture-compression/
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC1_TYPELESS,            8,  3, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC1_UNORM,               8,  3, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC1_UNORM_SRGB,          8,  3, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC2_TYPELESS,            16, 4, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC2_UNORM,               16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC2_UNORM_SRGB,          16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC3_TYPELESS,            16, 4, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC3_UNORM,               16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC3_UNORM_SRGB,          16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC4_TYPELESS,            8,  1, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC4_UNORM,               8,  1, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC4_SNORM,               8,  1, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC5_TYPELESS,            16, 2, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC5_UNORM,               16, 2, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC5_SNORM,               16, 2, COMPONENT_TYPE_COMPRESSED, false, 4,4);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_B5G6R5_UNORM,            2, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_B5G5R5A1_UNORM,          2, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BGRA8_UNORM,             1, 4, COMPONENT_TYPE_UNORM,    false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BGRX8_UNORM,             1, 4, COMPONENT_TYPE_UNORM,    false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,  4, 1, COMPONENT_TYPE_COMPOUND, false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BGRA8_TYPELESS,          1, 4, COMPONENT_TYPE_UNDEFINED,     true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BGRA8_UNORM_SRGB,        1, 4, COMPONENT_TYPE_UNORM_SRGB,   false, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BGRX8_TYPELESS,          1, 4, COMPONENT_TYPE_UNDEFINED,     true, 1,1);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BGRX8_UNORM_SRGB,        1, 4, COMPONENT_TYPE_UNORM_SRGB,   false, 1,1);

            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC6H_TYPELESS,           16, 3, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC6H_UF16,               16, 3, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC6H_SF16,               16, 3, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC7_TYPELESS,            16, 4, COMPONENT_TYPE_COMPRESSED,  true, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC7_UNORM,               16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
            INIT_TEX_FORMAT_INFO(TEX_FORMAT_BC7_UNORM_SRGB,          16, 4, COMPONENT_TYPE_COMPRESSED, false, 4,4);
#undef  INIT_TEX_FORMAT_INFO
            // clang-format on
            static_assert(TEX_FORMAT_NUM_FORMATS == TEX_FORMAT_BC7_UNORM_SRGB + 1, "Not all texture formats initialized.");

#ifdef DILIGENT_DEBUG
            for (Uint32 Fmt = TEX_FORMAT_UNKNOWN; Fmt < TEX_FORMAT_NUM_FORMATS; ++Fmt)
                VERIFY(FmtAttribs[Fmt].Format == static_cast<TEXTURE_FORMAT>(Fmt), "Uninitialized format");
#endif
        }

        const TextureFormatAttribs& operator[](TEXTURE_FORMAT Format) const
        {
            if (Format >= TEX_FORMAT_UNKNOWN && Format < TEX_FORMAT_NUM_FORMATS)
            {
                const auto& Attribs = FmtAttribs[Format];
                VERIFY(Attribs.Format == Format, "Unexpected format");
                return Attribs;
            }
            else
            {
                UNEXPECTED("Texture format (", int{Format}, ") is out of allowed range [0, ", int{TEX_FORMAT_NUM_FORMATS} - 1, "]");
                return FmtAttribs[0];
            }
        }

    private:
        std::array<TextureFormatAttribs, TEX_FORMAT_NUM_FORMATS> FmtAttribs{};
    };

    static const FormatToTextureFormatAttribsMap FormatToTexFormatAttribs;
    return FormatToTexFormatAttribs[Format];
}


const Char* GetTexViewTypeLiteralName(TEXTURE_VIEW_TYPE ViewType)
{
    struct TexViewTypeToLiteralNameMap
    {
        TexViewTypeToLiteralNameMap()
        {
#define INIT_TEX_VIEW_TYPE_NAME(ViewType) TexViewLiteralNames[ViewType] = #ViewType
            INIT_TEX_VIEW_TYPE_NAME(TEXTURE_VIEW_UNDEFINED);
            INIT_TEX_VIEW_TYPE_NAME(TEXTURE_VIEW_SHADER_RESOURCE);
            INIT_TEX_VIEW_TYPE_NAME(TEXTURE_VIEW_RENDER_TARGET);
            INIT_TEX_VIEW_TYPE_NAME(TEXTURE_VIEW_DEPTH_STENCIL);
            INIT_TEX_VIEW_TYPE_NAME(TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL);
            INIT_TEX_VIEW_TYPE_NAME(TEXTURE_VIEW_UNORDERED_ACCESS);
            INIT_TEX_VIEW_TYPE_NAME(TEXTURE_VIEW_SHADING_RATE);
#undef INIT_TEX_VIEW_TYPE_NAME
            static_assert(TEXTURE_VIEW_NUM_VIEWS == 7, "Not all texture views names are initialized.");
        }

        const Char* operator[](TEXTURE_VIEW_TYPE ViewType) const
        {
            if (ViewType >= TEXTURE_VIEW_UNDEFINED && ViewType < TEXTURE_VIEW_NUM_VIEWS)
            {
                return TexViewLiteralNames[ViewType];
            }
            else
            {
                UNEXPECTED("Texture view type (", static_cast<Uint32>(ViewType), ") is out of allowed range [0, ", static_cast<Uint32>(TEXTURE_VIEW_NUM_VIEWS) - 1, "]");
                return "<Unknown texture view type>";
            }
        }

    private:
        std::array<const Char*, TEXTURE_VIEW_NUM_VIEWS> TexViewLiteralNames{};
    };

    static const TexViewTypeToLiteralNameMap TexViewTypeToLiteralName;
    return TexViewTypeToLiteralName[ViewType];
}

const Char* GetBufferViewTypeLiteralName(BUFFER_VIEW_TYPE ViewType)
{
    struct BuffViewTypeToLiteralNameMap
    {
        BuffViewTypeToLiteralNameMap()
        {
            // clang-format off
#define INIT_BUFF_VIEW_TYPE_NAME(ViewType)BuffViewLiteralNames[ViewType] = #ViewType
            INIT_BUFF_VIEW_TYPE_NAME( BUFFER_VIEW_UNDEFINED );
            INIT_BUFF_VIEW_TYPE_NAME( BUFFER_VIEW_SHADER_RESOURCE );
            INIT_BUFF_VIEW_TYPE_NAME( BUFFER_VIEW_UNORDERED_ACCESS );
 #undef INIT_BUFF_VIEW_TYPE_NAME
            // clang-format on
            static_assert(BUFFER_VIEW_NUM_VIEWS == 3, "Not all buffer views names initialized.");
        }

        const Char* operator[](BUFFER_VIEW_TYPE ViewType) const
        {
            if (ViewType >= BUFFER_VIEW_UNDEFINED && ViewType < BUFFER_VIEW_NUM_VIEWS)
            {
                return BuffViewLiteralNames[ViewType];
            }
            else
            {
                UNEXPECTED("Buffer view type (", static_cast<Uint32>(ViewType), ") is out of allowed range [0, ", static_cast<Uint32>(BUFFER_VIEW_NUM_VIEWS) - 1, "]");
                return "<Unknown buffer view type>";
            }
        }

    private:
        std::array<const Char*, BUFFER_VIEW_NUM_VIEWS> BuffViewLiteralNames{};
    };

    static const BuffViewTypeToLiteralNameMap BuffViewTypeToLiteralName;
    return BuffViewTypeToLiteralName[ViewType];
}

const Char* GetShaderTypeLiteralName(SHADER_TYPE ShaderType)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please handle the new shader type in the switch below");
    switch (ShaderType)
    {
        // clang-format off
#define RETURN_SHADER_TYPE_NAME(ShaderType)\
        case ShaderType: return #ShaderType;

        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_UNKNOWN         )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_VERTEX          )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_PIXEL           )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_GEOMETRY        )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_HULL            )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_DOMAIN          )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_COMPUTE         )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_AMPLIFICATION   )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_MESH            )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_RAY_GEN         )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_RAY_MISS        )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_RAY_CLOSEST_HIT )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_RAY_ANY_HIT     )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_RAY_INTERSECTION)
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_CALLABLE        )
        RETURN_SHADER_TYPE_NAME(SHADER_TYPE_TILE            )
#undef  RETURN_SHADER_TYPE_NAME
            // clang-format on

        default: UNEXPECTED("Unknown shader type constant ", Uint32{ShaderType}); return "<Unknown shader type>";
    }
}

String GetShaderStagesString(SHADER_TYPE ShaderStages)
{
    String StagesStr;
    for (Uint32 Stage = SHADER_TYPE_VERTEX; ShaderStages != 0 && Stage <= SHADER_TYPE_LAST; Stage <<= 1)
    {
        if (ShaderStages & Stage)
        {
            if (StagesStr.length())
                StagesStr += ", ";
            StagesStr += GetShaderTypeLiteralName(static_cast<SHADER_TYPE>(Stage));
            ShaderStages &= ~static_cast<SHADER_TYPE>(Stage);
        }
    }
    VERIFY_EXPR(ShaderStages == 0);
    return StagesStr;
}

const Char* GetShaderVariableTypeLiteralName(SHADER_RESOURCE_VARIABLE_TYPE VarType, bool bGetFullName)
{
    struct VarTypeToLiteralNameMap
    {
        VarTypeToLiteralNameMap()
        {
            ShortVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_STATIC]  = "static";
            ShortVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE] = "mutable";
            ShortVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC] = "dynamic";
            FullVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_STATIC]   = "SHADER_RESOURCE_VARIABLE_TYPE_STATIC";
            FullVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE]  = "SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE";
            FullVarTypeNameStrings[SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC]  = "SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC";

            static_assert(SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES == 3, "Not all shader variable types initialized.");
        }

        const Char* Get(SHADER_RESOURCE_VARIABLE_TYPE VarType, bool bGetFullName) const
        {
            if (VarType >= SHADER_RESOURCE_VARIABLE_TYPE_STATIC && VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES)
                return (bGetFullName ? FullVarTypeNameStrings : ShortVarTypeNameStrings)[VarType];
            else
            {
                UNEXPECTED("Unknown shader variable type");
                return "unknown";
            }
        }

    private:
        std::array<const Char*, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES> ShortVarTypeNameStrings{};
        std::array<const Char*, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES> FullVarTypeNameStrings{};
    };

    static const VarTypeToLiteralNameMap VarTypeToLiteralName;
    return VarTypeToLiteralName.Get(VarType, bGetFullName);
}


const Char* GetShaderResourceTypeLiteralName(SHADER_RESOURCE_TYPE ResourceType, bool bGetFullName)
{
    static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update the switch below to handle the new shader resource type");
    switch (ResourceType)
    {
        // clang-format off
        case SHADER_RESOURCE_TYPE_UNKNOWN:          return bGetFullName ?  "SHADER_RESOURCE_TYPE_UNKNOWN"          : "unknown";
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:  return bGetFullName ?  "SHADER_RESOURCE_TYPE_CONSTANT_BUFFER"  : "constant buffer";
        case SHADER_RESOURCE_TYPE_TEXTURE_SRV:      return bGetFullName ?  "SHADER_RESOURCE_TYPE_TEXTURE_SRV"      : "texture SRV";
        case SHADER_RESOURCE_TYPE_BUFFER_SRV:       return bGetFullName ?  "SHADER_RESOURCE_TYPE_BUFFER_SRV"       : "buffer SRV";
        case SHADER_RESOURCE_TYPE_TEXTURE_UAV:      return bGetFullName ?  "SHADER_RESOURCE_TYPE_TEXTURE_UAV"      : "texture UAV";
        case SHADER_RESOURCE_TYPE_BUFFER_UAV:       return bGetFullName ?  "SHADER_RESOURCE_TYPE_BUFFER_UAV"       : "buffer UAV";
        case SHADER_RESOURCE_TYPE_SAMPLER:          return bGetFullName ?  "SHADER_RESOURCE_TYPE_SAMPLER"          : "sampler";
        case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT: return bGetFullName ?  "SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT" : "input attachment";
        case SHADER_RESOURCE_TYPE_ACCEL_STRUCT:     return bGetFullName ?  "SHADER_RESOURCE_TYPE_ACCEL_STRUCT"     : "acceleration structure";
        // clang-format on
        default:
            UNEXPECTED("Unexpected resource type (", Uint32{ResourceType}, ")");
            return "UNKNOWN";
    }
}

const Char* GetFilterTypeLiteralName(FILTER_TYPE FilterType, bool bGetFullName)
{
    static_assert(FILTER_TYPE_NUM_FILTERS == 13, "Please update the switch below to handle the new filter type");
    switch (FilterType)
    {
        // clang-format off
        case FILTER_TYPE_UNKNOWN:                return bGetFullName ? "FILTER_TYPE_UNKNOWN"                : "unknown";
        case FILTER_TYPE_POINT:                  return bGetFullName ? "FILTER_TYPE_POINT"                  : "point";
        case FILTER_TYPE_LINEAR:                 return bGetFullName ? "FILTER_TYPE_LINEAR"                 : "linear";
        case FILTER_TYPE_ANISOTROPIC:            return bGetFullName ? "FILTER_TYPE_ANISOTROPIC"            : "anisotropic";
        case FILTER_TYPE_COMPARISON_POINT:       return bGetFullName ? "FILTER_TYPE_COMPARISON_POINT"       : "comparison point";
        case FILTER_TYPE_COMPARISON_LINEAR:      return bGetFullName ? "FILTER_TYPE_COMPARISON_LINEAR"      : "comparison linear";
        case FILTER_TYPE_COMPARISON_ANISOTROPIC: return bGetFullName ? "FILTER_TYPE_COMPARISON_ANISOTROPIC" : "comparison anisotropic";
        case FILTER_TYPE_MINIMUM_POINT:          return bGetFullName ? "FILTER_TYPE_MINIMUM_POINT"          : "minimum point";
        case FILTER_TYPE_MINIMUM_LINEAR:         return bGetFullName ? "FILTER_TYPE_MINIMUM_LINEAR"         : "minimum linear";
        case FILTER_TYPE_MINIMUM_ANISOTROPIC:    return bGetFullName ? "FILTER_TYPE_MINIMUM_ANISOTROPIC"    : "minimum anisotropic";
        case FILTER_TYPE_MAXIMUM_POINT:          return bGetFullName ? "FILTER_TYPE_MAXIMUM_POINT"          : "maximum point";
        case FILTER_TYPE_MAXIMUM_LINEAR:         return bGetFullName ? "FILTER_TYPE_MAXIMUM_LINEAR"         : "maximum linear";
        case FILTER_TYPE_MAXIMUM_ANISOTROPIC:    return bGetFullName ? "FILTER_TYPE_MAXIMUM_ANISOTROPIC"    : "maximum anisotropic";
        // clang-format on
        default:
            UNEXPECTED("Unexpected filter type (", Uint32{FilterType}, ")");
            return "UNKNOWN";
    }
}

const Char* GetTextureAddressModeLiteralName(TEXTURE_ADDRESS_MODE AddressMode, bool bGetFullName)
{
    static_assert(TEXTURE_ADDRESS_NUM_MODES == 6, "Please update the switch below to handle the new texture address mode");
    switch (AddressMode)
    {
        // clang-format off
        case TEXTURE_ADDRESS_UNKNOWN:     return bGetFullName ? "TEXTURE_ADDRESS_UNKNOWN"     : "unknown";
        case TEXTURE_ADDRESS_WRAP:        return bGetFullName ? "TEXTURE_ADDRESS_WRAP"        : "wrap";
        case TEXTURE_ADDRESS_MIRROR:      return bGetFullName ? "TEXTURE_ADDRESS_MIRROR"      : "mirror";
        case TEXTURE_ADDRESS_CLAMP:       return bGetFullName ? "TEXTURE_ADDRESS_CLAMP"       : "clamp";
        case TEXTURE_ADDRESS_BORDER:      return bGetFullName ? "TEXTURE_ADDRESS_BORDER"      : "border";
        case TEXTURE_ADDRESS_MIRROR_ONCE: return bGetFullName ? "TEXTURE_ADDRESS_MIRROR_ONCE" : "mirror once";
        // clang-format on
        default:
            UNEXPECTED("Unexpected texture address mode (", Uint32{AddressMode}, ")");
            return "UNKNOWN";
    }
}

const Char* GetComparisonFunctionLiteralName(COMPARISON_FUNCTION ComparisonFunc, bool bGetFullName)
{
    static_assert(COMPARISON_FUNC_NUM_FUNCTIONS == 9, "Please update the switch below to handle the new comparison function");
    switch (ComparisonFunc)
    {
        // clang-format off
        case COMPARISON_FUNC_UNKNOWN:       return bGetFullName ? "COMPARISON_FUNC_UNKNOWN"      : "unknown";
        case COMPARISON_FUNC_NEVER:         return bGetFullName ? "COMPARISON_FUNC_NEVER"        : "never";
        case COMPARISON_FUNC_LESS:          return bGetFullName ? "COMPARISON_FUNC_LESS"         : "less";
        case COMPARISON_FUNC_EQUAL:         return bGetFullName ? "COMPARISON_FUNC_EQUAL"        : "equal";
        case COMPARISON_FUNC_LESS_EQUAL:    return bGetFullName ? "COMPARISON_FUNC_LESS_EQUAL"   : "less equal";
        case COMPARISON_FUNC_GREATER:       return bGetFullName ? "COMPARISON_FUNC_GREATER"      : "greater";
        case COMPARISON_FUNC_NOT_EQUAL:     return bGetFullName ? "COMPARISON_FUNC_NOT_EQUAL"    : "not equal";
        case COMPARISON_FUNC_GREATER_EQUAL: return bGetFullName ? "COMPARISON_FUNC_GREATER_EQUAL": "greater equal";
        case COMPARISON_FUNC_ALWAYS:        return bGetFullName ? "COMPARISON_FUNC_ALWAYS"       : "always";
        // clang-format on
        default:
            UNEXPECTED("Unexpected comparison function (", Uint32{ComparisonFunc}, ")");
            return "UNKNOWN";
    }
}

const Char* GetStencilOpLiteralName(STENCIL_OP StencilOp)
{
#define STENCIL_OP_TO_STR(Op) \
    case Op: return #Op

    static_assert(STENCIL_OP_NUM_OPS == 9, "Please update the switch below to handle the new stencil op");
    switch (StencilOp)
    {
        STENCIL_OP_TO_STR(STENCIL_OP_UNDEFINED);
        STENCIL_OP_TO_STR(STENCIL_OP_KEEP);
        STENCIL_OP_TO_STR(STENCIL_OP_ZERO);
        STENCIL_OP_TO_STR(STENCIL_OP_REPLACE);
        STENCIL_OP_TO_STR(STENCIL_OP_INCR_SAT);
        STENCIL_OP_TO_STR(STENCIL_OP_DECR_SAT);
        STENCIL_OP_TO_STR(STENCIL_OP_INVERT);
        STENCIL_OP_TO_STR(STENCIL_OP_INCR_WRAP);
        STENCIL_OP_TO_STR(STENCIL_OP_DECR_WRAP);

        default:
            UNEXPECTED("Unexpected stencil operation (", static_cast<Uint32>(StencilOp), ")");
            return "UNKNOWN";
    }
#undef STENCIL_OP_TO_STR
}

const Char* GetBlendFactorLiteralName(BLEND_FACTOR BlendFactor)
{
#define BLEND_FACTOR_TO_STR(Factor) \
    case Factor: return #Factor

    static_assert(BLEND_FACTOR_NUM_FACTORS == 18, "Please update the switch below to handle the new blend factor");
    switch (BlendFactor)
    {
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_UNDEFINED);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_ZERO);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_ONE);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_SRC_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_SRC_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_DEST_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_DEST_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_DEST_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_DEST_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC_ALPHA_SAT);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_BLEND_FACTOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_BLEND_FACTOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC1_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_SRC1_COLOR);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_SRC1_ALPHA);
        BLEND_FACTOR_TO_STR(BLEND_FACTOR_INV_SRC1_ALPHA);

        default:
            UNEXPECTED("Unexpected blend factor (", static_cast<int>(BlendFactor), ")");
            return "UNKNOWN";
    }
#undef BLEND_FACTOR_TO_STR
}

const Char* GetBlendOperationLiteralName(BLEND_OPERATION BlendOp)
{
#define BLEND_OP_TO_STR(BlendOp) \
    case BlendOp: return #BlendOp

    static_assert(BLEND_OPERATION_NUM_OPERATIONS == 6, "Please update the switch below to handle the new blend op");
    switch (BlendOp)
    {
        BLEND_OP_TO_STR(BLEND_OPERATION_UNDEFINED);
        BLEND_OP_TO_STR(BLEND_OPERATION_ADD);
        BLEND_OP_TO_STR(BLEND_OPERATION_SUBTRACT);
        BLEND_OP_TO_STR(BLEND_OPERATION_REV_SUBTRACT);
        BLEND_OP_TO_STR(BLEND_OPERATION_MIN);
        BLEND_OP_TO_STR(BLEND_OPERATION_MAX);

        default:
            UNEXPECTED("Unexpected blend operation (", static_cast<int>(BlendOp), ")");
            return "UNKNOWN";
    }
#undef BLEND_OP_TO_STR
}

const Char* GetFillModeLiteralName(FILL_MODE FillMode)
{
#define FILL_MODE_TO_STR(Mode) \
    case Mode: return #Mode

    static_assert(FILL_MODE_NUM_MODES == 3, "Please update the switch below to handle the new filter mode");
    switch (FillMode)
    {
        FILL_MODE_TO_STR(FILL_MODE_UNDEFINED);
        FILL_MODE_TO_STR(FILL_MODE_WIREFRAME);
        FILL_MODE_TO_STR(FILL_MODE_SOLID);

        default:
            UNEXPECTED("Unexpected fill mode (", static_cast<int>(FillMode), ")");
            return "UNKNOWN";
    }
#undef FILL_MODE_TO_STR
}

const Char* GetCullModeLiteralName(CULL_MODE CullMode)
{
#define CULL_MODE_TO_STR(Mode) \
    case Mode: return #Mode

    static_assert(CULL_MODE_NUM_MODES == 4, "Please update the switch below to handle the new cull mode");
    switch (CullMode)
    {
        CULL_MODE_TO_STR(CULL_MODE_UNDEFINED);
        CULL_MODE_TO_STR(CULL_MODE_NONE);
        CULL_MODE_TO_STR(CULL_MODE_FRONT);
        CULL_MODE_TO_STR(CULL_MODE_BACK);

        default:
            UNEXPECTED("Unexpected cull mode (", static_cast<int>(CullMode), ")");
            return "UNKNOWN";
    }
#undef CULL_MODE_TO_STR
}

const Char* GetMapTypeString(MAP_TYPE MapType)
{
    switch (MapType)
    {
        case MAP_READ: return "MAP_READ";
        case MAP_WRITE: return "MAP_WRITE";
        case MAP_READ_WRITE: return "MAP_READ_WRITE";

        default:
            UNEXPECTED("Unexpected map type");
            return "Unknown map type";
    }
}

/// Returns the string containing the usage
const Char* GetUsageString(USAGE Usage)
{
    struct UsageToStringMap
    {
        UsageToStringMap()
        {
#define INIT_USAGE_STR(Usage) UsageStrings[Usage] = #Usage
            INIT_USAGE_STR(USAGE_IMMUTABLE);
            INIT_USAGE_STR(USAGE_DEFAULT);
            INIT_USAGE_STR(USAGE_DYNAMIC);
            INIT_USAGE_STR(USAGE_STAGING);
            INIT_USAGE_STR(USAGE_UNIFIED);
            INIT_USAGE_STR(USAGE_SPARSE);
#undef INIT_USAGE_STR
            static_assert(USAGE_NUM_USAGES == 6, "Please update the map to handle the new usage type");
        }

        const Char* operator[](USAGE Usage) const
        {
            if (Usage >= USAGE_IMMUTABLE && Usage < USAGE_NUM_USAGES)
                return UsageStrings[Usage];
            else
            {
                UNEXPECTED("Unknown usage");
                return "Unknown usage";
            }
        }

    private:
        std::array<const Char*, USAGE_NUM_USAGES> UsageStrings{};
    };

    static const UsageToStringMap UsageStrings;
    return UsageStrings[Usage];
}

const Char* GetResourceDimString(RESOURCE_DIMENSION TexType)
{
    struct ResourceDimToStringMap
    {
        ResourceDimToStringMap()
        {
            TexTypeStrings[RESOURCE_DIM_UNDEFINED]      = "Undefined";
            TexTypeStrings[RESOURCE_DIM_BUFFER]         = "Buffer";
            TexTypeStrings[RESOURCE_DIM_TEX_1D]         = "Texture 1D";
            TexTypeStrings[RESOURCE_DIM_TEX_1D_ARRAY]   = "Texture 1D Array";
            TexTypeStrings[RESOURCE_DIM_TEX_2D]         = "Texture 2D";
            TexTypeStrings[RESOURCE_DIM_TEX_2D_ARRAY]   = "Texture 2D Array";
            TexTypeStrings[RESOURCE_DIM_TEX_3D]         = "Texture 3D";
            TexTypeStrings[RESOURCE_DIM_TEX_CUBE]       = "Texture Cube";
            TexTypeStrings[RESOURCE_DIM_TEX_CUBE_ARRAY] = "Texture Cube Array";
            static_assert(RESOURCE_DIM_NUM_DIMENSIONS == 9, "Not all texture type strings initialized.");
        }

        const Char* operator[](RESOURCE_DIMENSION TexType) const
        {
            if (TexType >= RESOURCE_DIM_UNDEFINED && TexType < RESOURCE_DIM_NUM_DIMENSIONS)
                return TexTypeStrings[TexType];
            else
            {
                UNEXPECTED("Unknown texture type");
                return "Unknown texture type";
            }
        }

    private:
        std::array<const Char*, RESOURCE_DIM_NUM_DIMENSIONS> TexTypeStrings{};
    };

    static const ResourceDimToStringMap TexTypeStrings;
    return TexTypeStrings[TexType];
}

const Char* GetBindFlagString(Uint32 BindFlag)
{
    VERIFY(BindFlag == BIND_NONE || IsPowerOfTwo(BindFlag), "More than one bind flag is specified");

    static_assert(BIND_FLAG_LAST == 0x800L, "Please handle the new bind flag in the switch below");
    switch (BindFlag)
    {
#define BIND_FLAG_STR_CASE(Flag) \
    case Flag: return #Flag;
        BIND_FLAG_STR_CASE(BIND_NONE)
        BIND_FLAG_STR_CASE(BIND_VERTEX_BUFFER)
        BIND_FLAG_STR_CASE(BIND_INDEX_BUFFER)
        BIND_FLAG_STR_CASE(BIND_UNIFORM_BUFFER)
        BIND_FLAG_STR_CASE(BIND_SHADER_RESOURCE)
        BIND_FLAG_STR_CASE(BIND_STREAM_OUTPUT)
        BIND_FLAG_STR_CASE(BIND_RENDER_TARGET)
        BIND_FLAG_STR_CASE(BIND_DEPTH_STENCIL)
        BIND_FLAG_STR_CASE(BIND_UNORDERED_ACCESS)
        BIND_FLAG_STR_CASE(BIND_INDIRECT_DRAW_ARGS)
        BIND_FLAG_STR_CASE(BIND_INPUT_ATTACHMENT)
        BIND_FLAG_STR_CASE(BIND_RAY_TRACING)
        BIND_FLAG_STR_CASE(BIND_SHADING_RATE)
#undef BIND_FLAG_STR_CASE
        default: UNEXPECTED("Unexpected bind flag ", BindFlag); return "";
    }
}

String GetBindFlagsString(Uint32 BindFlags, const char* Delimiter)
{
    if (BindFlags == 0)
        return "0";
    String Str;
    for (Uint32 Flag = 1; BindFlags && Flag <= BIND_FLAG_LAST; Flag <<= 1)
    {
        if (BindFlags & Flag)
        {
            if (!Str.empty())
                Str += Delimiter;
            Str += GetBindFlagString(Flag);
            BindFlags &= ~Flag;
        }
    }
    VERIFY(BindFlags == 0, "Unknown bind flags left");
    return Str;
}


static const Char* GetSingleCPUAccessFlagString(Uint32 CPUAccessFlag)
{
    VERIFY(CPUAccessFlag == CPU_ACCESS_NONE || IsPowerOfTwo(CPUAccessFlag), "More than one access flag is specified");
    switch (CPUAccessFlag)
    {
        // clang-format off
#define CPU_ACCESS_FLAG_STR_CASE(Flag) case Flag: return #Flag;
        CPU_ACCESS_FLAG_STR_CASE(CPU_ACCESS_NONE)
        CPU_ACCESS_FLAG_STR_CASE(CPU_ACCESS_READ)
        CPU_ACCESS_FLAG_STR_CASE(CPU_ACCESS_WRITE)
#undef  CPU_ACCESS_FLAG_STR_CASE
        // clang-format on
        default: UNEXPECTED("Unexpected CPU access flag ", CPUAccessFlag); return "";
    }
}

String GetCPUAccessFlagsString(Uint32 CpuAccessFlags)
{
    if (CpuAccessFlags == 0)
        return "0";
    String Str;
    for (Uint32 Flag = CPU_ACCESS_READ; CpuAccessFlags && Flag <= CPU_ACCESS_WRITE; Flag <<= 1)
    {
        if (CpuAccessFlags & Flag)
        {
            if (Str.length())
                Str += '|';
            Str += GetSingleCPUAccessFlagString(Flag);
            CpuAccessFlags &= ~Flag;
        }
    }
    VERIFY(CpuAccessFlags == 0, "Unknown CPU access flags left");
    return Str;
}

// There is a nice standard function to_string, but there is a bug on gcc
// and it does not work
template <typename T>
String ToString(T Val)
{
    std::stringstream ss;
    ss << Val;
    return ss.str();
}

String GetTextureDescString(const TextureDesc& Desc)
{
    String Str = "Type: ";
    Str += GetResourceDimString(Desc.Type);
    Str += "; size: ";
    Str += ToString(Desc.Width);

    if (Desc.Is2D() || Desc.Is3D())
    {
        Str += "x";
        Str += ToString(Desc.Height);
    }

    if (Desc.Is3D())
    {
        Str += "x";
        Str += ToString(Desc.Depth);
    }

    if (Desc.IsArray())
    {
        Str += "; Num Slices: ";
        Str += ToString(Desc.ArraySize);
    }

    auto FmtName = GetTextureFormatAttribs(Desc.Format).Name;
    Str += "; Format: ";
    Str += FmtName;

    Str += "; Mip levels: ";
    Str += ToString(Desc.MipLevels);

    Str += "; Sample Count: ";
    Str += ToString(Desc.SampleCount);

    Str += "; Usage: ";
    Str += GetUsageString(Desc.Usage);

    Str += "; Bind Flags: ";
    Str += GetBindFlagsString(Desc.BindFlags);

    Str += "; CPU access: ";
    Str += GetCPUAccessFlagsString(Desc.CPUAccessFlags);

    return Str;
}

const Char* GetBufferModeString(BUFFER_MODE Mode)
{
    struct BufferModeToStringMap
    {
        BufferModeToStringMap()
        {
#define INIT_BUFF_MODE_STR(Mode) BufferModeStrings[Mode] = #Mode
            INIT_BUFF_MODE_STR(BUFFER_MODE_UNDEFINED);
            INIT_BUFF_MODE_STR(BUFFER_MODE_FORMATTED);
            INIT_BUFF_MODE_STR(BUFFER_MODE_STRUCTURED);
            INIT_BUFF_MODE_STR(BUFFER_MODE_RAW);
#undef INIT_BUFF_MODE_STR
            static_assert(BUFFER_MODE_NUM_MODES == 4, "Not all buffer mode strings initialized.");
        }

        const Char* operator[](BUFFER_MODE Mode) const
        {
            if (Mode >= BUFFER_MODE_UNDEFINED && Mode < BUFFER_MODE_NUM_MODES)
                return BufferModeStrings[Mode];
            else
            {
                UNEXPECTED("Unknown buffer mode");
                return "Unknown buffer mode";
            }
        }

    private:
        std::array<const Char*, BUFFER_MODE_NUM_MODES> BufferModeStrings{};
    };

    static const BufferModeToStringMap BuffModeToStr;
    return BuffModeToStr[Mode];
}

String GetBufferFormatString(const BufferFormat& Fmt)
{
    String Str;
    Str += GetValueTypeString(Fmt.ValueType);
    if (Fmt.IsNormalized)
        Str += " norm";
    Str += " x ";
    Str += ToString(Uint32{Fmt.NumComponents});
    return Str;
}

String GetBufferDescString(const BufferDesc& Desc)
{
    String Str;
    Str += "Size: ";
    bool bIsLarge = false;
    if (Desc.Size > (1 << 20))
    {
        Str += ToString(Desc.Size / (1 << 20));
        Str += " Mb (";
        bIsLarge = true;
    }
    else if (Desc.Size > (1 << 10))
    {
        Str += ToString(Desc.Size / (1 << 10));
        Str += " Kb (";
        bIsLarge = true;
    }

    Str += ToString(Desc.Size);
    Str += " bytes";
    if (bIsLarge)
        Str += ')';

    Str += "; Mode: ";
    Str += GetBufferModeString(Desc.Mode);

    Str += "; Usage: ";
    Str += GetUsageString(Desc.Usage);

    Str += "; Bind Flags: ";
    Str += GetBindFlagsString(Desc.BindFlags);

    Str += "; CPU access: ";
    Str += GetCPUAccessFlagsString(Desc.CPUAccessFlags);

    Str += "; stride: ";
    Str += ToString(Desc.ElementByteStride);
    Str += " bytes";

    return Str;
}

String GetShaderDescString(const ShaderDesc& Desc)
{
    String Str;
    Str += "Name: '";
    Str += Desc.Name != nullptr ? Desc.Name : "<NULL>";

    Str += "'; Type: ";
    Str += GetShaderTypeLiteralName(Desc.ShaderType);

    Str += "; combined samplers: ";
    Str += Desc.UseCombinedTextureSamplers ? '1' : '0';

    Str += "; sampler suffix: ";
    Str += Desc.CombinedSamplerSuffix != nullptr ? Desc.CombinedSamplerSuffix : "<NULL>";

    return Str;
}

const Char* GetResourceStateFlagString(RESOURCE_STATE State)
{
    VERIFY(State == RESOURCE_STATE_UNKNOWN || IsPowerOfTwo(State), "Single state is expected");
    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "Please update this function to handle the new resource state");
    switch (State)
    {
        // clang-format off
        case RESOURCE_STATE_UNKNOWN:           return "UNKNOWN";
        case RESOURCE_STATE_UNDEFINED:         return "UNDEFINED";
        case RESOURCE_STATE_VERTEX_BUFFER:     return "VERTEX_BUFFER";
        case RESOURCE_STATE_CONSTANT_BUFFER:   return "CONSTANT_BUFFER";
        case RESOURCE_STATE_INDEX_BUFFER:      return "INDEX_BUFFER";
        case RESOURCE_STATE_RENDER_TARGET:     return "RENDER_TARGET";
        case RESOURCE_STATE_UNORDERED_ACCESS:  return "UNORDERED_ACCESS";
        case RESOURCE_STATE_DEPTH_WRITE:       return "DEPTH_WRITE";
        case RESOURCE_STATE_DEPTH_READ:        return "DEPTH_READ";
        case RESOURCE_STATE_SHADER_RESOURCE:   return "SHADER_RESOURCE";
        case RESOURCE_STATE_STREAM_OUT:        return "STREAM_OUT";
        case RESOURCE_STATE_INDIRECT_ARGUMENT: return "INDIRECT_ARGUMENT";
        case RESOURCE_STATE_COPY_DEST:         return "COPY_DEST";
        case RESOURCE_STATE_COPY_SOURCE:       return "COPY_SOURCE";
        case RESOURCE_STATE_RESOLVE_DEST:      return "RESOLVE_DEST";
        case RESOURCE_STATE_RESOLVE_SOURCE:    return "RESOLVE_SOURCE";
        case RESOURCE_STATE_INPUT_ATTACHMENT:  return "INPUT_ATTACHMENT";
        case RESOURCE_STATE_PRESENT:           return "PRESENT";
        case RESOURCE_STATE_BUILD_AS_READ:     return "BUILD_AS_READ";
        case RESOURCE_STATE_BUILD_AS_WRITE:    return "BUILD_AS_WRITE";
        case RESOURCE_STATE_RAY_TRACING:       return "RAY_TRACING";
        case RESOURCE_STATE_COMMON:            return "COMMON";
        case RESOURCE_STATE_SHADING_RATE:      return "SHADING_RATE";
        // clang-format on
        default:
            UNEXPECTED("Unknown resource state");
            return "UNKNOWN";
    }
}

String GetResourceStateString(RESOURCE_STATE State)
{
    if (State == RESOURCE_STATE_UNKNOWN)
        return "UNKNOWN";

    String str;
    while (State != 0)
    {
        if (!str.empty())
            str.push_back('|');

        auto lsb = State & ~(State - 1);

        const auto* StateFlagString = GetResourceStateFlagString(static_cast<RESOURCE_STATE>(lsb));
        str.append(StateFlagString);
        State = static_cast<RESOURCE_STATE>(State & ~lsb);
    }
    return str;
}

const char* GetQueryTypeString(QUERY_TYPE QueryType)
{
    static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled");
    switch (QueryType)
    {
        // clang-format off
        case QUERY_TYPE_UNDEFINED:           return "QUERY_TYPE_UNDEFINED";
        case QUERY_TYPE_OCCLUSION:           return "QUERY_TYPE_OCCLUSION";
        case QUERY_TYPE_BINARY_OCCLUSION:    return "QUERY_TYPE_BINARY_OCCLUSION";
        case QUERY_TYPE_TIMESTAMP:           return "QUERY_TYPE_TIMESTAMP";
        case QUERY_TYPE_PIPELINE_STATISTICS: return "QUERY_TYPE_PIPELINE_STATISTICS";
        case QUERY_TYPE_DURATION:            return "QUERY_TYPE_DURATION";
        // clang-format on
        default:
            UNEXPECTED("Unexpected query type");
            return "Unknown";
    }
}

const char* GetSurfaceTransformString(SURFACE_TRANSFORM SrfTransform)
{
    // clang-format off
    switch (SrfTransform)
    {
#define SRF_TRANSFORM_CASE(Transform) case Transform: return #Transform

        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_OPTIMAL);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_IDENTITY);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_ROTATE_90);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_ROTATE_180);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_ROTATE_270);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_HORIZONTAL_MIRROR);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180);
        SRF_TRANSFORM_CASE(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270);

#undef SRF_TRANSFORM_CASE

        default:
            UNEXPECTED("Unexpected surface transform");
            return "UNKNOWN";
    }
    // clang-format on
}

const char* GetPipelineTypeString(PIPELINE_TYPE PipelineType)
{
    static_assert(PIPELINE_TYPE_LAST == 4, "Please update this function to handle the new pipeline type");
    switch (PipelineType)
    {
        // clang-format off
        case PIPELINE_TYPE_COMPUTE:     return "compute";
        case PIPELINE_TYPE_GRAPHICS:    return "graphics";
        case PIPELINE_TYPE_MESH:        return "mesh";
        case PIPELINE_TYPE_RAY_TRACING: return "ray tracing";
        case PIPELINE_TYPE_TILE:        return "tile";
        // clang-format on
        default:
            UNEXPECTED("Unexpected pipeline type");
            return "unknown";
    }
}

const char* GetShaderCompilerTypeString(SHADER_COMPILER Compiler)
{
    static_assert(SHADER_COMPILER_LAST == 3, "Please update this function to handle the new shader compiler");
    switch (Compiler)
    {
        // clang-format off
        case SHADER_COMPILER_DEFAULT: return "Default";
        case SHADER_COMPILER_GLSLANG: return "glslang";
        case SHADER_COMPILER_DXC:     return "DXC";
        case SHADER_COMPILER_FXC:     return "FXC";
        // clang-format on
        default:
            UNEXPECTED("Unexpected shader compiler");
            return "UNKNOWN";
    }
}

const char* GetArchiveDeviceDataFlagString(ARCHIVE_DEVICE_DATA_FLAGS Flag, bool bGetFullName)
{
    static_assert(ARCHIVE_DEVICE_DATA_FLAG_LAST == 2 << 6, "Please update this function to handle the new archive device data flag");
    switch (Flag)
    {
        // clang-format off
        case ARCHIVE_DEVICE_DATA_FLAG_NONE:        return bGetFullName ? "ARCHIVE_DEVICE_DATA_FLAG_NONE"        : "None";
        case ARCHIVE_DEVICE_DATA_FLAG_D3D11:       return bGetFullName ? "ARCHIVE_DEVICE_DATA_FLAG_D3D11"       : "D3D11";
        case ARCHIVE_DEVICE_DATA_FLAG_D3D12:       return bGetFullName ? "ARCHIVE_DEVICE_DATA_FLAG_D3D12"       : "D3D12";
        case ARCHIVE_DEVICE_DATA_FLAG_GL:          return bGetFullName ? "ARCHIVE_DEVICE_DATA_FLAG_GL"          : "OpenGL";
        case ARCHIVE_DEVICE_DATA_FLAG_GLES:        return bGetFullName ? "ARCHIVE_DEVICE_DATA_FLAG_GLES"        : "OpenGLES";
        case ARCHIVE_DEVICE_DATA_FLAG_VULKAN:      return bGetFullName ? "ARCHIVE_DEVICE_DATA_FLAG_VULKAN"      : "Vulkan";
        case ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS: return bGetFullName ? "ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS" : "Metal_MacOS";
        case ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS:   return bGetFullName ? "ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS"   : "Metal_IOS";
        // clang-format on
        default:
            UNEXPECTED("Unexpected device data flag (", Uint32{Flag}, ")");
            return "UNKNOWN";
    }
}

const char* GetDeviceFeatureStateString(DEVICE_FEATURE_STATE State, bool bGetFullName)
{
    switch (State)
    {
        // clang-format off
        case DEVICE_FEATURE_STATE_DISABLED: return bGetFullName ? "DEVICE_FEATURE_STATE_DISABLED" : "Disabled";
        case DEVICE_FEATURE_STATE_OPTIONAL: return bGetFullName ? "DEVICE_FEATURE_STATE_OPTIONAL" : "Optional";
        case DEVICE_FEATURE_STATE_ENABLED:  return bGetFullName ? "DEVICE_FEATURE_STATE_ENABLED"  : "Enabled";
        // clang-format on
        default:
            UNEXPECTED("Unexpected device feature state (", Uint32{State}, ")");
            return "UNKNOWN";
    }
}

const char* GetRenderDeviceTypeString(RENDER_DEVICE_TYPE DeviceType, bool bGetEnumString)
{
    static_assert(RENDER_DEVICE_TYPE_COUNT == 7, "Did you add a new device type? Please update the switch below.");
    switch (DeviceType)
    {
        // clang-format off
        case RENDER_DEVICE_TYPE_UNDEFINED: return bGetEnumString ? "RENDER_DEVICE_TYPE_UNDEFINED" : "Undefined";  break;
        case RENDER_DEVICE_TYPE_D3D11:     return bGetEnumString ? "RENDER_DEVICE_TYPE_D3D11"     : "Direct3D11"; break;
        case RENDER_DEVICE_TYPE_D3D12:     return bGetEnumString ? "RENDER_DEVICE_TYPE_D3D12"     : "Direct3D12"; break;
        case RENDER_DEVICE_TYPE_GL:        return bGetEnumString ? "RENDER_DEVICE_TYPE_GL"        : "OpenGL";     break;
        case RENDER_DEVICE_TYPE_GLES:      return bGetEnumString ? "RENDER_DEVICE_TYPE_GLES"      : "OpenGLES";   break;
        case RENDER_DEVICE_TYPE_VULKAN:    return bGetEnumString ? "RENDER_DEVICE_TYPE_VULKAN"    : "Vulkan";     break;
        case RENDER_DEVICE_TYPE_METAL:     return bGetEnumString ? "RENDER_DEVICE_TYPE_METAL"     : "Metal";      break;
        // clang-format on
        default: UNEXPECTED("Unknown/unsupported device type"); return "UNKNOWN";
    }
}

const char* GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE DeviceType, bool Capital)
{
    static_assert(RENDER_DEVICE_TYPE_COUNT == 7, "Did you add a new device type? Please update the switch below.");
    switch (DeviceType)
    {
        // clang-format off
        case RENDER_DEVICE_TYPE_UNDEFINED: return Capital ? "UNDEFINED" : "undefined"; break;
        case RENDER_DEVICE_TYPE_D3D11:     return Capital ? "D3D11"     : "d3d11";     break;
        case RENDER_DEVICE_TYPE_D3D12:     return Capital ? "D3D12"     : "d3d12";     break;
        case RENDER_DEVICE_TYPE_GL:        return Capital ? "GL"        : "gl";        break;
        case RENDER_DEVICE_TYPE_GLES:      return Capital ? "GLES"      : "gles";      break;
        case RENDER_DEVICE_TYPE_VULKAN:    return Capital ? "VK"        : "vk";        break;
        case RENDER_DEVICE_TYPE_METAL:     return Capital ? "MTL"       : "mtl";       break;
        // clang-format on
        default: UNEXPECTED("Unknown/unsupported device type"); return "UNKNOWN";
    }
}

const char* GetAdapterTypeString(ADAPTER_TYPE AdapterType, bool bGetEnumString)
{
    static_assert(ADAPTER_TYPE_COUNT == 4, "Did you add a new adapter type? Please update the switch below.");
    switch (AdapterType)
    {
        // clang-format off
        case ADAPTER_TYPE_UNKNOWN:    return bGetEnumString ? "ADAPTER_TYPE_UNKNOWN"    : "Unknown";    break;
        case ADAPTER_TYPE_SOFTWARE:   return bGetEnumString ? "ADAPTER_TYPE_SOFTWARE"   : "Software";   break;
        case ADAPTER_TYPE_INTEGRATED: return bGetEnumString ? "ADAPTER_TYPE_INTEGRATED" : "Integrated"; break;
        case ADAPTER_TYPE_DISCRETE:   return bGetEnumString ? "ADAPTER_TYPE_DISCRETE"   : "Discrete";   break;
        // clang-format on
        default: UNEXPECTED("Unknown/unsupported adapter type"); return "UNKNOWN";
    }
}

String GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAGS Flags, bool GetFullName /*= false*/, const char* DelimiterString /*= "|"*/)
{
    if (Flags == PIPELINE_RESOURCE_FLAG_NONE)
        return GetFullName ? "PIPELINE_RESOURCE_FLAG_NONE" : "UNKNOWN";
    String Str;
    while (Flags != PIPELINE_RESOURCE_FLAG_NONE)
    {
        if (!Str.empty())
            Str += DelimiterString;

        auto Flag = ExtractLSB(Flags);

        static_assert(PIPELINE_RESOURCE_FLAG_LAST == (1u << 4), "Please update the switch below to handle the new pipeline resource flag.");
        switch (Flag)
        {
            case PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS:
                Str.append(GetFullName ? "PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS" : "NO_DYNAMIC_BUFFERS");
                break;

            case PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER:
                Str.append(GetFullName ? "PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER" : "COMBINED_SAMPLER");
                break;

            case PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER:
                Str.append(GetFullName ? "PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER" : "FORMATTED_BUFFER");
                break;

            case PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY:
                Str.append(GetFullName ? "PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY" : "RUNTIME_ARRAY");
                break;

            case PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT:
                Str.append(GetFullName ? "PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT" : "GENERAL_INPUT_ATTACHMENT");
                break;

            default:
                UNEXPECTED("Unexpected pipeline resource flag");
        }
    }
    return Str;
}

const char* GetShaderCodeVariableClassString(SHADER_CODE_VARIABLE_CLASS Class)
{
    static_assert(SHADER_CODE_VARIABLE_CLASS_COUNT == 6, "Did you add a new variable class? Please update the switch below.");
    switch (Class)
    {
        // clang-format off
        case SHADER_CODE_VARIABLE_CLASS_UNKNOWN:        return "unknown";
        case SHADER_CODE_VARIABLE_CLASS_SCALAR:         return "scalar";
        case SHADER_CODE_VARIABLE_CLASS_VECTOR:         return "vector";
        case SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS:    return "matrix-rows";
        case SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS: return "matrix-columns";
        case SHADER_CODE_VARIABLE_CLASS_STRUCT:         return "struct";
        // clang-format on
        default: UNEXPECTED("Unknown/unsupported variable class"); return "UNKNOWN";
    }
}

const char* GetShaderCodeBasicTypeString(SHADER_CODE_BASIC_TYPE Type)
{
    static_assert(SHADER_CODE_BASIC_TYPE_COUNT == 21, "Did you add a new type? Please update the switch below.");
    switch (Type)
    {
        // clang-format off
        case SHADER_CODE_BASIC_TYPE_UNKNOWN:    return "unknown";
        case SHADER_CODE_BASIC_TYPE_VOID:       return "void";
        case SHADER_CODE_BASIC_TYPE_BOOL:       return "bool";
        case SHADER_CODE_BASIC_TYPE_INT:        return "int";
        case SHADER_CODE_BASIC_TYPE_INT8:       return "int8";
        case SHADER_CODE_BASIC_TYPE_INT16:      return "int16";
        case SHADER_CODE_BASIC_TYPE_INT64:      return "int64";
        case SHADER_CODE_BASIC_TYPE_UINT:       return "uint";
        case SHADER_CODE_BASIC_TYPE_UINT8:      return "uint8";
        case SHADER_CODE_BASIC_TYPE_UINT16:     return "uint16";
        case SHADER_CODE_BASIC_TYPE_UINT64:     return "uint64";
        case SHADER_CODE_BASIC_TYPE_FLOAT:      return "float";
        case SHADER_CODE_BASIC_TYPE_FLOAT16:    return "float16";
        case SHADER_CODE_BASIC_TYPE_DOUBLE:     return "double";
        case SHADER_CODE_BASIC_TYPE_MIN8FLOAT:  return "min8float";
        case SHADER_CODE_BASIC_TYPE_MIN10FLOAT: return "min10float";
        case SHADER_CODE_BASIC_TYPE_MIN16FLOAT: return "min16float";
        case SHADER_CODE_BASIC_TYPE_MIN12INT:   return "min12int";
        case SHADER_CODE_BASIC_TYPE_MIN16INT:   return "min16int";
        case SHADER_CODE_BASIC_TYPE_MIN16UINT:  return "min16uint";
        case SHADER_CODE_BASIC_TYPE_STRING:     return "string";
        // clang-format on
        default: UNEXPECTED("Unknown/unsupported variable class"); return "UNKNOWN";
    }
}

static void PrintShaderCodeVariables(std::stringstream& ss, size_t LevelIdent, size_t IdentShift, const ShaderCodeVariableDesc* pVars, Uint32 NumVars)
{
    if (pVars == nullptr || NumVars == 0)
        return;

    int MaxNameLen      = 0;
    int MaxTypeLen      = 0;
    int MaxArraySizeLen = 0;
    int MaxOffsetLen    = 0;
    int MaxClassLen     = 0;
    int MaxBasicTypeLen = 0;
    for (Uint32 i = 0; i < NumVars; ++i)
    {
        const auto& Var = pVars[i];
        if (Var.Name != nullptr)
            MaxNameLen = std::max(MaxNameLen, static_cast<int>(strlen(Var.Name)));
        if (Var.TypeName != nullptr)
            MaxTypeLen = std::max(MaxTypeLen, static_cast<int>(strlen(Var.TypeName)));
        MaxArraySizeLen = std::max(MaxArraySizeLen, static_cast<int>(GetPrintWidth(Var.ArraySize)));
        MaxOffsetLen    = std::max(MaxOffsetLen, static_cast<int>(GetPrintWidth(Var.Offset)));
        MaxClassLen     = std::max(MaxClassLen, static_cast<int>(strlen(GetShaderCodeVariableClassString(Var.Class))));
        MaxBasicTypeLen = std::max(MaxBasicTypeLen, static_cast<int>(strlen(GetShaderCodeBasicTypeString(Var.BasicType))));
    }

    for (Uint32 i = 0; i < NumVars; ++i)
    {
        const auto& Var = pVars[i];
        ss << std::setw(static_cast<int>(LevelIdent) + MaxNameLen) << (Var.Name ? Var.Name : "?")
           << ": " << std::setw(MaxTypeLen) << (Var.TypeName ? Var.TypeName : "")
           << ' ' << std::setw(MaxClassLen) << GetShaderCodeVariableClassString(Var.Class)
           << ' ' << std::setw(MaxBasicTypeLen) << GetShaderCodeBasicTypeString(Var.BasicType)
           << ' ' << Uint32{Var.NumRows} << 'x' << Uint32{Var.NumColumns} << " [" << std::setw(MaxArraySizeLen) << Var.ArraySize << ']'
           << " offset: " << std::setw(MaxOffsetLen) << Var.Offset << std::endl;

        PrintShaderCodeVariables(ss, LevelIdent + MaxNameLen + IdentShift, IdentShift, Var.pMembers, Var.NumMembers);
    }
}

/// Returns the string containing the shader buffer description.
String GetShaderCodeBufferDescString(const ShaderCodeBufferDesc& Desc, size_t GlobalIdent, size_t MemberIdent)
{
    std::stringstream ss;
    ss << std::setw(static_cast<int>(GlobalIdent)) << ' ' << "Size: " << Desc.Size << std::endl
       << std::setw(static_cast<int>(GlobalIdent)) << ' ' << "Vars: " << Desc.NumVariables << std::endl;
    PrintShaderCodeVariables(ss, GlobalIdent + MemberIdent, MemberIdent, Desc.pVariables, Desc.NumVariables);

    return ss.str();
}

String GetShaderCodeVariableDescString(const ShaderCodeVariableDesc& Desc, size_t GlobalIdent, size_t MemberIdent)
{
    std::stringstream ss;
    PrintShaderCodeVariables(ss, GlobalIdent, MemberIdent, &Desc, 1);
    return ss.str();
}

PIPELINE_RESOURCE_FLAGS GetValidPipelineResourceFlags(SHADER_RESOURCE_TYPE ResourceType)
{
    static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update the switch below to handle the new shader resource type");
    switch (ResourceType)
    {
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
            return PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY;

        case SHADER_RESOURCE_TYPE_TEXTURE_SRV:
            return PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER | PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY;

        case SHADER_RESOURCE_TYPE_BUFFER_SRV:
            return PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER | PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY;

        case SHADER_RESOURCE_TYPE_TEXTURE_UAV:
            return PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY;

        case SHADER_RESOURCE_TYPE_BUFFER_UAV:
            return PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER | PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY;

        case SHADER_RESOURCE_TYPE_SAMPLER:
            return PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY;

        case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT:
            return PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT;

        case SHADER_RESOURCE_TYPE_ACCEL_STRUCT:
            return PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY;

        default:
            UNEXPECTED("Unexpected resource type");
            return PIPELINE_RESOURCE_FLAG_NONE;
    }
}

PIPELINE_RESOURCE_FLAGS ShaderVariableFlagsToPipelineResourceFlags(SHADER_VARIABLE_FLAGS Flags)
{
    static_assert(SHADER_VARIABLE_FLAG_LAST == 0x02, "Please update the switch below to handle the new shader variable flags");
    switch (Flags)
    {
        case SHADER_VARIABLE_FLAG_NONE:
            return PIPELINE_RESOURCE_FLAG_NONE;

        case SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS:
            return PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS;

        case SHADER_VARIABLE_FLAG_GENERAL_INPUT_ATTACHMENT:
            return PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT;

        default:
            UNEXPECTED("Unexpected shader variable flag");
            return PIPELINE_RESOURCE_FLAG_NONE;
    }
}

BIND_FLAGS SwapChainUsageFlagsToBindFlags(SWAP_CHAIN_USAGE_FLAGS SwapChainUsage)
{
    BIND_FLAGS BindFlags = BIND_NONE;
    static_assert(SWAP_CHAIN_USAGE_LAST == 8, "Did you add a new swap chain usage flag? Please handle it here.");
    while (SwapChainUsage != SWAP_CHAIN_USAGE_NONE)
    {
        auto SCUsageBit = ExtractLSB(SwapChainUsage);
        switch (SCUsageBit)
        {
            case SWAP_CHAIN_USAGE_RENDER_TARGET:
                BindFlags |= BIND_RENDER_TARGET;
                break;

            case SWAP_CHAIN_USAGE_SHADER_RESOURCE:
                BindFlags |= BIND_SHADER_RESOURCE;
                break;

            case SWAP_CHAIN_USAGE_INPUT_ATTACHMENT:
                BindFlags |= BIND_INPUT_ATTACHMENT;
                break;

            case SWAP_CHAIN_USAGE_COPY_SOURCE:
                // No special bind flag needed
                break;

            default:
                UNEXPECTED("Unexpeced swap chain usage flag");
        }
    }
    return BindFlags;
}

Uint32 ComputeMipLevelsCount(Uint32 Width)
{
    if (Width == 0)
        return 0;

    Uint32 MipLevels = 0;
    while ((Width >> MipLevels) > 0)
        ++MipLevels;
    VERIFY(Width >= (1U << (MipLevels - 1)) && Width < (1U << MipLevels), "Incorrect number of Mip levels");
    return MipLevels;
}

Uint32 ComputeMipLevelsCount(Uint32 Width, Uint32 Height)
{
    return ComputeMipLevelsCount(std::max(Width, Height));
}

Uint32 ComputeMipLevelsCount(Uint32 Width, Uint32 Height, Uint32 Depth)
{
    return ComputeMipLevelsCount(std::max(std::max(Width, Height), Depth));
}

bool VerifyResourceStates(RESOURCE_STATE State, bool IsTexture)
{
    static_assert(RESOURCE_STATE_MAX_BIT == (1u << 21), "Please update this function to handle the new resource state");

    // clang-format off
#define VERIFY_EXCLUSIVE_STATE(ExclusiveState)\
if ( (State & ExclusiveState) != 0 && (State & ~ExclusiveState) != 0 )\
{\
    LOG_ERROR_MESSAGE("State ", GetResourceStateString(State), " is invalid: " #ExclusiveState " can't be combined with any other state");\
    return false;\
}

    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_COMMON);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_UNDEFINED);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_UNORDERED_ACCESS);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_RENDER_TARGET);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_DEPTH_WRITE);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_COPY_DEST);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_RESOLVE_DEST);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_PRESENT);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_BUILD_AS_WRITE);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_RAY_TRACING);
    VERIFY_EXCLUSIVE_STATE(RESOURCE_STATE_SHADING_RATE);
#undef VERIFY_EXCLUSIVE_STATE
    // clang-format on

    if (IsTexture)
    {
        // clang-format off
        if (State &
            (RESOURCE_STATE_VERTEX_BUFFER   |
             RESOURCE_STATE_CONSTANT_BUFFER |
             RESOURCE_STATE_INDEX_BUFFER    |
             RESOURCE_STATE_STREAM_OUT      |
             RESOURCE_STATE_INDIRECT_ARGUMENT))
        {
            LOG_ERROR_MESSAGE("State ", GetResourceStateString(State), " is invalid: states RESOURCE_STATE_VERTEX_BUFFER, "
                              "RESOURCE_STATE_CONSTANT_BUFFER, RESOURCE_STATE_INDEX_BUFFER, RESOURCE_STATE_STREAM_OUT, "
                              "RESOURCE_STATE_INDIRECT_ARGUMENT are not applicable to textures");
            return false;
        }
        // clang-format on
    }
    else
    {
        // clang-format off
        if (State &
            (RESOURCE_STATE_RENDER_TARGET  |
             RESOURCE_STATE_DEPTH_WRITE    |
             RESOURCE_STATE_DEPTH_READ     |
             RESOURCE_STATE_RESOLVE_SOURCE |
             RESOURCE_STATE_RESOLVE_DEST   |
             RESOURCE_STATE_PRESENT        |
             RESOURCE_STATE_SHADING_RATE   |
             RESOURCE_STATE_INPUT_ATTACHMENT))
        {
            LOG_ERROR_MESSAGE("State ", GetResourceStateString(State), " is invalid: states RESOURCE_STATE_RENDER_TARGET, "
                              "RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_RESOLVE_SOURCE, "
                              "RESOURCE_STATE_RESOLVE_DEST, RESOURCE_STATE_PRESENT, RESOURCE_STATE_INPUT_ATTACHMENT, RESOURCE_STATE_SHADING_RATE "
                              "are not applicable to buffers");
            return false;
        }
        // clang-format on
    }

    return true;
}

MipLevelProperties GetMipLevelProperties(const TextureDesc& TexDesc, Uint32 MipLevel)
{
    MipLevelProperties MipProps;
    const auto&        FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);

    MipProps.LogicalWidth  = std::max(TexDesc.GetWidth() >> MipLevel, 1u);
    MipProps.LogicalHeight = std::max(TexDesc.GetHeight() >> MipLevel, 1u);
    MipProps.Depth         = std::max(TexDesc.GetDepth() >> MipLevel, 1u);
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        VERIFY_EXPR(FmtAttribs.BlockWidth > 1 && FmtAttribs.BlockHeight > 1);
        VERIFY((FmtAttribs.BlockWidth & (FmtAttribs.BlockWidth - 1)) == 0, "Compressed block width is expected to be power of 2");
        VERIFY((FmtAttribs.BlockHeight & (FmtAttribs.BlockHeight - 1)) == 0, "Compressed block height is expected to be power of 2");
        // For block-compression formats, all parameters are still specified in texels rather than compressed texel blocks (18.4.1)
        MipProps.StorageWidth   = AlignUp(MipProps.LogicalWidth, Uint32{FmtAttribs.BlockWidth});
        MipProps.StorageHeight  = AlignUp(MipProps.LogicalHeight, Uint32{FmtAttribs.BlockHeight});
        MipProps.RowSize        = Uint64{MipProps.StorageWidth} / Uint32{FmtAttribs.BlockWidth} * Uint32{FmtAttribs.ComponentSize}; // ComponentSize is the block size
        MipProps.DepthSliceSize = MipProps.StorageHeight / Uint32{FmtAttribs.BlockHeight} * MipProps.RowSize;
        MipProps.MipSize        = MipProps.DepthSliceSize * MipProps.Depth;
    }
    else
    {
        MipProps.StorageWidth   = MipProps.LogicalWidth;
        MipProps.StorageHeight  = MipProps.LogicalHeight;
        MipProps.RowSize        = Uint64{MipProps.StorageWidth} * Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        MipProps.DepthSliceSize = MipProps.RowSize * MipProps.StorageHeight;
        MipProps.MipSize        = MipProps.DepthSliceSize * MipProps.Depth;
    }

    return MipProps;
}

namespace
{

enum ADAPTER_VENDOR_ID
{
    ADAPTER_VENDOR_ID_AMD      = 0x01002,
    ADAPTER_VENDOR_ID_NVIDIA   = 0x010DE,
    ADAPTER_VENDOR_ID_INTEL    = 0x08086,
    ADAPTER_VENDOR_ID_ARM      = 0x013B5,
    ADAPTER_VENDOR_ID_QUALCOMM = 0x05143,
    ADAPTER_VENDOR_ID_IMGTECH  = 0x01010,
    ADAPTER_VENDOR_ID_MSFT     = 0x01414,
    ADAPTER_VENDOR_ID_APPLE    = 0x0106B,
    ADAPTER_VENDOR_ID_MESA     = 0x10005,
    ADAPTER_VENDOR_ID_BROADCOM = 0x014e4
};

}

ADAPTER_VENDOR VendorIdToAdapterVendor(Uint32 VendorId)
{
    static_assert(ADAPTER_VENDOR_LAST == 10, "Please update the switch below to handle the new adapter type");
    switch (VendorId)
    {
#define VENDOR_ID_TO_VENDOR(Name) \
    case ADAPTER_VENDOR_ID_##Name: return ADAPTER_VENDOR_##Name

        VENDOR_ID_TO_VENDOR(AMD);
        VENDOR_ID_TO_VENDOR(NVIDIA);
        VENDOR_ID_TO_VENDOR(INTEL);
        VENDOR_ID_TO_VENDOR(ARM);
        VENDOR_ID_TO_VENDOR(QUALCOMM);
        VENDOR_ID_TO_VENDOR(IMGTECH);
        VENDOR_ID_TO_VENDOR(MSFT);
        VENDOR_ID_TO_VENDOR(APPLE);
        VENDOR_ID_TO_VENDOR(MESA);
        VENDOR_ID_TO_VENDOR(BROADCOM);
#undef VENDOR_ID_TO_VENDOR

        default:
            return ADAPTER_VENDOR_UNKNOWN;
    }
}

Uint32 AdapterVendorToVendorId(ADAPTER_VENDOR Vendor)
{
    static_assert(ADAPTER_VENDOR_LAST == 10, "Please update the switch below to handle the new adapter type");
    switch (Vendor)
    {
#define VENDOR_TO_VENDOR_ID(Name) \
    case ADAPTER_VENDOR_##Name: return ADAPTER_VENDOR_ID_##Name

        VENDOR_TO_VENDOR_ID(AMD);
        VENDOR_TO_VENDOR_ID(NVIDIA);
        VENDOR_TO_VENDOR_ID(INTEL);
        VENDOR_TO_VENDOR_ID(ARM);
        VENDOR_TO_VENDOR_ID(QUALCOMM);
        VENDOR_TO_VENDOR_ID(IMGTECH);
        VENDOR_TO_VENDOR_ID(MSFT);
        VENDOR_TO_VENDOR_ID(APPLE);
        VENDOR_TO_VENDOR_ID(MESA);
        VENDOR_TO_VENDOR_ID(BROADCOM);
#undef VENDOR_TO_VENDOR_ID

        default:
            return 0;
    }
}

bool IsConsistentShaderType(SHADER_TYPE ShaderType, PIPELINE_TYPE PipelineType)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the switch below to handle the new shader type");
    static_assert(PIPELINE_TYPE_LAST == 4, "Please update the switch below to handle the new pipeline type");
    switch (PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
            return ShaderType == SHADER_TYPE_VERTEX ||
                ShaderType == SHADER_TYPE_HULL ||
                ShaderType == SHADER_TYPE_DOMAIN ||
                ShaderType == SHADER_TYPE_GEOMETRY ||
                ShaderType == SHADER_TYPE_PIXEL;

        case PIPELINE_TYPE_COMPUTE:
            return ShaderType == SHADER_TYPE_COMPUTE;

        case PIPELINE_TYPE_MESH:
            return ShaderType == SHADER_TYPE_AMPLIFICATION ||
                ShaderType == SHADER_TYPE_MESH ||
                ShaderType == SHADER_TYPE_PIXEL;

        case PIPELINE_TYPE_RAY_TRACING:
            return ShaderType == SHADER_TYPE_RAY_GEN ||
                ShaderType == SHADER_TYPE_RAY_MISS ||
                ShaderType == SHADER_TYPE_RAY_CLOSEST_HIT ||
                ShaderType == SHADER_TYPE_RAY_ANY_HIT ||
                ShaderType == SHADER_TYPE_RAY_INTERSECTION ||
                ShaderType == SHADER_TYPE_CALLABLE;

        case PIPELINE_TYPE_TILE:
            return ShaderType == SHADER_TYPE_TILE;

        default:
            UNEXPECTED("Unexpected pipeline type");
            return false;
    }
}

Int32 GetShaderTypePipelineIndex(SHADER_TYPE ShaderType, PIPELINE_TYPE PipelineType)
{
    VERIFY(IsConsistentShaderType(ShaderType, PipelineType), "Shader type ", GetShaderTypeLiteralName(ShaderType),
           " is inconsistent with pipeline type ", GetPipelineTypeString(PipelineType));
    VERIFY(ShaderType == SHADER_TYPE_UNKNOWN || IsPowerOfTwo(ShaderType), "More than one shader type is specified");

    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the switch below to handle the new shader type");
    switch (ShaderType)
    {
        case SHADER_TYPE_UNKNOWN:
            return -1;

        case SHADER_TYPE_VERTEX:        // Graphics
        case SHADER_TYPE_AMPLIFICATION: // Mesh
        case SHADER_TYPE_COMPUTE:       // Compute
        case SHADER_TYPE_RAY_GEN:       // Ray tracing
        case SHADER_TYPE_TILE:          // Tile
            return 0;

        case SHADER_TYPE_HULL:     // Graphics
        case SHADER_TYPE_MESH:     // Mesh
        case SHADER_TYPE_RAY_MISS: // RayTracing
            return 1;

        case SHADER_TYPE_DOMAIN:          // Graphics
        case SHADER_TYPE_RAY_CLOSEST_HIT: // Ray tracing
            return 2;

        case SHADER_TYPE_GEOMETRY:    // Graphics
        case SHADER_TYPE_RAY_ANY_HIT: // Ray tracing
            return 3;

        case SHADER_TYPE_PIXEL:            // Graphics or Mesh
        case SHADER_TYPE_RAY_INTERSECTION: // Ray tracing
            return 4;

        case SHADER_TYPE_CALLABLE: // RayTracing
            return 5;

        default:
            UNEXPECTED("Unexpected shader type (", ShaderType, ")");
            return -1;
    }
}

SHADER_TYPE GetShaderTypeFromPipelineIndex(Int32 Index, PIPELINE_TYPE PipelineType)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the switch below to handle the new shader type");
    static_assert(PIPELINE_TYPE_LAST == 4, "Please update the switch below to handle the new pipeline type");
    switch (PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
            switch (Index)
            {
                case 0: return SHADER_TYPE_VERTEX;
                case 1: return SHADER_TYPE_HULL;
                case 2: return SHADER_TYPE_DOMAIN;
                case 3: return SHADER_TYPE_GEOMETRY;
                case 4: return SHADER_TYPE_PIXEL;

                default:
                    UNEXPECTED("Index ", Index, " is not a valid graphics pipeline shader index");
                    return SHADER_TYPE_UNKNOWN;
            }

        case PIPELINE_TYPE_COMPUTE:
            switch (Index)
            {
                case 0: return SHADER_TYPE_COMPUTE;

                default:
                    UNEXPECTED("Index ", Index, " is not a valid compute pipeline shader index");
                    return SHADER_TYPE_UNKNOWN;
            }

        case PIPELINE_TYPE_MESH:
            switch (Index)
            {
                case 0: return SHADER_TYPE_AMPLIFICATION;
                case 1: return SHADER_TYPE_MESH;
                case 4: return SHADER_TYPE_PIXEL;

                default:
                    UNEXPECTED("Index ", Index, " is not a valid mesh pipeline shader index");
                    return SHADER_TYPE_UNKNOWN;
            }

        case PIPELINE_TYPE_RAY_TRACING:
            switch (Index)
            {
                case 0: return SHADER_TYPE_RAY_GEN;
                case 1: return SHADER_TYPE_RAY_MISS;
                case 2: return SHADER_TYPE_RAY_CLOSEST_HIT;
                case 3: return SHADER_TYPE_RAY_ANY_HIT;
                case 4: return SHADER_TYPE_RAY_INTERSECTION;
                case 5: return SHADER_TYPE_CALLABLE;

                default:
                    UNEXPECTED("Index ", Index, " is not a valid ray tracing pipeline shader index");
                    return SHADER_TYPE_UNKNOWN;
            }

        case PIPELINE_TYPE_TILE:
            switch (Index)
            {
                case 0: return SHADER_TYPE_TILE;

                default:
                    UNEXPECTED("Index ", Index, " is not a valid tile pipeline shader index");
                    return SHADER_TYPE_UNKNOWN;
            }

        default:
            UNEXPECTED("Unexpected pipeline type");
            return SHADER_TYPE_UNKNOWN;
    }
}

PIPELINE_TYPE PipelineTypeFromShaderStages(SHADER_TYPE ShaderStages)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the code below to handle the new shader type");
    static_assert(PIPELINE_TYPE_LAST == 4, "Please update the code below to handle the new pipeline type");

    if (ShaderStages & (SHADER_TYPE_AMPLIFICATION | SHADER_TYPE_MESH))
    {
        VERIFY((ShaderStages & SHADER_TYPE_ALL_MESH) == ShaderStages,
               "Mesh shading pipeline stages can't be combined with other shader stages");
        return PIPELINE_TYPE_MESH;
    }
    if (ShaderStages & SHADER_TYPE_ALL_GRAPHICS)
    {
        VERIFY((ShaderStages & SHADER_TYPE_ALL_GRAPHICS) == ShaderStages,
               "Graphics pipeline stages can't be combined with other shader stages");
        return PIPELINE_TYPE_GRAPHICS;
    }
    if (ShaderStages & SHADER_TYPE_COMPUTE)
    {
        VERIFY((ShaderStages & SHADER_TYPE_COMPUTE) == ShaderStages,
               "Compute stage can't be combined with any other shader stage");
        return PIPELINE_TYPE_COMPUTE;
    }
    if (ShaderStages & SHADER_TYPE_TILE)
    {
        VERIFY((ShaderStages & SHADER_TYPE_TILE) == ShaderStages,
               "Tile stage can't be combined with any other shader stage");
        return PIPELINE_TYPE_TILE;
    }
    if (ShaderStages & SHADER_TYPE_ALL_RAY_TRACING)
    {
        VERIFY((ShaderStages & SHADER_TYPE_ALL_RAY_TRACING) == ShaderStages,
               "Ray tracing pipeline stages can't be combined with other shader stages");
        return PIPELINE_TYPE_RAY_TRACING;
    }

    UNEXPECTED("Unknown shader stage");
    return PIPELINE_TYPE_INVALID;
}

Uint64 GetStagingTextureLocationOffset(const TextureDesc& TexDesc,
                                       Uint32             ArraySlice,
                                       Uint32             MipLevel,
                                       Uint32             Alignment,
                                       Uint32             LocationX,
                                       Uint32             LocationY,
                                       Uint32             LocationZ)
{
    VERIFY_EXPR(TexDesc.MipLevels > 0 && TexDesc.GetArraySize() > 0 && TexDesc.Width > 0 && TexDesc.Height > 0 && TexDesc.Format != TEX_FORMAT_UNKNOWN);
    VERIFY_EXPR(ArraySlice < TexDesc.GetArraySize() && MipLevel < TexDesc.MipLevels || ArraySlice == TexDesc.GetArraySize() && MipLevel == 0);

    Uint64 Offset = 0;
    if (ArraySlice > 0)
    {
        Uint64 ArraySliceSize = 0;
        for (Uint32 mip = 0; mip < TexDesc.MipLevels; ++mip)
        {
            auto MipInfo = GetMipLevelProperties(TexDesc, mip);
            ArraySliceSize += AlignUp(MipInfo.MipSize, Alignment);
        }

        Offset = ArraySliceSize;
        if (TexDesc.IsArray())
            Offset *= ArraySlice;
    }

    for (Uint32 mip = 0; mip < MipLevel; ++mip)
    {
        auto MipInfo = GetMipLevelProperties(TexDesc, mip);
        Offset += AlignUp(MipInfo.MipSize, Alignment);
    }

    if (ArraySlice == TexDesc.GetArraySize())
    {
        VERIFY(LocationX == 0 && LocationY == 0 && LocationZ == 0,
               "Staging buffer size is requested: location must be (0,0,0).");
    }
    else if (LocationX != 0 || LocationY != 0 || LocationZ != 0)
    {
        const auto& MipLevelAttribs = GetMipLevelProperties(TexDesc, MipLevel);
        const auto& FmtAttribs      = GetTextureFormatAttribs(TexDesc.Format);
        VERIFY(LocationX < MipLevelAttribs.LogicalWidth && LocationY < MipLevelAttribs.LogicalHeight && LocationZ < MipLevelAttribs.Depth,
               "Specified location is out of bounds");
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
        {
            VERIFY((LocationX % FmtAttribs.BlockWidth) == 0 && (LocationY % FmtAttribs.BlockHeight) == 0,
                   "For compressed texture formats, location must be a multiple of compressed block size.");
        }

        // For compressed-block formats, RowSize is the size of one compressed row.
        // For non-compressed formats, BlockHeight is 1.
        Offset += (LocationZ * MipLevelAttribs.StorageHeight + LocationY) / FmtAttribs.BlockHeight * MipLevelAttribs.RowSize;

        // For non-compressed formats, BlockWidth is 1.
        Offset += Uint64{LocationX / FmtAttribs.BlockWidth} * FmtAttribs.GetElementSize();

        // Note: this addressing complies with how Vulkan (as well as OpenGL/GLES and Metal) address
        // textures when copying data to/from buffers:
        //      address of (x,y,z) = bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)
    }

    return Offset;
}

BufferToTextureCopyInfo GetBufferToTextureCopyInfo(TEXTURE_FORMAT Format,
                                                   const Box&     Region,
                                                   Uint32         RowStrideAlignment)
{
    BufferToTextureCopyInfo CopyInfo;

    const auto& FmtAttribs = GetTextureFormatAttribs(Format);
    VERIFY_EXPR(Region.IsValid());
    const auto UpdateRegionWidth  = Region.Width();
    const auto UpdateRegionHeight = Region.Height();
    const auto UpdateRegionDepth  = Region.Depth();
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        // Align update region size by the block size
        VERIFY_EXPR(IsPowerOfTwo(FmtAttribs.BlockWidth));
        VERIFY_EXPR(IsPowerOfTwo(FmtAttribs.BlockHeight));
        const auto BlockAlignedRegionWidth  = AlignUp(UpdateRegionWidth, Uint32{FmtAttribs.BlockWidth});
        const auto BlockAlignedRegionHeight = AlignUp(UpdateRegionHeight, Uint32{FmtAttribs.BlockHeight});

        CopyInfo.RowSize  = Uint64{BlockAlignedRegionWidth} / Uint32{FmtAttribs.BlockWidth} * Uint32{FmtAttribs.ComponentSize};
        CopyInfo.RowCount = BlockAlignedRegionHeight / FmtAttribs.BlockHeight;
    }
    else
    {
        CopyInfo.RowSize  = Uint64{UpdateRegionWidth} * Uint32{FmtAttribs.ComponentSize} * Uint32{FmtAttribs.NumComponents};
        CopyInfo.RowCount = UpdateRegionHeight;
    }

    VERIFY_EXPR(IsPowerOfTwo(RowStrideAlignment));
    CopyInfo.RowStride = AlignUp(CopyInfo.RowSize, RowStrideAlignment);
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
    {
        CopyInfo.RowStrideInTexels = StaticCast<Uint32>(CopyInfo.RowStride / Uint64{FmtAttribs.ComponentSize} * Uint64{FmtAttribs.BlockWidth});
    }
    else
    {
        CopyInfo.RowStrideInTexels = StaticCast<Uint32>(CopyInfo.RowStride / (Uint64{FmtAttribs.ComponentSize} * Uint64{FmtAttribs.NumComponents}));
    }
    CopyInfo.DepthStride = CopyInfo.RowCount * CopyInfo.RowStride;
    CopyInfo.MemorySize  = UpdateRegionDepth * CopyInfo.DepthStride;
    CopyInfo.Region      = Region;
    return CopyInfo;
}


void CopyTextureSubresource(const TextureSubResData& SrcSubres,
                            Uint32                   NumRows,
                            Uint32                   NumDepthSlices,
                            Uint64                   RowSize,
                            void*                    pDstData,
                            Uint64                   DstRowStride,
                            Uint64                   DstDepthStride)
{
    VERIFY_EXPR(SrcSubres.pSrcBuffer == nullptr && SrcSubres.pData != nullptr);
    VERIFY_EXPR(pDstData != nullptr);
    VERIFY(SrcSubres.Stride >= RowSize, "Source data row stride (", SrcSubres.Stride, ") is smaller than the row size (", RowSize, ")");
    VERIFY(DstRowStride >= RowSize, "Dst data row stride (", DstRowStride, ") is smaller than the row size (", RowSize, ")");
    for (Uint32 z = 0; z < NumDepthSlices; ++z)
    {
        const auto* pSrcSlice = reinterpret_cast<const Uint8*>(SrcSubres.pData) + SrcSubres.DepthStride * z;
        auto*       pDstSlice = reinterpret_cast<Uint8*>(pDstData) + DstDepthStride * z;

        for (Uint32 y = 0; y < NumRows; ++y)
        {
            memcpy(pDstSlice + DstRowStride * y,
                   pSrcSlice + SrcSubres.Stride * y,
                   StaticCast<size_t>(RowSize));
        }
    }
}

String GetCommandQueueTypeString(COMMAND_QUEUE_TYPE Type)
{
    static_assert(COMMAND_QUEUE_TYPE_MAX_BIT == 0x7, "Please update the code below to handle the new command queue type");

    if (Type == COMMAND_QUEUE_TYPE_UNKNOWN)
        return "UNKNOWN";

    String Result;
    if ((Type & COMMAND_QUEUE_TYPE_GRAPHICS) == COMMAND_QUEUE_TYPE_GRAPHICS)
        Result = "GRAPHICS";
    else if ((Type & COMMAND_QUEUE_TYPE_COMPUTE) == COMMAND_QUEUE_TYPE_COMPUTE)
        Result = "COMPUTE";
    else if ((Type & COMMAND_QUEUE_TYPE_TRANSFER) == COMMAND_QUEUE_TYPE_TRANSFER)
        Result = "TRANSFER";
    else
    {
        UNEXPECTED("Unexpected context type");
        Result = "UNKNOWN";
    }

    if ((Type & COMMAND_QUEUE_TYPE_SPARSE_BINDING) != 0)
        Result += " | SPARSE_BINDING";

    return Result;
}

const Char* GetFenceTypeString(FENCE_TYPE Type)
{
    static_assert(FENCE_TYPE_LAST == 1, "Please update the switch below to handle the new fence type");
    switch (Type)
    {
        // clang-format off
        case FENCE_TYPE_CPU_WAIT_ONLY: return "CPU_WAIT_ONLY";
        case FENCE_TYPE_GENERAL:       return "GENERAL";
        // clang-format on
        default:
            UNEXPECTED("Unexpected fence type");
            return "Unknown";
    }
}

TEXTURE_FORMAT TexFormatToSRGB(TEXTURE_FORMAT Fmt)
{
    switch (Fmt)
    {
        case TEX_FORMAT_RGBA8_UNORM:
            return TEX_FORMAT_RGBA8_UNORM_SRGB;

        case TEX_FORMAT_BC1_UNORM:
            return TEX_FORMAT_BC1_UNORM_SRGB;

        case TEX_FORMAT_BC2_UNORM:
            return TEX_FORMAT_BC2_UNORM_SRGB;

        case TEX_FORMAT_BC3_UNORM:
            return TEX_FORMAT_BC3_UNORM_SRGB;

        case TEX_FORMAT_BGRA8_UNORM:
            return TEX_FORMAT_BGRA8_UNORM_SRGB;

        case TEX_FORMAT_BGRX8_UNORM:
            return TEX_FORMAT_BGRX8_UNORM_SRGB;

        case TEX_FORMAT_BC7_UNORM:
            return TEX_FORMAT_BC7_UNORM_SRGB;

        default:
            return Fmt;
    }
}

String GetPipelineShadingRateFlagsString(PIPELINE_SHADING_RATE_FLAGS Flags)
{
    if (Flags == PIPELINE_SHADING_RATE_FLAG_NONE)
        return "NONE";

    String Result;
    while (Flags != PIPELINE_SHADING_RATE_FLAG_NONE)
    {
        auto Bit = ExtractLSB(Flags);

        if (!Result.empty())
            Result += " | ";

        static_assert(PIPELINE_SHADING_RATE_FLAG_LAST == 0x02, "Please update the switch below to handle the new pipeline shading rate flag");
        switch (Bit)
        {
            // clang-format off
            case PIPELINE_SHADING_RATE_FLAG_PER_PRIMITIVE: Result += "PER_PRIMITIVE"; break;
            case PIPELINE_SHADING_RATE_FLAG_TEXTURE_BASED: Result += "TEXTURE_BASED"; break;
            // clang-format on
            default:
                UNEXPECTED("Unexpected pipeline shading rate");
                Result += "Unknown";
        }
    }
    return Result;
}

SparseTextureProperties GetStandardSparseTextureProperties(const TextureDesc& TexDesc)
{
    constexpr Uint32 SparseBlockSize = 64 << 10;
    const auto&      FmtAttribs      = GetTextureFormatAttribs(TexDesc.Format);
    const Uint32     TexelSize       = FmtAttribs.GetElementSize();
    VERIFY_EXPR(IsPowerOfTwo(TexelSize));
    VERIFY_EXPR(TexelSize >= 1 && TexelSize <= 16);
    VERIFY_EXPR(TexDesc.Is2D() || TexDesc.Is3D());
    VERIFY(TexDesc.MipLevels > 0, "Number of mipmap calculation is not supported");
    VERIFY(TexDesc.SampleCount == 1 || TexDesc.MipLevels == 1, "Multisampled textures must have 1 mip level");

    SparseTextureProperties Props;

    if (TexDesc.Is3D())
    {
        DEV_CHECK_ERR(FmtAttribs.ComponentType != COMPONENT_TYPE_COMPRESSED, "Compressed sparse 3D textures are currently not supported");

        //  | Texel size  |    Tile shape   |
        //  |-------------|-----------------|
        //  |     8-Bit   |   64 x 32 x 32  |
        //  |    16-Bit   |   32 x 32 x 32  |
        //  |    32-Bit   |   32 x 32 x 16  |
        //  |    64-Bit   |   32 x 16 x 16  |
        //  |   128-Bit   |   16 x 16 x 16  |
        Props.TileSize[0] = 64;
        Props.TileSize[1] = 32;
        Props.TileSize[2] = 32;

        constexpr size_t Remap[] = {0, 2, 1};
        for (Uint32 i = 0; (1u << i) < TexelSize; ++i)
        {
            Props.TileSize[Remap[i % 3]] /= 2;
        }
    }
    else if (TexDesc.SampleCount > 1)
    {
        VERIFY_EXPR(FmtAttribs.ComponentType != COMPONENT_TYPE_COMPRESSED);

        //  | Texel size  |   Tile shape 2x  |   Tile shape 4x  |   Tile shape 8x  |   Tile shape 16x  |
        //  |-------------|------------------|------------------|------------------|-------------------|
        //  |     8-Bit   |   128 x 256 x 1  |   128 x 128 x 1  |   64 x 128 x 1   |    64 x 64 x 1    |
        //  |    16-Bit   |   128 x 128 x 1  |   128 x  64 x 1  |   64 x  64 x 1   |    64 x 32 x 1    |
        //  |    32-Bit   |    64 x 128 x 1  |    64 x  64 x 1  |   32 x  64 x 1   |    32 x 32 x 1    |
        //  |    64-Bit   |    64 x  64 x 1  |    64 x  32 x 1  |   32 x  32 x 1   |    32 x 16 x 1    |
        //  |   128-Bit   |    32 x  64 x 1  |    32 x  32 x 1  |   16 x  32 x 1   |    16 x 16 x 1    |
        VERIFY_EXPR(IsPowerOfTwo(TexDesc.SampleCount));
        Props.TileSize[0] = 128 >> (TexDesc.SampleCount >= 8 ? 1 : 0);
        Props.TileSize[1] = 256 >> (TexDesc.SampleCount >= 4 ? (TexDesc.SampleCount >= 16 ? 2 : 1) : 0);
        Props.TileSize[2] = 1;

        constexpr size_t Remap[] = {1, 0};
        for (Uint32 i = 0; (1u << i) < TexelSize; ++i)
        {
            Props.TileSize[Remap[i & 1]] /= 2;
        }
    }
    else
    {
        Props.TileSize[0] = 256;
        Props.TileSize[1] = 256;
        Props.TileSize[2] = 1;
        if (FmtAttribs.ComponentType != COMPONENT_TYPE_COMPRESSED)
        {
            //  | Texel size  |    Tile shape   |
            //  |-------------|-----------------|
            //  |     8-Bit   |  256 x 256 x 1  |
            //  |    16-Bit   |  256 x 128 x 1  |
            //  |    32-Bit   |  128 x 128 x 1  |
            //  |    64-Bit   |  128 x  64 x 1  |
            //  |   128-Bit   |   64 x  64 x 1  |
            constexpr size_t Remap[] = {1, 0};
            for (Uint32 i = 0; (1u << i) < TexelSize; ++i)
            {
                Props.TileSize[Remap[i & 1]] /= 2;
            }
        }
        else
        {
            for (Uint32 i = 0; (FmtAttribs.ComponentSize << i) < (FmtAttribs.BlockWidth * FmtAttribs.BlockHeight); ++i)
            {
                Props.TileSize[i & 1] *= 2;
            }
        }
    }

    const auto BytesPerTile =
        (Props.TileSize[0] / FmtAttribs.BlockWidth) *
        (Props.TileSize[1] / FmtAttribs.BlockHeight) *
        Props.TileSize[2] * TexDesc.SampleCount * TexelSize;
    VERIFY_EXPR(BytesPerTile == SparseBlockSize);

    Uint64 SliceSize     = 0;
    Props.FirstMipInTail = ~0u;
    for (Uint32 Mip = 0; Mip < TexDesc.MipLevels; ++Mip)
    {
        const auto MipProps  = GetMipLevelProperties(TexDesc, Mip);
        const auto MipWidth  = MipProps.StorageWidth;
        const auto MipHeight = MipProps.StorageHeight;
        const auto MipDepth  = MipProps.Depth;

        // When the size of a texture mipmap level is at least one standard tile shape for its
        // format, the mipmap level is guaranteed to be nonpacked.
        const auto IsUnpacked =
            MipWidth >= Props.TileSize[0] &&
            MipHeight >= Props.TileSize[1] &&
            MipDepth >= Props.TileSize[2];

        if (!IsUnpacked)
        {
            // Mip tail
            if (Props.FirstMipInTail == ~0u)
            {
                Props.FirstMipInTail = Mip;
                Props.MipTailOffset  = SliceSize;
            }
            Props.MipTailSize += MipProps.MipSize;
        }
        else
        {
            const auto NumTilesInMip = GetNumSparseTilesInBox(Box{0, MipWidth, 0, MipHeight, 0, MipDepth}, Props.TileSize);
            SliceSize += Uint64{NumTilesInMip.x} * NumTilesInMip.y * NumTilesInMip.z * SparseBlockSize;
        }
    }

    Props.FirstMipInTail   = std::min(Props.FirstMipInTail, TexDesc.MipLevels);
    Props.MipTailSize      = AlignUp(Props.MipTailSize, SparseBlockSize);
    SliceSize              = SliceSize + Props.MipTailSize;
    Props.MipTailStride    = TexDesc.IsArray() ? SliceSize : 0;
    Props.AddressSpaceSize = SliceSize * TexDesc.GetArraySize();
    Props.BlockSize        = SparseBlockSize;
    Props.Flags            = SPARSE_TEXTURE_FLAG_NONE;

    VERIFY_EXPR(Props.MipTailSize % SparseBlockSize == 0);
    VERIFY_EXPR(Props.MipTailStride % SparseBlockSize == 0);
    VERIFY_EXPR(Props.AddressSpaceSize % SparseBlockSize == 0);

    return Props;
}

bool IsIdentityComponentMapping(const TextureComponentMapping& Mapping)
{
    return ((Mapping.R == TEXTURE_COMPONENT_SWIZZLE_IDENTITY || Mapping.R == TEXTURE_COMPONENT_SWIZZLE_R) &&
            (Mapping.G == TEXTURE_COMPONENT_SWIZZLE_IDENTITY || Mapping.G == TEXTURE_COMPONENT_SWIZZLE_G) &&
            (Mapping.B == TEXTURE_COMPONENT_SWIZZLE_IDENTITY || Mapping.B == TEXTURE_COMPONENT_SWIZZLE_B) &&
            (Mapping.A == TEXTURE_COMPONENT_SWIZZLE_IDENTITY || Mapping.A == TEXTURE_COMPONENT_SWIZZLE_A));
}

} // namespace Diligent
