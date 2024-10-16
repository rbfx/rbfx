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

#pragma once

/// \file
/// Declaration of Diligent::PipelineResourceSignatureVkImpl class

#include <array>

#include "EngineVkImplTraits.hpp"
#include "PipelineResourceSignatureBase.hpp"

// ShaderVariableManagerVk, ShaderResourceCacheVk, and ShaderResourceBindingVkImpl
// are required by PipelineResourceSignatureBase
#include "ShaderResourceCacheVk.hpp"
#include "ShaderVariableManagerVk.hpp"
#include "ShaderResourceBindingVkImpl.hpp"

#include "PipelineResourceAttribsVk.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "SRBMemoryAllocator.hpp"

namespace Diligent
{

struct SPIRVShaderResourceAttribs;
class DeviceContextVkImpl;

struct ImmutableSamplerAttribsVk
{
    Uint32 DescrSet     = ~0u;
    Uint32 BindingIndex = ~0u;
};
ASSERT_SIZEOF(ImmutableSamplerAttribsVk, 8, "The struct is used in serialization and must be tightly packed");

struct PipelineResourceSignatureInternalDataVk : PipelineResourceSignatureInternalData<PipelineResourceAttribsVk, ImmutableSamplerAttribsVk>
{
    Uint16 DynamicUniformBufferCount = 0;
    Uint16 DynamicStorageBufferCount = 0;

    PipelineResourceSignatureInternalDataVk() noexcept
    {}

    explicit PipelineResourceSignatureInternalDataVk(const PipelineResourceSignatureInternalData& InternalData) noexcept :
        PipelineResourceSignatureInternalData{InternalData}
    {}
};

/// Implementation of the Diligent::PipelineResourceSignatureVkImpl class
class PipelineResourceSignatureVkImpl final : public PipelineResourceSignatureBase<EngineVkImplTraits>
{
public:
    using TPipelineResourceSignatureBase = PipelineResourceSignatureBase<EngineVkImplTraits>;

    using ResourceAttribs = TPipelineResourceSignatureBase::PipelineResourceAttribsType;

    // Descriptor set identifier (this is not the descriptor set index in the set layout!)
    enum DESCRIPTOR_SET_ID : size_t
    {
        // Static/mutable variables descriptor set id
        DESCRIPTOR_SET_ID_STATIC_MUTABLE = 0,

        // Dynamic variables descriptor set id
        DESCRIPTOR_SET_ID_DYNAMIC,

        DESCRIPTOR_SET_ID_NUM_SETS
    };

    // Static/mutable and dynamic descriptor sets
    static constexpr Uint32 MAX_DESCRIPTOR_SETS = DESCRIPTOR_SET_ID_NUM_SETS;

    static_assert(ResourceAttribs::MaxDescriptorSets >= MAX_DESCRIPTOR_SETS, "Not enough bits to store descriptor set index");

    PipelineResourceSignatureVkImpl(IReferenceCounters*                  pRefCounters,
                                    RenderDeviceVkImpl*                  pDevice,
                                    const PipelineResourceSignatureDesc& Desc,
                                    SHADER_TYPE                          ShaderStages      = SHADER_TYPE_UNKNOWN,
                                    bool                                 bIsDeviceInternal = false);

    PipelineResourceSignatureVkImpl(IReferenceCounters*                            pRefCounters,
                                    RenderDeviceVkImpl*                            pDevice,
                                    const PipelineResourceSignatureDesc&           Desc,
                                    const PipelineResourceSignatureInternalDataVk& InternalData);

    ~PipelineResourceSignatureVkImpl();

    Uint32 GetDynamicOffsetCount() const { return m_DynamicUniformBufferCount + m_DynamicStorageBufferCount; }
    Uint32 GetDynamicUniformBufferCount() const { return m_DynamicUniformBufferCount; }
    Uint32 GetDynamicStorageBufferCount() const { return m_DynamicStorageBufferCount; }
    Uint32 GetNumDescriptorSets() const
    {
        static_assert(DESCRIPTOR_SET_ID_NUM_SETS == 2, "Please update this method with new descriptor set id");
        return (HasDescriptorSet(DESCRIPTOR_SET_ID_STATIC_MUTABLE) ? 1 : 0) + (HasDescriptorSet(DESCRIPTOR_SET_ID_DYNAMIC) ? 1 : 0);
    }

    VkDescriptorSetLayout GetVkDescriptorSetLayout(DESCRIPTOR_SET_ID SetId) const { return m_VkDescrSetLayouts[SetId]; }

    bool   HasDescriptorSet(DESCRIPTOR_SET_ID SetId) const { return m_VkDescrSetLayouts[SetId] != VK_NULL_HANDLE; }
    Uint32 GetDescriptorSetSize(DESCRIPTOR_SET_ID SetId) const { return m_DescriptorSetSizes[SetId]; }

