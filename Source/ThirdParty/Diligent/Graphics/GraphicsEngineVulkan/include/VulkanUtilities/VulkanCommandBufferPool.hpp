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

#include <deque>
#include <memory>
#include <mutex>
#include <atomic>
#include "VulkanHeaders.h"
#include "VulkanLogicalDevice.hpp"
#include "VulkanObjectWrappers.hpp"

namespace VulkanUtilities
{

class VulkanCommandBufferPool
{
public:
    VulkanCommandBufferPool(std::shared_ptr<const VulkanLogicalDevice> LogicalDevice,
                            HardwareQueueIndex                         queueFamilyIndex,
                            VkCommandPoolCreateFlags                   flags);

    // clang-format off
    VulkanCommandBufferPool             (const VulkanCommandBufferPool&)  = delete;
    VulkanCommandBufferPool             (      VulkanCommandBufferPool&&) = delete;
    VulkanCommandBufferPool& operator = (const VulkanCommandBufferPool&)  = delete;
    VulkanCommandBufferPool& operator = (      VulkanCommandBufferPool&&) = delete;
    // clang-format on

    ~VulkanCommandBufferPool();

    VkCommandBuffer GetCommandBuffer(const char* DebugName = "");
    // The GPU must have finished with the command buffer being returned to the pool
    void RecycleCommandBuffer(VkCommandBuffer&& CmdBuffer);

    VkPipelineStageFlags GetSupportedStagesMask() const { return m_SupportedStagesMask; }
    VkAccessFlags        GetSupportedAccessMask() const { return m_SupportedAccessMask; }

private:
    // Shared point to logical device must be defined before the command pool
    std::shared_ptr<const VulkanLogicalDevice> m_LogicalDevice;

    CommandPoolWrapper m_CmdPool;

    std::mutex                  m_Mutex;
    std::deque<VkCommandBuffer> m_CmdBuffers;
    const VkPipelineStageFlags  m_SupportedStagesMask;
    const VkAccessFlags         m_SupportedAccessMask;

#ifdef DILIGENT_DEVELOPMENT
    std::atomic<int32_t> m_BuffCounter{0};
#endif
};

} // namespace VulkanUtilities
