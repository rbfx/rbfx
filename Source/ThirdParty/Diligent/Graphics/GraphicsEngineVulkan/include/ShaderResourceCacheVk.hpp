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
/// Declaration of Diligent::ShaderResourceCacheVk class

// Shader resource cache stores Vk resources in a continuous chunk of memory:
//
//                                 |Vulkan Descriptor Set|
//                                           A    ___________________________________________________________
//  m_pMemory                                |   |              m_pResources, m_NumResources == m            |
//  |               m_DescriptorSetAllocation|   |                                                           |
//  V                                        |   |                                                           V
//  |  DescriptorSet[0]  |   ....    |  DescriptorSet[Ns-1]  |  Res[0]  |  ... |  Res[n-1]  |    ....     | Res[0]  |  ... |  Res[m-1]  |
//         |    |                                                A \
//         |    |                                                |  \
//         |    |________________________________________________|   \RefCntAutoPtr
//         |             m_pResources, m_NumResources == n            \_________
//         |                                                          |  Object |
//         | m_DescriptorSetAllocation                                 ---------
//         V
//  |Vulkan Descriptor Set|
//
//  Ns = m_NumSets
//
//
// Descriptor set for static and mutable resources is assigned during cache initialization
// Descriptor set for dynamic resources is assigned at every draw call

#include <vector>
#include <memory>

#include "DescriptorPoolManager.hpp"
#include "SPIRVShaderResources.hpp"
#include "BufferVkImpl.hpp"
#include "ShaderResourceCacheCommon.hpp"
#include "PipelineResourceAttribsVk.hpp"
#include "VulkanUtilities/VulkanLogicalDevice.hpp"

namespace Diligent
{

class DeviceContextVkImpl;

// sizeof(ShaderResourceCacheVk) == 24 (x64, msvc, Release)
class ShaderResourceCacheVk : public ShaderResourceCacheBase
{
public:
    explicit ShaderResourceCacheVk(ResourceCacheContentType ContentType) noexcept :
        m_TotalResources{0},
        m_ContentType{static_cast<Uint32>(ContentType)}
    {
        VERIFY_EXPR(GetContentType() == ContentType);
    }

    // clang-format off
    ShaderResourceCacheVk             (const ShaderResourceCacheVk&) = delete;
    ShaderResourceCacheVk             (ShaderResourceCacheVk&&)      = delete;
    ShaderResourceCacheVk& operator = (const ShaderResourceCacheVk&) = delete;
    ShaderResourceCacheVk& operator = (ShaderResourceCacheVk&&)      = delete;
    // clang-format on

    ~ShaderResourceCacheVk();

    static size_t GetRequiredMemorySize(Uint32 NumSets, const Uint32* SetSizes);

    void InitializeSets(IMemoryAllocator& MemAllocator, Uint32 NumSets, const Uint32* SetSizes);
    void InitializeResources(Uint32 Set, Uint32 Offset, Uint32 ArraySize, DescriptorType Type, bool HasImmutableSampler);

    // sizeof(Resource) == 32 (x64, msvc, Release)
    struct Resource
    {
        explicit Resource(DescriptorType _Type, bool _HasImmutableSampler) noexcept :
            Type{_Type},
            HasImmutableSampler{_HasImmutableSampler}
        {
            VERIFY(Type == DescriptorType::CombinedImageSampler || Type == DescriptorType::Sampler || !HasImmutableSampler,
                   "Immutable sampler can only be assigned to a combined image sampler or a separate sampler");
        }

        // clang-format off
        Resource             (const Resource&) = delete;
        Resource             (Resource&&)      = delete;
        Resource& operator = (const Resource&) = delete;
        Resource& operator = (Resource&&)      = delete;

/* 0 */ const DescriptorType         Type;
/* 1 */ const bool                   HasImmutableSampler;
/*2-3*/ // Unused
/* 4 */ Uint32                       BufferDynamicOffset = 0;
/* 8 */ RefCntAutoPtr<IDeviceObject> pObject;

        // For uniform and storage buffers only
/*16 */ Uint64                       BufferBaseOffset = 0;
/*24 */ Uint64                       BufferRangeSize  = 0;

