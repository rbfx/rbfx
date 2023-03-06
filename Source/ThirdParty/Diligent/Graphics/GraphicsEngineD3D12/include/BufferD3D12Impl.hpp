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
/// Declaration of Diligent::BufferD3D12Impl class

#include "EngineD3D12ImplTraits.hpp"
#include "BufferBase.hpp"
#include "BufferViewD3D12Impl.hpp" // Required by BufferBase
#include "D3D12ResourceBase.hpp"
#include "D3D12DynamicHeap.hpp"
#include "DescriptorHeap.hpp"
#include "IndexWrapper.hpp"

namespace Diligent
{

class DeviceContextD3D12Impl;

/// Buffer object implementation in Direct3D12 backend.
class BufferD3D12Impl final : public BufferBase<EngineD3D12ImplTraits>, public D3D12ResourceBase
{
public:
    using TBufferBase = BufferBase<EngineD3D12ImplTraits>;

    BufferD3D12Impl(IReferenceCounters*        pRefCounters,
                    FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                    RenderDeviceD3D12Impl*     pDeviceD3D12,
                    const BufferDesc&          BuffDesc,
                    const BufferData*          pBuffData = nullptr);

    BufferD3D12Impl(IReferenceCounters*        pRefCounters,
                    FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                    RenderDeviceD3D12Impl*     pDeviceD3D12,
                    const BufferDesc&          BuffDesc,
                    RESOURCE_STATE             InitialState,
                    ID3D12Resource*            pd3d12Buffer);
    ~BufferD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_BufferD3D12, TBufferBase)

#ifdef DILIGENT_DEVELOPMENT
    void DvpVerifyDynamicAllocation(const DeviceContextD3D12Impl* pCtx) const;
#endif

    /// Implementation of IBufferD3D12::GetD3D12Buffer().
    virtual ID3D12Resource* DILIGENT_CALL_TYPE GetD3D12Buffer(Uint64& DataStartByteOffset, IDeviceContext* pContext) override final;

    /// Implementation of IBuffer::GetNativeHandle().
    virtual Uint64 DILIGENT_CALL_TYPE GetNativeHandle() override final
    {
        VERIFY(GetD3D12Resource() != nullptr, "The buffer is dynamic and has no pointer to D3D12 resource");
        Uint64 DataStartByteOffset = 0;
        auto*  pd3d12Buffer        = GetD3D12Buffer(DataStartByteOffset, 0);
        VERIFY(DataStartByteOffset == 0, "0 offset expected");
        return BitCast<Uint64>(pd3d12Buffer);
    }

    /// Implementation of IBufferD3D12::SetD3D12ResourceState().
    virtual void DILIGENT_CALL_TYPE SetD3D12ResourceState(D3D12_RESOURCE_STATES state) override final;

    /// Implementation of IBufferD3D12::GetD3D12ResourceState().
    virtual D3D12_RESOURCE_STATES DILIGENT_CALL_TYPE GetD3D12ResourceState() const override final;

    /// Implementation of IBuffer::GetSparseProperties().
    virtual SparseBufferProperties DILIGENT_CALL_TYPE GetSparseProperties() const override final;

    __forceinline D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(DeviceContextIndex ContextId, const DeviceContextD3D12Impl* pCtx) const
    {
        if (m_Desc.Usage == USAGE_DYNAMIC)
        {
#ifdef DILIGENT_DEVELOPMENT
            if (pCtx != nullptr)
            {
                DvpVerifyDynamicAllocation(pCtx);
            }
#endif
            return m_DynamicData[ContextId].GPUAddress;
        }
        else
        {
            return GetD3D12Resource()->GetGPUVirtualAddress();
        }
    }

    __forceinline D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress()
    {
        VERIFY_EXPR(m_Desc.Usage != USAGE_DYNAMIC);
        return GetD3D12Resource()->GetGPUVirtualAddress();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCBVHandle() { return m_CBVDescriptorAllocation.GetCpuHandle(); }

    void CreateCBV(D3D12_CPU_DESCRIPTOR_HANDLE CBVDescriptor, Uint64 Offset = 0, Uint64 Size = 0) const;

private:
    virtual void CreateViewInternal(const struct BufferViewDesc& ViewDesc, IBufferView** ppView, bool bIsDefaultView) override;

    void CreateUAV(struct BufferViewDesc& UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE UAVDescriptor) const;
    void CreateSRV(struct BufferViewDesc& SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE SRVDescriptor) const;

    DescriptorHeapAllocation m_CBVDescriptorAllocation;

    // Align the struct size to the cache line size to avoid false sharing
    static constexpr size_t CacheLineSize = 64;
    struct alignas(CacheLineSize) CtxDynamicData : D3D12DynamicAllocation
    {
        CtxDynamicData& operator=(const D3D12DynamicAllocation& Allocation)
        {
            *static_cast<D3D12DynamicAllocation*>(this) = Allocation;
            return *this;
        }
        Uint8 Padding[CacheLineSize - sizeof(D3D12DynamicAllocation)] = {};
    };
    static_assert(sizeof(CtxDynamicData) == CacheLineSize, "Unexpected sizeof(CtxDynamicData)");

    friend DeviceContextD3D12Impl;
    // Array of dynamic allocations for every device context.
    std::vector<CtxDynamicData, STDAllocatorRawMem<CtxDynamicData>> m_DynamicData;
};

} // namespace Diligent
