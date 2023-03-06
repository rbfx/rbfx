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

#include <array>

#include "GraphicsAccessories.hpp"
#include "../../../../Graphics/GraphicsEngine/include/PrivateConstants.h"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(GraphicsAccessories_GraphicsAccessories, GetFilterTypeLiteralName)
{
#define TEST_FILTER_TYPE_ENUM(ENUM_VAL, ShortName)                          \
    {                                                                       \
        EXPECT_STREQ(GetFilterTypeLiteralName(ENUM_VAL, true), #ENUM_VAL);  \
        EXPECT_STREQ(GetFilterTypeLiteralName(ENUM_VAL, false), ShortName); \
    }

    // clang-format off
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_UNKNOWN,                "unknown");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_POINT,                  "point");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_LINEAR,                 "linear");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_ANISOTROPIC,            "anisotropic");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_COMPARISON_POINT,       "comparison point");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_COMPARISON_LINEAR,      "comparison linear");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_COMPARISON_ANISOTROPIC, "comparison anisotropic");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MINIMUM_POINT,          "minimum point");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MINIMUM_LINEAR,         "minimum linear");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MINIMUM_ANISOTROPIC,    "minimum anisotropic");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MAXIMUM_POINT,          "maximum point");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MAXIMUM_LINEAR,         "maximum linear");
    TEST_FILTER_TYPE_ENUM(FILTER_TYPE_MAXIMUM_ANISOTROPIC,    "maximum anisotropic");
    // clang-format on
#undef TEST_FILTER_TYPE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetTextureAddressModeLiteralName)
{
#define TEST_TEX_ADDRESS_MODE_ENUM(ENUM_VAL, ShortName)                             \
    {                                                                               \
        EXPECT_STREQ(GetTextureAddressModeLiteralName(ENUM_VAL, true), #ENUM_VAL);  \
        EXPECT_STREQ(GetTextureAddressModeLiteralName(ENUM_VAL, false), ShortName); \
    }

    // clang-format off
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_UNKNOWN,     "unknown");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_WRAP,        "wrap");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_MIRROR,      "mirror");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_CLAMP,       "clamp");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_BORDER,      "border");
    TEST_TEX_ADDRESS_MODE_ENUM(TEXTURE_ADDRESS_MIRROR_ONCE, "mirror once");
    // clang-format on
#undef TEST_TEX_ADDRESS_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetComparisonFunctionLiteralName)
{
#define TEST_COMPARISON_FUNC_ENUM(ENUM_VAL, ShortName)                              \
    {                                                                               \
        EXPECT_STREQ(GetComparisonFunctionLiteralName(ENUM_VAL, true), #ENUM_VAL);  \
        EXPECT_STREQ(GetComparisonFunctionLiteralName(ENUM_VAL, false), ShortName); \
    }

    // clang-format off
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_UNKNOWN,       "unknown");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_NEVER,         "never");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_LESS,          "less");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_EQUAL,         "equal");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_LESS_EQUAL,    "less equal");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_GREATER,       "greater");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_NOT_EQUAL,     "not equal");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_GREATER_EQUAL, "greater equal");
    TEST_COMPARISON_FUNC_ENUM(COMPARISON_FUNC_ALWAYS,        "always");
    // clang-format on
#undef TEST_TEX_ADDRESS_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetBlendFactorLiteralName)
{
#define TEST_BLEND_FACTOR_ENUM(ENUM_VAL)                              \
    {                                                                 \
        EXPECT_STREQ(GetBlendFactorLiteralName(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_UNDEFINED);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_ZERO);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_ONE);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_SRC_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_SRC_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_DEST_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_DEST_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_DEST_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_DEST_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC_ALPHA_SAT);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_BLEND_FACTOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_BLEND_FACTOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC1_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_SRC1_COLOR);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_SRC1_ALPHA);
    TEST_BLEND_FACTOR_ENUM(BLEND_FACTOR_INV_SRC1_ALPHA);
#undef TEST_TEX_ADDRESS_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetBlendOperationLiteralName)
{
#define TEST_BLEND_OP_ENUM(ENUM_VAL)                                     \
    {                                                                    \
        EXPECT_STREQ(GetBlendOperationLiteralName(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_BLEND_OP_ENUM(BLEND_OPERATION_UNDEFINED);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_ADD);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_SUBTRACT);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_REV_SUBTRACT);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_MIN);
    TEST_BLEND_OP_ENUM(BLEND_OPERATION_MAX);
#undef TEST_BLEND_OP_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetFillModeLiteralName)
{
#define TEST_FILL_MODE_ENUM(ENUM_VAL)                              \
    {                                                              \
        EXPECT_STREQ(GetFillModeLiteralName(ENUM_VAL), #ENUM_VAL); \
    }
    TEST_FILL_MODE_ENUM(FILL_MODE_UNDEFINED);
    TEST_FILL_MODE_ENUM(FILL_MODE_WIREFRAME);
    TEST_FILL_MODE_ENUM(FILL_MODE_SOLID);
#undef TEST_FILL_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetCullModeLiteralName)
{
#define TEST_CULL_MODE_ENUM(ENUM_VAL)                              \
    {                                                              \
        EXPECT_STREQ(GetCullModeLiteralName(ENUM_VAL), #ENUM_VAL); \
    }
    TEST_CULL_MODE_ENUM(CULL_MODE_UNDEFINED);
    TEST_CULL_MODE_ENUM(CULL_MODE_NONE);
    TEST_CULL_MODE_ENUM(CULL_MODE_FRONT);
    TEST_CULL_MODE_ENUM(CULL_MODE_BACK);
#undef TEST_CULL_MODE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetStencilOpLiteralName)
{
#define TEST_STENCIL_OP_ENUM(ENUM_VAL)                              \
    {                                                               \
        EXPECT_STREQ(GetStencilOpLiteralName(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_STENCIL_OP_ENUM(STENCIL_OP_UNDEFINED);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_KEEP);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_ZERO);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_REPLACE);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_INCR_SAT);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_DECR_SAT);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_INVERT);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_INCR_WRAP);
    TEST_STENCIL_OP_ENUM(STENCIL_OP_DECR_WRAP);
#undef TEST_STENCIL_OP_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetQueryTypeString)
{
#define TEST_QUERY_TYPE_ENUM(ENUM_VAL)                         \
    {                                                          \
        EXPECT_STREQ(GetQueryTypeString(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_UNDEFINED);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_OCCLUSION);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_BINARY_OCCLUSION);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_TIMESTAMP);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_PIPELINE_STATISTICS);
    TEST_QUERY_TYPE_ENUM(QUERY_TYPE_DURATION);
    static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are tested");