        VkDescriptorBufferInfo GetUniformBufferDescriptorWriteInfo()                     const;
        VkDescriptorBufferInfo GetStorageBufferDescriptorWriteInfo()                     const;
        VkDescriptorImageInfo  GetImageDescriptorWriteInfo  ()                           const;
        VkBufferView           GetBufferViewWriteInfo       ()                           const;
        VkDescriptorImageInfo  GetSamplerDescriptorWriteInfo()                           const;
        VkDescriptorImageInfo  GetInputAttachmentDescriptorWriteInfo()                   const;
        VkWriteDescriptorSetAccelerationStructureKHR GetAccelerationStructureWriteInfo() const;
        // clang-format on

        template <DescriptorType DescrType>
        auto GetDescriptorWriteInfo() const;

        void SetUniformBuffer(RefCntAutoPtr<IDeviceObject>&& _pBuffer, Uint64 _RangeOffset, Uint64 _RangeSize);
        void SetStorageBuffer(RefCntAutoPtr<IDeviceObject>&& _pBufferView);

        bool IsNull() const { return pObject == nullptr; }

        explicit operator bool() const { return !IsNull(); }
    };

    // sizeof(DescriptorSet) == 48 (x64, msvc, Release)
    class DescriptorSet
    {
    public:
        // clang-format off
        DescriptorSet(Uint32 NumResources, Resource *pResources) :
            m_NumResources  {NumResources},
            m_pResources    {pResources  }
        {}

        DescriptorSet             (const DescriptorSet&) = delete;
        DescriptorSet             (DescriptorSet&&)      = delete;
        DescriptorSet& operator = (const DescriptorSet&) = delete;
        DescriptorSet& operator = (DescriptorSet&&)      = delete;
        // clang-format on

        const Resource& GetResource(Uint32 CacheOffset) const
        {
            VERIFY(CacheOffset < m_NumResources, "Offset ", CacheOffset, " is out of range");
            return m_pResources[CacheOffset];
        }

        Uint32 GetSize() const { return m_NumResources; }

        VkDescriptorSet GetVkDescriptorSet() const
        {
            return m_DescriptorSetAllocation.GetVkDescriptorSet();
        }

        // clang-format off
/* 0 */ const Uint32 m_NumResources = 0;
    private:
/* 8 */ Resource* const m_pResources = nullptr;
/*16 */ DescriptorSetAllocation m_DescriptorSetAllocation;
/*48 */ // End of structure
        // clang-format on

    private:
        friend ShaderResourceCacheVk;
        Resource& GetResource(Uint32 CacheOffset)
        {
            VERIFY(CacheOffset < m_NumResources, "Offset ", CacheOffset, " is out of range");
            return m_pResources[CacheOffset];
        }
    };

    const DescriptorSet& GetDescriptorSet(Uint32 Index) const
    {
        VERIFY_EXPR(Index < m_NumSets);
        return reinterpret_cast<const DescriptorSet*>(m_pMemory.get())[Index];
    }

    void AssignDescriptorSetAllocation(Uint32 SetIndex, DescriptorSetAllocation&& Allocation)
    {
        auto& DescrSet = GetDescriptorSet(SetIndex);
        VERIFY(DescrSet.GetSize() > 0, "Descriptor set is empty");
        VERIFY(!DescrSet.m_DescriptorSetAllocation, "Descriptor set allocation has already been initialized");
        DescrSet.m_DescriptorSetAllocation = std::move(Allocation);
    }

    struct SetResourceInfo
    {
        const Uint32 BindingIndex = 0;
        const Uint32 ArrayIndex   = 0;

        RefCntAutoPtr<IDeviceObject> pObject;

        const Uint64 BufferBaseOffset = 0;
        const Uint64 BufferRangeSize  = 0;

        SetResourceInfo() noexcept
        {
        }

        SetResourceInfo(Uint32                         _BindingIndex,
                        Uint32                         _ArrayIndex,
                        RefCntAutoPtr<IDeviceObject>&& _pObject,
                        Uint64                         _BufferBaseOffset = 0,
                        Uint64                         _BufferRangeSize  = 0) noexcept :
            // clang-format off
            BindingIndex    {_BindingIndex      },
            ArrayIndex      {_ArrayIndex        },
            pObject         {std::move(_pObject)},
            BufferBaseOffset{_BufferBaseOffset  },
            BufferRangeSize {_BufferRangeSize   }
        // clang-format on
        {
        }
    };
    // Sets the resource at the given descriptor set index and offset
    const Resource& SetResource(const VulkanUtilities::VulkanLogicalDevice* pLogicalDevice,
                                Uint32                                      DescrSetIndex,
                                Uint32                                      CacheOffset,
                                SetResourceInfo&&                           SrcRes);

