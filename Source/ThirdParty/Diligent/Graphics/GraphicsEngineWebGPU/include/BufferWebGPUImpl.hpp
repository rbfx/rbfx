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

#pragma once

/// \file
/// Declaration of Diligent::BufferWebGPUImpl class

#include "EngineWebGPUImplTraits.hpp"
#include "BufferBase.hpp"
#include "BufferViewWebGPUImpl.hpp" // Required by BufferBase
#include "DynamicMemoryManagerWebGPU.hpp"
#include "WebGPUObjectWrappers.hpp"
#include "IndexWrapper.hpp"
#include "UploadMemoryManagerWebGPU.hpp"
#include "WebGPUResourceBase.hpp"

namespace Diligent
{

/// Buffer implementation in WebGPU backend.
class BufferWebGPUImpl final : public BufferBase<EngineWebGPUImplTraits>, public WebGPUResourceBase
{
public:
    using TBufferBase = BufferBase<EngineWebGPUImplTraits>;

    BufferWebGPUImpl(IReferenceCounters*        pRefCounters,
                     FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                     RenderDeviceWebGPUImpl*    pDevice,
                     const BufferDesc&          Desc,
                     const BufferData*          pInitData,
                     bool                       bIsDeviceInternal);

    // Attaches to an existing WebGPU resource
    BufferWebGPUImpl(IReferenceCounters*        pRefCounters,
                     FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                     RenderDeviceWebGPUImpl*    pDevice,
                     const BufferDesc&          Desc,
                     RESOURCE_STATE             InitialState,
                     WGPUBuffer                 wgpuBuffer,
                     bool                       bIsDeviceInternal);

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_BufferWebGPU, TBufferBase)

    /// Implementation of IBuffer::GetNativeHandle().
    Uint64 DILIGENT_CALL_TYPE GetNativeHandle() override final;

    /// Implementation of IBuffer::GetSparseProperties().
    SparseBufferProperties DILIGENT_CALL_TYPE GetSparseProperties() const override final;

    /// Implementation of IBufferWebGPU::GetWebGPUBuffer().
    WGPUBuffer DILIGENT_CALL_TYPE GetWebGPUBuffer() const override final;

    Uint64 GetDynamicOffset(DeviceContextIndex CtxId, const DeviceContextWebGPUImpl* pCtx) const
    {
        if (m_wgpuBuffer)
        {
            return 0;
        }
        else
        {
            VERIFY(m_Desc.Usage == USAGE_DYNAMIC, "Dynamic buffer is expected");
            VERIFY_EXPR(!m_DynamicAllocations.empty());
#ifdef DILIGENT_DEVELOPMENT
            if (pCtx != nullptr)
            {
                DvpVerifyDynamicAllocation(pCtx);
            }
#endif
            auto& DynAlloc = GetDynamicAllocation(CtxId);
            return DynAlloc.Offset;
        }
    }

#ifdef DILIGENT_DEVELOPMENT
    void DvpVerifyDynamicAllocation(const DeviceContextWebGPUImpl* pCtx) const;
#endif

    void* Map(MAP_TYPE MapType);

    void Unmap();

    Uint32 GetAlignment() const;

    const DynamicMemoryManagerWebGPU::Allocation& GetDynamicAllocation(DeviceContextIndex CtxId) const;

    void SetDynamicAllocation(DeviceContextIndex CtxId, DynamicMemoryManagerWebGPU::Allocation&& Allocation);

    StagingBufferInfo* GetStagingBuffer();

private:
    void CreateViewInternal(const BufferViewDesc& ViewDesc, IBufferView** ppView, bool IsDefaultView) override;

private:
    friend class DeviceContextWebGPUImpl;

    // Use 64-byte alignment to avoid cache issues
    static constexpr size_t CacheLineSize = 64;
    struct alignas(64) DynamicAllocation : DynamicMemoryManagerWebGPU::Allocation
    {
        DynamicAllocation& operator=(const Allocation& Allocation)
        {
            *static_cast<DynamicMemoryManagerWebGPU::Allocation*>(this) = Allocation;
            return *this;
        }
        Uint8 Padding[CacheLineSize - sizeof(Allocation)] = {};
    };
    static_assert(sizeof(DynamicAllocation) == CacheLineSize, "Unexpected sizeof(DynamicAllocation)");

    using DynamicAllocationList = std::vector<DynamicAllocation, STDAllocatorRawMem<DynamicAllocation>>;

    static constexpr Uint32 MaxStagingReadBuffers = 16;

    WebGPUBufferWrapper   m_wgpuBuffer;
    DynamicAllocationList m_DynamicAllocations;
    const Uint32          m_Alignment;
};

} // namespace Diligent