#undef TEST_QUERY_TYPE_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetSurfaceTransformString)
{
#define TEST_SURFACE_TRANSFORM_ENUM(ENUM_VAL)                         \
    {                                                                 \
        EXPECT_STREQ(GetSurfaceTransformString(ENUM_VAL), #ENUM_VAL); \
    }

    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_OPTIMAL);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_IDENTITY);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_ROTATE_90);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_ROTATE_180);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_ROTATE_270);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_HORIZONTAL_MIRROR);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180);
    TEST_SURFACE_TRANSFORM_ENUM(SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270);
#undef TEST_SURFACE_TRANSFORM_ENUM
}

TEST(GraphicsAccessories_GraphicsAccessories, GetTextureFormatAttribs)
{
    auto CheckFormatSize = [](TEXTURE_FORMAT* begin, TEXTURE_FORMAT* end, Uint32 RefSize) //
    {
        for (auto fmt = begin; fmt != end; ++fmt)
        {
            auto FmtAttrs = GetTextureFormatAttribs(*fmt);
            EXPECT_EQ(Uint32{FmtAttrs.ComponentSize} * Uint32{FmtAttrs.NumComponents}, RefSize);
        }
    };

    auto CheckNumComponents = [](TEXTURE_FORMAT* begin, TEXTURE_FORMAT* end, Uint32 RefComponents) //
    {
        for (auto fmt = begin; fmt != end; ++fmt)
        {
            auto FmtAttrs = GetTextureFormatAttribs(*fmt);
            EXPECT_EQ(FmtAttrs.NumComponents, RefComponents);
        }
    };

    auto CheckComponentType = [](TEXTURE_FORMAT* begin, TEXTURE_FORMAT* end, COMPONENT_TYPE RefType) //
    {
        for (auto fmt = begin; fmt != end; ++fmt)
        {
            auto FmtAttrs = GetTextureFormatAttribs(*fmt);
            EXPECT_EQ(FmtAttrs.ComponentType, RefType);
        }
    };

    TEXTURE_FORMAT _16ByteFormats[] =
        {
            TEX_FORMAT_RGBA32_TYPELESS, TEX_FORMAT_RGBA32_FLOAT, TEX_FORMAT_RGBA32_UINT, TEX_FORMAT_RGBA32_SINT //
        };
    CheckFormatSize(std::begin(_16ByteFormats), std::end(_16ByteFormats), 16);

    TEXTURE_FORMAT _12ByteFormats[] =
        {
            TEX_FORMAT_RGB32_TYPELESS, TEX_FORMAT_RGB32_FLOAT, TEX_FORMAT_RGB32_UINT, TEX_FORMAT_RGB32_SINT //
        };
    CheckFormatSize(std::begin(_12ByteFormats), std::end(_12ByteFormats), 12);

    TEXTURE_FORMAT _8ByteFormats[] =
        {
            TEX_FORMAT_RGBA16_TYPELESS, TEX_FORMAT_RGBA16_FLOAT, TEX_FORMAT_RGBA16_UNORM, TEX_FORMAT_RGBA16_UINT, TEX_FORMAT_RGBA16_SNORM, TEX_FORMAT_RGBA16_SINT,
            TEX_FORMAT_RG32_TYPELESS, TEX_FORMAT_RG32_FLOAT, TEX_FORMAT_RG32_UINT, TEX_FORMAT_RG32_SINT,
            TEX_FORMAT_R32G8X24_TYPELESS, TEX_FORMAT_D32_FLOAT_S8X24_UINT, TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS, TEX_FORMAT_X32_TYPELESS_G8X24_UINT //
        };
    CheckFormatSize(std::begin(_8ByteFormats), std::end(_8ByteFormats), 8);

    TEXTURE_FORMAT _4ByteFormats[] =
        {
            TEX_FORMAT_RGB10A2_TYPELESS, TEX_FORMAT_RGB10A2_UNORM, TEX_FORMAT_RGB10A2_UINT, TEX_FORMAT_R11G11B10_FLOAT,
            TEX_FORMAT_RGBA8_TYPELESS, TEX_FORMAT_RGBA8_UNORM, TEX_FORMAT_RGBA8_UNORM_SRGB, TEX_FORMAT_RGBA8_UINT, TEX_FORMAT_RGBA8_SNORM, TEX_FORMAT_RGBA8_SINT,
            TEX_FORMAT_RG16_TYPELESS, TEX_FORMAT_RG16_FLOAT, TEX_FORMAT_RG16_UNORM, TEX_FORMAT_RG16_UINT, TEX_FORMAT_RG16_SNORM, TEX_FORMAT_RG16_SINT,
            TEX_FORMAT_R32_TYPELESS, TEX_FORMAT_D32_FLOAT, TEX_FORMAT_R32_FLOAT, TEX_FORMAT_R32_UINT, TEX_FORMAT_R32_SINT,
            TEX_FORMAT_R24G8_TYPELESS, TEX_FORMAT_D24_UNORM_S8_UINT, TEX_FORMAT_R24_UNORM_X8_TYPELESS, TEX_FORMAT_X24_TYPELESS_G8_UINT,
            TEX_FORMAT_RGB9E5_SHAREDEXP, TEX_FORMAT_RG8_B8G8_UNORM, TEX_FORMAT_G8R8_G8B8_UNORM,
            TEX_FORMAT_BGRA8_UNORM, TEX_FORMAT_BGRX8_UNORM, TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
            TEX_FORMAT_BGRA8_TYPELESS, TEX_FORMAT_BGRA8_UNORM_SRGB, TEX_FORMAT_BGRX8_TYPELESS, TEX_FORMAT_BGRX8_UNORM_SRGB //
        };
    CheckFormatSize(std::begin(_4ByteFormats), std::end(_4ByteFormats), 4);

    TEXTURE_FORMAT _2ByteFormats[] =
        {
            TEX_FORMAT_RG8_TYPELESS, TEX_FORMAT_RG8_UNORM, TEX_FORMAT_RG8_UINT, TEX_FORMAT_RG8_SNORM, TEX_FORMAT_RG8_SINT,
            TEX_FORMAT_R16_TYPELESS, TEX_FORMAT_R16_FLOAT, TEX_FORMAT_D16_UNORM, TEX_FORMAT_R16_UNORM, TEX_FORMAT_R16_UINT, TEX_FORMAT_R16_SNORM, TEX_FORMAT_R16_SINT,
            TEX_FORMAT_B5G6R5_UNORM, TEX_FORMAT_B5G5R5A1_UNORM //
        };
    CheckFormatSize(std::begin(_2ByteFormats), std::end(_2ByteFormats), 2);

    TEXTURE_FORMAT _1ByteFormats[] =
        {
            TEX_FORMAT_R8_TYPELESS, TEX_FORMAT_R8_UNORM, TEX_FORMAT_R8_UINT, TEX_FORMAT_R8_SNORM, TEX_FORMAT_R8_SINT, TEX_FORMAT_A8_UNORM, //TEX_FORMAT_R1_UNORM
        };
    CheckFormatSize(std::begin(_1ByteFormats), std::end(_1ByteFormats), 1);

    TEXTURE_FORMAT _4ComponentFormats[] =
        {
            TEX_FORMAT_RGBA32_TYPELESS, TEX_FORMAT_RGBA32_FLOAT, TEX_FORMAT_RGBA32_UINT, TEX_FORMAT_RGBA32_SINT,
            TEX_FORMAT_RGBA16_TYPELESS, TEX_FORMAT_RGBA16_FLOAT, TEX_FORMAT_RGBA16_UNORM, TEX_FORMAT_RGBA16_UINT, TEX_FORMAT_RGBA16_SNORM, TEX_FORMAT_RGBA16_SINT,
            TEX_FORMAT_RGBA8_TYPELESS, TEX_FORMAT_RGBA8_UNORM, TEX_FORMAT_RGBA8_UNORM_SRGB, TEX_FORMAT_RGBA8_UINT, TEX_FORMAT_RGBA8_SNORM, TEX_FORMAT_RGBA8_SINT,
            TEX_FORMAT_RG8_B8G8_UNORM, TEX_FORMAT_G8R8_G8B8_UNORM,
            TEX_FORMAT_BGRA8_UNORM, TEX_FORMAT_BGRX8_UNORM, TEX_FORMAT_BGRA8_TYPELESS, TEX_FORMAT_BGRA8_UNORM_SRGB, TEX_FORMAT_BGRX8_TYPELESS, TEX_FORMAT_BGRX8_UNORM_SRGB //
        };
    CheckNumComponents(std::begin(_4ComponentFormats), std::end(_4ComponentFormats), 4);

    TEXTURE_FORMAT _3ComponentFormats[] =
        {
            TEX_FORMAT_RGB32_TYPELESS,
            TEX_FORMAT_RGB32_FLOAT,
            TEX_FORMAT_RGB32_UINT,
            TEX_FORMAT_RGB32_SINT //
        };
    CheckNumComponents(std::begin(_3ComponentFormats), std::end(_3ComponentFormats), 3);

    TEXTURE_FORMAT _2ComponentFormats[] =
        {
            TEX_FORMAT_RG32_TYPELESS, TEX_FORMAT_RG32_FLOAT, TEX_FORMAT_RG32_UINT, TEX_FORMAT_RG32_SINT,
            TEX_FORMAT_R32G8X24_TYPELESS, TEX_FORMAT_D32_FLOAT_S8X24_UINT, TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS, TEX_FORMAT_X32_TYPELESS_G8X24_UINT,
            TEX_FORMAT_RG16_TYPELESS, TEX_FORMAT_RG16_FLOAT, TEX_FORMAT_RG16_UNORM, TEX_FORMAT_RG16_UINT, TEX_FORMAT_RG16_SNORM, TEX_FORMAT_RG16_SINT,
            TEX_FORMAT_RG8_TYPELESS, TEX_FORMAT_RG8_UNORM, TEX_FORMAT_RG8_UINT, TEX_FORMAT_RG8_SNORM, TEX_FORMAT_RG8_SINT //
        };
    CheckNumComponents(std::begin(_2ComponentFormats), std::end(_2ComponentFormats), 2);

    TEXTURE_FORMAT _1ComponentFormats[] =
        {
            TEX_FORMAT_RGB10A2_TYPELESS, TEX_FORMAT_RGB10A2_UNORM, TEX_FORMAT_RGB10A2_UINT, TEX_FORMAT_R11G11B10_FLOAT,
            TEX_FORMAT_R32_TYPELESS, TEX_FORMAT_D32_FLOAT, TEX_FORMAT_R32_FLOAT, TEX_FORMAT_R32_UINT, TEX_FORMAT_R32_SINT,
            TEX_FORMAT_R24G8_TYPELESS, TEX_FORMAT_D24_UNORM_S8_UINT, TEX_FORMAT_R24_UNORM_X8_TYPELESS, TEX_FORMAT_X24_TYPELESS_G8_UINT,
            TEX_FORMAT_R16_TYPELESS, TEX_FORMAT_R16_FLOAT, TEX_FORMAT_D16_UNORM, TEX_FORMAT_R16_UNORM, TEX_FORMAT_R16_UINT, TEX_FORMAT_R16_SNORM, TEX_FORMAT_R16_SINT,
            TEX_FORMAT_R8_TYPELESS, TEX_FORMAT_R8_UNORM, TEX_FORMAT_R8_UINT, TEX_FORMAT_R8_SNORM, TEX_FORMAT_R8_SINT, TEX_FORMAT_A8_UNORM, //TEX_FORMAT_R1_UNORM
            TEX_FORMAT_RGB9E5_SHAREDEXP,
            TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, TEX_FORMAT_B5G6R5_UNORM, TEX_FORMAT_B5G5R5A1_UNORM //
        };
    CheckNumComponents(std::begin(_1ComponentFormats), std::end(_1ComponentFormats), 1);


    TEXTURE_FORMAT FloatFormats[] =
        {
            TEX_FORMAT_RGBA32_FLOAT,
            TEX_FORMAT_RGB32_FLOAT,
            TEX_FORMAT_RGBA16_FLOAT,
            TEX_FORMAT_RG32_FLOAT,
            TEX_FORMAT_RG16_FLOAT,
            TEX_FORMAT_R32_FLOAT,
            TEX_FORMAT_R16_FLOAT //
        };
    CheckComponentType(std::begin(FloatFormats), std::end(FloatFormats), COMPONENT_TYPE_FLOAT);

    TEXTURE_FORMAT SintFormats[] =
        {
            TEX_FORMAT_RGBA32_SINT,
            TEX_FORMAT_RGB32_SINT,
            TEX_FORMAT_RGBA16_SINT,
            TEX_FORMAT_RG32_SINT,
            TEX_FORMAT_RGBA8_SINT,
            TEX_FORMAT_RG16_SINT,
            TEX_FORMAT_R32_SINT,
            TEX_FORMAT_RG8_SINT,
            TEX_FORMAT_R16_SINT,
            TEX_FORMAT_R8_SINT //
        };
    CheckComponentType(std::begin(SintFormats), std::end(SintFormats), COMPONENT_TYPE_SINT);

    TEXTURE_FORMAT UintFormats[] =
        {
            TEX_FORMAT_RGBA32_UINT,
            TEX_FORMAT_RGB32_UINT,
            TEX_FORMAT_RGBA16_UINT,
            TEX_FORMAT_RG32_UINT,
            TEX_FORMAT_RGBA8_UINT,
            TEX_FORMAT_RG16_UINT,
            TEX_FORMAT_R32_UINT,
            TEX_FORMAT_RG8_UINT,
            TEX_FORMAT_R16_UINT,
            TEX_FORMAT_R8_UINT //
        };
    CheckComponentType(std::begin(UintFormats), std::end(UintFormats), COMPONENT_TYPE_UINT);

    TEXTURE_FORMAT UnormFormats[] =
        {
            TEX_FORMAT_RGBA16_UNORM,
            TEX_FORMAT_RGBA8_UNORM,
            TEX_FORMAT_RG16_UNORM,
            TEX_FORMAT_RG8_UNORM,
            TEX_FORMAT_R16_UNORM,
            TEX_FORMAT_R8_UNORM,
            TEX_FORMAT_A8_UNORM,
            TEX_FORMAT_R1_UNORM,
            TEX_FORMAT_RG8_B8G8_UNORM,
            TEX_FORMAT_G8R8_G8B8_UNORM,
            TEX_FORMAT_BGRA8_UNORM,
            TEX_FORMAT_BGRX8_UNORM //
        };
    CheckComponentType(std::begin(UnormFormats), std::end(UnormFormats), COMPONENT_TYPE_UNORM);

    TEXTURE_FORMAT UnormSRGBFormats[] =
        {
            TEX_FORMAT_RGBA8_UNORM_SRGB,
            TEX_FORMAT_BGRA8_UNORM_SRGB,
            TEX_FORMAT_BGRX8_UNORM_SRGB //
        };
    CheckComponentType(std::begin(UnormSRGBFormats), std::end(UnormSRGBFormats), COMPONENT_TYPE_UNORM_SRGB);

    TEXTURE_FORMAT SnormFormats[] =
        {
            TEX_FORMAT_RGBA16_SNORM,
            TEX_FORMAT_RGBA8_SNORM,
            TEX_FORMAT_RG16_SNORM,
            TEX_FORMAT_RG8_SNORM,
            TEX_FORMAT_R16_SNORM,
            TEX_FORMAT_R8_SNORM //
        };
    CheckComponentType(std::begin(SnormFormats), std::end(SnormFormats), COMPONENT_TYPE_SNORM);

    TEXTURE_FORMAT UndefinedFormats[] =
        {
            TEX_FORMAT_RGBA32_TYPELESS,
            TEX_FORMAT_RGB32_TYPELESS,
            TEX_FORMAT_RGBA16_TYPELESS,
            TEX_FORMAT_RG32_TYPELESS,
            TEX_FORMAT_RGBA8_TYPELESS,
            TEX_FORMAT_RG16_TYPELESS,
            TEX_FORMAT_R32_TYPELESS,
            TEX_FORMAT_RG8_TYPELESS,
            TEX_FORMAT_R16_TYPELESS,
            TEX_FORMAT_R8_TYPELESS,
            TEX_FORMAT_BGRA8_TYPELESS,
            TEX_FORMAT_BGRX8_TYPELESS //
        };
    CheckComponentType(std::begin(UndefinedFormats), std::end(UndefinedFormats), COMPONENT_TYPE_UNDEFINED);

    TEXTURE_FORMAT DepthFormats[] =
        {
            TEX_FORMAT_D32_FLOAT,
            TEX_FORMAT_D16_UNORM //
        };
    CheckComponentType(std::begin(DepthFormats), std::end(DepthFormats), COMPONENT_TYPE_DEPTH);

    TEXTURE_FORMAT DepthStencilFormats[] =
        {
            TEX_FORMAT_R32G8X24_TYPELESS,
            TEX_FORMAT_D32_FLOAT_S8X24_UINT,
            TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,
            TEX_FORMAT_X32_TYPELESS_G8X24_UINT,
            TEX_FORMAT_R24G8_TYPELESS,
            TEX_FORMAT_D24_UNORM_S8_UINT,
            TEX_FORMAT_R24_UNORM_X8_TYPELESS,
            TEX_FORMAT_X24_TYPELESS_G8_UINT //
        };
    CheckComponentType(std::begin(DepthStencilFormats), std::end(DepthStencilFormats), COMPONENT_TYPE_DEPTH_STENCIL);

    TEXTURE_FORMAT CompoundFormats[] =
        {
            TEX_FORMAT_RGB10A2_TYPELESS,
            TEX_FORMAT_RGB10A2_UNORM,
            TEX_FORMAT_RGB10A2_UINT,
            TEX_FORMAT_RGB9E5_SHAREDEXP,
            TEX_FORMAT_R11G11B10_FLOAT,
            TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
            TEX_FORMAT_B5G6R5_UNORM,
            TEX_FORMAT_B5G5R5A1_UNORM //
        };
    CheckComponentType(std::begin(CompoundFormats), std::end(CompoundFormats), COMPONENT_TYPE_COMPOUND);


    TEXTURE_FORMAT CompressedFormats[] =
        {
            TEX_FORMAT_BC1_TYPELESS, TEX_FORMAT_BC1_UNORM, TEX_FORMAT_BC1_UNORM_SRGB,
            TEX_FORMAT_BC2_TYPELESS, TEX_FORMAT_BC2_UNORM, TEX_FORMAT_BC2_UNORM_SRGB,
            TEX_FORMAT_BC3_TYPELESS, TEX_FORMAT_BC3_UNORM, TEX_FORMAT_BC3_UNORM_SRGB,
            TEX_FORMAT_BC4_TYPELESS, TEX_FORMAT_BC4_UNORM, TEX_FORMAT_BC4_SNORM,
            TEX_FORMAT_BC5_TYPELESS, TEX_FORMAT_BC5_UNORM, TEX_FORMAT_BC5_SNORM,
            TEX_FORMAT_BC6H_TYPELESS, TEX_FORMAT_BC6H_UF16, TEX_FORMAT_BC6H_SF16,
            TEX_FORMAT_BC7_TYPELESS, TEX_FORMAT_BC7_UNORM, TEX_FORMAT_BC7_UNORM_SRGB //
        };
    CheckComponentType(std::begin(CompressedFormats), std::end(CompressedFormats), COMPONENT_TYPE_COMPRESSED);
}

