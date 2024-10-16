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
/// Declaration of Diligent::ShaderResourceCacheWebGPU class

#include "ShaderResourceCacheCommon.hpp"
#include "PipelineResourceAttribsWebGPU.hpp"
#include "STDAllocator.hpp"
#include "WebGPUObjectWrappers.hpp"
#include "IndexWrapper.hpp"

namespace Diligent
{

struct IMemoryAllocator;

class ShaderResourceCacheWebGPU : public ShaderResourceCacheBase
{
public:
    ShaderResourceCacheWebGPU(ResourceCacheContentType ContentType) noexcept;

    // clang-format off
    ShaderResourceCacheWebGPU           (const ShaderResourceCacheWebGPU&)  = delete;
    ShaderResourceCacheWebGPU& operator=(const ShaderResourceCacheWebGPU&)  = delete;
    ShaderResourceCacheWebGPU           (      ShaderResourceCacheWebGPU&&) = delete;
    ShaderResourceCacheWebGPU& operator=(      ShaderResourceCacheWebGPU&&) = delete;
    // clang-format on

    ~ShaderResourceCacheWebGPU();

    static size_t GetRequiredMemorySize(Uint32 NumGroups, const Uint32* GroupSizes);

    void InitializeGroups(IMemoryAllocator& MemAllocator, Uint32 NumGroups, const Uint32* GroupSizes);
    void InitializeResources(Uint32 GroupIdx, Uint32 Offset, Uint32 ArraySize, BindGroupEntryType Type, bool HasImmutableSampler);

    struct Resource
    {
        explicit Resource(BindGroupEntryType _Type, bool _HasImmutableSampler) noexcept :
            Type{_Type},
            HasImmutableSampler{_HasImmutableSampler}
        {
            VERIFY(Type == BindGroupEntryType::Texture || Type == BindGroupEntryType::Sampler || !HasImmutableSampler,
                   "Immutable sampler can only be assigned to a textre or a sampler");
        }

        // clang-format off
        Resource           (const Resource&)  = delete;
        Resource           (      Resource&&) = delete;
        Resource& operator=(const Resource&)  = delete;
        Resource& operator=(      Resource&&) = delete;

/* 0 */ const BindGroupEntryType     Type;
/* 1 */ const bool                   HasImmutableSampler;
/*2-3*/ // Unused
/* 4 */ Uint32                       BufferDynamicOffset = 0;
/* 8 */ RefCntAutoPtr<IDeviceObject> pObject;

        // For uniform and storage buffers only
/*16 */ Uint64                       BufferBaseOffset = 0;
/*24 */ Uint64                       BufferRangeSize  = 0;
        // clang-format on

        void SetUniformBuffer(RefCntAutoPtr<IDeviceObject>&& _pBuffer, Uint64 _RangeOffset, Uint64 _RangeSize);
        void SetStorageBuffer(RefCntAutoPtr<IDeviceObject>&& _pBufferView);

        template <typename ResType>
        Uint32 GetDynamicBufferOffset(DeviceContextIndex CtxId) const;

        explicit operator bool() const { return pObject != nullptr; }
    };

    class BindGroup
    {
    public:
        // clang-format off
        BindGroup(Uint32 NumResources, Resource* pResources, WGPUBindGroupEntry* pwgpuEntries) :
            m_NumResources{NumResources},
            m_pResources  {pResources  },
            m_wgpuEntries {pwgpuEntries}
        {}

        BindGroup           (const BindGroup&)  = delete;
        BindGroup           (      BindGroup&&) = delete;
        BindGroup& operator=(const BindGroup&)  = delete;
        BindGroup& operator=(      BindGroup&&) = delete;
        // clang-format on

        const Resource& GetResource(Uint32 CacheOffset) const
        {
            VERIFY(CacheOffset < m_NumResources, "Offset ", CacheOffset, " is out of range");
            return m_pResources[CacheOffset];
        }

        Uint32 GetSize() const { return m_NumResources; }

        WGPUBindGroup GetWGPUBindGroup() const
        {
            return m_wgpuBindGroup;
        }

