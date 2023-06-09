/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
/// Declaration of Diligent::ShaderResourceCacheD3D12 class


// Shader resource cache stores D3D12 resources in a continuous chunk of memory:
//
//
//                                         __________________________________________________________
//  m_pMemory                             |             m_pResources, m_NumResources == m            |
//  |                                     |                                                          |
//  V                                     |                                                          V
//  |  RootTable[0]  |   ....    |  RootTable[Nrt-1]  |  Res[0]  |  ... |  Res[n-1]  |    ....     | Res[0]  |  ... |  Res[m-1]  |  DescriptorHeapAllocation[0]  |  ...
//       |                                                A \
//       |                                                |  \
//       |________________________________________________|   \RefCntAutoPtr
//                    m_pResources, m_NumResources == n        \_________
//                                                             |  Object |
//                                                              ---------
//
//  Nrt = m_NumTables
//
//
// The cache is also assigned descriptor heap space to store descriptor handles.
// Static and mutable table resources are stored in shader-visible heap.
// Dynamic table resources are stored in CPU-only heap.
// Root views are not assigned descriptor space.
//
//
//      DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
//  |   DescrptHndl[0]  ...  DescrptHndl[n-1]   |  DescrptHndl[0]  ...  DescrptHndl[m-1] |
//          A                                           A
//          |                                           |
//          | TableStartOffset                          | TableStartOffset
//          |                                           |
//   |    RootTable[0]    |    RootTable[1]    |    RootTable[2]    |     ....      |   RootTable[Nrt]   |
//                              |                                                           |
//                              | TableStartOffset                                          | InvalidDescriptorOffset
//                              |                                                           |
//                              V                                                           V
//                      |   DescrptHndl[0]  ...  DescrptHndl[n-1]   |                       X
//                       DESCRIPTOR_HEAP_TYPE_SAMPLER
//
//
//
// The allocation is inexed by the offset from the beginning of the root table.
// Each root table is assigned the space to store exactly m_NumResources resources.
//
//
//
//   |      RootTable[i]       |       Res[0]      ...       Res[n-1]      |
//                      \
//       TableStartOffset\____
//                            \
//                             V
//                 .....       |   DescrptHndl[0]  ...  DescrptHndl[n-1]   |    ....
//
//
//
// The cache stores resources for both root tables and root views.
// Resources of root views are treated as single-descriptor tables
// Example:
//
//   Root Index |  Is Root View  | Num Resources
//      0       |      No        |    1+
//      1       |      Yes       |    1
//      2       |      Yes       |    1
//      3       |      No        |    1+
//      4       |      Yes       |    1+
//      5       |      Yes       |    1+
//      6       |      No        |    1
//      7       |      No        |    1
//      8       |      Yes       |    1+
// Note that resource cache that is used by signature may contain empty tables.


#include <array>
#include <memory>

#include "Shader.h"
#include "DescriptorHeap.hpp"
#include "RootParamsManager.hpp"
#include "ShaderResourceCacheCommon.hpp"
#include "BufferViewD3D12Impl.hpp"

namespace Diligent
{

class CommandContext;
class RenderDeviceD3D12Impl;

class ShaderResourceCacheD3D12 : public ShaderResourceCacheBase
{
public:
    explicit ShaderResourceCacheD3D12(ResourceCacheContentType ContentType) noexcept :
        m_ContentType{ContentType}
    {
        for (auto& HeapIndex : m_AllocationIndex)
            HeapIndex.fill(-1);
    }

    // clang-format off
    ShaderResourceCacheD3D12             (const ShaderResourceCacheD3D12&)  = delete;
    ShaderResourceCacheD3D12             (      ShaderResourceCacheD3D12&&) = delete;
    ShaderResourceCacheD3D12& operator = (const ShaderResourceCacheD3D12&)  = delete;
    ShaderResourceCacheD3D12& operator = (      ShaderResourceCacheD3D12&&) = delete;
    // clang-format on

    ~ShaderResourceCacheD3D12();

    struct MemoryRequirements
    {
        Uint32 NumTables                = 0;
        Uint32 TotalResources           = 0;
        Uint32 NumDescriptorAllocations = 0;
        size_t TotalSize                = 0;
    };
    static MemoryRequirements GetMemoryRequirements(const RootParamsManager& RootParams);

    // Initializes resource cache to hold the given number of root tables, no descriptor space
    // is allocated (this is used to initialize the cache for a pipeline resource signature).
    void Initialize(IMemoryAllocator& MemAllocator,
                    Uint32            NumTables,
                    const Uint32      TableSizes[]);

    // Initializes resource cache to hold the resources of a root parameters manager
    // (this is used to initialize the cache for an SRB).
    void Initialize(IMemoryAllocator&        MemAllocator,
                    RenderDeviceD3D12Impl*   pDevice,
                    const RootParamsManager& RootParams);

    static constexpr Uint32 InvalidDescriptorOffset = ~0u;

    struct Resource
    {
        Resource() noexcept {}

