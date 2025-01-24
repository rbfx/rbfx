/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
/// Definition of the Diligent::IRenderDeviceVk interface

#include "../../GraphicsEngine/interface/RenderDevice.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {AB8CF3A6-D959-41C1-AE00-A58AE9820E6A}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_RenderDeviceVk =
    {0xab8cf3a6, 0xd959, 0x41c1, {0xae, 0x0, 0xa5, 0x8a, 0xe9, 0x82, 0xe, 0x6a}};

#define DILIGENT_INTERFACE_NAME IRenderDeviceVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IRenderDeviceVkInclusiveMethods \
    IRenderDeviceInclusiveMethods;      \
    IRenderDeviceVkMethods RenderDeviceVk

// clang-format off

/// Exposes Vulkan-specific functionality of a render device.
DILIGENT_BEGIN_INTERFACE(IRenderDeviceVk, IRenderDevice)
{
    /// Returns a handle of the logical Vulkan device
    VIRTUAL VkDevice METHOD(GetVkDevice)(THIS) PURE;

    /// Returns a handle of the physical Vulkan device
    VIRTUAL VkPhysicalDevice METHOD(GetVkPhysicalDevice)(THIS) PURE;

    /// Returns Vulkan instance
    VIRTUAL VkInstance METHOD(GetVkInstance)(THIS) PURE;

    /// Returns Vulkan API version

    /// \note This version is the minimum of the instance version and what the physical device supports.
    VIRTUAL Uint32 METHOD(GetVkVersion)(THIS) PURE;

    /// Creates a texture object from native Vulkan image

    /// \param [in]  vkImage      - Vulkan image handle
    /// \param [in]  TexDesc      - Texture description. Vulkan provides no means to retrieve any
    ///                             image properties from the image handle, so complete texture
    ///                             description must be provided
    /// \param [in]  InitialState - Initial texture state. See Diligent::RESOURCE_STATE.
    /// \param [out] ppTexture    - Address of the memory location where the pointer to the
    ///                             texture interface will be stored.
    ///                             The function calls AddRef(), so that the new object will contain
    ///                             one reference.
    /// \note  Created texture object does not take ownership of the Vulkan image and will not
    ///        destroy it once released. The application must not destroy the image while it is
    ///        in use by the engine.
    VIRTUAL void METHOD(CreateTextureFromVulkanImage)(THIS_
                                                      VkImage               vkImage,
                                                      const TextureDesc REF TexDesc,
                                                      RESOURCE_STATE        InitialState,
                                                      ITexture**            ppTexture) PURE;

    /// Creates a buffer object from native Vulkan resource

    /// \param [in] vkBuffer      - Vulkan buffer handle
    /// \param [in] BuffDesc      - Buffer description. Vulkan provides no means to retrieve any
    ///                             buffer properties from the buffer handle, so complete buffer
    ///                             description must be provided
    /// \param [in]  InitialState - Initial buffer state. See Diligent::RESOURCE_STATE.
    /// \param [out] ppBuffer     - Address of the memory location where the pointer to the
    ///                             buffer interface will be stored.
    ///                             The function calls AddRef(), so that the new object will contain
    ///                             one reference.
    /// \note  Created buffer object does not take ownership of the Vulkan buffer and will not
    ///        destroy it once released. The application must not destroy Vulkan buffer while it is
    ///        in use by the engine.
    VIRTUAL void METHOD(CreateBufferFromVulkanResource)(THIS_
                                                        VkBuffer             vkBuffer,
                                                        const BufferDesc REF BuffDesc,
                                                        RESOURCE_STATE       InitialState,
                                                        IBuffer**            ppBuffer) PURE;

    /// Creates a bottom-level AS object from native Vulkan resource

    /// \param [in]  vkBLAS       - Vulkan acceleration structure handle.
    /// \param [in]  Desc         - Bottom-level AS description.
    /// \param [in]  InitialState - Initial BLAS state. Can be RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_BUILD_AS_READ, RESOURCE_STATE_BUILD_AS_WRITE.
    ///                             See Diligent::RESOURCE_STATE.
    /// \param [out] ppBLAS       - Address of the memory location where the pointer to the
    ///                             bottom-level AS interface will be stored.
    ///                             The function calls AddRef(), so that the new object will contain
    ///                             one reference.
    /// \note  Created bottom-level AS object does not take ownership of the Vulkan acceleration structure and will not
    ///        destroy it once released. The application must not destroy Vulkan acceleration structure while it is
    ///        in use by the engine.
    VIRTUAL void METHOD(CreateBLASFromVulkanResource)(THIS_
                                                      VkAccelerationStructureKHR  vkBLAS,
                                                      const BottomLevelASDesc REF Desc,
                                                      RESOURCE_STATE              InitialState,
                                                      IBottomLevelAS**            ppBLAS) PURE;

    /// Creates a top-level AS object from native Vulkan resource

    /// \param [in]  vkTLAS       - Vulkan acceleration structure handle.
    /// \param [in]  Desc         - Bottom-level AS description.
    /// \param [in]  InitialState - Initial TLAS state. Can be RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_BUILD_AS_READ, RESOURCE_STATE_BUILD_AS_WRITE, RESOURCE_STATE_RAY_TRACING.
    ///                             See Diligent::RESOURCE_STATE.
    /// \param [out] ppTLAS       - Address of the memory location where the pointer to the
    ///                             top-level AS interface will be stored.
    ///                             The function calls AddRef(), so that the new object will contain
    ///                             one reference.
    /// \note  Created top-level AS object does not take ownership of the Vulkan acceleration structure and will not
    ///        destroy it once released. The application must not destroy Vulkan acceleration structure while it is
    ///        in use by the engine.
    VIRTUAL void METHOD(CreateTLASFromVulkanResource)(THIS_
                                                      VkAccelerationStructureKHR vkTLAS,
                                                      const TopLevelASDesc REF   Desc,
                                                      RESOURCE_STATE             InitialState,
                                                      ITopLevelAS**              ppTLAS) PURE;

    /// Creates a fence object from native Vulkan resource

    /// \param [in]  vkTimelineSemaphore - Vulkan timeline semaphore handle.
    /// \param [in]  Desc                - Fence description.
    /// \param [out] ppFence             - Address of the memory location where the pointer to the
    ///                                    fence interface will be stored.
    ///                                    The function calls AddRef(), so that the new object will contain
    ///                                    one reference.
    /// \note  Created fence object does not take ownership of the Vulkan semaphore and will not
    ///        destroy it once released. The application must not destroy Vulkan semaphore while it is
    ///        in use by the engine.
    VIRTUAL void METHOD(CreateFenceFromVulkanResource)(THIS_
                                                       VkSemaphore         vkTimelineSemaphore,
                                                       const FenceDesc REF Desc,
                                                       IFence**            ppFence) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IRenderDeviceVk_GetVkDevice(This)                         CALL_IFACE_METHOD(RenderDeviceVk, GetVkDevice,                    This)
#    define IRenderDeviceVk_GetVkPhysicalDevice(This)                 CALL_IFACE_METHOD(RenderDeviceVk, GetVkPhysicalDevice,            This)
#    define IRenderDeviceVk_GetVkInstance(This)                       CALL_IFACE_METHOD(RenderDeviceVk, GetVkInstance,                  This)
#    define IRenderDeviceVk_CreateTextureFromVulkanImage(This, ...)   CALL_IFACE_METHOD(RenderDeviceVk, CreateTextureFromVulkanImage,   This, __VA_ARGS__)
#    define IRenderDeviceVk_CreateBufferFromVulkanResource(This, ...) CALL_IFACE_METHOD(RenderDeviceVk, CreateBufferFromVulkanResource, This, __VA_ARGS__)
#    define IRenderDeviceVk_CreateBLASFromVulkanResource(This, ...)   CALL_IFACE_METHOD(RenderDeviceVk, CreateBLASFromVulkanResource,   This, __VA_ARGS__)
#    define IRenderDeviceVk_CreateTLASFromVulkanResource(This, ...)   CALL_IFACE_METHOD(RenderDeviceVk, CreateTLASFromVulkanResource,   This, __VA_ARGS__)
#    define IRenderDeviceVk_CreateFenceFromVulkanResource(This, ...)  CALL_IFACE_METHOD(RenderDeviceVk, CreateFenceFromVulkanResource,  This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
