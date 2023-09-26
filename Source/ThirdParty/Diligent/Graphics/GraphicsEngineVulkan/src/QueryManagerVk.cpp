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

#include "QueryManagerVk.hpp"

#include <algorithm>

#include "RenderDeviceVkImpl.hpp"
#include "GraphicsAccessories.hpp"
#include "VulkanUtilities/VulkanCommandBuffer.hpp"

namespace Diligent
{

void QueryManagerVk::QueryPoolInfo::Init(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                                         const VkQueryPoolCreateInfo&                QueryPoolCI,
                                         QUERY_TYPE                                  Type)
{
    m_Type        = Type;
    m_QueryCount  = QueryPoolCI.queryCount;
    m_vkQueryPool = LogicalDevice.CreateQueryPool(QueryPoolCI, "QueryManagerVk: query pool");

    m_StaleQueries.resize(m_QueryCount);
    for (Uint32 i = 0; i < m_QueryCount; ++i)
        m_StaleQueries[i] = i;
}

QueryManagerVk::QueryPoolInfo::~QueryPoolInfo()
{
    auto OutstandingQueries = GetQueryCount() - (m_AvailableQueries.size() + m_StaleQueries.size());
    if (OutstandingQueries == 1)
    {
        LOG_ERROR_MESSAGE("One query of type ", GetQueryTypeString(m_Type),
                          " has not been returned to the query manager");
    }
    else if (OutstandingQueries > 1)
    {
        LOG_ERROR_MESSAGE(OutstandingQueries, " queries of type ", GetQueryTypeString(m_Type),
                          " have not been returned to the query manager");
    }
}

Uint32 QueryManagerVk::QueryPoolInfo::Allocate()
{
    Uint32 Index = InvalidIndex;

    std::lock_guard<std::mutex> Lock{m_QueriesMtx};
    if (!m_AvailableQueries.empty())
    {
        Index = m_AvailableQueries.back();
        m_AvailableQueries.pop_back();
        m_MaxAllocatedQueries = std::max(m_MaxAllocatedQueries, m_QueryCount - static_cast<Uint32>(m_AvailableQueries.size()));
    }

    return Index;
}

void QueryManagerVk::QueryPoolInfo::Discard(Uint32 Index)
{
    std::lock_guard<std::mutex> Lock{m_QueriesMtx};

    VERIFY(Index < m_QueryCount, "Query index ", Index, " is out of range");
    VERIFY(m_vkQueryPool != VK_NULL_HANDLE, "Query pool is not initialized");
    VERIFY(std::find(m_AvailableQueries.begin(), m_AvailableQueries.end(), Index) == m_AvailableQueries.end(),
           "Index ", Index, " already present in available queries list");
    VERIFY(std::find(m_StaleQueries.begin(), m_StaleQueries.end(), Index) == m_StaleQueries.end(),
           "Index ", Index, " already present in stale queries list");

    m_StaleQueries.push_back(Index);
}

Uint32 QueryManagerVk::QueryPoolInfo::ResetStaleQueries(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                                                        VulkanUtilities::VulkanCommandBuffer&       CmdBuff)
{
    if (m_StaleQueries.empty())
        return 0;

    const auto& EnabledExtFeatures = LogicalDevice.GetEnabledExtFeatures();

    auto ResetQueries = [&](uint32_t FirstQuery, uint32_t QueryCount) //
    {
        if (EnabledExtFeatures.HostQueryReset.hostQueryReset)
        {
            LogicalDevice.ResetQueryPool(m_vkQueryPool, FirstQuery, QueryCount);
        }
        else
        {
            // Note that vkCmdResetQueryPool must be called outside of a render pass,
            // so it is suboptimal to postpone query reset until it is used.
            CmdBuff.ResetQueryPool(m_vkQueryPool, FirstQuery, QueryCount);
        }
    };

    std::lock_guard<std::mutex> Lock{m_QueriesMtx};
    VERIFY(!IsNull(), "Query pool is not initialized");

    // After query pool creation, each query must be reset before it is used.
    // Queries must also be reset between uses (17.2).
    Uint32 NumCommands = 0;
    if (m_StaleQueries.size() == m_QueryCount)
    {
        ResetQueries(0, m_QueryCount);
        m_AvailableQueries.resize(m_QueryCount);
        for (Uint32 i = 0; i < m_QueryCount; ++i)
            m_AvailableQueries[i] = i;
        NumCommands = 1;
    }
    else
    {
        for (auto& StaleQuery : m_StaleQueries)
        {
            ResetQueries(StaleQuery, 1);
            m_AvailableQueries.push_back(StaleQuery);
        }
        NumCommands = static_cast<Uint32>(m_StaleQueries.size());
    }
    m_StaleQueries.clear();

    return NumCommands;
}

QueryManagerVk::QueryManagerVk(RenderDeviceVkImpl* pRenderDeviceVk,
                               const Uint32        QueryHeapSizes[],
                               SoftwareQueueIndex  CmdQueueInd) :
    m_CommandQueueId{CmdQueueInd}
{
    const auto& LogicalDevice  = pRenderDeviceVk->GetLogicalDevice();
    const auto& PhysicalDevice = pRenderDeviceVk->GetPhysicalDevice();

    auto timestampPeriod = PhysicalDevice.GetProperties().limits.timestampPeriod;
    m_CounterFrequency   = static_cast<Uint64>(1000000000.0 / timestampPeriod);

    const auto  QueueFamilyIndex       = HardwareQueueIndex{pRenderDeviceVk->GetCommandQueue(CmdQueueInd).GetQueueFamilyIndex()};
    const auto& EnabledFeatures        = LogicalDevice.GetEnabledFeatures();
    const auto  StageMask              = LogicalDevice.GetSupportedStagesMask(QueueFamilyIndex);
    const auto  QueueFlags             = PhysicalDevice.GetQueueProperties()[QueueFamilyIndex].queueFlags;
    const auto& DeviceInfo             = pRenderDeviceVk->GetDeviceInfo();
    const bool  IsTransferQueue        = (QueueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)) == 0;
    const bool  QueueSupportsTimestamp = PhysicalDevice.GetQueueProperties()[QueueFamilyIndex].timestampValidBits > 0;