TEST(GraphicsAccessories_GraphicsAccessories, GetShaderTypeIndex)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the test below to handle the new shader type");

    // clang-format off
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_UNKNOWN),             -1);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_VERTEX),           VSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_PIXEL),            PSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_GEOMETRY),         GSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_HULL),             HSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_DOMAIN),           DSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_COMPUTE),          CSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_AMPLIFICATION),    ASInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_MESH),             MSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_RAY_GEN),          RGSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_RAY_MISS),         RMSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_RAY_CLOSEST_HIT),  RCHSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_RAY_ANY_HIT),      RAHSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_RAY_INTERSECTION), RISInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_CALLABLE),         RCSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_TILE),             TLSInd);
    EXPECT_EQ(GetShaderTypeIndex(SHADER_TYPE_LAST),             LastShaderInd);
    // clang-format on

    for (Int32 i = 0; i <= LastShaderInd; ++i)
    {
        auto ShaderType = static_cast<SHADER_TYPE>(1 << i);
        EXPECT_EQ(GetShaderTypeIndex(ShaderType), i);
    }
}

TEST(GraphicsAccessories_GraphicsAccessories, GetShaderTypeFromIndex)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the test below to handle the new shader type");

    EXPECT_EQ(GetShaderTypeFromIndex(VSInd), SHADER_TYPE_VERTEX);
    EXPECT_EQ(GetShaderTypeFromIndex(PSInd), SHADER_TYPE_PIXEL);
    EXPECT_EQ(GetShaderTypeFromIndex(GSInd), SHADER_TYPE_GEOMETRY);
    EXPECT_EQ(GetShaderTypeFromIndex(HSInd), SHADER_TYPE_HULL);
    EXPECT_EQ(GetShaderTypeFromIndex(DSInd), SHADER_TYPE_DOMAIN);
    EXPECT_EQ(GetShaderTypeFromIndex(CSInd), SHADER_TYPE_COMPUTE);
    EXPECT_EQ(GetShaderTypeFromIndex(ASInd), SHADER_TYPE_AMPLIFICATION);
    EXPECT_EQ(GetShaderTypeFromIndex(MSInd), SHADER_TYPE_MESH);
    EXPECT_EQ(GetShaderTypeFromIndex(RGSInd), SHADER_TYPE_RAY_GEN);
    EXPECT_EQ(GetShaderTypeFromIndex(RMSInd), SHADER_TYPE_RAY_MISS);
    EXPECT_EQ(GetShaderTypeFromIndex(RCHSInd), SHADER_TYPE_RAY_CLOSEST_HIT);
    EXPECT_EQ(GetShaderTypeFromIndex(RAHSInd), SHADER_TYPE_RAY_ANY_HIT);
    EXPECT_EQ(GetShaderTypeFromIndex(RISInd), SHADER_TYPE_RAY_INTERSECTION);
    EXPECT_EQ(GetShaderTypeFromIndex(RCSInd), SHADER_TYPE_CALLABLE);
    EXPECT_EQ(GetShaderTypeFromIndex(TLSInd), SHADER_TYPE_TILE);

    EXPECT_EQ(GetShaderTypeFromIndex(LastShaderInd), SHADER_TYPE_LAST);

    for (Int32 i = 0; i <= LastShaderInd; ++i)
    {
        auto ShaderType = static_cast<SHADER_TYPE>(1 << i);
        EXPECT_EQ(GetShaderTypeFromIndex(i), ShaderType);
    }
}

