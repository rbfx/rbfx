/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "DynamicBuffer.hpp"

#include <algorithm>
#include <vector>

#include "DebugUtilities.hpp"
#include "Align.hpp"

namespace Diligent
{

namespace
{

bool VerifySparseBufferCompatibility(IRenderDevice* pDevice)
{
    VERIFY_EXPR(pDevice != nullptr);

    const auto& DeviceInfo = pDevice->GetDeviceInfo().Features;
    if (!DeviceInfo.SparseResources)
    {
        LOG_WARNING_MESSAGE("SparseResources device feature is not enabled.");
        return false;
    }

    const auto& SparseRes = pDevice->GetAdapterInfo().SparseResources;
    if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_BUFFER) == 0)
    {
        LOG_WARNING_MESSAGE("This device does not support sparse buffers.");
        return false;
    }

    return true;
}

} // namespace

DynamicBuffer::DynamicBuffer(IRenderDevice*                 pDevice,
                             const DynamicBufferCreateInfo& CI) :
    m_Name{CI.Desc.Name != nullptr ? CI.Desc.Name : "Dynamic buffer"},
    m_Desc{CI.Desc},
    m_VirtualSize{CI.Desc.Usage == USAGE_SPARSE ? CI.VirtualSize : 0},
    m_MemoryPageSize{CI.MemoryPageSize}
{
    DEV_CHECK_ERR(CI.Desc.Usage != USAGE_SPARSE || CI.VirtualSize > 0, "Virtual size must not be 0 for sparse buffers");

    m_Desc.Name   = m_Name.c_str();
    m_PendingSize = m_Desc.Size;
    m_Desc.Size   = 0; // Current buffer size
    if (pDevice != nullptr && (m_PendingSize > 0 || m_Desc.Usage == USAGE_SPARSE))
    {
        InitBuffer(pDevice);
    }
}

void DynamicBuffer::CreateSparseBuffer(IRenderDevice* pDevice)
{
    VERIFY_EXPR(pDevice != nullptr);
    VERIFY_EXPR(!m_pBuffer && !m_pMemory);
    VERIFY_EXPR(m_Desc.Usage == USAGE_SPARSE);

    if (!VerifySparseBufferCompatibility(pDevice))
    {
        LOG_WARNING_MESSAGE("This device does not support capabilities required for sparse buffers. USAGE_DEFAULT buffer will be used instead.");
        m_Desc.Usage = USAGE_DEFAULT;
        return;
    }

    const auto& SparseResources    = pDevice->GetAdapterInfo().SparseResources;
    const auto  SparseMemBlockSize = SparseResources.StandardBlockSize;

    m_MemoryPageSize = std::max(AlignUpNonPw2(m_MemoryPageSize, SparseMemBlockSize), SparseMemBlockSize);

    {
        auto Desc = m_Desc;

        Desc.Size    = AlignUpNonPw2(m_VirtualSize, m_MemoryPageSize);
        auto MaxSize = AlignDownNonPw2(SparseResources.ResourceSpaceSize, m_MemoryPageSize);
        if (m_Desc.BindFlags & (BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS))
        {
            VERIFY_EXPR(m_Desc.ElementByteStride != 0);
            // Buffer size must be a multiple of the element stride
            Desc.Size = AlignUpNonPw2(Desc.Size, m_Desc.ElementByteStride);
            MaxSize   = AlignDownNonPw2(MaxSize, m_Desc.ElementByteStride);
        }
        Desc.Size = std::min(Desc.Size, MaxSize);

        pDevice->CreateBuffer(Desc, nullptr, &m_pBuffer);
        DEV_CHECK_ERR(m_pBuffer, "Failed to create sparse buffer");
        if (!m_pBuffer)
            return;

        m_Desc.Size = 0; // No memory is committed
    }

    // Create backing memory
    {
        DeviceMemoryCreateInfo MemCI;
        MemCI.Desc.Name     = "Sparse dynamic buffer memory pool";
        MemCI.Desc.Type     = DEVICE_MEMORY_TYPE_SPARSE;
        MemCI.Desc.PageSize = m_MemoryPageSize;

        MemCI.InitialSize = m_MemoryPageSize;

        IDeviceObject* pCompatibleRes[]{m_pBuffer};
        MemCI.ppCompatibleResources = pCompatibleRes;
        MemCI.NumResources          = _countof(pCompatibleRes);

        pDevice->CreateDeviceMemory(MemCI, &m_pMemory);
        DEV_CHECK_ERR(m_pMemory, "Failed to create device memory");
    }

    // Note: D3D11 does not support general fences
    if (pDevice->GetDeviceInfo().Type != RENDER_DEVICE_TYPE_D3D11)
    {
        FenceDesc Desc;
        Desc.Type = FENCE_TYPE_GENERAL;

        Desc.Name = "Dynamic buffer before-resize fence";
        pDevice->CreateFence(Desc, &m_pBeforeResizeFence);
        Desc.Name = "Dynamic buffer after-resize fence";
        pDevice->CreateFence(Desc, &m_pAfterResizeFence);
    }
}

