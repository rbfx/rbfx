/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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
/// Definition of the Diligent::IDeviceMemoryMtl interface

#include "../../GraphicsEngine/interface/DeviceMemory.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {FAA1CD77-590A-408B-B0E8-C21CD062542C}
static const INTERFACE_ID IID_DeviceMemoryMtl =
    {0xfaa1cd77, 0x590a, 0x408b, {0xb0, 0xe8, 0xc2, 0x1c, 0xd0, 0x62, 0x54, 0x2c}};

#define DILIGENT_INTERFACE_NAME IDeviceMemoryMtl
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceMemoryMtlInclusiveMethods \
    IDeviceMemoryInclusiveMethods;       \
    IDeviceMemoryMtlMethods DeviceMemoryMtl

// clang-format off

/// Exposes Metal-specific functionality of a device memory object.
DILIGENT_BEGIN_INTERFACE(IDeviceMemoryMtl, IDeviceMemory)
{
    /// Returns a pointer to a Metal heap object.
    VIRTUAL id<MTLHeap> METHOD(GetMtlResource)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IDeviceMemoryMtl_GetMtlResource(This) CALL_IFACE_METHOD(DeviceMemoryMtl, GetMtlResource, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
