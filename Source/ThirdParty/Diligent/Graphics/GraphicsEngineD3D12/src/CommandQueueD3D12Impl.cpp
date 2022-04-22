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

#include "WinHPreface.h"
#include <dxgi1_4.h>
#include "WinHPostface.h"

#include "CommandQueueD3D12Impl.hpp"
#include "NVApiLoader.hpp"

namespace Diligent
{

CommandQueueD3D12Impl::CommandQueueD3D12Impl(IReferenceCounters* pRefCounters,
                                             ID3D12CommandQueue* pd3d12NativeCmdQueue,
                                             ID3D12Fence*        pd3d12Fence) :
    // clang-format off
    TBase{pRefCounters},
    m_pd3d12CmdQueue       {pd3d12NativeCmdQueue},
    m_d3d12CmdQueueDesc    {pd3d12NativeCmdQueue->GetDesc()},
    m_d3d12Fence           {pd3d12Fence         },
    m_NextFenceValue       {1                   },
    m_WaitForGPUEventHandle{CreateEvent(nullptr, false, false, nullptr)}
// clang-format on
{
    VERIFY_EXPR(m_WaitForGPUEventHandle != INVALID_HANDLE_VALUE);
    m_d3d12Fence->Signal(0);
}

CommandQueueD3D12Impl::~CommandQueueD3D12Impl()
{
    CloseHandle(m_WaitForGPUEventHandle);
}

Uint64 CommandQueueD3D12Impl::Submit(Uint32                    NumCommandLists,
                                     ID3D12CommandList* const* ppCommandLists)
{
    std::lock_guard<std::mutex> Lock{m_QueueMtx};

    // Increment the value before submitting the list
    auto FenceValue = m_NextFenceValue.fetch_add(1);

    // Render device submits null command list to signal the fence and
    // discard all resources.
    if (NumCommandLists != 0 && ppCommandLists != nullptr)
    {
#ifdef DILIGENT_DEBUG
        for (Uint32 i = 0; i < NumCommandLists; ++i)
        {
            VERIFY(ppCommandLists[i] != nullptr, "Command list must not be null");
        }
#endif
        m_pd3d12CmdQueue->ExecuteCommandLists(NumCommandLists, ppCommandLists);
    }

    // Signal the fence. This must be done atomically with command list submission.
    m_pd3d12CmdQueue->Signal(m_d3d12Fence, FenceValue);

    return FenceValue;
}

Uint64 CommandQueueD3D12Impl::WaitForIdle()
{
    std::lock_guard<std::mutex> Lock{m_QueueMtx};

    Uint64 LastSignaledFenceValue = m_NextFenceValue.fetch_add(1);

    m_pd3d12CmdQueue->Signal(m_d3d12Fence, LastSignaledFenceValue);

    if (GetCompletedFenceValue() < LastSignaledFenceValue)
    {
        m_d3d12Fence->SetEventOnCompletion(LastSignaledFenceValue, m_WaitForGPUEventHandle);
        WaitForSingleObject(m_WaitForGPUEventHandle, INFINITE);
        VERIFY(GetCompletedFenceValue() == LastSignaledFenceValue, "Unexpected signaled fence value");
    }
    return LastSignaledFenceValue;
}

Uint64 CommandQueueD3D12Impl::GetCompletedFenceValue()
{
    auto CompletedFenceValue = m_d3d12Fence->GetCompletedValue();
    VERIFY(CompletedFenceValue != UINT64_MAX, "If the device has been removed, the return value will be UINT64_MAX");

    auto CurrValue = m_LastCompletedFenceValue.load();
    while (!m_LastCompletedFenceValue.compare_exchange_weak(CurrValue, std::max(CurrValue, CompletedFenceValue)))
    {
        // If exchange fails, CurrValue will hold the actual value of m_LastCompletedFenceValue
    }

    return m_LastCompletedFenceValue.load();
}

void CommandQueueD3D12Impl::EnqueueSignal(ID3D12Fence* pFence, Uint64 Value)
{
    DEV_CHECK_ERR(pFence, "Fence must not be null");

    std::lock_guard<std::mutex> Lock{m_QueueMtx};
    m_pd3d12CmdQueue->Signal(pFence, Value);
}

void CommandQueueD3D12Impl::WaitFence(ID3D12Fence* pFence, Uint64 Value)
{
    DEV_CHECK_ERR(pFence, "Fence must not be null");

    std::lock_guard<std::mutex> Lock{m_QueueMtx};
    m_pd3d12CmdQueue->Wait(pFence, Value);
}

void CommandQueueD3D12Impl::UpdateTileMappings(ResourceTileMappingsD3D12* pMappings, Uint32 Count)
{
    DEV_CHECK_ERR(pMappings != nullptr, "Tile mappings must not be null");

    std::lock_guard<std::mutex> Lock{m_QueueMtx};

    for (Uint32 i = 0; i < Count; ++i)
    {
        const auto& Mapping = pMappings[i];
        if (Mapping.UseNVApi)
        {
#ifdef DILIGENT_ENABLE_D3D_NVAPI
            if (NvAPI_D3D12_UpdateTileMappings(
                    m_pd3d12CmdQueue,
                    Mapping.pResource,
                    Mapping.NumResourceRegions,
                    Mapping.pResourceRegionStartCoordinates,
                    Mapping.pResourceRegionSizes,
                    Mapping.pHeap,
                    Mapping.NumRanges,
                    Mapping.pRangeFlags,
                    Mapping.pHeapRangeStartOffsets,
                    Mapping.pRangeTileCounts,
                    Mapping.Flags) != NVAPI_OK)
            {
                LOG_ERROR_MESSAGE("NvAPI_D3D12_UpdateTileMappings() failed");
            }
#else
            LOG_ERROR_MESSAGE("NvAPI is not enabled");
#endif
        }
        else
        {
            m_pd3d12CmdQueue->UpdateTileMappings(
                Mapping.pResource,
                Mapping.NumResourceRegions,
                Mapping.pResourceRegionStartCoordinates,
                Mapping.pResourceRegionSizes,
                Mapping.pHeap,
                Mapping.NumRanges,
                Mapping.pRangeFlags,
                Mapping.pHeapRangeStartOffsets,
                Mapping.pRangeTileCounts,
                Mapping.Flags);
        }
    }
}

} // namespace Diligent
