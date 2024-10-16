/*
 *  Copyright 2019-2023 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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
/// Definition of the Diligent::IPipelineStateMtl interface

#include "../../GraphicsEngine/interface/PipelineState.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {B6A17C51-CCA9-44E1-A2DC-5DE250CF85AD}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_PipelineStateMtl =
    {0xb6a17c51, 0xcca9, 0x44e1, {0xa2, 0xdc, 0x5d, 0xe2, 0x50, 0xcf, 0x85, 0xad}};

#define DILIGENT_INTERFACE_NAME IPipelineStateMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineStateMtlInclusiveMethods \
    IPipelineStateInclusiveMethods;       \
    IPipelineStateMtlMethods PipelineStateMtl

/// Exposes Metal-specific functionality of a pipeline state object.
DILIGENT_BEGIN_INTERFACE(IPipelineStateMtl, IPipelineState)
{
    /// Returns a pointer to Metal render pipeline (MTLRenderPipelineState)
    VIRTUAL id<MTLRenderPipelineState> METHOD(GetMtlRenderPipeline)(THIS) CONST PURE;

    /// Returns a pointer to Metal compute pipeline (MTLComputePipelineState)
    VIRTUAL id<MTLComputePipelineState> METHOD(GetMtlComputePipeline)(THIS) CONST PURE;

    /// Returns a pointer to Metal depth-stencil state object (MTLDepthStencilState)
    VIRTUAL id<MTLDepthStencilState> METHOD(GetMtlDepthStencilState)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IPipelineStateMtl_GetMtlRenderPipeline(This)    CALL_IFACE_METHOD(PipelineStateMtl, GetMtlRenderPipeline,    This)
#    define IPipelineStateMtl_GetMtlComputePipeline(This)   CALL_IFACE_METHOD(PipelineStateMtl, GetMtlComputePipeline,   This)
#    define IPipelineStateMtl_GetMtlDepthStencilState(This) CALL_IFACE_METHOD(PipelineStateMtl, GetMtlDepthStencilState, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