    for (Uint32 query_type = QUERY_TYPE_UNDEFINED + 1; query_type < QUERY_TYPE_NUM_TYPES; ++query_type)
    {
        const auto QueryType = static_cast<QUERY_TYPE>(query_type);
        if ((QueryType == QUERY_TYPE_OCCLUSION && !EnabledFeatures.occlusionQueryPrecise) ||
            (QueryType == QUERY_TYPE_PIPELINE_STATISTICS && !EnabledFeatures.pipelineStatisticsQuery))
            continue;

        // Time and duration queries are supported in all queues
        if (QueryType == QUERY_TYPE_TIMESTAMP || QueryType == QUERY_TYPE_DURATION)
        {
            if (!QueueSupportsTimestamp)
                continue;

            if (IsTransferQueue && !DeviceInfo.Features.TransferQueueTimestampQueries)
                continue; // Not supported in transfer queue
        }
        // Other queries are supported only in graphics queue
        else if ((QueueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
            continue;


        // clang-format off
        static_assert(QUERY_TYPE_OCCLUSION          == 1, "Unexpected value of QUERY_TYPE_OCCLUSION. EngineVkCreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_BINARY_OCCLUSION   == 2, "Unexpected value of QUERY_TYPE_BINARY_OCCLUSION. EngineVkCreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_TIMESTAMP          == 3, "Unexpected value of QUERY_TYPE_TIMESTAMP. EngineVkCreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_PIPELINE_STATISTICS== 4, "Unexpected value of QUERY_TYPE_PIPELINE_STATISTICS. EngineVkCreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_DURATION           == 5, "Unexpected value of QUERY_TYPE_DURATION. EngineVkCreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_NUM_TYPES          == 6, "Unexpected value of QUERY_TYPE_NUM_TYPES. EngineVkCreateInfo::QueryPoolSizes must be updated");
        // clang-format on

        VkQueryPoolCreateInfo QueryPoolCI{};
        QueryPoolCI.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        QueryPoolCI.pNext = nullptr;
        QueryPoolCI.flags = 0;

        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled below");
        switch (QueryType)
        {
            case QUERY_TYPE_OCCLUSION:
            case QUERY_TYPE_BINARY_OCCLUSION:
                QueryPoolCI.queryType = VK_QUERY_TYPE_OCCLUSION;
                break;

            case QUERY_TYPE_TIMESTAMP:
            case QUERY_TYPE_DURATION:
                QueryPoolCI.queryType = VK_QUERY_TYPE_TIMESTAMP;
                break;

            case QUERY_TYPE_PIPELINE_STATISTICS:
            {
                QueryPoolCI.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
                QueryPoolCI.pipelineStatistics =
                    VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
                    VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

                if (StageMask & VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT)
                {
                    QueryPoolCI.pipelineStatistics |=
                        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
                        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT;
                }
                if (StageMask & VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT)
                    QueryPoolCI.pipelineStatistics |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT;
                if (StageMask & VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT)
                    QueryPoolCI.pipelineStatistics |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
            }
            break;

            default:
                UNEXPECTED("Unexpected query type");
        }

        QueryPoolCI.queryCount = QueryHeapSizes[QueryType];
        if (QueryType == QUERY_TYPE_DURATION)
            QueryPoolCI.queryCount *= 2;

        auto& PoolInfo = m_Pools[QueryType];
        PoolInfo.Init(LogicalDevice, QueryPoolCI, QueryType);
        VERIFY_EXPR(!PoolInfo.IsNull() && PoolInfo.GetQueryCount() == QueryPoolCI.queryCount && PoolInfo.GetType() == QueryType);
    }
}

QueryManagerVk::~QueryManagerVk()
{
    std::stringstream QueryUsageSS;
    QueryUsageSS << "Vulkan query manager peak usage:";
    for (Uint32 QueryType = QUERY_TYPE_UNDEFINED + 1; QueryType < QUERY_TYPE_NUM_TYPES; ++QueryType)
    {
        auto& PoolInfo = m_Pools[QueryType];
        if (PoolInfo.IsNull())
            continue;

        QueryUsageSS << std::endl
                     << std::setw(30) << std::left << GetQueryTypeString(static_cast<QUERY_TYPE>(QueryType)) << ": "
                     << std::setw(4) << std::right << PoolInfo.GetMaxAllocatedQueries()
                     << '/' << std::setw(4) << PoolInfo.GetQueryCount();
    }
    LOG_INFO_MESSAGE(QueryUsageSS.str());
}

Uint32 QueryManagerVk::AllocateQuery(QUERY_TYPE Type)
{
    return m_Pools[Type].Allocate();
}

void QueryManagerVk::DiscardQuery(QUERY_TYPE Type, Uint32 Index)
{
    m_Pools[Type].Discard(Index);
}

Uint32 QueryManagerVk::ResetStaleQueries(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice, VulkanUtilities::VulkanCommandBuffer& CmdBuff)
{
    Uint32 NumQueriesReset = 0;
    for (auto& PoolInfo : m_Pools)
    {
        NumQueriesReset += PoolInfo.ResetStaleQueries(LogicalDevice, CmdBuff);
    }
    return NumQueriesReset;
}

} // namespace Diligent