TEST(GraphicsAccessories_GraphicsAccessories, IsConsistentShaderType)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the code below to handle the new shader type");
    static_assert(PIPELINE_TYPE_LAST == 4, "Please update the code below to handle the new pipeline type");

    {
        std::array<bool, LastShaderInd + 1> ValidGraphicsStages;
        ValidGraphicsStages.fill(false);
        ValidGraphicsStages[VSInd] = true;
        ValidGraphicsStages[HSInd] = true;
        ValidGraphicsStages[DSInd] = true;
        ValidGraphicsStages[GSInd] = true;
        ValidGraphicsStages[PSInd] = true;

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            EXPECT_EQ(IsConsistentShaderType(ShaderType, PIPELINE_TYPE_GRAPHICS), ValidGraphicsStages[i]);
        }
    }

    {
        std::array<bool, LastShaderInd + 1> ValidComputeStages;
        ValidComputeStages.fill(false);
        ValidComputeStages[CSInd] = true;

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            EXPECT_EQ(IsConsistentShaderType(ShaderType, PIPELINE_TYPE_COMPUTE), ValidComputeStages[i]);
        }
    }

    {
        std::array<bool, LastShaderInd + 1> ValidMeshStages;
        ValidMeshStages.fill(false);
        ValidMeshStages[ASInd] = true;
        ValidMeshStages[MSInd] = true;
        ValidMeshStages[PSInd] = true;

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            EXPECT_EQ(IsConsistentShaderType(ShaderType, PIPELINE_TYPE_MESH), ValidMeshStages[i]);
        }
    }

    {
        std::array<bool, LastShaderInd + 1> ValidRayTracingStages;
        ValidRayTracingStages.fill(false);
        ValidRayTracingStages[RGSInd]  = true;
        ValidRayTracingStages[RMSInd]  = true;
        ValidRayTracingStages[RCHSInd] = true;
        ValidRayTracingStages[RAHSInd] = true;
        ValidRayTracingStages[RISInd]  = true;
        ValidRayTracingStages[RCSInd]  = true;

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            EXPECT_EQ(IsConsistentShaderType(ShaderType, PIPELINE_TYPE_RAY_TRACING), ValidRayTracingStages[i]);
        }
    }

    {
        std::array<bool, LastShaderInd + 1> ValidTileStages;
        ValidTileStages.fill(false);
        ValidTileStages[TLSInd] = true;

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            EXPECT_EQ(IsConsistentShaderType(ShaderType, PIPELINE_TYPE_TILE), ValidTileStages[i]);
        }
    }
}

