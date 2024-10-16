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

#pragma once

/// \file
/// Defines graphics engine utilities

#include "../../GraphicsEngine/interface/GraphicsTypes.h"
#include "../../GraphicsEngine/interface/DepthStencilState.h"
#include "../../GraphicsEngine/interface/BlendState.h"
#include "../../GraphicsEngine/interface/RasterizerState.h"
#include "../../GraphicsEngine/interface/Sampler.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


// =========================== Depth-stencil states ===========================

static DILIGENT_CONSTEXPR DepthStencilStateDesc DSS_Default{};

static DILIGENT_CONSTEXPR DepthStencilStateDesc DSS_DisableDepth{
    False, // DepthEnable
    False  // DepthWriteEnable
};

static DILIGENT_CONSTEXPR DepthStencilStateDesc DSS_EnableDepthNoWrites{
    True, // DepthEnable
    False // DepthWriteEnable
};


// ============================ Rasterizer states =============================

static DILIGENT_CONSTEXPR RasterizerStateDesc RS_Default{};

static DILIGENT_CONSTEXPR RasterizerStateDesc RS_SolidFillNoCull{
    FILL_MODE_SOLID,
    CULL_MODE_NONE,
};

static DILIGENT_CONSTEXPR RasterizerStateDesc RS_SolidFillCullBack{
    FILL_MODE_SOLID,
    CULL_MODE_BACK,
};

static DILIGENT_CONSTEXPR RasterizerStateDesc RS_SolidFillCullFront{
    FILL_MODE_SOLID,
    CULL_MODE_FRONT,
};

static DILIGENT_CONSTEXPR RasterizerStateDesc RS_SolidFillCullBackCCW{
    FILL_MODE_SOLID,
    CULL_MODE_BACK,
    True,
};

static DILIGENT_CONSTEXPR RasterizerStateDesc RS_SolidFillCullFrontCCW{
    FILL_MODE_SOLID,
    CULL_MODE_FRONT,
    True,
};

static DILIGENT_CONSTEXPR RasterizerStateDesc RS_WireFillNoCull{
    FILL_MODE_WIREFRAME,
    CULL_MODE_NONE,
};

static DILIGENT_CONSTEXPR RasterizerStateDesc RS_WireFillCullBack{
    FILL_MODE_WIREFRAME,
    CULL_MODE_BACK,
};

static DILIGENT_CONSTEXPR RasterizerStateDesc RS_WireFillCullFront{
    FILL_MODE_WIREFRAME,
    CULL_MODE_FRONT,
};


// =============================== Blend states ===============================

static DILIGENT_CONSTEXPR BlendStateDesc BS_Default{};

static DILIGENT_CONSTEXPR BlendStateDesc BS_AlphaBlend{
    False,                // AlphaToCoverageEnable
    False,                // IndependentBlendEnable
    RenderTargetBlendDesc // Render Target 0
    {
        True,                       // BlendEnable
        False,                      // LogicOperationEnable
        BLEND_FACTOR_SRC_ALPHA,     // SrcBlend
        BLEND_FACTOR_INV_SRC_ALPHA, // DestBlend
        BLEND_OPERATION_ADD,        // BlendOp
        BLEND_FACTOR_SRC_ALPHA,     // SrcBlendAlpha
        BLEND_FACTOR_INV_SRC_ALPHA, // DestBlendAlpha
        BLEND_OPERATION_ADD         // BlendOpAlpha
    },
};

static DILIGENT_CONSTEXPR BlendStateDesc BS_PremultipliedAlphaBlend{
    False,                // AlphaToCoverageEnable
    False,                // IndependentBlendEnable
    RenderTargetBlendDesc // Render Target 0
    {
        True,                       // BlendEnable
        False,                      // LogicOperationEnable
        BLEND_FACTOR_ONE,           // SrcBlend
        BLEND_FACTOR_INV_SRC_ALPHA, // DestBlend
        BLEND_OPERATION_ADD,        // BlendOp
        BLEND_FACTOR_ONE,           // SrcBlendAlpha
        BLEND_FACTOR_INV_SRC_ALPHA, // DestBlendAlpha
        BLEND_OPERATION_ADD,        // BlendOpAlpha
    },
};

static DILIGENT_CONSTEXPR BlendStateDesc BS_AdditiveBlend{
    False,                // AlphaToCoverageEnable
    False,                // IndependentBlendEnable
    RenderTargetBlendDesc // Render Target 0
    {
        True,                // BlendEnable
        False,               // LogicOperationEnable
        BLEND_FACTOR_ONE,    // SrcBlend
        BLEND_FACTOR_ONE,    // DestBlend
        BLEND_OPERATION_ADD, // BlendOp
        BLEND_FACTOR_ONE,    // SrcBlendAlpha
        BLEND_FACTOR_ONE,    // DestBlendAlpha
        BLEND_OPERATION_ADD, // BlendOpAlpha
    },
};


// ================================= Samplers =================================

static DILIGENT_CONSTEXPR SamplerDesc Sam_LinearClamp{
    FILTER_TYPE_LINEAR,
    FILTER_TYPE_LINEAR,
    FILTER_TYPE_LINEAR,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_PointClamp{
    FILTER_TYPE_POINT,
    FILTER_TYPE_POINT,
    FILTER_TYPE_POINT,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_LinearMirror{
    FILTER_TYPE_LINEAR,
    FILTER_TYPE_LINEAR,
    FILTER_TYPE_LINEAR,
    TEXTURE_ADDRESS_MIRROR,
    TEXTURE_ADDRESS_MIRROR,
    TEXTURE_ADDRESS_MIRROR,
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_PointWrap{
    FILTER_TYPE_POINT,
    FILTER_TYPE_POINT,
    FILTER_TYPE_POINT,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_LinearWrap{
    FILTER_TYPE_LINEAR,
    FILTER_TYPE_LINEAR,
    FILTER_TYPE_LINEAR,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_ComparisonLinearClamp{
    FILTER_TYPE_COMPARISON_LINEAR,
    FILTER_TYPE_COMPARISON_LINEAR,
    FILTER_TYPE_COMPARISON_LINEAR,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    SamplerDesc{}.MipLODBias,
    SamplerDesc{}.MaxAnisotropy,
    COMPARISON_FUNC_LESS,
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_Aniso2xClamp{
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    0.f, // MipLODBias
    2    // MaxAnisotropy
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_Aniso4xClamp{
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    0.f, // MipLODBias
    4    // MaxAnisotropy
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_Aniso8xClamp{
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    0.f, // MipLODBias
    8    // MaxAnisotropy
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_Aniso16xClamp{
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_CLAMP,
    0.f, // MipLODBias
    16   // MaxAnisotropy
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_Aniso2xWrap{
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    0.f, // MipLODBias
    2    // MaxAnisotropy
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_Aniso4xWrap{
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    0.f, // MipLODBias
    4    // MaxAnisotropy
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_Aniso8xWrap{
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    0.f, // MipLODBias
    8    // MaxAnisotropy
};

static DILIGENT_CONSTEXPR SamplerDesc Sam_Aniso16xWrap{
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    FILTER_TYPE_ANISOTROPIC,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_WRAP,
    0.f, // MipLODBias
    16   // MaxAnisotropy
};

DILIGENT_END_NAMESPACE // namespace Diligent
