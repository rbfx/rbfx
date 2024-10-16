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
/// Definition of the Diligent::IDeviceContextWebGPU interface

#include "../../GraphicsEngine/interface/DeviceContext.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {FC2A8031-0EDA-4115-B00E-DFB1FA8AD5C8}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_DeviceContextWebGPU =
    {0xFC2A8031, 0x0EDA, 0x4115, {0xB0, 0x0E, 0xDF, 0xB1, 0xFA, 0x8A, 0xD5, 0xC8}};

#define DILIGENT_INTERFACE_NAME IDeviceContextWebGPU
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceContextWebGPUInclusiveMethods \
    IDeviceContextInclusiveMethods;          \
    IDeviceContextWebGPUMethods DeviceContextWebGPU

// clang-format off

typedef void (*MapBufferAsyncCallback)(PVoid REF pMappedData, PVoid pUserData);

typedef void (*MapTextureSubresourceAsyncCallback)(MappedTextureSubresource REF MappedData, PVoid pUserData);

/// Exposes WebGPU-specific functionality of a device context.
DILIGENT_BEGIN_INTERFACE(IDeviceContextWebGPU, IDeviceContext)
{
    /// Returns a pointer to the WebGPU queue object associated with this device context.
    VIRTUAL WGPUQueue METHOD(GetWebGPUQueue)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off
#    define IDeviceContextWebGPU_MapBufferAsync(This, ...)              CALL_IFACE_METHOD(DeviceContextWebGPU, MapBufferAsync, This, __VA_ARGS__)
#    define IDeviceContextWebGPU_MapTextureSubresourceAsync(This, ...)  CALL_IFACE_METHOD(DeviceContextWebGPU, MapTextureSubresourceAsync, This, __VA_ARGS__)
#    define IDeviceContextWebGPU_GetWebGPUQueue(This)                   CALL_IFACE_METHOD(DeviceContextWebGPU, GetWebGPUQueue, This)
// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
