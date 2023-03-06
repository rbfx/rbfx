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

#include "QueryManagerD3D12.hpp"

#include <algorithm>

#include "D3D12TypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "CommandContext.hpp"
#include "Align.hpp"
#include "RenderDeviceD3D12Impl.hpp"

namespace Diligent
{

static Uint32 GetQueryDataSize(QUERY_TYPE QueryType)
{
    // clang-format off
    switch (QueryType)
    {
        case QUERY_TYPE_OCCLUSION:
        case QUERY_TYPE_BINARY_OCCLUSION:
        case QUERY_TYPE_TIMESTAMP:
        case QUERY_TYPE_DURATION:
            return sizeof(Uint64);

        case QUERY_TYPE_PIPELINE_STATISTICS:
            return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);

        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are tested");

        default:
            UNEXPECTED("Unexpected query type");
            return 0;
    }
    // clang-format on
}

void QueryManagerD3D12::QueryHeapInfo::Init(ID3D12Device*                pd3d12Device,
                                            const D3D12_QUERY_HEAP_DESC& d3d12HeapDesc,
                                            QUERY_TYPE                   QueryType,
                                            Uint32&                      CurrResolveBufferOffset)
{
    VERIFY_EXPR(!m_pd3d12QueryHeap);

    m_Type       = QueryType;
    m_QueryCount = d3d12HeapDesc.Count;

    auto hr = pd3d12Device->CreateQueryHeap(&d3d12HeapDesc, __uuidof(m_pd3d12QueryHeap), reinterpret_cast<void**>(&m_pd3d12QueryHeap));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create D3D12 query heap");

    // AlignedDestinationBufferOffset must be a multiple of 8 bytes.
    // https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#resolvequerydata
    m_AlignedQueryDataSize    = AlignUp(GetQueryDataSize(QueryType), Uint32{8});
    m_ResolveBufferBaseOffset = CurrResolveBufferOffset;
    CurrResolveBufferOffset += m_AlignedQueryDataSize * m_QueryCount;

    m_AvailableQueries.resize(m_QueryCount);
    for (Uint32 i = 0; i < m_QueryCount; ++i)
        m_AvailableQueries[i] = i;
}

Uint32 QueryManagerD3D12::QueryHeapInfo::Allocate()
{
    Uint32 Index = InvalidIndex;

    std::lock_guard<std::mutex> Lock{m_AvailableQueriesMtx};
    if (!m_AvailableQueries.empty())
    {
        Index = m_AvailableQueries.back();
        m_AvailableQueries.pop_back();
        m_MaxAllocatedQueries = std::max(m_MaxAllocatedQueries, m_QueryCount - static_cast<Uint32>(m_AvailableQueries.size()));
    }

    return Index;
}

void QueryManagerD3D12::QueryHeapInfo::Release(Uint32 Index)
{
    VERIFY(Index < m_QueryCount, "Query index ", Index, " is out of range");

    std::lock_guard<std::mutex> Lock{m_AvailableQueriesMtx};
    VERIFY(std::find(m_AvailableQueries.begin(), m_AvailableQueries.end(), Index) == m_AvailableQueries.end(),
           "Index ", Index, " already present in available queries list");
    m_AvailableQueries.push_back(Index);
}

