/*
 *  Copyright 2023-2024 Diligent Graphics LLC
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

#include "QueryManagerWebGPU.hpp"
#include "WebGPUTypeConversions.hpp"

namespace Diligent
{

QueryManagerWebGPU::QueryManagerWebGPU(RenderDeviceWebGPUImpl* pRenderDeviceWebGPU, const Uint32 QueryHeapSizes[])
{
    const RenderDeviceInfo& DevInfo = pRenderDeviceWebGPU->GetDeviceInfo();

    // clang-format off
    static_assert(QUERY_TYPE_OCCLUSION           == 1, "Unexpected value of QUERY_TYPE_OCCLUSION. EngineWebGPUCreateInfo::QueryPoolSizes must be updated");
    static_assert(QUERY_TYPE_BINARY_OCCLUSION    == 2, "Unexpected value of QUERY_TYPE_BINARY_OCCLUSION. EngineWebGPUCreateInfo::QueryPoolSizes must be updated");
    static_assert(QUERY_TYPE_TIMESTAMP           == 3, "Unexpected value of QUERY_TYPE_TIMESTAMP. EngineWebGPUCreateInfo::QueryPoolSizes must be updated");
    static_assert(QUERY_TYPE_PIPELINE_STATISTICS == 4, "Unexpected value of QUERY_TYPE_PIPELINE_STATISTICS. EngineWebGPUCreateInfo::QueryPoolSizes must be updated");
    static_assert(QUERY_TYPE_DURATION            == 5, "Unexpected value of QUERY_TYPE_DURATION. EngineWebGPUCreateInfo::QueryPoolSizes must be updated");
    static_assert(QUERY_TYPE_NUM_TYPES           == 6, "Unexpected value of QUERY_TYPE_NUM_TYPES. EngineWebGPUCreateInfo::QueryPoolSizes must be updated");
    // clang-format on

    for (Uint32 QueryTypeIdx = QUERY_TYPE_UNDEFINED + 1; QueryTypeIdx < QUERY_TYPE_NUM_TYPES; ++QueryTypeIdx)
    {
        const QUERY_TYPE QueryType = static_cast<QUERY_TYPE>(QueryTypeIdx);

        // clang-format off
        if ((QueryType == QUERY_TYPE_OCCLUSION           && !DevInfo.Features.OcclusionQueries)           ||
            (QueryType == QUERY_TYPE_BINARY_OCCLUSION    && !DevInfo.Features.BinaryOcclusionQueries)     ||
            (QueryType == QUERY_TYPE_TIMESTAMP           && !DevInfo.Features.TimestampQueries)           ||
            (QueryType == QUERY_TYPE_PIPELINE_STATISTICS && !DevInfo.Features.PipelineStatisticsQueries)  ||
            (QueryType == QUERY_TYPE_DURATION            && !DevInfo.Features.DurationQueries))
            continue;
        // clang-format on

        auto& pQuerySetObject = m_QuerySets[QueryType];
        pQuerySetObject       = RefCntAutoPtr<QueryManagerWebGPU::QuerySetObject>{MakeNewRCObj<QueryManagerWebGPU::QuerySetObject>()(pRenderDeviceWebGPU, QueryHeapSizes[QueryType], QueryType)};
        VERIFY_EXPR(pQuerySetObject->GetType() == QueryType);
    }
}

QueryManagerWebGPU::~QueryManagerWebGPU()
{
    std::stringstream QueryUsageSS;
    QueryUsageSS << "WebGPU query manager peak usage:";
    for (Uint32 QueryType = QUERY_TYPE_UNDEFINED + 1; QueryType < QUERY_TYPE_NUM_TYPES; ++QueryType)
    {
        auto& pQuerySetObject = m_QuerySets[QueryType];
        if (!pQuerySetObject)
            continue;

        QueryUsageSS << std::endl
                     << std::setw(30) << std::left << GetQueryTypeString(static_cast<QUERY_TYPE>(QueryType)) << ": "
                     << std::setw(4) << std::right << pQuerySetObject->GetMaxAllocatedQueries()
                     << '/' << std::setw(4) << pQuerySetObject->GetQueryCount();
    }
    LOG_INFO_MESSAGE(QueryUsageSS.str());
}

Uint32 QueryManagerWebGPU::AllocateQuery(QUERY_TYPE Type)
{
    return m_QuerySets[Type]->Allocate();
}

void QueryManagerWebGPU::DiscardQuery(QUERY_TYPE Type, Uint32 Index)
{
    return m_QuerySets[Type]->Discard(Index);
}

WGPUQuerySet QueryManagerWebGPU::GetQuerySet(QUERY_TYPE Type) const
{
    return m_QuerySets[Type]->GetWebGPUQuerySet();
}

Uint64 QueryManagerWebGPU::GetQueryResult(QUERY_TYPE Type, Uint32 Index) const
{
    return m_QuerySets[Type]->GetQueryResult(Index);
}

void QueryManagerWebGPU::ResolveQuerySet(RenderDeviceWebGPUImpl* pDevice, DeviceContextWebGPUImpl* pDeviceContext)
{
    for (Uint32 QueryType = QUERY_TYPE_UNDEFINED + 1; QueryType < QUERY_TYPE_NUM_TYPES; ++QueryType)
    {
        auto& pQuerySetObject = m_QuerySets[QueryType];
        if (pQuerySetObject)
            pQuerySetObject->ResolveQueries(pDevice, pDeviceContext);
    }
}

QueryManagerWebGPU::QuerySetObject::QuerySetObject(IReferenceCounters*     pRefCounters,
                                                   RenderDeviceWebGPUImpl* pDevice,
                                                   Uint32                  HeapSize,
                                                   QUERY_TYPE              QueryType) :
    ObjectBase<IDeviceObject>{pRefCounters},
    WebGPUResourceBase{*this, 16}
{
    String QuerySetName = String{"QueryManagerWebGPU: Query set ["} + GetQueryTypeString(QueryType) + "]";

    WGPUQuerySetDescriptor wgpuQuerySetDesc{};
    wgpuQuerySetDesc.type  = QueryTypeToWGPUQueryType(QueryType);
    wgpuQuerySetDesc.count = HeapSize;
    wgpuQuerySetDesc.label = QuerySetName.c_str();

    if (QueryType == QUERY_TYPE_DURATION)
        wgpuQuerySetDesc.count *= 2;

    m_Type       = QueryType;
    m_QueryCount = wgpuQuerySetDesc.count;
    m_wgpuQuerySet.Reset(wgpuDeviceCreateQuerySet(pDevice->GetWebGPUDevice(), &wgpuQuerySetDesc));
    if (!m_wgpuQuerySet)
        LOG_ERROR_AND_THROW("Failed to create '", wgpuQuerySetDesc.label, "'");

    String QueryResolveBufferName = String{"QueryManagerWebGPU: Query resolve buffer ["} + GetQueryTypeString(QueryType) + "]";

    WGPUBufferDescriptor wgpuResolveBufferDesc{};
    wgpuResolveBufferDesc.usage = WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc;
    wgpuResolveBufferDesc.size  = static_cast<Uint64>(m_QueryCount) * sizeof(Uint64);
    wgpuResolveBufferDesc.label = QueryResolveBufferName.c_str();
    m_wgpuResolveBuffer.Reset(wgpuDeviceCreateBuffer(pDevice->GetWebGPUDevice(), &wgpuResolveBufferDesc));
    if (!m_wgpuResolveBuffer)
        LOG_ERROR_AND_THROW("Failed to create resolve buffer for '", wgpuQuerySetDesc.label, "'");

    m_AvailableQueries.resize(m_QueryCount);
    for (Uint32 QueryIdx = 0; QueryIdx < m_QueryCount; ++QueryIdx)
        m_AvailableQueries[QueryIdx] = QueryIdx;
    m_MappedData.resize(static_cast<size_t>(wgpuResolveBufferDesc.size));

    String QueryObjectName = String{"QueryManagerWebGPU: QuerySetObject["} + GetQueryTypeString(m_Type) + "]";

    auto  StrSize  = QueryObjectName.size() + 1;
    auto* NameCopy = ALLOCATE(GetStringAllocator(), "Object name copy", char, StrSize);
    memcpy(NameCopy, QueryObjectName.c_str(), StrSize);
    m_Desc.Name = NameCopy;
}

QueryManagerWebGPU::QuerySetObject::~QuerySetObject()
{
    FREE(GetStringAllocator(), const_cast<Char*>(m_Desc.Name));

    if (m_AvailableQueries.size() != m_QueryCount)
    {
        const auto OutstandingQueries = m_QueryCount - m_AvailableQueries.size();
        if (OutstandingQueries == 1)
        {
            LOG_ERROR_MESSAGE("One query of type ", GetQueryTypeString(m_Type),
                              " has not been returned to the query manager");
        }
        else
        {
            LOG_ERROR_MESSAGE(OutstandingQueries, " queries of type ", GetQueryTypeString(m_Type),
                              " have not been returned to the query manager");
        }
    }
}

Uint32 QueryManagerWebGPU::QuerySetObject::Allocate()
{
    Uint32 Index = InvalidIndex;

    if (!m_AvailableQueries.empty())
    {
        Index = m_AvailableQueries.back();
        m_AvailableQueries.pop_back();
        m_MaxAllocatedQueries = std::max(m_MaxAllocatedQueries, m_QueryCount - static_cast<Uint32>(m_AvailableQueries.size()));
    }
    return Index;
}

void QueryManagerWebGPU::QuerySetObject::Discard(Uint32 Index)
{
    VERIFY(Index < m_QueryCount, "Query index ", Index, " is out of range");
    VERIFY(std::find(m_AvailableQueries.begin(), m_AvailableQueries.end(), Index) == m_AvailableQueries.end(),
           "Index ", Index, " already present in available queries list");
    m_AvailableQueries.push_back(Index);
}

QUERY_TYPE QueryManagerWebGPU::QuerySetObject::GetType() const
{
    return m_Type;
}

Uint32 QueryManagerWebGPU::QuerySetObject::GetQueryCount() const
{
    return m_QueryCount;
}

Uint64 QueryManagerWebGPU::QuerySetObject::GetQueryResult(Uint32 Index) const
{
    return reinterpret_cast<const Uint64*>(m_MappedData.data())[Index];
}

WGPUQuerySet QueryManagerWebGPU::QuerySetObject::GetWebGPUQuerySet() const
{
    return m_wgpuQuerySet.Get();
}

Uint32 QueryManagerWebGPU::QuerySetObject::GetMaxAllocatedQueries() const
{
    return m_MaxAllocatedQueries;
}

void QueryManagerWebGPU::QuerySetObject::ResolveQueries(RenderDeviceWebGPUImpl* pDevice, DeviceContextWebGPUImpl* pDeviceContext)
{
    if (m_AvailableQueries.size() != m_QueryCount)
    {
        WebGPUResourceBase::StagingBufferInfo* pDstStagingBuffer = GetStagingBuffer(pDevice->GetWebGPUDevice(), CPU_ACCESS_READ);
        wgpuCommandEncoderResolveQuerySet(pDeviceContext->GetCommandEncoder(), m_wgpuQuerySet, 0, m_QueryCount, m_wgpuResolveBuffer, 0);
        wgpuCommandEncoderCopyBufferToBuffer(pDeviceContext->GetCommandEncoder(), m_wgpuResolveBuffer, 0, pDstStagingBuffer->wgpuBuffer, 0, m_QueryCount * sizeof(Uint64));
        pDeviceContext->m_PendingStagingReads.emplace(pDstStagingBuffer, RefCntAutoPtr<IObject>{this});
    }
}

const DeviceObjectAttribs& QueryManagerWebGPU::QuerySetObject::GetDesc() const
{
    return m_Desc;
}

Int32 QueryManagerWebGPU::QuerySetObject::GetUniqueID() const
{
    UNEXPECTED("Undefined behavior. This method should not be called.");
    return 0;
}

void QueryManagerWebGPU::QuerySetObject::SetUserData(Diligent::IObject* pUserData)
{
    UNEXPECTED("Undefined behavior. This method should not be called.");
}

IObject* QueryManagerWebGPU::QuerySetObject::GetUserData() const
{
    UNEXPECTED("Undefined behavior. This method should not be called.");
    return nullptr;
}

} // namespace Diligent
