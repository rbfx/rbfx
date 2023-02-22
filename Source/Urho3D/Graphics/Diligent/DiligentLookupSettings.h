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
}
