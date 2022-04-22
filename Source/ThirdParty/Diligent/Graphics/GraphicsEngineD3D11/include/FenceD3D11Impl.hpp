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
/// Declaration of Diligent::FenceD3D11Impl class

#include <deque>
#include <algorithm>

#include <atlbase.h>

#include "EngineD3D11ImplTraits.hpp"
#include "FenceBase.hpp"

namespace Diligent
{

/// Fence implementation in Direct3D11 backend.
class FenceD3D11Impl final : public FenceBase<EngineD3D11ImplTraits>
{
public:
    using TFenceBase = FenceBase<EngineD3D11ImplTraits>;

    FenceD3D11Impl(IReferenceCounters*    pRefCounters,
                   RenderDeviceD3D11Impl* pDevice,
                   const FenceDesc&       Desc);
    ~FenceD3D11Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_FenceD3D11, TFenceBase)

    /// Implementation of IFence::GetCompletedValue() in Direct3D11 backend.
    virtual Uint64 DILIGENT_CALL_TYPE GetCompletedValue() override final;

    /// Implementation of IFence::Signal() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE Signal(Uint64 Value) override final;

    /// Implementation of IFence::Wait() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE Wait(Uint64 Value) override final;

    void AddPendingQuery(CComPtr<ID3D11DeviceContext1> pCtx, CComPtr<ID3D11Query> pQuery, Uint64 Value)
    {
        m_PendingQueries.emplace_back(std::move(pCtx), std::move(pQuery), Value);
        DvpSignal(Value);

        m_MaxPendingQueries = std::max(m_MaxPendingQueries, m_PendingQueries.size());
    }

    void Wait(Uint64 Value, bool FlushCommands);

private:
    struct PendingFenceData
    {
        CComPtr<ID3D11DeviceContext1> pd3d11Ctx;
        CComPtr<ID3D11Query>          pd3d11Query;
        const Uint64                  Value;

        PendingFenceData(CComPtr<ID3D11DeviceContext1> pCtx, CComPtr<ID3D11Query> pQuery, Uint64 _Value) :
            // clang-format off
            pd3d11Ctx  {std::move(pCtx)},
            pd3d11Query{std::move(pQuery)},
            Value      {_Value}
        // clang-format on
        {}
    };
    std::deque<PendingFenceData> m_PendingQueries;

    size_t m_MaxPendingQueries = 0;
};

} // namespace Diligent
