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

#pragma once

/// \file
/// Definition of the Diligent::ISamplerWebGPU interface

#include "../../GraphicsEngine/interface/Sampler.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {5425A1DD-7C75-43D1-9171-30ED61B3F9A2}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_SamplerWebGPU =
    {0x5425A1DD, 0x7C75, 0x43D1, {0x91, 0x71, 0x30, 0xED, 0x61, 0xB3, 0xF9, 0xA2}};

#define DILIGENT_INTERFACE_NAME ISamplerWebGPU
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISamplerWebGPUInclusiveMethods \
    ISamplerInclusiveMethods;          \
    ISamplerWebGPUMethods SamplerWebGPU

/// Exposes WebGPU-specific functionality of a sampler object.
DILIGENT_BEGIN_INTERFACE(ISamplerWebGPU, ISampler)
{
    /// Returns a WebGPU handle of the internal sampler object.
    VIRTUAL WGPUSampler METHOD(GetWebGPUSampler)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define ISamplerWebGPU_GetWebGPUSampler(This) CALL_IFACE_METHOD(SamplerWebGPU, GetWebGPUSampler, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
