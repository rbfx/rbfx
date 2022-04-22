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
/// Definition of the Diligent::IDeviceMemoryVk interface

#include "../../GraphicsEngine/interface/DeviceMemory.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {401D0549-66EF-42AD-9F67-22002718806D}
static const INTERFACE_ID IID_DeviceMemoryVk =
    {0x401d0549, 0x66ef, 0x42ad, {0x9f, 0x67, 0x22, 0x0, 0x27, 0x18, 0x80, 0x6d}};


#define DILIGENT_INTERFACE_NAME IDeviceMemoryVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceMemoryVkInclusiveMethods \
    IDeviceMemoryInclusiveMethods;      \
    IDeviceMemoryVkMethods DeviceMemoryVk

// clang-format off

/// This structure is returned by IDeviceMemoryVk::GetRange()
struct DeviceMemoryRangeVk
{
    /// Vulkan memory object.
    VkDeviceMemory Handle  DEFAULT_INITIALIZER(VK_NULL_HANDLE);

    /// Offset to the start of the memory range, in bytes.
    VkDeviceSize   Offset  DEFAULT_INITIALIZER(0);

    /// Memory range size in bytes.
    /// When IDeviceMemoryVk::GetRange() succeeds, the size is equal to the Size argument
    /// that was given to the function, and zero otherwise.
    VkDeviceSize   Size    DEFAULT_INITIALIZER(0);
};
typedef struct DeviceMemoryRangeVk DeviceMemoryRangeVk;

/// Exposes Vulkan-specific functionality of a device memory object.
DILIGENT_BEGIN_INTERFACE(IDeviceMemoryVk, IDeviceMemory)
{
    /// Returns a DeviceMemoryRangeVk object with the information about
    /// the Vulkan device memory associated with the specified memory range.
    VIRTUAL DeviceMemoryRangeVk METHOD(GetRange)(THIS_
                                                 Uint64 Offset,
                                                 Uint64 Size) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IDeviceMemoryVk_GetRange(This, ...)  CALL_IFACE_METHOD(DeviceMemoryVk, GetRange, This, __VA_ARGS__)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