        Resource(SHADER_RESOURCE_TYPE           _Type,
                 D3D12_CPU_DESCRIPTOR_HANDLE    _CPUDescriptorHandle,
                 RefCntAutoPtr<IDeviceObject>&& _pObject,
                 Uint64                         _BufferBaseOffset = 0,
                 Uint64                         _BufferRangeSize  = 0) :
            // clang-format off
            Type{_Type},
            CPUDescriptorHandle{_CPUDescriptorHandle},
            pObject            {std::move(_pObject) },
            BufferBaseOffset   {_BufferBaseOffset   },
            BufferRangeSize    {_BufferRangeSize    }
        // clang-format on
        {
            VERIFY(Type == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER || (BufferBaseOffset == 0 && BufferRangeSize == 0),
                   "Buffer range may only be specified for constant buffers");
            if (pObject && (Type == SHADER_RESOURCE_TYPE_BUFFER_SRV || Type == SHADER_RESOURCE_TYPE_BUFFER_UAV))
            {
                const auto& BuffViewDesc{pObject.ConstPtr<BufferViewD3D12Impl>()->GetDesc()};
                BufferBaseOffset = BuffViewDesc.ByteOffset;
                BufferRangeSize  = BuffViewDesc.ByteWidth;
            }
        }

        SHADER_RESOURCE_TYPE Type = SHADER_RESOURCE_TYPE_UNKNOWN;

        Uint64 BufferDynamicOffset = 0;

        // CPU descriptor handle of a cached resource in CPU-only descriptor heap.
        // This handle may be null for CBVs that address the buffer range.
        D3D12_CPU_DESCRIPTOR_HANDLE  CPUDescriptorHandle = {};
        RefCntAutoPtr<IDeviceObject> pObject;

        Uint64 BufferBaseOffset = 0;
        Uint64 BufferRangeSize  = 0;

        bool IsNull() const { return pObject == nullptr; }

        // Transitions resource to the shader resource state required by Type member.
        __forceinline void TransitionResource(CommandContext& Ctx);

#ifdef DILIGENT_DEVELOPMENT
        // Verifies that resource is in correct shader resource state required by Type member.
        void DvpVerifyResourceState();
#endif
    };

    class RootTable
    {
    public:
        RootTable(Uint32    _NumResources,
                  Resource* _pResources,
                  bool      _IsRootView,
                  Uint32    _TableStartOffset = InvalidDescriptorOffset) noexcept :
            // clang-format off
            m_NumResources    {_NumResources        },
            m_IsRootView      {_IsRootView ? 1u : 0u},
            m_pResources      {_pResources          },
            m_TableStartOffset{_TableStartOffset    }
        // clang-format on
        {
            VERIFY_EXPR(GetSize() == _NumResources);
            VERIFY_EXPR(IsRootView() == _IsRootView);
            VERIFY(!IsRootView() || GetSize() == 1, "Root views may only contain one resource");
        }

        const Resource& GetResource(Uint32 OffsetFromTableStart) const
        {
            VERIFY(OffsetFromTableStart < m_NumResources, "Root table is not large enough to store descriptor at offset ", OffsetFromTableStart);
            return m_pResources[OffsetFromTableStart];
        }

        Uint32 GetSize() const { return m_NumResources; }
        Uint32 GetStartOffset() const { return m_TableStartOffset; }
        bool   IsRootView() const { return m_IsRootView != 0; }

    private:
        friend class ShaderResourceCacheD3D12;
        Resource& GetResource(Uint32 OffsetFromTableStart)
        {
            VERIFY(OffsetFromTableStart < m_NumResources, "Root table is not large enough to store descriptor at offset ", OffsetFromTableStart);
            return m_pResources[OffsetFromTableStart];
        }

    private:
        // Offset from the start of the descriptor heap allocation to the start of the table
        const Uint32 m_TableStartOffset;

        // The total number of resources in the table, accounting for array sizes
        const Uint32 m_NumResources : 31;

        // Flag indicating if this table stores the resource of a root view
        const Uint32 m_IsRootView : 1;

        Resource* const m_pResources;
    };

    // Sets the resource at the given root index and offset from the table start
    const Resource& SetResource(Uint32     RootIndex,
                                Uint32     OffsetFromTableStart,
                                Resource&& SrcRes);

    // Copies the resource to the given root index and offset from the table start
    const Resource& CopyResource(ID3D12Device*   pd3d12Device,
                                 Uint32          RootIndex,
                                 Uint32          OffsetFromTableStart,
                                 const Resource& SrcRes);

    // Resets the resource at the given root index and offset from the table start to default state
    const Resource& ResetResource(Uint32 RootIndex,
                                  Uint32 OffsetFromTableStart)
    {
        return SetResource(RootIndex, OffsetFromTableStart, {});
    }

    void SetBufferDynamicOffset(Uint32 RootIndex,
                                Uint32 OffsetFromTableStart,
                                Uint32 BufferDynamicOffset);

    const RootTable& GetRootTable(Uint32 RootIndex) const
    {
        VERIFY_EXPR(RootIndex < m_NumTables);
        return reinterpret_cast<const RootTable*>(m_pMemory.get())[RootIndex];
    }