    const Resource& ResetResource(Uint32 SetIndex,
                                  Uint32 Offset)
    {
        return SetResource(nullptr, SetIndex, Offset, {});
    }

    void SetDynamicBufferOffset(Uint32 DescrSetIndex,
                                Uint32 CacheOffset,
                                Uint32 DynamicBufferOffset);


    Uint32 GetNumDescriptorSets() const { return m_NumSets; }
    bool   HasDynamicResources() const { return m_NumDynamicBuffers > 0; }

    ResourceCacheContentType GetContentType() const { return static_cast<ResourceCacheContentType>(m_ContentType); }

#ifdef DILIGENT_DEBUG
    // Only for debug purposes: indicates what types of resources are stored in the cache
    void DbgVerifyResourceInitialization() const;
    void DbgVerifyDynamicBuffersCounter() const;
#endif

    template <bool VerifyOnly>
    void TransitionResources(DeviceContextVkImpl* pCtxVkImpl);

    __forceinline Uint32 GetDynamicBufferOffsets(DeviceContextIndex     CtxId,
                                                 std::vector<uint32_t>& Offsets,
                                                 Uint32                 StartInd) const;

private:
    Resource* GetFirstResourcePtr()
    {
        return reinterpret_cast<Resource*>(reinterpret_cast<DescriptorSet*>(m_pMemory.get()) + m_NumSets);
    }
    const Resource* GetFirstResourcePtr() const
    {
        return reinterpret_cast<const Resource*>(reinterpret_cast<const DescriptorSet*>(m_pMemory.get()) + m_NumSets);
    }

    DescriptorSet& GetDescriptorSet(Uint32 Index)
    {
        VERIFY_EXPR(Index < m_NumSets);
        return reinterpret_cast<DescriptorSet*>(m_pMemory.get())[Index];
    }

    std::unique_ptr<void, STDDeleter<void, IMemoryAllocator>> m_pMemory;

    Uint16 m_NumSets = 0;

    // Total actual number of dynamic buffers (that were created with USAGE_DYNAMIC) bound in the resource cache
    // regardless of the variable type. Note this variable is not equal to dynamic offsets count, which is constant.
    Uint16 m_NumDynamicBuffers = 0;
    Uint32 m_TotalResources : 31;

