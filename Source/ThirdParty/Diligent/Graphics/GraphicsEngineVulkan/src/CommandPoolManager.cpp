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

#include "pch.h"
#include "CommandPoolManager.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "VulkanUtilities/VulkanDebug.hpp"

namespace Diligent
{

CommandPoolManager::CommandPoolManager(const CreateInfo& CI) noexcept :
    // clang-format off
    m_LogicalDevice   {CI.LogicalDevice   },
    m_Name            {CI.Name            },
    m_QueueFamilyIndex{CI.queueFamilyIndex},
    m_CmdPoolFlags    {CI.flags           },
    m_CmdPools        (STD_ALLOCATOR_RAW_MEM(VulkanUtilities::CommandPoolWrapper, GetRawAllocator(), "Allocator for deque<VulkanUtilities::CommandPoolWrapper>"))
// clang-format on
{
}

VulkanUtilities::CommandPoolWrapper CommandPoolManager::AllocateCommandPool(const char* DebugName)
{
    std::lock_guard<std::mutex> LockGuard{m_Mutex};

    VulkanUtilities::CommandPoolWrapper CmdPool;
    if (!m_CmdPools.empty())
    {
        CmdPool = std::move(m_CmdPools.front());
        m_CmdPools.pop_front();

        m_LogicalDevice.ResetCommandPool(CmdPool);
    }

    if (CmdPool == VK_NULL_HANDLE)
    {
        VkCommandPoolCreateInfo CmdPoolCI = {};

        CmdPoolCI.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CmdPoolCI.pNext            = nullptr;
        CmdPoolCI.queueFamilyIndex = m_QueueFamilyIndex;
        CmdPoolCI.flags            = m_CmdPoolFlags;

        CmdPool = m_LogicalDevice.CreateCommandPool(CmdPoolCI);
        DEV_CHECK_ERR(CmdPool != VK_NULL_HANDLE, "Failed to create Vulkan command pool");
    }

    VulkanUtilities::SetCommandPoolName(m_LogicalDevice.GetVkDevice(), CmdPool, DebugName);

#ifdef DILIGENT_DEVELOPMENT
    ++m_AllocatedPoolCounter;
#endif
    return CmdPool;
}

void CommandPoolManager::RecycleCommandPool(VulkanUtilities::CommandPoolWrapper&& CmdPool)
{
    std::lock_guard<std::mutex> LockGuard{m_Mutex};
#ifdef DILIGENT_DEVELOPMENT
    --m_AllocatedPoolCounter;
#endif
    m_CmdPools.emplace_back(std::move(CmdPool));
}

void CommandPoolManager::DestroyPools()
{
    std::lock_guard<std::mutex> LockGuard{m_Mutex};
    DEV_CHECK_ERR(m_AllocatedPoolCounter == 0, m_AllocatedPoolCounter, " pool(s) have not been recycled. This will cause a crash if the references to these pools are still in release queues when CommandPoolManager::RecycleCommandPool() is called for destroyed CommandPoolManager object.");
    LOG_INFO_MESSAGE(m_Name, " allocated descriptor pool count: ", m_CmdPools.size());
    m_CmdPools.clear();
}

CommandPoolManager::~CommandPoolManager()
{
    DEV_CHECK_ERR(m_CmdPools.empty() && m_AllocatedPoolCounter == 0, "Command pools have not been destroyed");
}

} // namespace Diligent