void DynamicBuffer::InitBuffer(IRenderDevice* pDevice)
{
    VERIFY_EXPR(pDevice != nullptr);
    VERIFY_EXPR(!m_pBuffer && !m_pMemory);

    if (m_Desc.Usage == USAGE_SPARSE)
    {
        CreateSparseBuffer(pDevice);
    }

    // NB: m_Desc.Usage may be changed by CreateSparseBuffer()
    if (m_Desc.Usage == USAGE_DEFAULT && m_PendingSize > 0)
    {
        auto Desc = m_Desc;
        Desc.Size = m_PendingSize;
        pDevice->CreateBuffer(Desc, nullptr, &m_pBuffer);
        if (m_Desc.Size == 0)
        {
            // The array was previously empty - nothing to copy
            m_Desc.Size = m_PendingSize;
        }
    }
    DEV_CHECK_ERR(m_pBuffer, "Failed to create buffer for a dynamic buffer");

    m_Version.fetch_add(1);
}

void DynamicBuffer::ResizeSparseBuffer(IDeviceContext* pContext)
{
    VERIFY_EXPR(m_pBuffer && m_pMemory);
    VERIFY_EXPR(m_PendingSize % m_MemoryPageSize == 0);
    DEV_CHECK_ERR(m_PendingSize <= m_pBuffer->GetDesc().Size,
                  "New size (", m_PendingSize, ") exceeds the buffer virtual size (", m_pBuffer->GetDesc().Size, ").");

    if (m_pMemory->GetCapacity() < m_PendingSize)
        m_pMemory->Resize(m_PendingSize); // Allocate additional memory

    SparseBufferMemoryBindInfo BufferBindInfo;
    BufferBindInfo.pBuffer = m_pBuffer;

    auto StartOffset = m_Desc.Size;
    auto EndOffset   = m_PendingSize;
    if (StartOffset > EndOffset)
        std::swap(StartOffset, EndOffset);
    VERIFY_EXPR((EndOffset - StartOffset) % m_MemoryPageSize == 0);
    const auto NumPages = StaticCast<Uint32>((EndOffset - StartOffset) / m_MemoryPageSize);

    std::vector<SparseBufferMemoryBindRange> Ranges;
    Ranges.reserve(NumPages);
    for (Uint32 i = 0; i < NumPages; ++i)
    {
        SparseBufferMemoryBindRange Range;
        Range.BufferOffset = StartOffset + i * Uint64{m_MemoryPageSize};
        Range.MemorySize   = m_MemoryPageSize;
        Range.pMemory      = m_PendingSize > m_Desc.Size ? m_pMemory.RawPtr() : nullptr;
        Range.MemoryOffset = Range.pMemory != nullptr ? Range.BufferOffset : 0;
        Ranges.emplace_back(Range);
    }

    BufferBindInfo.pRanges   = Ranges.data();
    BufferBindInfo.NumRanges = static_cast<Uint32>(Ranges.size());

    BindSparseResourceMemoryAttribs BindMemAttribs;
    BindMemAttribs.NumBufferBinds = 1;
    BindMemAttribs.pBufferBinds   = &BufferBindInfo;

    Uint64  WaitFenceValue = 0;
    IFence* pWaitFence     = nullptr;
    if (m_pBeforeResizeFence)
    {
        WaitFenceValue = m_NextBeforeResizeFenceValue++;
        pWaitFence     = m_pBeforeResizeFence;

        BindMemAttribs.NumWaitFences    = 1;
        BindMemAttribs.pWaitFenceValues = &WaitFenceValue;
        BindMemAttribs.ppWaitFences     = &pWaitFence;

        pContext->EnqueueSignal(m_pBeforeResizeFence, WaitFenceValue);
    }

    Uint64  SignalFenceValue = 0;
    IFence* pSignalFence     = nullptr;
    if (m_pAfterResizeFence)
    {
        SignalFenceValue = m_NextAfterResizeFenceValue++;
        pSignalFence     = m_pAfterResizeFence;

        BindMemAttribs.NumSignalFences    = 1;
        BindMemAttribs.pSignalFenceValues = &SignalFenceValue;
        BindMemAttribs.ppSignalFences     = &pSignalFence;
    }

    pContext->BindSparseResourceMemory(BindMemAttribs);

    if (m_pMemory->GetCapacity() > m_PendingSize)
        m_pMemory->Resize(m_PendingSize); // Release unused memory
}