TEST(GraphicsAccessories_GraphicsAccessories, GetShaderTypePipelineIndex)
{
    auto TestPipelineType = [](PIPELINE_TYPE PipelineType) {
        std::array<Int32, MAX_SHADERS_IN_PIPELINE> IndexUsed;
        IndexUsed.fill(false);

        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            if (IsConsistentShaderType(ShaderType, PipelineType))
            {
                auto Index = GetShaderTypePipelineIndex(ShaderType, PipelineType);
                ASSERT_LE(Index, static_cast<Int32>(MAX_SHADERS_IN_PIPELINE)) << " shader pipeline type index " << Index << " is out of range";
                EXPECT_FALSE(IndexUsed[Index]) << " shader pipeline type index " << Index << " is already used for another shader stage";
                IndexUsed[Index] = true;
            }
        }
    };

    TestPipelineType(PIPELINE_TYPE_GRAPHICS);
    TestPipelineType(PIPELINE_TYPE_COMPUTE);
    TestPipelineType(PIPELINE_TYPE_MESH);
}

TEST(GraphicsAccessories_GraphicsAccessories, GetShaderTypeFromPipelineIndex)
{
    auto TestPipelineType = [](PIPELINE_TYPE PipelineType) {
        for (Int32 i = 0; i <= LastShaderInd; ++i)
        {
            auto ShaderType = GetShaderTypeFromIndex(i);
            if (IsConsistentShaderType(ShaderType, PipelineType))
            {
                auto Index = GetShaderTypePipelineIndex(ShaderType, PipelineType);
                EXPECT_EQ(GetShaderTypeFromPipelineIndex(Index, PipelineType), ShaderType);
            }
        }
    };

    TestPipelineType(PIPELINE_TYPE_GRAPHICS);
    TestPipelineType(PIPELINE_TYPE_COMPUTE);
    TestPipelineType(PIPELINE_TYPE_MESH);
}