QueryManagerD3D12::QueryHeapInfo::~QueryHeapInfo()
{
    if (m_AvailableQueries.size() != m_QueryCount)
    {
        auto OutstandingQueries = m_QueryCount - m_AvailableQueries.size();
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

QueryManagerD3D12::QueryManagerD3D12(RenderDeviceD3D12Impl* pDeviceD3D12Impl,
                                     const Uint32           QueryHeapSizes[],
                                     SoftwareQueueIndex     CommandQueueId,
                                     HardwareQueueIndex     HwQueueInd) :
    m_CommandQueueId{CommandQueueId}
{
    const auto& DevInfo      = pDeviceD3D12Impl->GetDeviceInfo();
    auto*       pd3d12Device = pDeviceD3D12Impl->GetD3D12Device();

    Uint32 ResolveBufferOffset = 0;
    for (Uint32 query_type = QUERY_TYPE_UNDEFINED + 1; query_type < QUERY_TYPE_NUM_TYPES; ++query_type)
    {
        const auto QueryType = static_cast<QUERY_TYPE>(query_type);

        // clang-format off
        static_assert(QUERY_TYPE_OCCLUSION          == 1, "Unexpected value of QUERY_TYPE_OCCLUSION. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_BINARY_OCCLUSION   == 2, "Unexpected value of QUERY_TYPE_BINARY_OCCLUSION. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_TIMESTAMP          == 3, "Unexpected value of QUERY_TYPE_TIMESTAMP. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_PIPELINE_STATISTICS== 4, "Unexpected value of QUERY_TYPE_PIPELINE_STATISTICS. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_DURATION           == 5, "Unexpected value of QUERY_TYPE_DURATION. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        static_assert(QUERY_TYPE_NUM_TYPES          == 6, "Unexpected value of QUERY_TYPE_NUM_TYPES. EngineD3D12CreateInfo::QueryPoolSizes must be updated");
        // clang-format on

        // Time and duration queries are supported in all queues
        if (QueryType == QUERY_TYPE_TIMESTAMP || QueryType == QUERY_TYPE_DURATION)
        {
            if (HwQueueInd == D3D12HWQueueIndex_Copy && !DevInfo.Features.TransferQueueTimestampQueries)
                continue; // Not supported in transfer queue
        }
        // Other queries are only supported in graphics queue
        else if (HwQueueInd != D3D12HWQueueIndex_Graphics)
            continue;

        D3D12_QUERY_HEAP_DESC d3d12HeapDesc{};
        d3d12HeapDesc.Type  = QueryTypeToD3D12QueryHeapType(QueryType, HwQueueInd);
        d3d12HeapDesc.Count = QueryHeapSizes[QueryType];
        if (QueryType == QUERY_TYPE_DURATION)
            d3d12HeapDesc.Count *= 2;

        auto& HeapInfo = m_Heaps[QueryType];
        HeapInfo.Init(pd3d12Device, d3d12HeapDesc, QueryType, ResolveBufferOffset);
        VERIFY_EXPR(!HeapInfo.IsNull() && HeapInfo.GetType() == QueryType && HeapInfo.GetQueryCount() == d3d12HeapDesc.Count);
    }

    if (ResolveBufferOffset > 0)
    {
        D3D12_RESOURCE_DESC D3D12BuffDesc{};
        D3D12BuffDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        D3D12BuffDesc.Alignment          = 0;
        D3D12BuffDesc.Width              = ResolveBufferOffset;
        D3D12BuffDesc.Height             = 1;
        D3D12BuffDesc.DepthOrArraySize   = 1;
        D3D12BuffDesc.MipLevels          = 1;
        D3D12BuffDesc.Format             = DXGI_FORMAT_UNKNOWN;
        D3D12BuffDesc.SampleDesc.Count   = 1;
        D3D12BuffDesc.SampleDesc.Quality = 0;
        // Layout must be D3D12_TEXTURE_LAYOUT_ROW_MAJOR, as buffer memory layouts are
        // understood by applications and row-major texture data is commonly marshaled through buffers.
        D3D12BuffDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12BuffDesc.Flags  = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES HeapProps{};
        HeapProps.Type                 = D3D12_HEAP_TYPE_READBACK;
        HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProps.CreationNodeMask     = 1;
        HeapProps.VisibleNodeMask      = 1;

        // The destination buffer of a query resolve operation must be in the D3D12_RESOURCE_USAGE_COPY_DEST state.
        // ResolveQueryData works with all heap types (default, upload, readback).
        // https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#resolvequerydata
        auto hr = pd3d12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                        &D3D12BuffDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                        __uuidof(m_pd3d12ResolveBuffer),
                                                        reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12ResolveBuffer)));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create D3D12 resolve buffer");
    }
}

