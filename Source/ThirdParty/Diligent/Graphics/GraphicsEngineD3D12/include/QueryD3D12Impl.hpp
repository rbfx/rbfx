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

/// \file
/// Declaration of Diligent::QueryD3D12Impl class

#include <array>

#include "EngineD3D12ImplTraits.hpp"
#include "QueryBase.hpp"
#include "QueryManagerD3D12.hpp"

namespace Diligent
{

// https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html#queries

/// Query implementation in Direct3D12 backend.
class QueryD3D12Impl final : public QueryBase<EngineD3D12ImplTraits>
{
public:
    using TQueryBase = QueryBase<EngineD3D12ImplTraits>;

    QueryD3D12Impl(IReferenceCounters*    pRefCounters,
                   RenderDeviceD3D12Impl* pDevice,
                   const QueryDesc&       Desc);
    ~QueryD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_QueryD3D12, TQueryBase)

    /// Implementation of IQuery::GetData().
    virtual bool DILIGENT_CALL_TYPE GetData(void* pData, Uint32 DataSize, bool AutoInvalidate) override final;

    /// Implementation of IQuery::Invalidate().
    virtual void DILIGENT_CALL_TYPE Invalidate() override final;

    /// Implementation of IQueryD3D12::GetD3D12QueryHeap().
    virtual ID3D12QueryHeap* DILIGENT_CALL_TYPE GetD3D12QueryHeap() override final;

    /// Implementation of IQueryD3D12::GetQueryHeapIndex().
    virtual Uint32 DILIGENT_CALL_TYPE GetQueryHeapIndex(Uint32 QueryId) const override final
    {
        VERIFY_EXPR(QueryId == 0 || m_Desc.Type == QUERY_TYPE_DURATION && QueryId == 1);
        return m_QueryHeapIndex[QueryId];
    }

    bool OnBeginQuery(DeviceContextD3D12Impl* pContext);
    bool OnEndQuery(DeviceContextD3D12Impl* pContext);

private:
    bool AllocateQueries();
    void DiscardQueries();

    // Begin/end query indices
    std::array<Uint32, 2> m_QueryHeapIndex = {QueryManagerD3D12::InvalidIndex, QueryManagerD3D12::InvalidIndex};

    Uint64 m_QueryEndFenceValue = ~Uint64{0};

    QueryManagerD3D12* m_pQueryMgr = nullptr;
};

} // namespace Diligent
