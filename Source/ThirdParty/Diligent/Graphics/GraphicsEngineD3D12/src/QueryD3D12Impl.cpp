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

#include "QueryD3D12Impl.hpp"

#include "WinHPreface.h"
#include <atlbase.h>
#include "WinHPostface.h"

#include "RenderDeviceD3D12Impl.hpp"
#include "GraphicsAccessories.hpp"
#include "DeviceContextD3D12Impl.hpp"

namespace Diligent
{

QueryD3D12Impl::QueryD3D12Impl(IReferenceCounters*    pRefCounters,
                               RenderDeviceD3D12Impl* pDevice,
                               const QueryDesc&       Desc) :
    TQueryBase{pRefCounters, pDevice, Desc}
{
}


bool QueryD3D12Impl::AllocateQueries()
{
    DiscardQueries();
    VERIFY_EXPR(m_pContext != nullptr);
    m_pQueryMgr = &m_pContext->GetQueryManager();
    for (Uint32 i = 0; i < (m_Desc.Type == QUERY_TYPE_DURATION ? Uint32{2} : Uint32{1}); ++i)
    {
        m_QueryHeapIndex[i] = m_pQueryMgr->AllocateQuery(m_Desc.Type);
        if (m_QueryHeapIndex[i] == QueryManagerD3D12::InvalidIndex)
        {
            LOG_ERROR_MESSAGE("Failed to allocate D3D12 query for type ", GetQueryTypeString(m_Desc.Type),
                              ". Increase the query pool size in EngineD3D12CreateInfo.");
            DiscardQueries();
            return false;
        }
    }
    return true;
}

QueryD3D12Impl::~QueryD3D12Impl()
{
    DiscardQueries();
}

void QueryD3D12Impl::DiscardQueries()
{
    for (auto& HeapIdx : m_QueryHeapIndex)
    {
        if (HeapIdx != QueryManagerD3D12::InvalidIndex)
        {
            VERIFY_EXPR(m_pQueryMgr != nullptr);
            m_pQueryMgr->ReleaseQuery(m_Desc.Type, HeapIdx);
            HeapIdx = QueryManagerD3D12::InvalidIndex;
        }
    }
    m_pQueryMgr          = nullptr;
    m_QueryEndFenceValue = ~Uint64{0};
}

void QueryD3D12Impl::Invalidate()
{
    DiscardQueries();
    TQueryBase::Invalidate();
}

bool QueryD3D12Impl::OnBeginQuery(DeviceContextD3D12Impl* pContext)
{
    TQueryBase::OnBeginQuery(pContext);

    return AllocateQueries();
}

bool QueryD3D12Impl::OnEndQuery(DeviceContextD3D12Impl* pContext)
{
    TQueryBase::OnEndQuery(pContext);

    if (m_Desc.Type == QUERY_TYPE_TIMESTAMP)
    {
        if (!AllocateQueries())
            return false;
    }

    if (m_QueryHeapIndex[0] == QueryManagerD3D12::InvalidIndex || (m_Desc.Type == QUERY_TYPE_DURATION && m_QueryHeapIndex[1] == QueryManagerD3D12::InvalidIndex))
    {
        LOG_ERROR_MESSAGE("Query '", m_Desc.Name, "' is invalid: D3D12 query allocation failed");
        return false;
    }

    VERIFY_EXPR(m_pQueryMgr != nullptr);
    auto CmdQueueId      = m_pQueryMgr->GetCommandQueueId();
    m_QueryEndFenceValue = m_pDevice->GetNextFenceValue(CmdQueueId);

    return true;
}

bool QueryD3D12Impl::GetData(void* pData, Uint32 DataSize, bool AutoInvalidate)
{
    TQueryBase::CheckQueryDataPtr(pData, DataSize);

    VERIFY_EXPR(m_pQueryMgr != nullptr);
    auto CmdQueueId          = m_pQueryMgr->GetCommandQueueId();
    auto CompletedFenceValue = m_pDevice->GetCompletedFenceValue(CmdQueueId);
    if (CompletedFenceValue >= m_QueryEndFenceValue)
    {
        auto GetTimestampFrequency = [this](SoftwareQueueIndex CmdQueueId) //
        {
            const auto& CmdQueue    = m_pDevice->GetCommandQueue(CmdQueueId);
            auto*       pd3d12Queue = const_cast<ICommandQueueD3D12&>(CmdQueue).GetD3D12CommandQueue();

            // https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#timestamp-frequency
            UINT64 TimestampFrequency = 0;
            pd3d12Queue->GetTimestampFrequency(&TimestampFrequency);
            return TimestampFrequency;
        };

        switch (m_Desc.Type)
        {
            case QUERY_TYPE_OCCLUSION:
            {
                UINT64 NumSamples;
                m_pQueryMgr->ReadQueryData(m_Desc.Type, m_QueryHeapIndex[0], &NumSamples, sizeof(NumSamples));
                if (pData != nullptr)
                {
                    auto& QueryData      = *reinterpret_cast<QueryDataOcclusion*>(pData);
                    QueryData.NumSamples = NumSamples;
                }
            }
            break;

            case QUERY_TYPE_BINARY_OCCLUSION:
            {
                UINT64 AnySamplePassed;
                m_pQueryMgr->ReadQueryData(m_Desc.Type, m_QueryHeapIndex[0], &AnySamplePassed, sizeof(AnySamplePassed));
                if (pData != nullptr)
                {
                    auto& QueryData = *reinterpret_cast<QueryDataBinaryOcclusion*>(pData);
                    // Binary occlusion queries write 64-bits per query. The least significant bit is either 0 or 1. The rest of the bits are 0.
                    // https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#resolvequerydata
                    QueryData.AnySamplePassed = AnySamplePassed != 0;
                }
            }
            break;

            case QUERY_TYPE_TIMESTAMP:
            {
                UINT64 Counter;
                m_pQueryMgr->ReadQueryData(m_Desc.Type, m_QueryHeapIndex[0], &Counter, sizeof(Counter));
                if (pData != nullptr)
                {
                    auto& QueryData     = *reinterpret_cast<QueryDataTimestamp*>(pData);
                    QueryData.Counter   = Counter;
                    QueryData.Frequency = GetTimestampFrequency(CmdQueueId);
                }
            }
            break;

            case QUERY_TYPE_PIPELINE_STATISTICS:
            {
                D3D12_QUERY_DATA_PIPELINE_STATISTICS d3d12QueryData;
                m_pQueryMgr->ReadQueryData(m_Desc.Type, m_QueryHeapIndex[0], &d3d12QueryData, sizeof(d3d12QueryData));
                if (pData != nullptr)
                {
                    auto& QueryData = *reinterpret_cast<QueryDataPipelineStatistics*>(pData);

                    QueryData.InputVertices       = d3d12QueryData.IAVertices;
                    QueryData.InputPrimitives     = d3d12QueryData.IAPrimitives;
                    QueryData.GSPrimitives        = d3d12QueryData.GSPrimitives;
                    QueryData.ClippingInvocations = d3d12QueryData.CInvocations;
                    QueryData.ClippingPrimitives  = d3d12QueryData.CPrimitives;
                    QueryData.VSInvocations       = d3d12QueryData.VSInvocations;
                    QueryData.GSInvocations       = d3d12QueryData.GSInvocations;
                    QueryData.PSInvocations       = d3d12QueryData.PSInvocations;
                    QueryData.HSInvocations       = d3d12QueryData.HSInvocations;
                    QueryData.DSInvocations       = d3d12QueryData.DSInvocations;
                    QueryData.CSInvocations       = d3d12QueryData.CSInvocations;
                }
            }
            break;

            case QUERY_TYPE_DURATION:
            {
                UINT64 StartCounter, EndCounter;
                m_pQueryMgr->ReadQueryData(m_Desc.Type, m_QueryHeapIndex[0], &StartCounter, sizeof(StartCounter));
                m_pQueryMgr->ReadQueryData(m_Desc.Type, m_QueryHeapIndex[1], &EndCounter, sizeof(EndCounter));
                if (pData != nullptr)
                {
                    auto& QueryData     = *reinterpret_cast<QueryDataDuration*>(pData);
                    QueryData.Duration  = EndCounter - StartCounter;
                    QueryData.Frequency = GetTimestampFrequency(CmdQueueId);
                }
            }
            break;

            default:
                UNEXPECTED("Unexpected query type");
        }

        if (pData != nullptr && AutoInvalidate)
        {
            Invalidate();
        }

        return true;
    }
    else
    {
        return false;
    }
}

ID3D12QueryHeap* QueryD3D12Impl::GetD3D12QueryHeap()
{
    VERIFY_EXPR(m_pQueryMgr != nullptr);
    return m_pQueryMgr->GetQueryHeap(m_Desc.Type);
}

} // namespace Diligent