    private:
        /* 0 */ const Uint32              m_NumResources = 0;
        /* 5*/ bool                       m_IsDirty      = true;
        /* 8 */ Resource* const           m_pResources   = nullptr;
        /*16 */ WGPUBindGroupEntry* const m_wgpuEntries  = nullptr;
        /*24 */ WebGPUBindGroupWrapper    m_wgpuBindGroup;
        /*40 */ // End of structure

    private:
        friend ShaderResourceCacheWebGPU;
        Resource& GetResource(Uint32 CacheOffset)
        {
            VERIFY(CacheOffset < m_NumResources, "Offset ", CacheOffset, " is out of range");
            return m_pResources[CacheOffset];
        }
        WGPUBindGroupEntry& GetWGPUEntry(Uint32 CacheOffset)
        {
            VERIFY(CacheOffset < m_NumResources, "Offset ", CacheOffset, " is out of range");
            return m_wgpuEntries[CacheOffset];
        }
    };

    const BindGroup& GetBindGroup(Uint32 Index) const
    {
        VERIFY_EXPR(Index < m_NumBindGroups);
        return reinterpret_cast<const BindGroup*>(m_pMemory.get())[Index];
    }

    // Sets the resource at the given descriptor set index and offset
    const Resource& SetResource(Uint32                       BindGroupIdx,
                                Uint32                       CacheOffset,
                                RefCntAutoPtr<IDeviceObject> pObject,
                                Uint64                       BufferBaseOffset = 0,
                                Uint64                       BufferRangeSize  = 0);

    const Resource& ResetResource(Uint32 SetIndex,
                                  Uint32 Offset)
    {
        return SetResource(SetIndex, Offset, {});
    }

    void SetDynamicBufferOffset(Uint32 DescrSetIndex,
                                Uint32 CacheOffset,
                                Uint32 DynamicBufferOffset);

    Uint32 GetNumBindGroups() const { return m_NumBindGroups; }
    bool   HasDynamicResources() const { return m_NumDynamicBuffers > 0; }

    ResourceCacheContentType GetContentType() const { return static_cast<ResourceCacheContentType>(m_ContentType); }

    WGPUBindGroup UpdateBindGroup(WGPUDevice wgpuDevice, Uint32 GroupIndex, WGPUBindGroupLayout wgpuGroupLayout);

    // Returns true if any dynamic offset has changed
    bool GetDynamicBufferOffsets(DeviceContextIndex     CtxId,
                                 std::vector<uint32_t>& Offsets,
                                 Uint32                 GroupIdx) const;

#ifdef DILIGENT_DEBUG
    // For debug purposes only
    void DbgVerifyResourceInitialization() const;
    void DbgVerifyDynamicBuffersCounter() const;
#endif

private:
#ifdef DILIGENT_DEBUG
    const Resource* GetFirstResourcePtr() const
    {
        return reinterpret_cast<const Resource*>(reinterpret_cast<const BindGroup*>(m_pMemory.get()) + m_NumBindGroups);
    }
#endif
    Resource* GetFirstResourcePtr()
    {
        return reinterpret_cast<Resource*>(reinterpret_cast<BindGroup*>(m_pMemory.get()) + m_NumBindGroups);
    }

    BindGroup& GetBindGroup(Uint32 Index)
    {
        VERIFY_EXPR(Index < m_NumBindGroups);
        return reinterpret_cast<BindGroup*>(m_pMemory.get())[Index];
    }

private:
    std::unique_ptr<void, STDDeleter<void, IMemoryAllocator>> m_pMemory;

    Uint16 m_NumBindGroups = 0;

    // The total actual number of dynamic buffers (that were created with USAGE_DYNAMIC) bound in the resource cache
    // regardless of the variable type.
    Uint16 m_NumDynamicBuffers = 0;
    Uint32 m_TotalResources : 31;

    // Indicates what types of resources are stored in the cache
    const Uint32 m_ContentType : 1;

#ifdef DILIGENT_DEBUG
    // Debug array that stores flags indicating if resources in the cache have been initialized
    std::vector<std::vector<bool>> m_DbgInitializedResources;
#endif
};

} // namespace Diligent