TEST(GraphicsAccessories_GraphicsAccessories, PipelineTypeFromShaderStages)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the code below to handle the new shader type");
    static_assert(PIPELINE_TYPE_LAST == 4, "Please update the code below to handle the new pipeline type");

    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_VERTEX), PIPELINE_TYPE_GRAPHICS);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_PIXEL), PIPELINE_TYPE_GRAPHICS);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_GEOMETRY), PIPELINE_TYPE_GRAPHICS);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_HULL), PIPELINE_TYPE_GRAPHICS);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_DOMAIN), PIPELINE_TYPE_GRAPHICS);

    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_COMPUTE), PIPELINE_TYPE_COMPUTE);

    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_AMPLIFICATION), PIPELINE_TYPE_MESH);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_MESH), PIPELINE_TYPE_MESH);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_AMPLIFICATION | SHADER_TYPE_PIXEL), PIPELINE_TYPE_MESH);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_MESH | SHADER_TYPE_PIXEL), PIPELINE_TYPE_MESH);

    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_RAY_GEN), PIPELINE_TYPE_RAY_TRACING);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_RAY_MISS), PIPELINE_TYPE_RAY_TRACING);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_RAY_CLOSEST_HIT), PIPELINE_TYPE_RAY_TRACING);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_RAY_ANY_HIT), PIPELINE_TYPE_RAY_TRACING);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_RAY_INTERSECTION), PIPELINE_TYPE_RAY_TRACING);
    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_CALLABLE), PIPELINE_TYPE_RAY_TRACING);

    EXPECT_EQ(PipelineTypeFromShaderStages(SHADER_TYPE_TILE), PIPELINE_TYPE_TILE);
}

TEST(GraphicsAccessories_GraphicsAccessories, GetPipelineResourceFlagsString)
{
    static_assert(PIPELINE_RESOURCE_FLAG_LAST == (1u << 4), "Please add a test for the new flag here");

    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_NONE, true).c_str(), "PIPELINE_RESOURCE_FLAG_NONE");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_NONE).c_str(), "UNKNOWN");

    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS, true).c_str(), "PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER, true).c_str(), "PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER, true).c_str(), "PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT, true).c_str(), "PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT");

    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS).c_str(), "NO_DYNAMIC_BUFFERS");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER).c_str(), "COMBINED_SAMPLER");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER).c_str(), "FORMATTED_BUFFER");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_GENERAL_INPUT_ATTACHMENT).c_str(), "GENERAL_INPUT_ATTACHMENT");

    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER, true).c_str(),
                 "PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS|PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER, true).c_str(),
                 "PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS|PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER, true).c_str(),
                 "PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER|PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER");

    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER).c_str(),
                 "NO_DYNAMIC_BUFFERS|FORMATTED_BUFFER");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER | PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER).c_str(),
                 "NO_DYNAMIC_BUFFERS|COMBINED_SAMPLER|FORMATTED_BUFFER");

    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY, true).c_str(), "PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY");
    EXPECT_STREQ(GetPipelineResourceFlagsString(PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY).c_str(), "RUNTIME_ARRAY");
}

TEST(GraphicsAccessories_GraphicsAccessories, GetPipelineShadingRateFlagsString)
{
    static_assert(PIPELINE_SHADING_RATE_FLAG_LAST == 0x02, "Please update the switch below to handle the new pipeline shading rate flag");

    EXPECT_STREQ(GetPipelineShadingRateFlagsString(PIPELINE_SHADING_RATE_FLAG_NONE).c_str(), "NONE");
    EXPECT_STREQ(GetPipelineShadingRateFlagsString(PIPELINE_SHADING_RATE_FLAG_PER_PRIMITIVE).c_str(), "PER_PRIMITIVE");
    EXPECT_STREQ(GetPipelineShadingRateFlagsString(PIPELINE_SHADING_RATE_FLAG_TEXTURE_BASED).c_str(), "TEXTURE_BASED");
    EXPECT_STREQ(GetPipelineShadingRateFlagsString(PIPELINE_SHADING_RATE_FLAG_PER_PRIMITIVE | PIPELINE_SHADING_RATE_FLAG_TEXTURE_BASED).c_str(), "PER_PRIMITIVE | TEXTURE_BASED");
}

TEST(GraphicsAccessories_GraphicsAccessories, GetMipLevelProperties)
{
    TextureDesc        Desc;
    MipLevelProperties Props;

    Desc.Type   = RESOURCE_DIM_TEX_2D;
    Desc.Width  = 128;
    Desc.Height = 95;
    Desc.Format = TEX_FORMAT_RGBA8_UNORM;

    Props = GetMipLevelProperties(Desc, 1);
    EXPECT_EQ(Props.LogicalWidth, 64u);
    EXPECT_EQ(Props.LogicalHeight, 47u);
    EXPECT_EQ(Props.StorageWidth, 64u);
    EXPECT_EQ(Props.StorageHeight, 47u);
    EXPECT_EQ(Props.Depth, 1u);
    EXPECT_EQ(Props.RowSize, 256u);
    EXPECT_EQ(Props.DepthSliceSize, 12032u);
    EXPECT_EQ(Props.MipSize, 12032u);

    Desc.Format = TEX_FORMAT_BC1_UNORM;

    Props = GetMipLevelProperties(Desc, 1);
    EXPECT_EQ(Props.LogicalWidth, 64u);
    EXPECT_EQ(Props.LogicalHeight, 47u);
    EXPECT_EQ(Props.StorageWidth, 64u);
    EXPECT_EQ(Props.StorageHeight, 48u);
    EXPECT_EQ(Props.Depth, 1u);
    EXPECT_EQ(Props.RowSize, 128u);
    EXPECT_EQ(Props.DepthSliceSize, 1536u);
    EXPECT_EQ(Props.MipSize, 1536u);

    Props = GetMipLevelProperties(Desc, 4);
    EXPECT_EQ(Props.LogicalWidth, 8u);
    EXPECT_EQ(Props.LogicalHeight, 5u);
    EXPECT_EQ(Props.StorageWidth, 8u);
    EXPECT_EQ(Props.StorageHeight, 8u);
    EXPECT_EQ(Props.Depth, 1u);
    EXPECT_EQ(Props.RowSize, 16u);
    EXPECT_EQ(Props.DepthSliceSize, 32u);
    EXPECT_EQ(Props.MipSize, 32u);

    Props = GetMipLevelProperties(Desc, 5);
    EXPECT_EQ(Props.LogicalWidth, 4u);
    EXPECT_EQ(Props.LogicalHeight, 2u);
    EXPECT_EQ(Props.StorageWidth, 4u);
    EXPECT_EQ(Props.StorageHeight, 4u);
    EXPECT_EQ(Props.Depth, 1u);
    EXPECT_EQ(Props.RowSize, 8u);
    EXPECT_EQ(Props.DepthSliceSize, 8u);
    EXPECT_EQ(Props.MipSize, 8u);

    Desc.Type      = RESOURCE_DIM_TEX_2D_ARRAY;
    Desc.Width     = 128;
    Desc.Height    = 95;
    Desc.ArraySize = 32;
    Desc.Format    = TEX_FORMAT_RGBA8_UNORM;

    Props = GetMipLevelProperties(Desc, 1);
    EXPECT_EQ(Props.LogicalWidth, 64u);
    EXPECT_EQ(Props.LogicalHeight, 47u);
    EXPECT_EQ(Props.StorageWidth, 64u);
    EXPECT_EQ(Props.StorageHeight, 47u);
    EXPECT_EQ(Props.Depth, 1u);
    EXPECT_EQ(Props.RowSize, 256u);
    EXPECT_EQ(Props.DepthSliceSize, 12032u);
    EXPECT_EQ(Props.MipSize, 12032u);

    Desc.Type   = RESOURCE_DIM_TEX_3D;
    Desc.Width  = 128;
    Desc.Height = 95;
    Desc.Depth  = 55;
    Desc.Format = TEX_FORMAT_RGBA8_UNORM;

    Props = GetMipLevelProperties(Desc, 2);
    EXPECT_EQ(Props.LogicalWidth, 32u);
    EXPECT_EQ(Props.LogicalHeight, 23u);
    EXPECT_EQ(Props.StorageWidth, 32u);
    EXPECT_EQ(Props.StorageHeight, 23u);
    EXPECT_EQ(Props.Depth, 13u);
    EXPECT_EQ(Props.RowSize, 128u);
    EXPECT_EQ(Props.DepthSliceSize, 2944u);
    EXPECT_EQ(Props.MipSize, 38272u);

    Desc.Depth  = 64;
    Desc.Format = TEX_FORMAT_BC1_UNORM;

    Props = GetMipLevelProperties(Desc, 2);
    EXPECT_EQ(Props.LogicalWidth, 32u);
    EXPECT_EQ(Props.LogicalHeight, 23u);
    EXPECT_EQ(Props.StorageWidth, 32u);
    EXPECT_EQ(Props.StorageHeight, 24u);
    EXPECT_EQ(Props.Depth, 16u);
    EXPECT_EQ(Props.RowSize, 64u);
    EXPECT_EQ(Props.DepthSliceSize, 384u);
    EXPECT_EQ(Props.MipSize, 6144u);
}

