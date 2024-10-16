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

#include "BufferWebGPUImpl.hpp"
#include "RenderDeviceWebGPUImpl.hpp"
#include "DeviceContextWebGPUImpl.hpp"
#include "WebGPUTypeConversions.hpp"

namespace Diligent
{

namespace
{

Uint32 ComputeBufferAlignment(const RenderDeviceWebGPUImpl* pDevice, const BufferDesc& Desc)
{
    Uint32 Alignment = 16; // Which alignment to use for buffers that don't have any specific requirements?
    if (Desc.BindFlags & BIND_UNIFORM_BUFFER)
        Alignment = pDevice->GetAdapterInfo().Buffer.ConstantBufferOffsetAlignment;

    if (Desc.BindFlags & (BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE))
        Alignment = pDevice->GetAdapterInfo().Buffer.StructuredBufferOffsetAlignment;

    return Alignment;
}

} // namespace

BufferWebGPUImpl::BufferWebGPUImpl(IReferenceCounters*        pRefCounters,
                                   FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                                   RenderDeviceWebGPUImpl*    pDevice,
                                   const BufferDesc&          Desc,
                                   const BufferData*          pInitData,
                                   bool                       bIsDeviceInternal) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pDevice,
        Desc,
        bIsDeviceInternal
    },
    WebGPUResourceBase{*this, Desc.Usage == USAGE_STAGING ? ((Desc.CPUAccessFlags & CPU_ACCESS_READ) ? MaxStagingReadBuffers : 1): 0},
    m_DynamicAllocations(STD_ALLOCATOR_RAW_MEM(DynamicAllocation, GetRawAllocator(), "Allocator for vector<DynamicAllocation>")),
    m_Alignment{ComputeBufferAlignment(pDevice, m_Desc)}
