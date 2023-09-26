/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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
/// Definition of the Diligent::ITopLevelASVk interface

#include "../../GraphicsEngine/interface/TopLevelAS.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {356FFFFA-9E57-49F7-8FF4-7017B61BE6A8}
static const INTERFACE_ID IID_TopLevelASVk =
    {0x356ffffa, 0x9e57, 0x49f7, {0x8f, 0xf4, 0x70, 0x17, 0xb6, 0x1b, 0xe6, 0xa8}};

#define DILIGENT_INTERFACE_NAME ITopLevelASVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITopLevelASVkInclusiveMethods \
    ITopLevelASInclusiveMethods;      \
    ITopLevelASVkMethods TopLevelASVk

/// Exposes Vulkan-specific functionality of a Top-level acceleration structure object.
DILIGENT_BEGIN_INTERFACE(ITopLevelASVk, ITopLevelAS)
{
    /// Returns a Vulkan handle of the internal top-level AS object.
    VIRTUAL VkAccelerationStructureKHR METHOD(GetVkTLAS)(THIS) CONST PURE;

    /// Returns a Vulkan device address of the internal top-level AS object.
    VIRTUAL VkDeviceAddress METHOD(GetVkDeviceAddress)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define ITopLevelASVk_GetVkTLAS(This)          CALL_IFACE_METHOD(TopLevelASVk, GetVkTLAS, This)
#    define ITopLevelASVk_GetVkDeviceAddress(This) CALL_IFACE_METHOD(TopLevelASVk, GetVkDeviceAddress, This)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
