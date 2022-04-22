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

#include <mutex>
#include <array>
#include <vector>

#include "Query.h"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"
#include "VulkanUtilities/VulkanPhysicalDevice.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "VulkanUtilities/VulkanCommandBuffer.hpp"

namespace Diligent
{

class RenderDeviceVkImpl;

class QueryManagerVk
{
public:
    QueryManagerVk(RenderDeviceVkImpl* RenderDeviceVk,
                   const Uint32        QueryHeapSizes[],
                   SoftwareQueueIndex  CmdQueueInd);
    ~QueryManagerVk();

    // clang-format off
    QueryManagerVk           (const QueryManagerVk&)  = delete;
    QueryManagerVk           (      QueryManagerVk&&) = delete;
    QueryManagerVk& operator=(const QueryManagerVk&)  = delete;
    QueryManagerVk& operator=(      QueryManagerVk&&) = delete;
    // clang-format on

    static constexpr Uint32 InvalidIndex = static_cast<Uint32>(-1);

    Uint32 AllocateQuery(QUERY_TYPE Type);
    void   DiscardQuery(QUERY_TYPE Type, Uint32 Index);

    VkQueryPool GetQueryPool(QUERY_TYPE Type) const
    {
        return m_Pools[Type].GetVkQueryPool();
    }

    Uint64 GetCounterFrequency() const
    {
        return m_CounterFrequency;
    }

    Uint32 ResetStaleQueries(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                             VulkanUtilities::VulkanCommandBuffer&       CmdBuff);

    SoftwareQueueIndex GetCommandQueueId() const
    {
        return m_CommandQueueId;
    }

private:
    class QueryPoolInfo
    {
    public:
        void Init(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                  const VkQueryPoolCreateInfo&                QueryPoolCI,
                  QUERY_TYPE                                  Type);

        QueryPoolInfo() noexcept {}
        ~QueryPoolInfo();

        // clang-format off
        QueryPoolInfo           (const QueryPoolInfo&)  = delete;
        QueryPoolInfo           (      QueryPoolInfo&&) = delete;
        QueryPoolInfo& operator=(const QueryPoolInfo&)  = delete;
        QueryPoolInfo& operator=(      QueryPoolInfo&&) = delete;
        // clang-format on

        Uint32 Allocate();
        void   Discard(Uint32 Index);
        Uint32 ResetStaleQueries(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice, VulkanUtilities::VulkanCommandBuffer& CmdBuff);

        QUERY_TYPE GetType() const
        {
            return m_Type;
        }
        Uint32 GetQueryCount() const
        {
            return m_QueryCount;
        }
        VkQueryPool GetVkQueryPool() const
        {
            return m_vkQueryPool;
        }
        Uint32 GetMaxAllocatedQueries() const
        {
            return m_MaxAllocatedQueries;
        }
        bool IsNull() const
        {
            return m_vkQueryPool == VK_NULL_HANDLE;
        }

    private:
        VulkanUtilities::QueryPoolWrapper m_vkQueryPool;

        QUERY_TYPE m_Type                = QUERY_TYPE_UNDEFINED;
        Uint32     m_QueryCount          = 0;
        Uint32     m_MaxAllocatedQueries = 0;

        std::mutex          m_QueriesMtx;
        std::vector<Uint32> m_AvailableQueries;
        std::vector<Uint32> m_StaleQueries;
    };

    const SoftwareQueueIndex m_CommandQueueId;

    std::array<QueryPoolInfo, QUERY_TYPE_NUM_TYPES> m_Pools;

    Uint64 m_CounterFrequency = 0;
};

} // namespace Diligent