void DynamicBuffer::ResizeDefaultBuffer(IDeviceContext* pContext)
{
    if (!m_pStaleBuffer)
        return;

    VERIFY_EXPR(m_pBuffer);
    auto CopySize = std::min(m_pBuffer->GetDesc().Size, m_pStaleBuffer->GetDesc().Size);
    pContext->CopyBuffer(m_pStaleBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                         m_pBuffer, 0, CopySize, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pStaleBuffer.Release();
}

void DynamicBuffer::CommitResize(IRenderDevice*  pDevice,
                                 IDeviceContext* pContext,
                                 bool            AllowNull)
{
    if (!m_pBuffer && m_PendingSize > 0)
    {
        if (pDevice != nullptr)
            InitBuffer(pDevice);
        else
            DEV_CHECK_ERR(AllowNull, "Dynamic buffer must be initialized, but pDevice is null");
    }

    if (m_pBuffer && m_Desc.Size != m_PendingSize)
    {
        if (pContext != nullptr)
        {
            if (m_Desc.Usage == USAGE_SPARSE)
                ResizeSparseBuffer(pContext);
            else
                ResizeDefaultBuffer(pContext);

            LOG_INFO_MESSAGE("Dynamic buffer: expanding dynamic buffer '", m_Desc.Name, "' from ",
                             FormatMemorySize(m_Desc.Size, 1), " to ", FormatMemorySize(m_PendingSize, 1),
                             ". Version: ", GetVersion());

            m_Desc.Size = m_PendingSize;
        }
        else
        {
            DEV_CHECK_ERR(AllowNull, "Dynamic buffer must be resized, but pContext is null. Use PendingUpdate() to check if the buffer must be updated.");
        }
    }
}

IBuffer* DynamicBuffer::Resize(IRenderDevice*  pDevice,
                               IDeviceContext* pContext,
                               Uint64          NewSize,
                               bool            DiscardContent)
{
    if (m_Desc.Usage == USAGE_SPARSE)
    {
        DEV_CHECK_ERR(NewSize <= m_VirtualSize, "New size (", NewSize, ") exceeds the buffer virtual size (", m_VirtualSize, ").");
        NewSize = AlignUpNonPw2(NewSize, m_MemoryPageSize);
    }

    if (m_Desc.Size != NewSize)
    {
        m_PendingSize = NewSize;

        if (m_Desc.Usage != USAGE_SPARSE)
        {
            if (!m_pStaleBuffer)
                m_pStaleBuffer = std::move(m_pBuffer);
            else
            {
                DEV_CHECK_ERR(!m_pBuffer || NewSize == 0,
                              "There is a non-null stale buffer. This likely indicates that "
                              "Resize() has been called multiple times with different sizes, "
                              "but copy has not been committed by providing non-null device "
                              "context to either Resize() or Update()");
            }

            if (m_PendingSize == 0)
            {
                m_pStaleBuffer.Release();
                m_pBuffer.Release();
                m_Desc.Size = 0;
            }

            if (DiscardContent)
                m_pStaleBuffer.Release();
        }
    }

    CommitResize(pDevice, pContext, true /*AllowNull*/);

    return m_pBuffer;
}

IBuffer* DynamicBuffer::Update(IRenderDevice*  pDevice,
                               IDeviceContext* pContext)
{
    CommitResize(pDevice, pContext, false /*AllowNull*/);

    if (m_LastAfterResizeFenceValue + 1 < m_NextAfterResizeFenceValue)
    {
        DEV_CHECK_ERR(pContext != nullptr, "Device context is null, but waiting for the fence is required");
        VERIFY_EXPR(m_pAfterResizeFence);
        m_LastAfterResizeFenceValue = m_NextAfterResizeFenceValue - 1;
        pContext->DeviceWaitForFence(m_pAfterResizeFence, m_LastAfterResizeFenceValue);
    }

    return m_pBuffer;
}

} // namespace Diligent