TEST(GraphicsAccessories_GraphicsAccessories, GetStandardSparseTextureProperties)
{
    constexpr auto BlockSize = 64u << 10;

    TextureDesc             Desc;
    SparseTextureProperties Props;

    Desc.Type        = RESOURCE_DIM_TEX_2D;
    Desc.Width       = 1024;
    Desc.Height      = 1024;
    Desc.Format      = TEX_FORMAT_RGBA8_UNORM; // 32 bits
    Desc.MipLevels   = 11;
    Desc.SampleCount = 1;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 86 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 85 * BlockSize);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, BlockSize);
    EXPECT_EQ(Props.FirstMipInTail, 4u);
    EXPECT_EQ(Props.TileSize[0], 128u);
    EXPECT_EQ(Props.TileSize[1], 128u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.MipLevels = 1;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 64 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 0u);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, 0u);
    EXPECT_EQ(Props.FirstMipInTail, 1u);
    EXPECT_EQ(Props.TileSize[0], 128u);
    EXPECT_EQ(Props.TileSize[1], 128u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Type        = RESOURCE_DIM_TEX_2D;
    Desc.Width       = 253;
    Desc.Height      = 249;
    Desc.Format      = TEX_FORMAT_RGBA8_UNORM; // 32 bits
    Desc.MipLevels   = 8;
    Desc.SampleCount = 1;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 6 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 4 * BlockSize);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, 2 * BlockSize);
    EXPECT_EQ(Props.FirstMipInTail, 1u);
    EXPECT_EQ(Props.TileSize[0], 128u);
    EXPECT_EQ(Props.TileSize[1], 128u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Type        = RESOURCE_DIM_TEX_2D;
    Desc.Width       = 1024;
    Desc.Height      = 1024;
    Desc.Format      = TEX_FORMAT_BC1_UNORM; // 64 bits for 4x4 block
    Desc.MipLevels   = 11;
    Desc.SampleCount = 1;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 11 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 10 * BlockSize);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, BlockSize);
    EXPECT_EQ(Props.FirstMipInTail, 2u);
    EXPECT_EQ(Props.TileSize[0], 512u);
    EXPECT_EQ(Props.TileSize[1], 256u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Type        = RESOURCE_DIM_TEX_2D;
    Desc.Width       = 1024;
    Desc.Height      = 1024;
    Desc.Format      = TEX_FORMAT_BC5_UNORM; // 128 bits for 4x4 block
    Desc.MipLevels   = 11;
    Desc.SampleCount = 1;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 22 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 21 * BlockSize);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, BlockSize);
    EXPECT_EQ(Props.FirstMipInTail, 3u);
    EXPECT_EQ(Props.TileSize[0], 256u);
    EXPECT_EQ(Props.TileSize[1], 256u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Type        = RESOURCE_DIM_TEX_2D;
    Desc.Width       = 1024;
    Desc.Height      = 1024;
    Desc.Format      = TEX_FORMAT_RGBA16_UNORM; // 64 bits
    Desc.MipLevels   = 1;
    Desc.SampleCount = 4;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 512 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 0u);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, 0u);
    EXPECT_EQ(Props.FirstMipInTail, 1u);
    EXPECT_EQ(Props.TileSize[0], 64u);
    EXPECT_EQ(Props.TileSize[1], 32u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Format      = TEX_FORMAT_R8_UNORM; // 8 bits
    Desc.SampleCount = 2;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 32 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 0u);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, 0u);
    EXPECT_EQ(Props.FirstMipInTail, 1u);
    EXPECT_EQ(Props.TileSize[0], 128u);
    EXPECT_EQ(Props.TileSize[1], 256u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Format      = TEX_FORMAT_RGBA8_UNORM; // 32 bits
    Desc.SampleCount = 8;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 512 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 0u);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, 0u);
    EXPECT_EQ(Props.FirstMipInTail, 1u);
    EXPECT_EQ(Props.TileSize[0], 32u);
    EXPECT_EQ(Props.TileSize[1], 64u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Format      = TEX_FORMAT_RGBA32_FLOAT; // 128 bits
    Desc.SampleCount = 16;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 4096 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 0u);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, 0u);
    EXPECT_EQ(Props.FirstMipInTail, 1u);
    EXPECT_EQ(Props.TileSize[0], 16u);
    EXPECT_EQ(Props.TileSize[1], 16u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Type        = RESOURCE_DIM_TEX_2D_ARRAY;
    Desc.Width       = 1024;
    Desc.Height      = 1024;
    Desc.ArraySize   = 32;
    Desc.Format      = TEX_FORMAT_R8_UNORM; // 8 bits
    Desc.MipLevels   = 11;
    Desc.SampleCount = 1;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 704 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 21 * BlockSize);
    EXPECT_EQ(Props.MipTailStride, 22 * BlockSize);
    EXPECT_EQ(Props.MipTailSize, BlockSize);
    EXPECT_EQ(Props.FirstMipInTail, 3u);
    EXPECT_EQ(Props.TileSize[0], 256u);
    EXPECT_EQ(Props.TileSize[1], 256u);
    EXPECT_EQ(Props.TileSize[2], 1u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Type        = RESOURCE_DIM_TEX_3D;
    Desc.Width       = 256;
    Desc.Height      = 256;
    Desc.Depth       = 512;
    Desc.Format      = TEX_FORMAT_RGBA8_SNORM; // 32 bits
    Desc.MipLevels   = 10;
    Desc.SampleCount = 1;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 2341 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 2340 * BlockSize);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, BlockSize);
    EXPECT_EQ(Props.FirstMipInTail, 4u);
    EXPECT_EQ(Props.TileSize[0], 32u);
    EXPECT_EQ(Props.TileSize[1], 32u);
    EXPECT_EQ(Props.TileSize[2], 16u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);

    Desc.Type        = RESOURCE_DIM_TEX_3D;
    Desc.Width       = 512;
    Desc.Height      = 512;
    Desc.Depth       = 128;
    Desc.Format      = TEX_FORMAT_RGBA32_FLOAT; // 128 bits
    Desc.MipLevels   = 10;
    Desc.SampleCount = 1;

    Props = GetStandardSparseTextureProperties(Desc);
    EXPECT_EQ(Props.AddressSpaceSize, 9363 * BlockSize);
    EXPECT_EQ(Props.MipTailOffset, 9360 * BlockSize);
    EXPECT_EQ(Props.MipTailStride, 0u);
    EXPECT_EQ(Props.MipTailSize, 3 * BlockSize);
    EXPECT_EQ(Props.FirstMipInTail, 4u);
    EXPECT_EQ(Props.TileSize[0], 16u);
    EXPECT_EQ(Props.TileSize[1], 16u);
    EXPECT_EQ(Props.TileSize[2], 16u);
    EXPECT_EQ(Props.BlockSize, BlockSize);
    EXPECT_EQ(Props.Flags, SPARSE_TEXTURE_FLAG_NONE);
}

