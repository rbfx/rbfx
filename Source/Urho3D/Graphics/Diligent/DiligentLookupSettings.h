//
// Copyright (c) 2017-2020 the rbfx project.
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

#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <Diligent/Graphics/GraphicsEngine/interface/BlendState.h>
#include <Diligent/Graphics/GraphicsEngine/interface/DepthStencilState.h>
#include <Diligent/Graphics/GraphicsEngine/interface/RasterizerState.h>
#include "../Graphics.h"
namespace Urho3D
{
    using namespace Diligent;
    static const COMPARISON_FUNCTION DiligentCmpFunc[] =
    {
        COMPARISON_FUNC_ALWAYS,
        COMPARISON_FUNC_EQUAL,
        COMPARISON_FUNC_NOT_EQUAL,
        COMPARISON_FUNC_LESS,
        COMPARISON_FUNC_LESS_EQUAL,
        COMPARISON_FUNC_GREATER,
        COMPARISON_FUNC_GREATER_EQUAL
    };
    static const bool DiligentBlendEnable[] =
    {
        false,  // BLEND_REPLACE
        true,   // BLEND_ADD
        true,   // BLEND_MULTIPLY
        true,   // BLEND_ALPHA
        true,   // BLEND_ADDALPHA
        true,   // BLEND_PREMULALPHA
        true,   // BLEND_INVDESTALPHA
        true,   // BLEND_SUBTRACT
        true,   // BLEND_SUBTRACTALPHA
        true,   // BLEND_DEFERRED_DECAL
    };
    static_assert(sizeof(DiligentBlendEnable) / sizeof(DiligentBlendEnable[0]) == MAX_BLENDMODES, "");

    static const BLEND_FACTOR DiligentSrcBlend[] =
    {
        BLEND_FACTOR_ONE,            // BLEND_REPLACE
        BLEND_FACTOR_ONE,            // BLEND_ADD
        BLEND_FACTOR_DEST_COLOR,     // BLEND_MULTIPLY
        BLEND_FACTOR_SRC_ALPHA,      // BLEND_ALPHA
        BLEND_FACTOR_SRC_ALPHA,      // BLEND_ADDALPHA
        BLEND_FACTOR_ONE,            // BLEND_PREMULALPHA
        BLEND_FACTOR_INV_DEST_ALPHA, // BLEND_INVDESTALPHA
        BLEND_FACTOR_ONE,            // BLEND_SUBTRACT
        BLEND_FACTOR_SRC_ALPHA,      // BLEND_SUBTRACTALPHA
        BLEND_FACTOR_SRC_ALPHA,      // BLEND_DEFERRED_DECAL
    };
    static_assert(sizeof(DiligentSrcBlend) / sizeof(DiligentSrcBlend[0]) == MAX_BLENDMODES, "");

    static const BLEND_FACTOR DiligentDestBlend[] =
    {
        BLEND_FACTOR_ZERO,          // BLEND_REPLACE
        BLEND_FACTOR_ONE,           // BLEND_ADD
        BLEND_FACTOR_ZERO,          // BLEND_MULTIPLY
        BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_ALPHA
        BLEND_FACTOR_ONE,           // BLEND_ADDALPHA
        BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_PREMULALPHA
        BLEND_FACTOR_DEST_ALPHA,    // BLEND_INVDESTALPHA
        BLEND_FACTOR_ONE,           // BLEND_SUBTRACT
        BLEND_FACTOR_ONE,           // BLEND_SUBTRACTALPHA
        BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_DEFERRED_DECAL
    };
    static_assert(sizeof(DiligentDestBlend) / sizeof(DiligentDestBlend[0]) == MAX_BLENDMODES, "");

    static const BLEND_FACTOR DiligentSrcAlphaBlend[] =
    {
        BLEND_FACTOR_ONE,            // BLEND_REPLACE
        BLEND_FACTOR_ONE,            // BLEND_ADD
        BLEND_FACTOR_DEST_COLOR,     // BLEND_MULTIPLY
        BLEND_FACTOR_SRC_ALPHA,      // BLEND_ALPHA
        BLEND_FACTOR_SRC_ALPHA,      // BLEND_ADDALPHA
        BLEND_FACTOR_ONE,            // BLEND_PREMULALPHA
        BLEND_FACTOR_INV_DEST_ALPHA, // BLEND_INVDESTALPHA
        BLEND_FACTOR_ONE,            // BLEND_SUBTRACT
        BLEND_FACTOR_SRC_ALPHA,      // BLEND_SUBTRACTALPHA
        BLEND_FACTOR_ZERO,           // BLEND_DEFERRED_DECAL
    };
    static_assert(sizeof(DiligentSrcAlphaBlend) / sizeof(DiligentSrcAlphaBlend[0]) == MAX_BLENDMODES, "");

    static const BLEND_FACTOR DiligentDestAlphaBlend[] =
    {
        BLEND_FACTOR_ZERO,          // BLEND_REPLACE
        BLEND_FACTOR_ONE,           // BLEND_ADD
        BLEND_FACTOR_ZERO,          // BLEND_MULTIPLY
        BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_ALPHA
        BLEND_FACTOR_ONE,           // BLEND_ADDALPHA
        BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_PREMULALPHA
        BLEND_FACTOR_DEST_ALPHA,    // BLEND_INVDESTALPHA
        BLEND_FACTOR_ONE,           // BLEND_SUBTRACT
        BLEND_FACTOR_ONE,           // BLEND_SUBTRACTALPHA
        BLEND_FACTOR_ONE,           // BLEND_DEFERRED_DECAL
    };
    static_assert(sizeof(DiligentDestAlphaBlend) / sizeof(DiligentDestAlphaBlend[0]) == MAX_BLENDMODES, "");

