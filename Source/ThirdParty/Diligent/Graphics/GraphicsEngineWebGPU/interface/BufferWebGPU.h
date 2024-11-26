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
/// Definition of the Diligent::IBufferWebGPU interface

#include "../../GraphicsEngine/interface/Buffer.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {A3AB9014-8B47-4A13-AA96-F4975BFD13CA}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_BufferWebGPU =
    {0xA3AB9014, 0x8B47, 0x4A13, {0xAA, 0x96, 0xF4, 0x97, 0x5B, 0xFD, 0x13, 0xCA}};

#define DILIGENT_INTERFACE_NAME IBufferWebGPU
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IBufferWebGPUInclusiveMethods \
    IBufferInclusiveMethods;          \
    IBufferWebGPUMethods BufferWebGPU

// clang-format off

/// Exposes WebGPU-specific functionality of a buffer object.
DILIGENT_BEGIN_INTERFACE(IBufferWebGPU, IBuffer)
{
    /// The application must not destroy the returned buffer
    /// Returns a WebGPU handle of the internal buffer object.
    VIRTUAL WGPUBuffer METHOD(GetWebGPUBuffer)(THIS) CONST PURE;

};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IBufferWebGPU_GetWebGPUBuffer(This)         CALL_IFACE_METHOD(BufferWebGPU, GetWebGPUBuffer, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