// clang-format on
{
    ValidateBufferInitData(m_Desc, pInitData);

    if (m_Desc.Usage == USAGE_UNIFIED || m_Desc.Usage == USAGE_SPARSE)
        LOG_ERROR_AND_THROW("Unified and sparse resources are not supported in WebGPU");

    if (m_Desc.Usage == USAGE_STAGING && (m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == (CPU_ACCESS_READ | CPU_ACCESS_WRITE))
        LOG_ERROR_AND_THROW("Read-write staging buffers are not supported in WebGPU");

    const bool RequiresBackingBuffer = (m_Desc.BindFlags & BIND_UNORDERED_ACCESS) != 0 || ((m_Desc.BindFlags & BIND_SHADER_RESOURCE) != 0 && m_Desc.Mode == BUFFER_MODE_FORMATTED);
    const bool InitializeBuffer      = (pInitData != nullptr && pInitData->pData != nullptr);

    if (m_Desc.Usage == USAGE_DYNAMIC && !RequiresBackingBuffer)
    {
        size_t CtxCount = pDevice->GetNumImmediateContexts() + pDevice->GetNumDeferredContexts();
        m_DynamicAllocations.resize(CtxCount);
    }
    else
    {
        if (m_Desc.Usage == USAGE_STAGING)
        {
            m_MappedData.resize(static_cast<size_t>(AlignUp(m_Desc.Size, m_Alignment)));
            if (InitializeBuffer)
                memcpy(m_MappedData.data(), pInitData->pData, static_cast<size_t>(std::min(m_Desc.Size, pInitData->DataSize)));
        }
        else
        {
            WGPUBufferDescriptor wgpuBufferDesc{};
            wgpuBufferDesc.label = m_Desc.Name;
            wgpuBufferDesc.size  = AlignUp(m_Desc.Size, m_Alignment);
            wgpuBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;

            for (BIND_FLAGS BindFlags = m_Desc.BindFlags; BindFlags != 0;)
            {
                const BIND_FLAGS BindFlag = ExtractLSB(BindFlags);
                switch (BindFlag)
                {
                    case BIND_UNIFORM_BUFFER:
                        wgpuBufferDesc.usage |= WGPUBufferUsage_Uniform;
                        break;
                    case BIND_SHADER_RESOURCE:
                    case BIND_UNORDERED_ACCESS:
                        wgpuBufferDesc.usage |= WGPUBufferUsage_Storage;
                        break;
                    case BIND_VERTEX_BUFFER:
                        wgpuBufferDesc.usage |= WGPUBufferUsage_Vertex;
                        break;
                    case BIND_INDEX_BUFFER:
                        wgpuBufferDesc.usage |= WGPUBufferUsage_Index;
                        break;
                    case BIND_INDIRECT_DRAW_ARGS:
                        wgpuBufferDesc.usage |= WGPUBufferUsage_Indirect;
                        break;
                    default:
                        UNEXPECTED("unsupported buffer usage type");
                        break;
                }
            }

            wgpuBufferDesc.mappedAtCreation = InitializeBuffer;

            m_wgpuBuffer.Reset(wgpuDeviceCreateBuffer(pDevice->GetWebGPUDevice(), &wgpuBufferDesc));
            if (!m_wgpuBuffer)
                LOG_ERROR_AND_THROW("Failed to create WebGPU buffer ", " '", m_Desc.Name ? m_Desc.Name : "", '\'');

            if (wgpuBufferDesc.mappedAtCreation)
            {
                // Do NOT use WGPU_WHOLE_MAP_SIZE due to https://github.com/emscripten-core/emscripten/issues/20538
                void* pData = wgpuBufferGetMappedRange(m_wgpuBuffer, 0, StaticCast<size_t>(wgpuBufferDesc.size));
                memcpy(pData, pInitData->pData, StaticCast<size_t>(pInitData->DataSize));
                wgpuBufferUnmap(m_wgpuBuffer);
            }
        }
    }

    SetState(RESOURCE_STATE_UNDEFINED);
    m_MemoryProperties = MEMORY_PROPERTY_HOST_COHERENT;
}

BufferWebGPUImpl::BufferWebGPUImpl(IReferenceCounters*        pRefCounters,
                                   FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                                   RenderDeviceWebGPUImpl*    pDevice,
                                   const BufferDesc&          Desc,
                                   RESOURCE_STATE             InitialState,
                                   WGPUBuffer                 wgpuBuffer,
                                   bool                       bIsDeviceInternal) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pDevice,
        Desc,
        bIsDeviceInternal
    },
    WebGPUResourceBase{*this, 0},
    m_wgpuBuffer{wgpuBuffer, {true}},
    m_DynamicAllocations(STD_ALLOCATOR_RAW_MEM(DynamicAllocation, GetRawAllocator(), "Allocator for vector<DynamicAllocation>")),
    m_Alignment{ComputeBufferAlignment(pDevice, Desc)}
// clang-format on
{
    DEV_CHECK_ERR(Desc.Usage != USAGE_STAGING, "USAGE_STAGING buffer is not expected");

    VERIFY(m_Desc.Size % m_Alignment == 0, "Size of buffer must be aligned");
    SetState(InitialState);
    m_MemoryProperties = MEMORY_PROPERTY_HOST_COHERENT;
}

Uint64 BufferWebGPUImpl::GetNativeHandle()
{
    return BitCast<Uint64>(GetWebGPUBuffer());
}

SparseBufferProperties BufferWebGPUImpl::GetSparseProperties() const
{
    DEV_ERROR("IBuffer::GetSparseProperties() is not supported in WebGPU");
    return {};
}

WGPUBuffer BufferWebGPUImpl::GetWebGPUBuffer() const
{
    if (m_wgpuBuffer)
        return m_wgpuBuffer.Get();

    VERIFY(m_Desc.Usage == USAGE_DYNAMIC, "Dynamic buffer is expected");
    return m_pDevice->GetDynamicMemoryManager().GetWGPUBuffer();
}

void* BufferWebGPUImpl::Map(MAP_TYPE MapType)
{
    VERIFY(m_Desc.Usage == USAGE_STAGING, "Map is only allowed USAGE_STAGING buffers");
    return WebGPUResourceBase::Map(MapType, 0);
}