    static const BLEND_OPERATION DiligentBlendOp[] =
    {
        BLEND_OPERATION_ADD,          // BLEND_REPLACE
        BLEND_OPERATION_ADD,          // BLEND_ADD
        BLEND_OPERATION_ADD,          // BLEND_MULTIPLY
        BLEND_OPERATION_ADD,          // BLEND_ALPHA
        BLEND_OPERATION_ADD,          // BLEND_ADDALPHA
        BLEND_OPERATION_ADD,          // BLEND_PREMULALPHA
        BLEND_OPERATION_ADD,          // BLEND_INVDESTALPHA
        BLEND_OPERATION_REV_SUBTRACT, // BLEND_SUBTRACT
        BLEND_OPERATION_REV_SUBTRACT, // BLEND_SUBTRACTALPHA
        BLEND_OPERATION_ADD,          // BLEND_DEFERRED_DECAL
    };
    static_assert(sizeof(DiligentBlendOp) / sizeof(DiligentBlendOp[0]) == MAX_BLENDMODES, "");

    static const STENCIL_OP sStencilOp[] = {
        STENCIL_OP_KEEP,
        STENCIL_OP_ZERO,
        STENCIL_OP_REPLACE,
        STENCIL_OP_INCR_WRAP,
        STENCIL_OP_DECR_WRAP
    };

    static const CULL_MODE DiligentCullMode[] = {
        CULL_MODE_NONE,
        CULL_MODE_BACK,
        CULL_MODE_FRONT
    };

    static const FILL_MODE DiligentFillMode[] =
    {

        FILL_MODE_SOLID,
        FILL_MODE_WIREFRAME,
        FILL_MODE_WIREFRAME,
        // Point fill mode not supported
    };

    static const PRIMITIVE_TOPOLOGY DiligentPrimitiveTopology[] = {
        PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        PRIMITIVE_TOPOLOGY_LINE_LIST,
        PRIMITIVE_TOPOLOGY_POINT_LIST,
        PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        PRIMITIVE_TOPOLOGY_LINE_STRIP,
        PRIMITIVE_TOPOLOGY_UNDEFINED // Triangle FAN is not supported by the D3D backends
    };

    static const ea::unordered_map<ea::string, TextureUnit> DiligentTextureUnitLookup = {
        { "DiffMap", TextureUnit::TU_DIFFUSE },
        { "DiffCubeMap", TextureUnit::TU_DIFFUSE },
        { "NormalMap", TextureUnit::TU_NORMAL },
        { "NormalCubeMap", TextureUnit::TU_NORMAL },
        { "SpecMap", TextureUnit::TU_SPECULAR },
        { "EmissiveMap", TextureUnit::TU_EMISSIVE },
        { "EnvMap", TextureUnit::TU_ENVIRONMENT },
        { "EnvCubeMap", TextureUnit::TU_ENVIRONMENT },
        { "LightRampMap", TextureUnit::TU_LIGHTRAMP },
        { "LightSpotMap", TextureUnit::TU_LIGHTSHAPE },
        { "LightShapeMap", TextureUnit::TU_LIGHTSHAPE },
        { "LightCubeMap", TextureUnit::TU_LIGHTSHAPE },
        { "LightBufferMap", TextureUnit::TU_LIGHTBUFFER },
        { "LightBuffer", TextureUnit::TU_LIGHTBUFFER },
        { "ShadowMap", TextureUnit::TU_SHADOWMAP },
        { "VolumeMap", TextureUnit::TU_VOLUMEMAP },
        { "DepthBuffer", TextureUnit::TU_DEPTHBUFFER },
        { "DepthBufferMap", TextureUnit::TU_DEPTHBUFFER },
        { "ZoneBuffer", TextureUnit::TU_ZONE },
        { "ZoneCubeMap", TextureUnit::TU_ENVIRONMENT},
        { "ZoneVolumeMap", TextureUnit::TU_VOLUMEMAP },
        { "Custom1Map", TextureUnit::TU_CUSTOM1 },
        { "Custom2Map", TextureUnit::TU_CUSTOM2 },
        { "FaceSelectMap", TextureUnit::TU_FACESELECT },
        { "IndirectionMap", TextureUnit::TU_INDIRECTION }
    };

    static const Diligent::VALUE_TYPE DiligentIndexBufferType[] = {
        Diligent::VT_UNDEFINED,
        Diligent::VT_UINT16,
        Diligent::VT_UINT32
    };

    static const Diligent::SHADER_TYPE DiligentShaderType[] = {
        Diligent::SHADER_TYPE_VERTEX,
        Diligent::SHADER_TYPE_PIXEL,
        Diligent::SHADER_TYPE_GEOMETRY,
        Diligent::SHADER_TYPE_HULL,
        Diligent::SHADER_TYPE_DOMAIN,
        Diligent::SHADER_TYPE_COMPUTE,
    };
}
