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

#include "pch.h"

#include "ShaderResourceCacheD3D12.hpp"

#include "RenderDeviceD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"
#include "BufferViewD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "TextureViewD3D12Impl.hpp"
#include "TopLevelASD3D12Impl.hpp"

#include "CommandContext.hpp"

namespace Diligent
{

ShaderResourceCacheD3D12::MemoryRequirements ShaderResourceCacheD3D12::GetMemoryRequirements(const RootParamsManager& RootParams)
{
    const auto NumRootTables = RootParams.GetNumRootTables();
    const auto NumRootViews  = RootParams.GetNumRootViews();

    MemoryRequirements MemReqs;

    for (auto d3d12HeapType : {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER})
    {
        for (Uint32 group = 0; group < ROOT_PARAMETER_GROUP_COUNT; ++group)
        {
            const auto ParamGroupSize = RootParams.GetParameterGroupSize(d3d12HeapType, static_cast<ROOT_PARAMETER_GROUP>(group));
            if (ParamGroupSize != 0)
            {
                MemReqs.TotalResources += ParamGroupSize;
                // Every non-empty table from each group will need a descriptor table
                ++MemReqs.NumDescriptorAllocations;
            }
        }
    }
    // Root views' resources are stored in one-descriptor tables
    MemReqs.TotalResources += NumRootViews;

    static_assert(sizeof(RootTable) % sizeof(void*) == 0, "sizeof(RootTable) is not aligned by the sizeof(void*)");
    static_assert(sizeof(Resource) % sizeof(void*) == 0, "sizeof(Resource) is not aligned by the sizeof(void*)");

    MemReqs.NumTables = NumRootTables + NumRootViews;

    MemReqs.TotalSize = (MemReqs.NumTables * sizeof(RootTable) +
                         MemReqs.TotalResources * sizeof(Resource) +
                         MemReqs.NumDescriptorAllocations * sizeof(DescriptorHeapAllocation));

    return MemReqs;
}


// Memory layout:
//                                         __________________________________________________________
//  m_pMemory                             |             m_pResources, m_NumResources                 |
//  |                                     |                                                          |
//  V                                     |                                                          V
//  |  RootTable[0]  |   ....    |  RootTable[Nrt-1]  |  Res[0]  |  ... |  Res[n-1]  |    ....     | Res[0]  |  ... |  Res[m-1]  |  DescriptorHeapAllocation[0]  |  ...
//       |                                                A
//       |                                                |
//       |________________________________________________|
//                    m_pResources, m_NumResources
//

size_t ShaderResourceCacheD3D12::AllocateMemory(IMemoryAllocator& MemAllocator)
{
    VERIFY(!m_pMemory, "Memory has already been allocated");

    const auto MemorySize =
        (m_NumTables * sizeof(RootTable) +
         m_TotalResourceCount * sizeof(Resource) +
         m_NumDescriptorAllocations * sizeof(DescriptorHeapAllocation));

    if (MemorySize > 0)
    {
        m_pMemory = decltype(m_pMemory){
            ALLOCATE_RAW(MemAllocator, "Memory for shader resource cache data", MemorySize),
            STDDeleter<void, IMemoryAllocator>(MemAllocator) //
        };

        auto* const pTables     = reinterpret_cast<RootTable*>(m_pMemory.get());
        auto* const pResources  = reinterpret_cast<Resource*>(pTables + m_NumTables);
        m_DescriptorAllocations = m_NumDescriptorAllocations > 0 ? reinterpret_cast<DescriptorHeapAllocation*>(pResources + m_TotalResourceCount) : nullptr;

        for (Uint32 res = 0; res < m_TotalResourceCount; ++res)
            new (pResources + res) Resource{};

        for (Uint32 i = 0; i < m_NumDescriptorAllocations; ++i)
            new (m_DescriptorAllocations + i) DescriptorHeapAllocation{};
    }

    return MemorySize;
}

void ShaderResourceCacheD3D12::Initialize(IMemoryAllocator& MemAllocator,
                                          Uint32            NumTables,
                                          const Uint32      TableSizes[])
{
    VERIFY(GetContentType() == ResourceCacheContentType::Signature,
           "This method should be called to initialize the cache to store resources of a pipeline resource signature");

    DEV_CHECK_ERR(NumTables <= MaxRootTables, "The number of root tables (", NumTables, ") exceeds maximum allowed value (", MaxRootTables, ").");

    m_NumTables = static_cast<decltype(m_NumTables)>(NumTables);
    VERIFY_EXPR(m_NumTables == NumTables);

    m_TotalResourceCount = 0;
    for (Uint32 t = 0; t < m_NumTables; ++t)
        m_TotalResourceCount += TableSizes[t];

    m_DescriptorAllocations = 0;

    AllocateMemory(MemAllocator);

    Uint32 ResIdx = 0;
    for (Uint32 t = 0; t < m_NumTables; ++t)
    {
        new (&GetRootTable(t)) RootTable{TableSizes[t], TableSizes[t] > 0 ? &GetResource(ResIdx) : nullptr, false};
        ResIdx += TableSizes[t];
    }
    VERIFY_EXPR(ResIdx == m_TotalResourceCount);
}


void ShaderResourceCacheD3D12::Initialize(IMemoryAllocator&        MemAllocator,
                                          RenderDeviceD3D12Impl*   pDevice,
                                          const RootParamsManager& RootParams)
{
    VERIFY(GetContentType() == ResourceCacheContentType::SRB,
           "This method should be called to initialize the cache to store resources of an SRB");

    const auto MemReq = GetMemoryRequirements(RootParams);

    DEV_CHECK_ERR(MemReq.NumTables <= MaxRootTables, "The number of root tables (", MemReq.NumTables, ") exceeds maximum allowed value (", MaxRootTables, ").");

    m_NumTables = static_cast<decltype(m_NumTables)>(MemReq.NumTables);
    VERIFY_EXPR(m_NumTables == MemReq.NumTables);

    m_TotalResourceCount = MemReq.TotalResources;

    m_NumDescriptorAllocations = static_cast<decltype(m_NumDescriptorAllocations)>(MemReq.NumDescriptorAllocations);
    VERIFY_EXPR(m_NumDescriptorAllocations == MemReq.NumDescriptorAllocations);

    const auto MemSize = AllocateMemory(MemAllocator);
    VERIFY_EXPR(MemSize == MemReq.TotalSize);

#ifdef DILIGENT_DEBUG
    std::vector<bool> RootTableInitFlags(MemReq.NumTables);
#endif

    Uint32 ResIdx = 0;

    // Initialize root tables
    for (Uint32 i = 0; i < RootParams.GetNumRootTables(); ++i)
    {
        const auto& RootTbl = RootParams.GetRootTable(i);
        VERIFY(!RootTableInitFlags[RootTbl.RootIndex], "Root table at index ", RootTbl.RootIndex, " has already been initialized.");
        VERIFY_EXPR(RootTbl.TableOffsetInGroupAllocation != RootParameter::InvalidTableOffsetInGroupAllocation);

        const auto TableSize = RootTbl.GetDescriptorTableSize();
        VERIFY(TableSize > 0, "Unexpected empty descriptor table");

        new (&GetRootTable(RootTbl.RootIndex)) RootTable{
            TableSize,
            &GetResource(ResIdx),
            false, // IsRootView
            RootTbl.TableOffsetInGroupAllocation};
        ResIdx += TableSize;

#ifdef DILIGENT_DEBUG
        RootTableInitFlags[RootTbl.RootIndex] = true;
#endif
    }

    // Initialize one-descriptor tables for root views
    for (Uint32 i = 0; i < RootParams.GetNumRootViews(); ++i)
    {
        const auto& RootView = RootParams.GetRootView(i);
        VERIFY(!RootTableInitFlags[RootView.RootIndex], "Root table at index ", RootView.RootIndex, " has already been initialized.");
        VERIFY_EXPR(RootView.TableOffsetInGroupAllocation == RootParameter::InvalidTableOffsetInGroupAllocation);
        VERIFY_EXPR((RootView.d3d12RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV ||
                     RootView.d3d12RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV ||
                     RootView.d3d12RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV));

        new (&GetRootTable(RootView.RootIndex)) RootTable{
            1,
            &GetResource(ResIdx),
            true //IsRootView
        };
        ++ResIdx;

#ifdef DILIGENT_DEBUG
        RootTableInitFlags[RootView.RootIndex] = true;
#endif
    }
    VERIFY_EXPR(ResIdx == m_TotalResourceCount);

#ifdef DILIGENT_DEBUG
    for (size_t i = 0; i < RootTableInitFlags.size(); ++i)
    {
        VERIFY(RootTableInitFlags[i], "Root table at index ", i, " has not been initialized");
    }
#endif

    // Initialize descriptor heap allocations
    Uint32 AllocationCount = 0;
    for (auto d3d12HeapType : {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER})
    {
        for (Uint32 group = 0; group < ROOT_PARAMETER_GROUP_COUNT; ++group)
        {
            auto GroupType = static_cast<ROOT_PARAMETER_GROUP>(group);

            auto  TotalTableResources = RootParams.GetParameterGroupSize(d3d12HeapType, GroupType);
            auto& AllocationIndex     = m_AllocationIndex[d3d12HeapType][GroupType];
            if (TotalTableResources != 0)
            {
                VERIFY_EXPR(AllocationIndex == -1);
                AllocationIndex  = static_cast<Int8>(AllocationCount++);
                auto& Allocation = m_DescriptorAllocations[AllocationIndex];
                VERIFY_EXPR(Allocation.IsNull());

                if (GroupType == ROOT_PARAMETER_GROUP_STATIC_MUTABLE)
                {
                    // For static/mutable parameters, allocate GPU-visible descriptor space
                    Allocation = pDevice->AllocateGPUDescriptors(d3d12HeapType, TotalTableResources);
                }
                else if (GroupType == ROOT_PARAMETER_GROUP_DYNAMIC)
                {
                    // For dynamic parameters, allocate CPU-only descriptor space
                    Allocation = pDevice->AllocateDescriptors(d3d12HeapType, TotalTableResources);
                }
                else
                {
                    UNEXPECTED("Unexpected root parameter group type");
                }

                DEV_CHECK_ERR(!Allocation.IsNull(),
                              "Failed to allocate ", TotalTableResources, ' ',
                              (GroupType == ROOT_PARAMETER_GROUP_STATIC_MUTABLE ? "GPU-visible" : "CPU-only"), ' ',
                              (d3d12HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? "CBV/SRV/UAV" : "Sampler"),
                              " descriptor(s). Consider increasing ",
                              (GroupType == ROOT_PARAMETER_GROUP_STATIC_MUTABLE ? "GPUDescriptorHeapSize" : "CPUDescriptorHeapSize"),
                              '[', int{d3d12HeapType}, "] in EngineD3D12CreateInfo.");
            }
            else
            {
                AllocationIndex = -1;
            }
        }
    }
    VERIFY_EXPR(AllocationCount == m_NumDescriptorAllocations);
}

ShaderResourceCacheD3D12::~ShaderResourceCacheD3D12()
{
    if (m_pMemory)
    {
        for (Uint32 res = 0; res < m_TotalResourceCount; ++res)
            GetResource(res).~Resource();
        for (Uint32 t = 0; t < m_NumTables; ++t)
            GetRootTable(t).~RootTable();
        for (Uint32 i = 0; i < m_NumDescriptorAllocations; ++i)
            m_DescriptorAllocations[i].~DescriptorHeapAllocation();
    }
}



const ShaderResourceCacheD3D12::Resource& ShaderResourceCacheD3D12::SetResource(Uint32     RootIndex,
                                                                                Uint32     OffsetFromTableStart,
                                                                                Resource&& SrcRes)
{
    auto& Tbl = GetRootTable(RootIndex);
    if (Tbl.IsRootView())
    {
        const BufferD3D12Impl* pBuffer = nullptr;
        if (SrcRes.pObject)
        {
            switch (SrcRes.Type)
            {
                case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
                    pBuffer = SrcRes.pObject.ConstPtr<BufferD3D12Impl>();
                    break;

                case SHADER_RESOURCE_TYPE_BUFFER_SRV:
                case SHADER_RESOURCE_TYPE_BUFFER_UAV:
                    pBuffer = SrcRes.pObject.RawPtr<BufferViewD3D12Impl>()->GetBuffer<const BufferD3D12Impl>();
                    break;

                default:
                    UNEXPECTED("Only constant buffers and buffer SRVs/UAVs can be bound as root views.");
                    break;
            }
        }

        const Uint64 BufferBit = Uint64{1} << Uint64{RootIndex};
        m_DynamicRootBuffersMask &= ~BufferBit;
        m_NonDynamicRootBuffersMask &= ~BufferBit;
        if (pBuffer != nullptr)
        {
            const auto& BuffDesc = pBuffer->GetDesc();

            const auto IsDynamic =
                (BuffDesc.Usage == USAGE_DYNAMIC) ||
                (SrcRes.BufferRangeSize != 0 && SrcRes.BufferRangeSize < BuffDesc.Size);

            (IsDynamic ? m_DynamicRootBuffersMask : m_NonDynamicRootBuffersMask) |= BufferBit;
        }
    }
    else
    {
#ifdef DILIGENT_DEVELOPMENT
        if (GetContentType() == ResourceCacheContentType::SRB)
        {
            const BufferD3D12Impl* pBuffer = nullptr;
            switch (SrcRes.Type)
            {
                case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
                    pBuffer = SrcRes.pObject.ConstPtr<BufferD3D12Impl>();
                    break;

                case SHADER_RESOURCE_TYPE_BUFFER_SRV:
                case SHADER_RESOURCE_TYPE_BUFFER_UAV:
                    pBuffer = SrcRes.pObject.ConstPtr<BufferViewD3D12Impl>()->GetBuffer<const BufferD3D12Impl>();
                    break;

                default:
                    // Do nothing;
                    break;
            }
            if (pBuffer != nullptr && pBuffer->GetDesc().Usage == USAGE_DYNAMIC)
            {
                DEV_CHECK_ERR(pBuffer->GetD3D12Resource() != nullptr, "Dynamic buffers that don't have backing d3d12 resource must be bound as root views");
            }
        }
#endif
    }

    auto& DstRes = Tbl.GetResource(OffsetFromTableStart);

    DstRes = std::move(SrcRes);
    // Make sure dynamic offset is reset
    DstRes.BufferDynamicOffset = 0;

    UpdateRevision();

    return DstRes;
}

void ShaderResourceCacheD3D12::SetBufferDynamicOffset(Uint32 RootIndex,
                                                      Uint32 OffsetFromTableStart,
                                                      Uint32 BufferDynamicOffset)
{
    auto& Tbl = GetRootTable(RootIndex);
    VERIFY(Tbl.IsRootView(), "Dynamic offsets may only be set for root views");
    auto& Res = Tbl.GetResource(OffsetFromTableStart);
    VERIFY_EXPR(Res.Type == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER ||
                Res.Type == SHADER_RESOURCE_TYPE_BUFFER_SRV);
    Res.BufferDynamicOffset = BufferDynamicOffset;
}

const ShaderResourceCacheD3D12::Resource& ShaderResourceCacheD3D12::CopyResource(ID3D12Device*   pd3d12Device,
                                                                                 Uint32          RootIndex,
                                                                                 Uint32          OffsetFromTableStart,
                                                                                 const Resource& SrcRes)
{
    const auto& DstRes = SetResource(RootIndex, OffsetFromTableStart, Resource{SrcRes});

    if (m_ContentType == ResourceCacheContentType::SRB)
    {
        auto& Tbl = GetRootTable(RootIndex);
        if (!Tbl.IsRootView())
        {
            const auto HeapType = DstRes.Type == SHADER_RESOURCE_TYPE_SAMPLER ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

            auto DstDescrHandle = GetDescriptorTableHandle<D3D12_CPU_DESCRIPTOR_HANDLE>(
                HeapType, ROOT_PARAMETER_GROUP_STATIC_MUTABLE, RootIndex, OffsetFromTableStart);
            if (DstRes.CPUDescriptorHandle.ptr != 0)
            {
                pd3d12Device->CopyDescriptorsSimple(1, DstDescrHandle, SrcRes.CPUDescriptorHandle, HeapType);
            }
            else
            {
                VERIFY(DstRes.Type == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, "Null CPU descriptor is only allowed for constant buffers");
                const auto* pBuffer = DstRes.pObject.ConstPtr<BufferD3D12Impl>();
                VERIFY(DstRes.BufferRangeSize < pBuffer->GetDesc().Size, "Null CPU descriptor is only allowed for partial views of constant buffers");
                VERIFY(DstRes.BufferRangeSize < D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16, "Constant buffer range must not exceed 64Kb");
                pBuffer->CreateCBV(DstDescrHandle, DstRes.BufferBaseOffset, DstRes.BufferRangeSize);
            }
        }
    }

    return DstRes;
}


#ifdef DILIGENT_DEBUG
void ShaderResourceCacheD3D12::DbgValidateDynamicBuffersMask() const
{
    VERIFY_EXPR((m_DynamicRootBuffersMask & m_NonDynamicRootBuffersMask) == 0);
    for (Uint32 i = 0; i < GetNumRootTables(); ++i)
    {
        const auto&  Tbl              = GetRootTable(i);
        const Uint64 DynamicBufferBit = Uint64{1} << Uint64{i};
        if (Tbl.IsRootView())
        {
            VERIFY_EXPR(Tbl.GetSize() == 1);
            const auto& Res = Tbl.GetResource(0);

            const BufferD3D12Impl* pBuffer = nullptr;
            if (Res.pObject)
            {
                switch (Res.Type)
                {
                    case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
                        pBuffer = Res.pObject.ConstPtr<BufferD3D12Impl>();
                        break;

                    case SHADER_RESOURCE_TYPE_BUFFER_SRV:
                    case SHADER_RESOURCE_TYPE_BUFFER_UAV:
                        pBuffer = Res.pObject.RawPtr<BufferViewD3D12Impl>()->GetBuffer<const BufferD3D12Impl>();
                        break;

                    default:
                        UNEXPECTED("Only constant buffers and buffer SRVs/UAVs can be bound as root views.");
                        break;
                }
            }

            if (pBuffer != nullptr)
            {
                const auto& BuffDesc = pBuffer->GetDesc();

                const auto IsDynamicBuffer =
                    (BuffDesc.Usage == USAGE_DYNAMIC) ||
                    (Res.BufferRangeSize != 0 && Res.BufferRangeSize < BuffDesc.Size);

                VERIFY(((m_DynamicRootBuffersMask & DynamicBufferBit) != 0) == IsDynamicBuffer,
                       "Incorrect bit set in the dynamic buffer mask");
                VERIFY(((m_NonDynamicRootBuffersMask & DynamicBufferBit) != 0) == !IsDynamicBuffer,
                       "Incorrect bit set in the non-dynamic buffer mask");
            }
            else
            {
                VERIFY((m_DynamicRootBuffersMask & DynamicBufferBit) == 0, "Bit must not be set when there is no buffer.");
                VERIFY((m_NonDynamicRootBuffersMask & DynamicBufferBit) == 0, "Bit must not be set when there is no buffer.");
            }
        }
        else
        {
            VERIFY((m_DynamicRootBuffersMask & DynamicBufferBit) == 0, "Dynamic buffer mask bit may only be set for root views");
            VERIFY((m_NonDynamicRootBuffersMask & DynamicBufferBit) == 0, "Non-dynamic buffer mask bit may only be set for root views");
        }
    }
}
#endif


void ShaderResourceCacheD3D12::Resource::TransitionResource(CommandContext& Ctx)
{
    static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update this function to handle the new resource type");
    switch (Type)
    {
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
        {
            // No need to use QueryInterface() - types are verified when resources are bound
            auto* pBuffToTransition = pObject.RawPtr<BufferD3D12Impl>();
            if (pBuffToTransition->IsInKnownState() && !pBuffToTransition->CheckState(RESOURCE_STATE_CONSTANT_BUFFER))
                Ctx.TransitionResource(*pBuffToTransition, RESOURCE_STATE_CONSTANT_BUFFER);
        }
        break;

        case SHADER_RESOURCE_TYPE_BUFFER_SRV:
        {
            auto* pBuffViewD3D12    = pObject.RawPtr<BufferViewD3D12Impl>();
            auto* pBuffToTransition = pBuffViewD3D12->GetBuffer<BufferD3D12Impl>();
            if (pBuffToTransition->IsInKnownState() && !pBuffToTransition->CheckState(RESOURCE_STATE_SHADER_RESOURCE))
                Ctx.TransitionResource(*pBuffToTransition, RESOURCE_STATE_SHADER_RESOURCE);
        }
        break;

        case SHADER_RESOURCE_TYPE_BUFFER_UAV:
        {
            auto* pBuffViewD3D12    = pObject.RawPtr<BufferViewD3D12Impl>();
            auto* pBuffToTransition = pBuffViewD3D12->GetBuffer<BufferD3D12Impl>();
            if (pBuffToTransition->IsInKnownState())
            {
                // We must always call TransitionResource() even when the state is already
                // RESOURCE_STATE_UNORDERED_ACCESS as in this case UAV barrier must be executed
                Ctx.TransitionResource(*pBuffToTransition, RESOURCE_STATE_UNORDERED_ACCESS);
            }
        }
        break;

        case SHADER_RESOURCE_TYPE_TEXTURE_SRV:
        case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT:
        {
            auto* pTexViewD3D12    = pObject.RawPtr<TextureViewD3D12Impl>();
            auto* pTexToTransition = pTexViewD3D12->GetTexture<TextureD3D12Impl>();
            if (pTexToTransition->IsInKnownState() && !pTexToTransition->CheckAnyState(RESOURCE_STATE_SHADER_RESOURCE | RESOURCE_STATE_INPUT_ATTACHMENT))
                Ctx.TransitionResource(*pTexToTransition, RESOURCE_STATE_SHADER_RESOURCE);
        }
        break;

        case SHADER_RESOURCE_TYPE_TEXTURE_UAV:
        {
            auto* pTexViewD3D12    = pObject.RawPtr<TextureViewD3D12Impl>();
            auto* pTexToTransition = pTexViewD3D12->GetTexture<TextureD3D12Impl>();
            if (pTexToTransition->IsInKnownState())
            {
                // We must always call TransitionResource() even when the state is already
                // RESOURCE_STATE_UNORDERED_ACCESS as in this case UAV barrier must be executed
                Ctx.TransitionResource(*pTexToTransition, RESOURCE_STATE_UNORDERED_ACCESS);
            }
        }
        break;

        case SHADER_RESOURCE_TYPE_SAMPLER:
            // Nothing to transition
            break;

        case SHADER_RESOURCE_TYPE_ACCEL_STRUCT:
        {
            auto* pTlasD3D12 = pObject.RawPtr<TopLevelASD3D12Impl>();
            if (pTlasD3D12->IsInKnownState())
            {
                // We must always call TransitionResource() even when the state is already
                // RESOURCE_STATE_RAY_TRACING because it is treated as UAV
                Ctx.TransitionResource(*pTlasD3D12, RESOURCE_STATE_RAY_TRACING);
            }
        }
        break;

        default:
            // Resource is not bound
            VERIFY(Type == SHADER_RESOURCE_TYPE_UNKNOWN, "Unexpected resource type");
            VERIFY(pObject == nullptr && CPUDescriptorHandle.ptr == 0, "Bound resource is unexpected");
    }
}


#ifdef DILIGENT_DEVELOPMENT
void ShaderResourceCacheD3D12::Resource::DvpVerifyResourceState()
{
    static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update this function to handle the new resource type");
    switch (Type)
    {
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
        {
            // Not using QueryInterface() for the sake of efficiency
            const auto* pBufferD3D12 = pObject.ConstPtr<BufferD3D12Impl>();
            if (pBufferD3D12->IsInKnownState() && !pBufferD3D12->CheckState(RESOURCE_STATE_CONSTANT_BUFFER))
            {
                LOG_ERROR_MESSAGE("Buffer '", pBufferD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_CONSTANT_BUFFER state. Actual state: ",
                                  GetResourceStateString(pBufferD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the buffer state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case SHADER_RESOURCE_TYPE_BUFFER_SRV:
        {
            const auto* pBuffViewD3D12 = pObject.ConstPtr<BufferViewD3D12Impl>();
            const auto* pBufferD3D12   = pBuffViewD3D12->GetBuffer<const BufferD3D12Impl>();
            if (pBufferD3D12->IsInKnownState() && !pBufferD3D12->CheckState(RESOURCE_STATE_SHADER_RESOURCE))
            {
                LOG_ERROR_MESSAGE("Buffer '", pBufferD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_SHADER_RESOURCE state.  Actual state: ",
                                  GetResourceStateString(pBufferD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the buffer state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case SHADER_RESOURCE_TYPE_BUFFER_UAV:
        {
            const auto* pBuffViewD3D12 = pObject.ConstPtr<BufferViewD3D12Impl>();
            const auto* pBufferD3D12   = pBuffViewD3D12->GetBuffer<const BufferD3D12Impl>();
            if (pBufferD3D12->IsInKnownState() && !pBufferD3D12->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
            {
                LOG_ERROR_MESSAGE("Buffer '", pBufferD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_UNORDERED_ACCESS state. Actual state: ",
                                  GetResourceStateString(pBufferD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the buffer state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case SHADER_RESOURCE_TYPE_TEXTURE_SRV:
        case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT:
        {
            const auto* pTexViewD3D12 = pObject.ConstPtr<TextureViewD3D12Impl>();
            const auto* pTexD3D12     = pTexViewD3D12->GetTexture<TextureD3D12Impl>();
            const auto& TexDesc       = pTexD3D12->GetDesc();
            const auto& FmtAttribs    = GetTextureFormatAttribs(TexDesc.Format);

            auto RequiredStates = RESOURCE_STATE_SHADER_RESOURCE | RESOURCE_STATE_INPUT_ATTACHMENT;
            if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH || FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            {
                RequiredStates |= RESOURCE_STATE_DEPTH_READ;
            }
            if (pTexD3D12->IsInKnownState() && !pTexD3D12->CheckAnyState(RequiredStates))
            {
                LOG_ERROR_MESSAGE("Texture '", pTexD3D12->GetDesc().Name, "' must be in one of ", GetResourceStateString(RequiredStates), " states. Actual state: ",
                                  GetResourceStateString(pTexD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the texture state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case SHADER_RESOURCE_TYPE_TEXTURE_UAV:
        {
            const auto* pTexViewD3D12 = pObject.ConstPtr<TextureViewD3D12Impl>();
            const auto* pTexD3D12     = pTexViewD3D12->GetTexture<const TextureD3D12Impl>();
            if (pTexD3D12->IsInKnownState() && !pTexD3D12->CheckState(RESOURCE_STATE_UNORDERED_ACCESS))
            {
                LOG_ERROR_MESSAGE("Texture '", pTexD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_UNORDERED_ACCESS state. Actual state: ",
                                  GetResourceStateString(pTexD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the texture state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        case SHADER_RESOURCE_TYPE_SAMPLER:
            // No resource
            break;

        case SHADER_RESOURCE_TYPE_ACCEL_STRUCT:
        {
            const auto* pTLASD3D12 = pObject.ConstPtr<TopLevelASD3D12Impl>();
            if (pTLASD3D12->IsInKnownState() && !pTLASD3D12->CheckState(RESOURCE_STATE_RAY_TRACING))
            {
                LOG_ERROR_MESSAGE("TLAS '", pTLASD3D12->GetDesc().Name, "' must be in RESOURCE_STATE_RAY_TRACING state.  Actual state: ",
                                  GetResourceStateString(pTLASD3D12->GetState()),
                                  ". Call IDeviceContext::TransitionShaderResources(), use RESOURCE_STATE_TRANSITION_MODE_TRANSITION "
                                  "when calling IDeviceContext::CommitShaderResources() or explicitly transition the TLAS state "
                                  "with IDeviceContext::TransitionResourceStates().");
            }
        }
        break;

        default:
            // Resource is not bound
            VERIFY(Type == SHADER_RESOURCE_TYPE_UNKNOWN, "Unexpected resource type");
            VERIFY(pObject == nullptr && CPUDescriptorHandle.ptr == 0, "Bound resource is unexpected");
    }
}
#endif // DILIGENT_DEVELOPMENT

void ShaderResourceCacheD3D12::TransitionResourceStates(CommandContext& Ctx, StateTransitionMode Mode)
{
    for (Uint32 r = 0; r < m_TotalResourceCount; ++r)
    {
        auto& Res = GetResource(r);
        switch (Mode)
        {
            case StateTransitionMode::Transition:
                Res.TransitionResource(Ctx);
                break;

            case StateTransitionMode::Verify:
#ifdef DILIGENT_DEVELOPMENT
                Res.DvpVerifyResourceState();
#endif
                break;

            default:
                UNEXPECTED("Unexpected mode");
        }
    }
}

} // namespace Diligent