void BufferWebGPUImpl::Unmap()
{
    VERIFY(m_Desc.Usage == USAGE_STAGING, "Unmap is only allowed for USAGE_STAGING buffers");
    WebGPUResourceBase::Unmap();
}

Uint32 BufferWebGPUImpl::GetAlignment() const
{
    return m_Alignment;
}

const DynamicMemoryManagerWebGPU::Allocation& BufferWebGPUImpl::GetDynamicAllocation(DeviceContextIndex CtxId) const
{
    return m_DynamicAllocations[CtxId];
}

void BufferWebGPUImpl::SetDynamicAllocation(DeviceContextIndex CtxId, DynamicMemoryManagerWebGPU::Allocation&& Allocation)
{
    m_DynamicAllocations[CtxId] = std::move(Allocation);
}

BufferWebGPUImpl::StagingBufferInfo* BufferWebGPUImpl::GetStagingBuffer()
{
    VERIFY(m_Desc.Usage == USAGE_STAGING, "USAGE_STAGING buffer is expected");
    return WebGPUResourceBase::GetStagingBuffer(m_pDevice->GetWebGPUDevice(), m_Desc.CPUAccessFlags);
}

void BufferWebGPUImpl::CreateViewInternal(const BufferViewDesc& OrigViewDesc, IBufferView** ppView, bool IsDefaultView)
{
    VERIFY(ppView != nullptr, "Null pointer provided");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        RenderDeviceWebGPUImpl* const pDeviceWebGPU = GetDevice();

        BufferViewDesc ViewDesc = OrigViewDesc;
        ValidateAndCorrectBufferViewDesc(m_Desc, ViewDesc, pDeviceWebGPU->GetAdapterInfo().Buffer.StructuredBufferOffsetAlignment);

        auto& BuffViewAllocator = pDeviceWebGPU->GetBuffViewObjAllocator();
        VERIFY(&BuffViewAllocator == &m_dbgBuffViewAllocator, "Buffer view allocator does not match allocator provided at buffer initialization");

        if (ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS || ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE)
            *ppView = NEW_RC_OBJ(BuffViewAllocator, "BufferViewWebGPUImpl instance", BufferViewWebGPUImpl, IsDefaultView ? this : nullptr)(pDeviceWebGPU, ViewDesc, this, IsDefaultView, m_bIsDeviceInternal);

        if (!IsDefaultView && *ppView)
            (*ppView)->AddRef();
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetBufferViewTypeLiteralName(OrigViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", OrigViewDesc.Name ? OrigViewDesc.Name : "", "\" (", ViewTypeName, ") for buffer \"", m_Desc.Name, "\"");
    }
}


#ifdef DILIGENT_DEVELOPMENT
void BufferWebGPUImpl::DvpVerifyDynamicAllocation(const DeviceContextWebGPUImpl* pCtx) const
{
    if (!m_wgpuBuffer)
    {
        VERIFY(m_Desc.Usage == USAGE_DYNAMIC, "Dynamic buffer is expected");

        const DeviceContextIndex ContextId    = pCtx->GetContextId();
        const DynamicAllocation& DynAlloc     = m_DynamicAllocations[ContextId];
        const Uint64             CurrentFrame = pCtx->GetFrameNumber();
        DEV_CHECK_ERR(DynAlloc, "Dynamic buffer '", m_Desc.Name, "' has not been mapped before its first use. Context Id: ", ContextId, ". Note: memory for dynamic buffers is allocated when a buffer is mapped.");
        DEV_CHECK_ERR(DynAlloc.dvpFrameNumber == CurrentFrame, "Dynamic allocation of dynamic buffer '", m_Desc.Name, "' in frame ", CurrentFrame, " is out-of-date. Note: contents of all dynamic resources is discarded at the end of every frame. A buffer must be mapped before its first use in any frame.");
    }
}
#endif

} // namespace Diligent