    void InitSRBResourceCache(ShaderResourceCacheVk& ResourceCache);

    // Copies static resources from the static resource cache to the destination cache
    void CopyStaticResources(ShaderResourceCacheVk& ResourceCache) const;
    // Make the base class method visible
    using TPipelineResourceSignatureBase::CopyStaticResources;

    // Commits dynamic resources from ResourceCache to vkDynamicDescriptorSet
    void CommitDynamicResources(const ShaderResourceCacheVk& ResourceCache,
                                VkDescriptorSet              vkDynamicDescriptorSet) const;

#ifdef DILIGENT_DEVELOPMENT
    /// Verifies committed resource using the SPIRV resource attributes from the PSO.
    bool DvpValidateCommittedResource(const DeviceContextVkImpl*        pDeviceCtx,
                                      const SPIRVShaderResourceAttribs& SPIRVAttribs,
                                      Uint32                            ResIndex,
                                      const ShaderResourceCacheVk&      ResourceCache,
                                      const char*                       ShaderName,
                                      const char*                       PSOName) const;
#endif

    // Returns the descriptor set index in the resource cache
    template <DESCRIPTOR_SET_ID SetId>
    Uint32 GetDescriptorSetIndex() const;

    PipelineResourceSignatureInternalDataVk GetInternalData() const;

private:
    // Resource cache group identifier
    enum CACHE_GROUP : size_t
    {
        CACHE_GROUP_DYN_UB = 0,         // Uniform buffer with dynamic offset
        CACHE_GROUP_DYN_SB,             // Storage buffer with dynamic offset
        CACHE_GROUP_OTHER,              // Other resource type
        CACHE_GROUP_COUNT_PER_VAR_TYPE, // Cache group count per shader variable type

        CACHE_GROUP_DYN_UB_STAT_VAR = CACHE_GROUP_DYN_UB, // Uniform buffer with dynamic offset, static variable
        CACHE_GROUP_DYN_SB_STAT_VAR = CACHE_GROUP_DYN_SB, // Storage buffer with dynamic offset, static variable
        CACHE_GROUP_OTHER_STAT_VAR  = CACHE_GROUP_OTHER,  // Other resource type, static variable

        CACHE_GROUP_DYN_UB_DYN_VAR, // Uniform buffer with dynamic offset, dynamic variable
        CACHE_GROUP_DYN_SB_DYN_VAR, // Storage buffer with dynamic offset, dynamic variable
        CACHE_GROUP_OTHER_DYN_VAR,  // Other resource type, dynamic variable

        CACHE_GROUP_COUNT
    };
    static_assert(CACHE_GROUP_COUNT == CACHE_GROUP_COUNT_PER_VAR_TYPE * MAX_DESCRIPTOR_SETS, "Inconsistent cache group count");

    using CacheOffsetsType = std::array<Uint32, CACHE_GROUP_COUNT>; // [dynamic uniform buffers, dynamic storage buffers, other] x [descriptor sets] including ArraySize
    using BindingCountType = std::array<Uint32, CACHE_GROUP_COUNT>; // [dynamic uniform buffers, dynamic storage buffers, other] x [descriptor sets] not counting ArraySize

    void Destruct();

    void CreateSetLayouts(bool IsSerialized);

    static inline CACHE_GROUP       GetResourceCacheGroup(const PipelineResourceDesc& Res);
    static inline DESCRIPTOR_SET_ID VarTypeToDescriptorSetId(SHADER_RESOURCE_VARIABLE_TYPE VarType);

private:
    std::array<VulkanUtilities::DescriptorSetLayoutWrapper, DESCRIPTOR_SET_ID_NUM_SETS> m_VkDescrSetLayouts;

    // Descriptor set sizes indexed by the set index in the layout (not DESCRIPTOR_SET_ID!)
    std::array<Uint32, MAX_DESCRIPTOR_SETS> m_DescriptorSetSizes = {~0U, ~0U};

    // The total number of uniform buffers with dynamic offsets in both descriptor sets,
    // accounting for array size.
    Uint16 m_DynamicUniformBufferCount = 0;
    // The total number storage buffers with dynamic offsets in both descriptor sets,
    // accounting for array size.
    Uint16 m_DynamicStorageBufferCount = 0;
};

template <> Uint32 PipelineResourceSignatureVkImpl::GetDescriptorSetIndex<PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_STATIC_MUTABLE>() const;
template <> Uint32 PipelineResourceSignatureVkImpl::GetDescriptorSetIndex<PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_DYNAMIC>() const;

} // namespace Diligent