TEST(GraphicsAccessories_GraphicsAccessories, SwapChainUsageFlagsToBindFlags)
{
    EXPECT_EQ(SwapChainUsageFlagsToBindFlags(SWAP_CHAIN_USAGE_NONE), BIND_NONE);
    EXPECT_EQ(SwapChainUsageFlagsToBindFlags(SWAP_CHAIN_USAGE_RENDER_TARGET), BIND_RENDER_TARGET);
    EXPECT_EQ(SwapChainUsageFlagsToBindFlags(SWAP_CHAIN_USAGE_SHADER_RESOURCE), BIND_SHADER_RESOURCE);
    EXPECT_EQ(SwapChainUsageFlagsToBindFlags(SWAP_CHAIN_USAGE_INPUT_ATTACHMENT), BIND_INPUT_ATTACHMENT);
    EXPECT_EQ(SwapChainUsageFlagsToBindFlags(SWAP_CHAIN_USAGE_RENDER_TARGET | SWAP_CHAIN_USAGE_SHADER_RESOURCE), BIND_RENDER_TARGET | BIND_SHADER_RESOURCE);
    EXPECT_EQ(SwapChainUsageFlagsToBindFlags(SWAP_CHAIN_USAGE_SHADER_RESOURCE | SWAP_CHAIN_USAGE_INPUT_ATTACHMENT), BIND_SHADER_RESOURCE | BIND_INPUT_ATTACHMENT);
}

TEST(GraphicsAccessories_GraphicsAccessories, GetRenderDeviceTypeString)
{
    static_assert(RENDER_DEVICE_TYPE_COUNT == 7, "Please add the new device type to the test");

    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_UNDEFINED), "Undefined");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_D3D11), "Direct3D11");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_D3D12), "Direct3D12");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_GL), "OpenGL");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_GLES), "OpenGLES");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_VULKAN), "Vulkan");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_METAL), "Metal");

    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_UNDEFINED, true), "RENDER_DEVICE_TYPE_UNDEFINED");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_D3D11, true), "RENDER_DEVICE_TYPE_D3D11");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_D3D12, true), "RENDER_DEVICE_TYPE_D3D12");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_GL, true), "RENDER_DEVICE_TYPE_GL");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_GLES, true), "RENDER_DEVICE_TYPE_GLES");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_VULKAN, true), "RENDER_DEVICE_TYPE_VULKAN");
    EXPECT_STREQ(GetRenderDeviceTypeString(RENDER_DEVICE_TYPE_METAL, true), "RENDER_DEVICE_TYPE_METAL");
}

TEST(GraphicsAccessories_GraphicsAccessories, GetRenderDeviceTypeShortString)
{
    static_assert(RENDER_DEVICE_TYPE_COUNT == 7, "Please add the new device type to the test");

    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_UNDEFINED), "undefined");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_D3D11), "d3d11");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_D3D12), "d3d12");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_GL), "gl");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_GLES), "gles");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_VULKAN), "vk");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_METAL), "mtl");

    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_UNDEFINED, true), "UNDEFINED");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_D3D11, true), "D3D11");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_D3D12, true), "D3D12");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_GL, true), "GL");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_GLES, true), "GLES");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_VULKAN, true), "VK");
    EXPECT_STREQ(GetRenderDeviceTypeShortString(RENDER_DEVICE_TYPE_METAL, true), "MTL");
}

TEST(GraphicsAccessories_GraphicsAccessories, GetAdapterTypeString)
{
    static_assert(ADAPTER_TYPE_COUNT == 4, "Please add the new adapter type to the test");

    EXPECT_STREQ(GetAdapterTypeString(ADAPTER_TYPE_UNKNOWN), "Unknown");
    EXPECT_STREQ(GetAdapterTypeString(ADAPTER_TYPE_SOFTWARE), "Software");
    EXPECT_STREQ(GetAdapterTypeString(ADAPTER_TYPE_INTEGRATED), "Integrated");
    EXPECT_STREQ(GetAdapterTypeString(ADAPTER_TYPE_DISCRETE), "Discrete");

    EXPECT_STREQ(GetAdapterTypeString(ADAPTER_TYPE_UNKNOWN, true), "ADAPTER_TYPE_UNKNOWN");
    EXPECT_STREQ(GetAdapterTypeString(ADAPTER_TYPE_SOFTWARE, true), "ADAPTER_TYPE_SOFTWARE");
    EXPECT_STREQ(GetAdapterTypeString(ADAPTER_TYPE_INTEGRATED, true), "ADAPTER_TYPE_INTEGRATED");
    EXPECT_STREQ(GetAdapterTypeString(ADAPTER_TYPE_DISCRETE, true), "ADAPTER_TYPE_DISCRETE");
}

} // namespace
