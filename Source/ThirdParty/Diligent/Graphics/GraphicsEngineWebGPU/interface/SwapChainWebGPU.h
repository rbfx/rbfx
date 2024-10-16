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
/// Definition of the Diligent::ISwapChainWebGPU interface

#include "../../GraphicsEngine/interface/SwapChain.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {C298E077-D10D-4704-A50E-3A1598B09595}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_SwapChainWebGPU =
    {0xC298E077, 0xD10D, 0x4704, {0xA5, 0x0E, 0x3A, 0x15, 0x98, 0xB0, 0x95, 0x95}};

#define DILIGENT_INTERFACE_NAME ISwapChainWebGPU
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISwapChainWebGPUInclusiveMethods \
    ISwapChainInclusiveMethods;          \
    ISwapChainWebGPUMethods SwapChainWebGPU

/// Exposes WebGPU-specific functionality of a swap chain.
DILIGENT_BEGIN_INTERFACE(ISwapChainWebGPU, ISwapChain)
{
    /// Returns a WebGPU handle to the internal surface object.
    VIRTUAL WGPUSurface METHOD(GetWebGPUSurface)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ISwapChainWebGPU_GetWebGPUSurface(This)  CALL_IFACE_METHOD(SwapChainWebGPU, GetWebGPUSurface, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
