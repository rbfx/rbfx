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

#include "FenceD3D11Impl.hpp"
#include "RenderDeviceD3D11Impl.hpp"
#include "EngineMemory.h"

namespace Diligent
{

FenceD3D11Impl::FenceD3D11Impl(IReferenceCounters*    pRefCounters,
                               RenderDeviceD3D11Impl* pDevice,
                               const FenceDesc&       Desc) :
    TFenceBase{pRefCounters, pDevice, Desc}
{
    if (m_Desc.Type != FENCE_TYPE_CPU_WAIT_ONLY)
        LOG_ERROR_AND_THROW("Description of Fence '", m_Desc.Name, "' is invalid: ", GetFenceTypeString(m_Desc.Type), " is not supported in Direct3D11.");
}

FenceD3D11Impl::~FenceD3D11Impl()
{
    if (m_MaxPendingQueries < 10)
        LOG_INFO_MESSAGE("Max pending queries: ", m_MaxPendingQueries);
    else
        LOG_WARNING_MESSAGE("Max pending queries (", m_MaxPendingQueries, ") is large. This may indicate that none of GetCompletedValue() or Wait() have been used.");
}

Uint64 FenceD3D11Impl::GetCompletedValue()
{
    while (!m_PendingQueries.empty())
    {
        auto& QueryData = m_PendingQueries.front();
        BOOL  Data;
        auto  res = QueryData.pd3d11Ctx->GetData(QueryData.pd3d11Query, &Data, sizeof(Data), D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if (res == S_OK)
        {
            VERIFY_EXPR(Data == TRUE);
            UpdateLastCompletedFenceValue(QueryData.Value);
            m_PendingQueries.pop_front();
        }
        else
        {
            break;
        }
    }

    return m_LastCompletedFenceValue.load();
}

void FenceD3D11Impl::Wait(Uint64 Value)
{
    return Wait(Value, true);
}

void FenceD3D11Impl::Wait(Uint64 Value, bool FlushCommands)
{
    while (!m_PendingQueries.empty())
    {
        auto& QueryData = m_PendingQueries.front();
        if (QueryData.Value > Value)
            break;

        BOOL Data;
        while (QueryData.pd3d11Ctx->GetData(QueryData.pd3d11Query, &Data, sizeof(Data), FlushCommands ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
            std::this_thread::sleep_for(std::chrono::microseconds{1});

        VERIFY_EXPR(Data == TRUE);
        UpdateLastCompletedFenceValue(QueryData.Value);
        m_PendingQueries.pop_front();
    }
}

void FenceD3D11Impl::Signal(Uint64 Value)
{
    DEV_ERROR("Signal() is not supported in Direct3D11 backend");
}

} // namespace Diligent
