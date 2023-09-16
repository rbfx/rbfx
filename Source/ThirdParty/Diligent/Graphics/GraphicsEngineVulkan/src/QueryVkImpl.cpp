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

#include "QueryVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "DeviceContextVkImpl.hpp"
#include "EngineMemory.h"

namespace Diligent
{

QueryVkImpl::QueryVkImpl(IReferenceCounters* pRefCounters,
                         RenderDeviceVkImpl* pRenderDeviceVkImpl,
                         const QueryDesc&    Desc,
                         bool                IsDeviceInternal) :
    // clang-format off
    TQueryBase
    {
        pRefCounters,
        pRenderDeviceVkImpl,
        Desc,
        IsDeviceInternal
    }
// clang-format on
{
}

QueryVkImpl::~QueryVkImpl()
{
    DiscardQueries();
}

void QueryVkImpl::DiscardQueries()
{
    for (auto& QueryPoolIdx : m_QueryPoolIndex)
    {
        if (QueryPoolIdx != QueryManagerVk::InvalidIndex)
        {
            VERIFY_EXPR(m_pQueryMgr != nullptr);
            m_pQueryMgr->DiscardQuery(m_Desc.Type, QueryPoolIdx);
            QueryPoolIdx = QueryManagerVk::InvalidIndex;
        }
    }
    m_pQueryMgr          = nullptr;
    m_QueryEndFenceValue = ~Uint64{0};
}

void QueryVkImpl::Invalidate()
{
    DiscardQueries();
    TQueryBase::Invalidate();
}

bool QueryVkImpl::AllocateQueries()
{
    DiscardQueries();

    VERIFY_EXPR(m_pQueryMgr == nullptr);
    VERIFY_EXPR(m_pContext != nullptr);
    m_pQueryMgr = m_pContext->GetQueryManager();
    VERIFY_EXPR(m_pQueryMgr != nullptr);
    for (Uint32 i = 0; i < (m_Desc.Type == QUERY_TYPE_DURATION ? Uint32{2} : Uint32{1}); ++i)
    {
        auto& QueryPoolIdx = m_QueryPoolIndex[i];
        VERIFY_EXPR(QueryPoolIdx == QueryManagerVk::InvalidIndex);

        QueryPoolIdx = m_pQueryMgr->AllocateQuery(m_Desc.Type);
        if (QueryPoolIdx == QueryManagerVk::InvalidIndex)
        {
            LOG_ERROR_MESSAGE("Failed to allocate Vulkan query for type ", GetQueryTypeString(m_Desc.Type),
                              ". Increase the query pool size in EngineVkCreateInfo.");
            DiscardQueries();
            return false;
        }
    }

    return true;
}

bool QueryVkImpl::OnBeginQuery(DeviceContextVkImpl* pContext)
{
    TQueryBase::OnBeginQuery(pContext);

    return AllocateQueries();
}

bool QueryVkImpl::OnEndQuery(DeviceContextVkImpl* pContext)
{
    TQueryBase::OnEndQuery(pContext);

    if (m_Desc.Type == QUERY_TYPE_TIMESTAMP)
    {
        if (!AllocateQueries())
            return false;
    }

    if (m_QueryPoolIndex[0] == QueryManagerVk::InvalidIndex || (m_Desc.Type == QUERY_TYPE_DURATION && m_QueryPoolIndex[1] == QueryManagerVk::InvalidIndex))
    {
        LOG_ERROR_MESSAGE("Query '", m_Desc.Name, "' is invalid: Vulkan query allocation failed");
        return false;
    }

    VERIFY_EXPR(m_pQueryMgr != nullptr);
    auto CmdQueueId      = m_pQueryMgr->GetCommandQueueId();
    m_QueryEndFenceValue = m_pDevice->GetNextFenceValue(CmdQueueId);

    return true;
}


namespace
{

template <size_t NumElements>
inline bool GetQueryResults(
    const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
    VkQueryPool                                 vkQueryPool,
    uint32_t                                    QueryIdx,
    std::array<uint64_t, NumElements>&          Results)
{
    static_assert(NumElements >= 2, "The number of elements must be at least 2 as the last one is used to get the query status.");

    // If VK_QUERY_RESULT_WITH_AVAILABILITY_BIT is set, the final integer value written for each query
    // is non-zero if the query's status was available or zero if the status was unavailable.

    // Applications must take care to ensure that use of the VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
    // bit has the desired effect.
    // For example, if a query has been used previously and a command buffer records the commands
    // vkCmdResetQueryPool, vkCmdBeginQuery, and vkCmdEndQuery for that query, then the query will
    // remain in the available state until vkResetQueryPoolEXT is called or the vkCmdResetQueryPool
    // command executes on a queue. Applications can use fences or events to ensure that a query has
    // already been reset before checking for its results or availability status. Otherwise, a stale
    // value could be returned from a previous use of the query.
    auto vkRes = LogicalDevice.GetQueryPoolResults(vkQueryPool,
                                                   QueryIdx,
                                                   1,                                   // Query Count
                                                   sizeof(Results[0]) * Results.size(), // Data Size
                                                   Results.data(),
                                                   0, // Stride
                                                   VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

    auto DataAvailable = vkRes == VK_SUCCESS;

    constexpr auto IsSingleElementQuery = NumElements == 2;
    if (IsSingleElementQuery)
    {
        // For single-element queries (timestamp, occlusion, duration, etc.),
        // the second element always contains the availability flag.
        // The number of elements returned for the occlusion query depends on the
        // stage flags, so the availability flag index varies.
        DataAvailable = DataAvailable && (Results[1] != 0);
    }

    return DataAvailable;
}

inline bool GetOcclusionQueryData(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                                  VkQueryPool                                 vkQueryPool,
                                  uint32_t                                    QueryIdx,
                                  void*                                       pData,
                                  Uint32                                      DataSize)
{
    std::array<uint64_t, 2> Results{};

    const auto DataAvailable = GetQueryResults(LogicalDevice, vkQueryPool, QueryIdx, Results);
    if (DataAvailable && pData != nullptr)
    {
        auto& QueryData = *reinterpret_cast<QueryDataOcclusion*>(pData);
        VERIFY_EXPR(DataSize == sizeof(QueryData));
        QueryData.NumSamples = Results[0];
    }

    return DataAvailable;
}

inline bool GetBinaryOcclusionQueryData(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                                        VkQueryPool                                 vkQueryPool,
                                        uint32_t                                    QueryIdx,
                                        void*                                       pData,
                                        Uint32                                      DataSize)
{
    std::array<uint64_t, 2> Results{};

    const auto DataAvailable = GetQueryResults(LogicalDevice, vkQueryPool, QueryIdx, Results);
    if (DataAvailable && pData != nullptr)
    {
        auto& QueryData = *reinterpret_cast<QueryDataBinaryOcclusion*>(pData);
        VERIFY_EXPR(DataSize == sizeof(QueryData));
        QueryData.AnySamplePassed = Results[0] != 0;
    }

    return DataAvailable;
}

inline bool GetTimestampQueryData(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                                  VkQueryPool                                 vkQueryPool,
                                  uint32_t                                    QueryIdx,
                                  Uint64                                      CounterFrequency,
                                  void*                                       pData,
                                  Uint32                                      DataSize)
{
    std::array<uint64_t, 2> Results{};

    const auto DataAvailable = GetQueryResults(LogicalDevice, vkQueryPool, QueryIdx, Results);
    if (DataAvailable && pData != nullptr)
    {
        auto& QueryData = *reinterpret_cast<QueryDataTimestamp*>(pData);
        VERIFY_EXPR(DataSize == sizeof(QueryData));
        QueryData.Counter   = Results[0];
        QueryData.Frequency = CounterFrequency;
    }

    return DataAvailable;
}

inline bool GetDurationQueryData(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                                 VkQueryPool                                 vkQueryPool,
                                 const std::array<Uint32, 2>&                QueryIdx,
                                 Uint64                                      CounterFrequency,
                                 void*                                       pData,
                                 Uint32                                      DataSize)
{
    uint64_t StartCounter = 0;
    uint64_t EndCounter   = 0;

    auto DataAvailable = true;
    for (Uint32 i = 0; i < 2; ++i)
    {
        std::array<uint64_t, 2> Results{};
        if (!GetQueryResults(LogicalDevice, vkQueryPool, QueryIdx[i], Results))
            DataAvailable = false;

        (i == 0 ? StartCounter : EndCounter) = Results[0];
    }

    if (DataAvailable && pData != nullptr)
    {
        auto& QueryData = *reinterpret_cast<QueryDataDuration*>(pData);
        VERIFY_EXPR(DataSize == sizeof(QueryData));
        VERIFY_EXPR(EndCounter >= StartCounter);
        QueryData.Duration  = EndCounter - StartCounter;
        QueryData.Frequency = CounterFrequency;
    }

    return DataAvailable;
}

inline bool GetStatisticsQueryData(const VulkanUtilities::VulkanLogicalDevice& LogicalDevice,
                                   VkQueryPool                                 vkQueryPool,
                                   Uint32                                      QueryIdx,
                                   HardwareQueueIndex                          QueueFamilyIndex,
                                   void*                                       pData,
                                   Uint32                                      DataSize)
{
    // Pipeline statistics queries write one integer value for each bit that is enabled in the
    // pipelineStatistics when the pool is created, and the statistics values are written in bit
    // order starting from the least significant bit. (17.2)

    std::array<Uint64, 12> Results{};

    auto DataAvailable = GetQueryResults(LogicalDevice, vkQueryPool, QueryIdx, Results);
    if (DataAvailable && pData != nullptr)
    {
        const auto StageMask = LogicalDevice.GetSupportedStagesMask(QueueFamilyIndex);

        auto& QueryData = *reinterpret_cast<QueryDataPipelineStatistics*>(pData);
        VERIFY_EXPR(DataSize == sizeof(QueryData));

        size_t Idx = 0;

        QueryData.InputVertices   = Results[Idx++]; // INPUT_ASSEMBLY_VERTICES_BIT   = 0x00000001
        QueryData.InputPrimitives = Results[Idx++]; // INPUT_ASSEMBLY_PRIMITIVES_BIT = 0x00000002
        QueryData.VSInvocations   = Results[Idx++]; // VERTEX_SHADER_INVOCATIONS_BIT = 0x00000004
        if (StageMask & VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT)
        {
            QueryData.GSInvocations = Results[Idx++]; // GEOMETRY_SHADER_INVOCATIONS_BIT = 0x00000008
            QueryData.GSPrimitives  = Results[Idx++]; // GEOMETRY_SHADER_PRIMITIVES_BIT  = 0x00000010
        }
        QueryData.ClippingInvocations = Results[Idx++]; // CLIPPING_INVOCATIONS_BIT         = 0x00000020
        QueryData.ClippingPrimitives  = Results[Idx++]; // CLIPPING_PRIMITIVES_BIT          = 0x00000040
        QueryData.PSInvocations       = Results[Idx++]; // FRAGMENT_SHADER_INVOCATIONS_BIT  = 0x00000080

        if (StageMask & VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT)
            QueryData.HSInvocations = Results[Idx++]; // TESSELLATION_CONTROL_SHADER_PATCHES_BIT        = 0x00000100

        if (StageMask & VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT)
            QueryData.DSInvocations = Results[Idx++]; // TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT = 0x00000200

        QueryData.CSInvocations = Results[Idx++]; // COMPUTE_SHADER_INVOCATIONS_BIT = 0x00000400

        DataAvailable = DataAvailable && (Results[Idx] != 0);
    }

    return DataAvailable;
}

} // namespace

bool QueryVkImpl::GetData(void* pData, Uint32 DataSize, bool AutoInvalidate)
{
    TQueryBase::CheckQueryDataPtr(pData, DataSize);

    DEV_CHECK_ERR(m_pQueryMgr != nullptr, "Requesting data from query that has not been ended or has been invalidated");
    const auto CmdQueueId          = m_pQueryMgr->GetCommandQueueId();
    const auto CompletedFenceValue = m_pDevice->GetCompletedFenceValue(CmdQueueId);
    bool       DataAvailable       = false;
    if (CompletedFenceValue >= m_QueryEndFenceValue)
    {
        const auto& LogicalDevice = m_pDevice->GetLogicalDevice();
        auto        vkQueryPool   = m_pQueryMgr->GetQueryPool(m_Desc.Type);

        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled below");
        switch (m_Desc.Type)
        {
            case QUERY_TYPE_OCCLUSION:
                DataAvailable = GetOcclusionQueryData(LogicalDevice, vkQueryPool, m_QueryPoolIndex[0], pData, DataSize);
                break;

            case QUERY_TYPE_BINARY_OCCLUSION:
                DataAvailable = GetBinaryOcclusionQueryData(LogicalDevice, vkQueryPool, m_QueryPoolIndex[0], pData, DataSize);
                break;

            case QUERY_TYPE_TIMESTAMP:
                DataAvailable = GetTimestampQueryData(LogicalDevice, vkQueryPool, m_QueryPoolIndex[0], m_pQueryMgr->GetCounterFrequency(), pData, DataSize);
                break;

            case QUERY_TYPE_PIPELINE_STATISTICS:
                DataAvailable = GetStatisticsQueryData(LogicalDevice, vkQueryPool, m_QueryPoolIndex[0], m_pDevice->GetQueueFamilyIndex(CmdQueueId), pData, DataSize);
                break;

            case QUERY_TYPE_DURATION:
                DataAvailable = GetDurationQueryData(LogicalDevice, vkQueryPool, m_QueryPoolIndex, m_pQueryMgr->GetCounterFrequency(), pData, DataSize);
                break;

            default:
                UNEXPECTED("Unexpected query type");
        }
    }

    if (DataAvailable && pData != nullptr && AutoInvalidate)
    {
        Invalidate();
    }

    return DataAvailable;
}

} // namespace Diligent
