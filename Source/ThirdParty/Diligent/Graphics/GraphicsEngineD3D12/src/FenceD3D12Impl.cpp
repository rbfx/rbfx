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

#include "FenceD3D12Impl.hpp"

#include <thread>

#include "WinHPreface.h"
#include <atlbase.h>
#include "WinHPostface.h"

#include "RenderDeviceD3D12Impl.hpp"

namespace Diligent
{

FenceD3D12Impl::FenceD3D12Impl(IReferenceCounters*    pRefCounters,
                               RenderDeviceD3D12Impl* pDevice,
                               const FenceDesc&       Desc) :
    TFenceBase{pRefCounters, pDevice, Desc},
    m_FenceCompleteEvent //
    {
        CreateEvent(NULL,  // default security attributes
                    TRUE,  // manual-reset event
                    FALSE, // initial state is nonsignaled
                    NULL)  // object name
    }
{
    VERIFY(m_FenceCompleteEvent != NULL, "Failed to create fence complete event");

    const auto  Flags        = (m_Desc.Type == FENCE_TYPE_GENERAL && pDevice->GetNumImmediateContexts() > 1) ? D3D12_FENCE_FLAG_SHARED : D3D12_FENCE_FLAG_NONE;
    auto* const pd3d12Device = pDevice->GetD3D12Device();
    auto        hr           = pd3d12Device->CreateFence(0, Flags, __uuidof(m_pd3d12Fence), reinterpret_cast<void**>(static_cast<ID3D12Fence**>(&m_pd3d12Fence)));
    CHECK_D3D_RESULT_THROW(hr, "Failed to create D3D12 fence");
    if (m_Desc.Name != nullptr)
        m_pd3d12Fence->SetName(WidenString(m_Desc.Name).c_str());
}

FenceD3D12Impl::~FenceD3D12Impl()
{
    // D3D12 object can only be destroyed when it is no longer used by the GPU
    GetDevice()->SafeReleaseDeviceObject(std::move(m_pd3d12Fence), ~0ull);
    if (m_FenceCompleteEvent != NULL)
    {
        CloseHandle(m_FenceCompleteEvent);
    }
}

Uint64 FenceD3D12Impl::GetCompletedValue()
{
    Uint64 Result = m_pd3d12Fence->GetCompletedValue();
    VERIFY(Result != UINT64_MAX, "If the device has been removed, the return value will be UINT64_MAX");
    return Result;
}

void FenceD3D12Impl::Signal(Uint64 Value)
{
    DEV_CHECK_ERR(m_Desc.Type == FENCE_TYPE_GENERAL, "Fence must be created with FENCE_TYPE_GENERAL");
    DEV_CHECK_ERR(GetDevice()->GetFeatures().NativeFence, "CPU side fence signal requires NativeFence feature");
    DvpSignal(Value);

    m_pd3d12Fence->Signal(Value);
}

void FenceD3D12Impl::Wait(Uint64 Value)
{
    if (GetCompletedValue() >= Value)
        return;

    if (m_FenceCompleteEvent != NULL)
    {
        m_pd3d12Fence->SetEventOnCompletion(Value, m_FenceCompleteEvent);
        WaitForSingleObject(m_FenceCompleteEvent, INFINITE);
    }
    else
    {
        while (GetCompletedValue() < Value)
            std::this_thread::sleep_for(std::chrono::microseconds{1});
    }
}

} // namespace Diligent