QueryManagerD3D12::~QueryManagerD3D12()
{
    std::stringstream QueryUsageSS;
    QueryUsageSS << "D3D12 query manager peak usage:";

    for (Uint32 query_type = QUERY_TYPE_UNDEFINED + 1; query_type < QUERY_TYPE_NUM_TYPES; ++query_type)
    {
        const auto  QueryType = static_cast<QUERY_TYPE>(query_type);
        const auto& HeapInfo  = m_Heaps[QueryType];
        if (HeapInfo.IsNull())
            continue;

        QueryUsageSS << std::endl
                     << std::setw(30) << std::left << GetQueryTypeString(QueryType) << ": "
                     << std::setw(4) << std::right << HeapInfo.GetMaxAllocatedQueries()
                     << '/' << std::setw(4) << HeapInfo.GetQueryCount();
    }
    LOG_INFO_MESSAGE(QueryUsageSS.str());
}

Uint32 QueryManagerD3D12::AllocateQuery(QUERY_TYPE Type)
{
    return m_Heaps[Type].Allocate();
}

void QueryManagerD3D12::ReleaseQuery(QUERY_TYPE Type, Uint32 Index)
{
    m_Heaps[Type].Release(Index);
}

void QueryManagerD3D12::BeginQuery(CommandContext& Ctx, QUERY_TYPE Type, Uint32 Index) const
{
    const auto  d3d12QueryType = QueryTypeToD3D12QueryType(Type);
    const auto& HeapInfo       = m_Heaps[Type];
    VERIFY_EXPR(HeapInfo.GetType() == Type);
    VERIFY(Index < HeapInfo.GetQueryCount(), "Query index ", Index, " is out of range");

    Ctx.BeginQuery(HeapInfo.GetD3D12QueryHeap(), d3d12QueryType, Index);
}

void QueryManagerD3D12::EndQuery(CommandContext& Ctx, QUERY_TYPE Type, Uint32 Index) const
{
    const auto  d3d12QueryType = QueryTypeToD3D12QueryType(Type);
    const auto& HeapInfo       = m_Heaps[Type];
    VERIFY_EXPR(HeapInfo.GetType() == Type);

    VERIFY(Index < HeapInfo.GetQueryCount(), "Query index ", Index, " is out of range");
    Ctx.EndQuery(HeapInfo.GetD3D12QueryHeap(), d3d12QueryType, Index);

    // https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#resolvequerydata
    Ctx.ResolveQueryData(HeapInfo.GetD3D12QueryHeap(), d3d12QueryType, Index, 1, m_pd3d12ResolveBuffer, HeapInfo.GetResolveBufferOffset(Index));
}

void QueryManagerD3D12::ReadQueryData(QUERY_TYPE Type, Uint32 Index, void* pDataPtr, Uint32 DataSize) const
{
    const auto& HeapInfo = m_Heaps[Type];
    VERIFY_EXPR(HeapInfo.GetType() == Type);
    const auto QueryDataSize = GetQueryDataSize(Type);
    VERIFY_EXPR(QueryDataSize == DataSize);
    const auto  Offset = HeapInfo.GetResolveBufferOffset(Index);
    D3D12_RANGE ReadRange;
    ReadRange.Begin = Offset;
    ReadRange.End   = SIZE_T{Offset} + SIZE_T{QueryDataSize};

    void* pBufferData = nullptr;
    // The pointer returned by Map is never offset by any values in pReadRange.
    m_pd3d12ResolveBuffer->Map(0, &ReadRange, &pBufferData);
    memcpy(pDataPtr, reinterpret_cast<const Uint8*>(pBufferData) + Offset, QueryDataSize);
    m_pd3d12ResolveBuffer->Unmap(0, nullptr);
}

} // namespace Diligent
