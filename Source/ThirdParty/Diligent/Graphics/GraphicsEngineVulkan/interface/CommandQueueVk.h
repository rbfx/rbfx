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
/// Definition of the Diligent::ICommandQueueVk interface

#include "../../GraphicsEngine/interface/CommandQueue.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {9FBF582F-3069-41B9-AC05-344D5AF5CE8C}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_CommandQueueVk =
    {0x9fbf582f, 0x3069, 0x41b9, {0xac, 0x5, 0x34, 0x4d, 0x5a, 0xf5, 0xce, 0x8c}};

#define DILIGENT_INTERFACE_NAME ICommandQueueVk
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ICommandQueueVkInclusiveMethods \
    ICommandQueueInclusiveMethods;      \
    ICommandQueueVkMethods CommandQueueVk

// clang-format off

/// Command queue interface
DILIGENT_BEGIN_INTERFACE(ICommandQueueVk, ICommandQueue)
{
    /// Submits a given command buffer to the command queue

    /// \return Fence value associated with the submitted command buffer
    VIRTUAL Uint64 METHOD(SubmitCmdBuffer)(THIS_
                                           VkCommandBuffer cmdBuffer) PURE;

    /// Submits a given work batch to the internal Vulkan command queue

    /// \return Fence value associated with the submitted command buffer
    VIRTUAL Uint64 METHOD(Submit)(THIS_
                                  const VkSubmitInfo REF SubmitInfo) PURE;

    /// Presents the current swap chain image on the screen
    VIRTUAL VkResult METHOD(Present)(THIS_
                                     const VkPresentInfoKHR REF PresentInfo) PURE;

    /// Submits a sparse bind commands to the internal Vulkan command queue

    /// \return Fence value associated with the submitted sparse bind command
    VIRTUAL Uint64 METHOD(BindSparse)(THIS_
                                      const VkBindSparseInfo REF BindInfo) PURE;

    /// Returns Vulkan command queue handle. May return VK_NULL_HANDLE if queue is unavailable
    ///
    /// \warning  Access to the VkQueue must be externally synchronized.
    ///           Don't use this method to submit commands directly, use SubmitCmdBuffer() or Submit(),
    ///           which are thread-safe.
    VIRTUAL VkQueue METHOD(GetVkQueue)(THIS) PURE;

    /// Returns vulkan command queue family index
    VIRTUAL uint32_t METHOD(GetQueueFamilyIndex)(THIS) CONST PURE;

    /// Signals the given fence
    VIRTUAL void METHOD(EnqueueSignalFence)(THIS_
                                            VkFence vkFence) PURE;

    /// Signals the given timeline semaphore.
    ///
    /// \note  Requires NativeFence feature, see Diligent::DeviceFeatures.
    VIRTUAL void METHOD(EnqueueSignal)(THIS_
                                       VkSemaphore vkTimelineSemaphore,
                                       Uint64      Value) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ICommandQueueVk_SubmitCmdBuffer(This, ...)        CALL_IFACE_METHOD(CommandQueueVk, SubmitCmdBuffer,        This, __VA_ARGS__)
#    define ICommandQueueVk_Submit(This, ...)                 CALL_IFACE_METHOD(CommandQueueVk, Submit,                 This, __VA_ARGS__)
#    define ICommandQueueVk_Present(This, ...)                CALL_IFACE_METHOD(CommandQueueVk, Present,                This, __VA_ARGS__)
#    define ICommandQueueVk_GetVkQueue(This)                  CALL_IFACE_METHOD(CommandQueueVk, GetVkQueue,             This)
#    define ICommandQueueVk_GetQueueFamilyIndex(This)         CALL_IFACE_METHOD(CommandQueueVk, GetQueueFamilyIndex,    This)
#    define ICommandQueueVk_EnqueueSignalFence(This, ...)     CALL_IFACE_METHOD(CommandQueueVk, EnqueueSignalFence,     This, __VA_ARGS__)
#    define ICommandQueueVk_EnqueueSignal(This, ...)          CALL_IFACE_METHOD(CommandQueueVk, EnqueueSignal,          This, __VA_ARGS__)
#    define ICommandQueueVk_BindSparse(This, ...)             CALL_IFACE_METHOD(CommandQueueVk, BindSparse,             This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
