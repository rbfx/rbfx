/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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
/// Definition of the Diligent::IDeviceMemoryD3D11 interface

#include "../../GraphicsEngine/interface/DeviceMemory.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {99DADE81-04F7-4C81-AE06-32B25F5B45AA}
static const INTERFACE_ID IID_DeviceMemoryD3D11 =
    {0x99dade81, 0x4f7, 0x4c81, {0xae, 0x6, 0x32, 0xb2, 0x5f, 0x5b, 0x45, 0xaa}};

#define DILIGENT_INTERFACE_NAME IDeviceMemoryD3D11
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceMemoryD3D11InclusiveMethods \
    IDeviceMemoryInclusiveMethods;         \
    IDeviceMemoryD3D11Methods DeviceMemoryD3D11

/// Exposes Direct3D11-specific functionality of a device memory object.
DILIGENT_BEGIN_INTERFACE(IDeviceMemoryD3D11, IDeviceMemory)
{
    /// Returns a pointer to the ID3D11Buffer interface of the internal Direct3D11 object.

    /// The method does *NOT* increment the reference counter of the returned object,
    /// so Release() must not be called.
    VIRTUAL ID3D11Buffer* METHOD(GetD3D11TilePool)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IDeviceMemoryD3D11_GetD3D11TilePool(This) CALL_IFACE_METHOD(DeviceMemoryD3D11, GetD3D11TilePool, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
