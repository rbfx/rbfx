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

#pragma once

/// \file
/// Definition of the Diligent::IPipelineStateWebGPU interface

#include "../../GraphicsEngine/interface/PipelineState.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {22EC81D8-47C9-480F-8DC5-02D6133BDF3C}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_PipelineStateWebGPU =
    {0x22EC81D8, 0x47C9, 0x480F, {0x8D, 0xC5, 0x02, 0xD6, 0x13, 0x3B, 0xDF, 0x3C}};

#define DILIGENT_INTERFACE_NAME IPipelineStateWebGPU
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IPipelineStateWebGPUInclusiveMethods \
    IPipelineStateInclusiveMethods;          \
    IPipelineStateWebGPUMethods PipelineStateWebGPU

/// Exposes WebGPU-specific functionality of a pipeline state object.
DILIGENT_BEGIN_INTERFACE(IPipelineStateWebGPU, IPipelineState)
{
    /// Returns a pointer to WebGPU render pipeline (WGPURenderPipeline)
    VIRTUAL WGPURenderPipeline METHOD(GetWebGPURenderPipeline)(THIS) CONST PURE;

    /// Returns a pointer to WebGPU compute pipeline (WGPUComputePipeline)
    VIRTUAL WGPUComputePipeline METHOD(GetWebGPUComputePipeline)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IPipelineStateWebGPU_GetWebGPURenderPipeline(This)  CALL_IFACE_METHOD(PipelineStateWebGPU, GetWebGPURenderPipeline,  This)
#    define IPipelineStateWebGPU_GetWebGPUComputePipeline(This) CALL_IFACE_METHOD(PipelineStateWebGPU, GetWebGPUComputePipeline, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
