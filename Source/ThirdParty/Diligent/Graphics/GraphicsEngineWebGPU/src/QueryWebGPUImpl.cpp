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

#include "QueryWebGPUImpl.hpp"

namespace Diligent
{

QueryWebGPUImpl::QueryWebGPUImpl(IReferenceCounters*     pRefCounters,
                                 RenderDeviceWebGPUImpl* pDevice,
                                 const QueryDesc&        Desc) :
    // clang-format off
    TQueryBase
    {
        pRefCounters,
        pDevice,
        Desc
    }
// clang-format on
{
}

QueryWebGPUImpl::~QueryWebGPUImpl()
{
    DiscardQueries();
}

bool QueryWebGPUImpl::AllocateQueries()
{
    DiscardQueries();

    VERIFY_EXPR(m_pQueryMgr == nullptr);
    VERIFY_EXPR(m_pContext != nullptr);
    m_pQueryMgr = &m_pContext->GetQueryManager();
    VERIFY_EXPR(m_pQueryMgr != nullptr);
    for (Uint32 Index = 0; Index < (m_Desc.Type == QUERY_TYPE_DURATION ? Uint32{2} : Uint32{1}); ++Index)
    {
        auto& QuerySetIdx = m_QuerySetIndices[Index];
        VERIFY_EXPR(QuerySetIdx == QueryManagerWebGPU::InvalidIndex);
        QuerySetIdx = m_pQueryMgr->AllocateQuery(m_Desc.Type);

        if (QuerySetIdx == QueryManagerWebGPU::InvalidIndex)
        {
            LOG_ERROR_MESSAGE("Failed to allocate WebGPU query for type ", GetQueryTypeString(m_Desc.Type),
                              ". Increase the query pool size in EngineWebGPUCreateInfo.");
            DiscardQueries();
            return false;
        }
    }
    return true;
}

void QueryWebGPUImpl::DiscardQueries()
{
    for (auto& QuerySetIdx : m_QuerySetIndices)
    {
        if (QuerySetIdx != QueryManagerWebGPU::InvalidIndex)
        {
            VERIFY_EXPR(m_pQueryMgr != nullptr);
            m_pQueryMgr->DiscardQuery(m_Desc.Type, QuerySetIdx);
            QuerySetIdx = QueryManagerWebGPU::InvalidIndex;
        }
    }
    m_pQueryMgr = nullptr;
}

bool QueryWebGPUImpl::GetData(void* pData, Uint32 DataSize, bool AutoInvalidate)
{
    TQueryBase::CheckQueryDataPtr(pData, DataSize);
    DEV_CHECK_ERR(m_pQueryMgr != nullptr, "Requesting data from query that has not been ended or has been invalidated");

    VERIFY_EXPR(m_pDevice->GetNumImmediateContexts() == 1);
    DeviceContextWebGPUImpl* pContext = m_pDevice->GetImmediateContext(0);
    if (pContext->GetCompletedFenceValue() >= m_QueryEndFenceValue)
    {
        switch (m_Desc.Type)
        {
            case QUERY_TYPE_TIMESTAMP:
            {
                const auto Timestamp = m_pQueryMgr->GetQueryResult(m_Desc.Type, m_QuerySetIndices[0]);
                if (pData != nullptr)
                {
                    auto& QueryData     = *reinterpret_cast<QueryDataTimestamp*>(pData);
                    QueryData.Counter   = Timestamp;
                    QueryData.Frequency = 1000000000u; // Nanoseconds to seconds
                }
                break;
            }
            case QUERY_TYPE_DURATION:
            {
                const auto T0 = m_pQueryMgr->GetQueryResult(m_Desc.Type, m_QuerySetIndices[0]);
                const auto T1 = m_pQueryMgr->GetQueryResult(m_Desc.Type, m_QuerySetIndices[1]);
                if (pData != nullptr)
                {
                    auto& QueryData     = *reinterpret_cast<QueryDataDuration*>(pData);
                    QueryData.Duration  = T1 - T0;
                    QueryData.Frequency = 1000000000u; // Nanoseconds to seconds
                }
                break;
            }
            case QUERY_TYPE_OCCLUSION:
            {
                const auto NumSamples = m_pQueryMgr->GetQueryResult(m_Desc.Type, m_QuerySetIndices[0]);
                if (pData != nullptr)
                {
                    auto& QueryData      = *reinterpret_cast<QueryDataOcclusion*>(pData);
                    QueryData.NumSamples = NumSamples;
                }
                break;
            }
            default:
                UNEXPECTED("Unexpected query type");
        }

        if (pData != nullptr && AutoInvalidate)
            Invalidate();

        return true;
    }
    else
    {
        return false;
    }
}

void QueryWebGPUImpl::Invalidate()
{
    DiscardQueries();
    TQueryBase::Invalidate();
}

Uint32 QueryWebGPUImpl::GetIndexInsideQuerySet(Uint32 QueryId) const
{
    VERIFY_EXPR(QueryId == 0 || m_Desc.Type == QUERY_TYPE_DURATION && QueryId == 1);
    return m_QuerySetIndices[QueryId];
}

bool QueryWebGPUImpl::OnBeginQuery(DeviceContextWebGPUImpl* pContext)
{
    TQueryBase::OnBeginQuery(pContext);
    return AllocateQueries();
}

bool QueryWebGPUImpl::OnEndQuery(DeviceContextWebGPUImpl* pContext)
{
    TQueryBase::OnEndQuery(pContext);

    if (m_Desc.Type == QUERY_TYPE_TIMESTAMP)
    {
        if (!AllocateQueries())
            return false;
    }

    if (m_QuerySetIndices[0] == QueryManagerWebGPU::InvalidIndex || (m_Desc.Type == QUERY_TYPE_DURATION && m_QuerySetIndices[1] == QueryManagerWebGPU::InvalidIndex))
    {
        LOG_ERROR_MESSAGE("Query '", m_Desc.Name, "' is invalid: WebGPU query allocation failed");
        return false;
    }

    m_QueryEndFenceValue = pContext->GetNextFenceValue();
    return true;
}

} // namespace Diligent