    Uint32 GetNumRootTables() const { return m_NumTables; }

    ID3D12DescriptorHeap* GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, ROOT_PARAMETER_GROUP Group) const
    {
        auto AllocationIdx = m_AllocationIndex[HeapType][Group];
        return AllocationIdx >= 0 ? m_DescriptorAllocations[AllocationIdx].GetDescriptorHeap() : nullptr;
    }

    // Returns CPU/GPU descriptor handle of a descriptor heap allocation
    template <typename HandleType>
    HandleType GetDescriptorTableHandle(
        D3D12_DESCRIPTOR_HEAP_TYPE HeapType,
        ROOT_PARAMETER_GROUP       Group,
        Uint32                     RootParamInd,
        Uint32                     OffsetFromTableStart = 0) const
    {
        const auto& RootParam = GetRootTable(RootParamInd);
        VERIFY(RootParam.GetStartOffset() != InvalidDescriptorOffset, "This root parameter is not assigned a valid descriptor table offset");
        VERIFY(OffsetFromTableStart < RootParam.GetSize(), "Offset is out of range");

        const auto AllocationIdx = m_AllocationIndex[HeapType][Group];
        VERIFY(AllocationIdx >= 0, "Descriptor space is not assigned to this table");
        VERIFY_EXPR(AllocationIdx < m_NumDescriptorAllocations);

        return m_DescriptorAllocations[AllocationIdx].GetHandle<HandleType>(RootParam.GetStartOffset() + OffsetFromTableStart);
    }

    const DescriptorHeapAllocation& GetDescriptorAllocation(D3D12_DESCRIPTOR_HEAP_TYPE HeapType,
                                                            ROOT_PARAMETER_GROUP       Group) const
    {
        const auto AllocationIdx = m_AllocationIndex[HeapType][Group];
        VERIFY(AllocationIdx >= 0, "Descriptor space is not assigned to this combination of heap type and parameter group");
        VERIFY_EXPR(AllocationIdx < m_NumDescriptorAllocations);
        return m_DescriptorAllocations[AllocationIdx];
    }

    enum class StateTransitionMode
    {
        Transition,
        Verify
    };
    // Transitions all resources in the cache
    void TransitionResourceStates(CommandContext& Ctx, StateTransitionMode Mode);

    ResourceCacheContentType GetContentType() const { return m_ContentType; }

    // Returns the bitmask indicating root views with bound dynamic buffers (including buffer ranges)
    Uint64 GetDynamicRootBuffersMask() const { return m_DynamicRootBuffersMask; }

    // Returns the bitmask indicating root views with bound non-dynamic buffers
    Uint64 GetNonDynamicRootBuffersMask() const { return m_NonDynamicRootBuffersMask; }

    // Returns true if the cache contains at least one dynamic resource, i.e.
    // dynamic buffer or a buffer range.
    bool HasDynamicResources() const { return GetDynamicRootBuffersMask() != 0; }

#ifdef DILIGENT_DEBUG
    void DbgValidateDynamicBuffersMask() const;
#endif

private:
    RootTable& GetRootTable(Uint32 RootIndex)
    {
        VERIFY_EXPR(RootIndex < m_NumTables);
        return reinterpret_cast<RootTable*>(m_pMemory.get())[RootIndex];
    }

    Resource& GetResource(Uint32 Idx)
    {
        VERIFY_EXPR(Idx < m_TotalResourceCount);
        return reinterpret_cast<Resource*>(reinterpret_cast<RootTable*>(m_pMemory.get()) + m_NumTables)[Idx];
    }

    size_t AllocateMemory(IMemoryAllocator& MemAllocator);

private:
    static constexpr Uint32 MaxRootTables = 64;

    std::unique_ptr<void, STDDeleter<void, IMemoryAllocator>> m_pMemory;

    // Descriptor heap allocations, indexed by m_AllocationIndex
    DescriptorHeapAllocation* m_DescriptorAllocations = nullptr;

    // The total number of resources in all descriptor tables
    Uint32 m_TotalResourceCount = 0;

    // The number of descriptor tables in the cache
    Uint16 m_NumTables = 0;

    // The number of descriptor heap allocations
    Uint8 m_NumDescriptorAllocations = 0;

    // Indicates what types of resources are stored in the cache
    const ResourceCacheContentType m_ContentType;

    // Descriptor allocation index in m_DescriptorAllocations array for every descriptor heap type
    // (CBV_SRV_UAV, SAMPLER) and GPU visibility (false, true).
    // -1 indicates no allocation.
    std::array<std::array<Int8, ROOT_PARAMETER_GROUP_COUNT>, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER + 1> m_AllocationIndex{};

    // The bitmask indicating root views with bound dynamic buffers (including buffer ranges)
    Uint64 m_DynamicRootBuffersMask = Uint64{0};

    // The bitmask indicating root views with bound non-dynamic buffers
    Uint64 m_NonDynamicRootBuffersMask = Uint64{0};
};

} // namespace Diligent