    // Indicates what types of resources are stored in the cache
    const Uint32 m_ContentType : 1;

#ifdef DILIGENT_DEBUG
    // Debug array that stores flags indicating if resources in the cache have been initialized
    std::vector<std::vector<bool>> m_DbgInitializedResources;
#endif
};


template <>
__forceinline auto ShaderResourceCacheVk::Resource::GetDescriptorWriteInfo<DescriptorType::UniformBuffer>() const { return GetUniformBufferDescriptorWriteInfo(); }
template <>
__forceinline auto ShaderResourceCacheVk::Resource::GetDescriptorWriteInfo<DescriptorType::StorageBuffer>() const { return GetStorageBufferDescriptorWriteInfo(); }
template <>
__forceinline auto ShaderResourceCacheVk::Resource::GetDescriptorWriteInfo<DescriptorType::SeparateImage>() const { return GetImageDescriptorWriteInfo(); }
template <>
__forceinline auto ShaderResourceCacheVk::Resource::GetDescriptorWriteInfo<DescriptorType::UniformTexelBuffer>() const { return GetBufferViewWriteInfo(); }
template <>
__forceinline auto ShaderResourceCacheVk::Resource::GetDescriptorWriteInfo<DescriptorType::Sampler>() const { return GetSamplerDescriptorWriteInfo(); }
template <>
__forceinline auto ShaderResourceCacheVk::Resource::GetDescriptorWriteInfo<DescriptorType::InputAttachment>() const { return GetInputAttachmentDescriptorWriteInfo(); }
template <>
__forceinline auto ShaderResourceCacheVk::Resource::GetDescriptorWriteInfo<DescriptorType::InputAttachment_General>() const { return GetInputAttachmentDescriptorWriteInfo(); }
template <>
__forceinline auto ShaderResourceCacheVk::Resource::GetDescriptorWriteInfo<DescriptorType::AccelerationStructure>() const { return GetAccelerationStructureWriteInfo(); }


__forceinline Uint32 ShaderResourceCacheVk::GetDynamicBufferOffsets(DeviceContextIndex     CtxId,
                                                                    std::vector<uint32_t>& Offsets,
                                                                    Uint32                 StartInd) const
{
    // If any of the sets being bound include dynamic uniform or storage buffers, then
    // pDynamicOffsets includes one element for each array element in each dynamic descriptor
    // type binding in each set. Values are taken from pDynamicOffsets in an order such that
    // all entries for set N come before set N+1; within a set, entries are ordered by the binding
    // numbers (unclear if this is SPIRV binding or VkDescriptorSetLayoutBinding number) in the
    // descriptor set layouts; and within a binding array, elements are in order. (13.2.5)

    // In each descriptor set, all uniform buffers with dynamic offsets (DescriptorType::UniformBufferDynamic)
    // for every shader stage come first, followed by all storage buffers with dynamic offsets
    // (DescriptorType::StorageBufferDynamic and DescriptorType::StorageBufferDynamic_ReadOnly) for every shader stage,
    // followed by all other resources.
    Uint32 OffsetInd = StartInd;
    for (Uint32 set = 0; set < m_NumSets; ++set)
    {
        const auto& DescrSet = GetDescriptorSet(set);
        const auto  SetSize  = DescrSet.GetSize();

        Uint32 res = 0;
        while (res < SetSize)
        {
            const auto& Res = DescrSet.GetResource(res);
            if (Res.Type == DescriptorType::UniformBufferDynamic)
            {
                const auto* pBufferVk = Res.pObject.ConstPtr<BufferVkImpl>();
                // Do not verify dynamic allocation here as there may be some buffers that are not used by the PSO.
                // The allocations of the buffers that are actually used will be verified by
                // PipelineResourceSignatureVkImpl::DvpValidateCommittedResource().
                const auto Offset = pBufferVk != nullptr ? pBufferVk->GetDynamicOffset(CtxId, nullptr /* Do not verify allocation*/) : 0;
                // The effective offset used for dynamic uniform and storage buffer bindings is the sum of the relative
                // offset taken from pDynamicOffsets, and the base address of the buffer plus base offset in the descriptor set.
                // The range of the dynamic uniform and storage buffer bindings is the buffer range as specified in the descriptor set.
                Offsets[OffsetInd++] = StaticCast<Uint32>(Res.BufferDynamicOffset + Offset);
                ++res;
            }
            else
                break;
        }

        while (res < SetSize)
        {
            const auto& Res = DescrSet.GetResource(res);
            if (Res.Type == DescriptorType::StorageBufferDynamic ||
                Res.Type == DescriptorType::StorageBufferDynamic_ReadOnly)
            {
                const auto* pBufferVkView = Res.pObject.ConstPtr<BufferViewVkImpl>();
                const auto* pBufferVk     = pBufferVkView != nullptr ? pBufferVkView->GetBuffer<const BufferVkImpl>() : nullptr;
                // Do not verify dynamic allocation here as there may be some buffers that are not used by the PSO.
                // The allocations of the buffers that are actually used will be verified by
                // PipelineResourceSignatureVkImpl::DvpValidateCommittedResource().
                const auto Offset = pBufferVk != nullptr ? pBufferVk->GetDynamicOffset(CtxId, nullptr /* Do not verify allocation*/) : 0;
                // The effective offset used for dynamic uniform and storage buffer bindings is the sum of the relative
                // offset taken from pDynamicOffsets, and the base address of the buffer plus base offset in the descriptor set.
                // The range of the dynamic uniform and storage buffer bindings is the buffer range as specified in the descriptor set.
                Offsets[OffsetInd++] = StaticCast<Uint32>(Res.BufferDynamicOffset + Offset);
                ++res;
            }
            else
                break;
        }

#ifdef DILIGENT_DEBUG
        for (; res < SetSize; ++res)
        {
            const auto& Res = DescrSet.GetResource(res);
            VERIFY((Res.Type != DescriptorType::UniformBufferDynamic &&
                    Res.Type != DescriptorType::StorageBufferDynamic &&
                    Res.Type != DescriptorType::StorageBufferDynamic_ReadOnly),
                   "All dynamic uniform and storage buffers are expected to go first in the beginning of each descriptor set");
        }
#endif
    }
    return OffsetInd - StartInd;
}

} // namespace Diligent
