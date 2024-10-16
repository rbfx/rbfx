/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include "RenderDeviceWebGPUImpl.hpp"
#include "PipelineResourceSignatureWebGPUImpl.hpp"
#include "WebGPUTypeConversions.hpp"
#include "BufferWebGPUImpl.hpp"
#include "BufferViewWebGPUImpl.hpp"
#include "TextureViewWebGPUImpl.hpp"

namespace Diligent
{

namespace
{

constexpr SHADER_TYPE WEB_GPU_SUPPORTED_SHADER_STAGES =
    SHADER_TYPE_VERTEX |
    SHADER_TYPE_PIXEL |
    SHADER_TYPE_COMPUTE;


BindGroupEntryType GetBindGroupEntryType(const PipelineResourceDesc& Res)
{
    VERIFY((Res.Flags & ~GetValidPipelineResourceFlags(Res.ResourceType)) == 0,
           "Invalid resource flags. This error should've been caught by ValidatePipelineResourceSignatureDesc.");

    const bool WithDynamicOffset = (Res.Flags & PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS) == 0;

    static_assert(SHADER_RESOURCE_TYPE_LAST == SHADER_RESOURCE_TYPE_ACCEL_STRUCT, "Please update the switch below to handle the new shader resource type");
    switch (Res.ResourceType)
    {
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
            return WithDynamicOffset ? BindGroupEntryType::UniformBufferDynamic : BindGroupEntryType::UniformBuffer;

        case SHADER_RESOURCE_TYPE_TEXTURE_SRV:
            return BindGroupEntryType::Texture;

        case SHADER_RESOURCE_TYPE_BUFFER_SRV:
            return WithDynamicOffset ? BindGroupEntryType::StorageBufferDynamic_ReadOnly : BindGroupEntryType::StorageBuffer_ReadOnly;

        case SHADER_RESOURCE_TYPE_TEXTURE_UAV:
            switch (Res.WebGPUAttribs.BindingType)
            {
                case WEB_GPU_BINDING_TYPE_DEFAULT:
                case WEB_GPU_BINDING_TYPE_WRITE_ONLY_TEXTURE_UAV:
                    return BindGroupEntryType::StorageTexture_WriteOnly;

                case WEB_GPU_BINDING_TYPE_READ_ONLY_TEXTURE_UAV:
                    return BindGroupEntryType::StorageTexture_ReadOnly;

                case WEB_GPU_BINDING_TYPE_READ_WRITE_TEXTURE_UAV:
                    return BindGroupEntryType::StorageTexture_ReadWrite;

                default:
                    UNEXPECTED("Unknown texture UAV binding type");
                    return BindGroupEntryType::StorageTexture_WriteOnly;
            }

        case SHADER_RESOURCE_TYPE_BUFFER_UAV:
            return WithDynamicOffset ? BindGroupEntryType::StorageBufferDynamic : BindGroupEntryType::StorageBuffer;

        case SHADER_RESOURCE_TYPE_SAMPLER:
            return BindGroupEntryType::Sampler;

        case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT:
            return BindGroupEntryType::Texture;

        default:
            UNEXPECTED("Unknown resource type");
            return BindGroupEntryType::Count;
    }
}

WGPUBufferBindingType GetWGPUBufferBindingType(BindGroupEntryType EntryType)
{
    switch (EntryType)
    {
        case BindGroupEntryType::UniformBuffer:
        case BindGroupEntryType::UniformBufferDynamic:
            return WGPUBufferBindingType_Uniform;

        case BindGroupEntryType::StorageBuffer:
        case BindGroupEntryType::StorageBufferDynamic:
            return WGPUBufferBindingType_Storage;

        case BindGroupEntryType::StorageBuffer_ReadOnly:
        case BindGroupEntryType::StorageBufferDynamic_ReadOnly:
            return WGPUBufferBindingType_ReadOnlyStorage;

        default:
            return WGPUBufferBindingType_Undefined;
    }
}

WGPUSamplerBindingType GetWGPUSamplerBindingType(BindGroupEntryType EntryType, WEB_GPU_BINDING_TYPE BindingType)
{
    if (EntryType == BindGroupEntryType::Sampler)
    {
        switch (BindingType)
        {
            case WEB_GPU_BINDING_TYPE_DEFAULT:
            case WEB_GPU_BINDING_TYPE_FILTERING_SAMPLER:
                return WGPUSamplerBindingType_Filtering;

            case WEB_GPU_BINDING_TYPE_NON_FILTERING_SAMPLER:
                return WGPUSamplerBindingType_NonFiltering;

            case WEB_GPU_BINDING_TYPE_COMPARISON_SAMPLER:
                return WGPUSamplerBindingType_Comparison;

            default:
                UNEXPECTED("Invalid sampler binding type");
                return WGPUSamplerBindingType_Filtering;
        }
    }
    else
    {
        return WGPUSamplerBindingType_Undefined;
    }
}

WGPUSamplerBindingType SamplerDescToWGPUSamplerBindingType(const SamplerDesc& Desc)
{
    VERIFY(IsComparisonFilter(Desc.MinFilter) == IsComparisonFilter(Desc.MagFilter), "Inconsistent min/mag filters");
    if (IsComparisonFilter(Desc.MinFilter))
    {
        return WGPUSamplerBindingType_Comparison;
    }
    else if (Desc.MinFilter == FILTER_TYPE_POINT &&
             Desc.MagFilter == FILTER_TYPE_POINT &&
             Desc.MipFilter == FILTER_TYPE_POINT)
    {
        return WGPUSamplerBindingType_NonFiltering;
    }
    else
    {
        return WGPUSamplerBindingType_Filtering;
    }
}

WGPUTextureSampleType GetWGPUTextureSampleType(BindGroupEntryType EntryType, WEB_GPU_BINDING_TYPE BindingType)
{
    if (EntryType == BindGroupEntryType::Texture)
    {
        switch (BindingType)
        {
            case WEB_GPU_BINDING_TYPE_DEFAULT:
            case WEB_GPU_BINDING_TYPE_FLOAT_TEXTURE:
            case WEB_GPU_BINDING_TYPE_FLOAT_TEXTURE_MS:
                return WGPUTextureSampleType_Float;

            case WEB_GPU_BINDING_TYPE_UNFILTERABLE_FLOAT_TEXTURE:
            case WEB_GPU_BINDING_TYPE_UNFILTERABLE_FLOAT_TEXTURE_MS:
                return WGPUTextureSampleType_UnfilterableFloat;

            case WEB_GPU_BINDING_TYPE_DEPTH_TEXTURE:
            case WEB_GPU_BINDING_TYPE_DEPTH_TEXTURE_MS:
                return WGPUTextureSampleType_Depth;

            case WEB_GPU_BINDING_TYPE_SINT_TEXTURE:
            case WEB_GPU_BINDING_TYPE_SINT_TEXTURE_MS:
                return WGPUTextureSampleType_Sint;

            case WEB_GPU_BINDING_TYPE_UINT_TEXTURE:
            case WEB_GPU_BINDING_TYPE_UINT_TEXTURE_MS:
                return WGPUTextureSampleType_Uint;

            default:
                UNEXPECTED("Invalid texture binding type");
                return WGPUTextureSampleType_Float;
        }
    }
    else
    {
        return WGPUTextureSampleType_Undefined;
    }
}

WGPUStorageTextureAccess GetWGPUStorageTextureAccess(BindGroupEntryType EntryType)
{
    switch (EntryType)
    {
        case BindGroupEntryType::StorageTexture_WriteOnly:
            return WGPUStorageTextureAccess_WriteOnly;

        case BindGroupEntryType::StorageTexture_ReadOnly:
            return WGPUStorageTextureAccess_ReadOnly;

        case BindGroupEntryType::StorageTexture_ReadWrite:
            return WGPUStorageTextureAccess_ReadWrite;

        default:
            return WGPUStorageTextureAccess_Undefined;
    }
}

bool IsMSTextureGPUBinding(WEB_GPU_BINDING_TYPE BindingType)
{
    return (BindingType == WEB_GPU_BINDING_TYPE_FLOAT_TEXTURE_MS ||
            BindingType == WEB_GPU_BINDING_TYPE_UNFILTERABLE_FLOAT_TEXTURE_MS ||
            BindingType == WEB_GPU_BINDING_TYPE_DEPTH_TEXTURE_MS ||
            BindingType == WEB_GPU_BINDING_TYPE_SINT_TEXTURE_MS ||
            BindingType == WEB_GPU_BINDING_TYPE_UINT_TEXTURE_MS);
}

WGPUBindGroupLayoutEntry GetWGPUBindGroupLayoutEntry(const PipelineResourceAttribsWebGPU& Attribs,
                                                     const PipelineResourceDesc&          ResDesc,
                                                     Uint32                               ArrayElement)
{
    WGPUBindGroupLayoutEntry wgpuBGLayoutEntry{};

    wgpuBGLayoutEntry.binding    = Attribs.BindingIndex + ArrayElement;
    wgpuBGLayoutEntry.visibility = ShaderStagesToWGPUShaderStageFlags(ResDesc.ShaderStages & WEB_GPU_SUPPORTED_SHADER_STAGES);

    const BindGroupEntryType EntryType = Attribs.GetBindGroupEntryType();
    static_assert(static_cast<size_t>(BindGroupEntryType::Count) == 12, "Please update the switch below to handle the new bind group entry type");
    if (WGPUBufferBindingType wgpuBufferBindingType = GetWGPUBufferBindingType(EntryType))
    {
        wgpuBGLayoutEntry.buffer.type = wgpuBufferBindingType;
        wgpuBGLayoutEntry.buffer.hasDynamicOffset =
            (EntryType == BindGroupEntryType::UniformBufferDynamic ||
             EntryType == BindGroupEntryType::StorageBufferDynamic ||
             EntryType == BindGroupEntryType::StorageBufferDynamic_ReadOnly);

        // If this is 0, it is ignored by pipeline creation, and instead draw/dispatch commands validate that each binding
        // in the GPUBindGroup satisfies the minimum buffer binding size of the variable.
        wgpuBGLayoutEntry.buffer.minBindingSize = 0;
    }
    else if (WGPUSamplerBindingType wgpuSamplerBindingType = GetWGPUSamplerBindingType(EntryType, ResDesc.WebGPUAttribs.BindingType))
    {
        wgpuBGLayoutEntry.sampler.type = wgpuSamplerBindingType;
    }
    else if (WGPUTextureSampleType wgpuTextureSampleType = GetWGPUTextureSampleType(EntryType, ResDesc.WebGPUAttribs.BindingType))
    {
        wgpuBGLayoutEntry.texture.sampleType = wgpuTextureSampleType;

        wgpuBGLayoutEntry.texture.viewDimension = ResDesc.WebGPUAttribs.TextureViewDim != RESOURCE_DIM_UNDEFINED ?
            ResourceDimensionToWGPUTextureViewDimension(ResDesc.WebGPUAttribs.TextureViewDim) :
            WGPUTextureViewDimension_2D;

        wgpuBGLayoutEntry.texture.multisampled = IsMSTextureGPUBinding(ResDesc.WebGPUAttribs.BindingType);
    }
    else if (WGPUStorageTextureAccess wgpuStorageTextureAccess = GetWGPUStorageTextureAccess(EntryType))
    {
        wgpuBGLayoutEntry.storageTexture.access = wgpuStorageTextureAccess;

        wgpuBGLayoutEntry.storageTexture.format = ResDesc.WebGPUAttribs.UAVTextureFormat != TEX_FORMAT_UNKNOWN ?
            TextureFormatToWGPUFormat(ResDesc.WebGPUAttribs.UAVTextureFormat) :
            WGPUTextureFormat_RGBA32Float;

        wgpuBGLayoutEntry.storageTexture.viewDimension = ResDesc.WebGPUAttribs.TextureViewDim != RESOURCE_DIM_UNDEFINED ?
            ResourceDimensionToWGPUTextureViewDimension(ResDesc.WebGPUAttribs.TextureViewDim) :
            WGPUTextureViewDimension_2D;
    }
    else
    {
        UNEXPECTED("Unexpected bind group entry type");
    }

    return wgpuBGLayoutEntry;
}

} // namespace


inline PipelineResourceSignatureWebGPUImpl::CACHE_GROUP PipelineResourceSignatureWebGPUImpl::GetResourceCacheGroup(const PipelineResourceDesc& Res)
{
    // NB: GroupId is always 0 for static/mutable variables, and 1 - for dynamic ones.
    //     It is not the actual bind group index in the group layout!
    const size_t GroupId           = VarTypeToBindGroupId(Res.VarType);
    const bool   WithDynamicOffset = (Res.Flags & PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS) == 0;
    VERIFY((Res.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) == 0, "Formatted buffers are not supported");
    VERIFY((Res.Flags & PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY) == 0, "Run-time arrays are not supported");
    if (WithDynamicOffset)
    {
        if (Res.ResourceType == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER)
            return static_cast<CACHE_GROUP>(GroupId * CACHE_GROUP_COUNT_PER_VAR_TYPE + CACHE_GROUP_DYN_UB);

        if (Res.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV || Res.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV)
            return static_cast<CACHE_GROUP>(GroupId * CACHE_GROUP_COUNT_PER_VAR_TYPE + CACHE_GROUP_DYN_SB);
    }
    return static_cast<CACHE_GROUP>(GroupId * CACHE_GROUP_COUNT_PER_VAR_TYPE + CACHE_GROUP_OTHER);
}

inline PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID PipelineResourceSignatureWebGPUImpl::VarTypeToBindGroupId(SHADER_RESOURCE_VARIABLE_TYPE VarType)
{
    return VarType == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC ? BIND_GROUP_ID_DYNAMIC : BIND_GROUP_ID_STATIC_MUTABLE;
}

PipelineResourceSignatureWebGPUImpl::PipelineResourceSignatureWebGPUImpl(IReferenceCounters*                  pRefCounters,
                                                                         RenderDeviceWebGPUImpl*              pDevice,
                                                                         const PipelineResourceSignatureDesc& Desc,
                                                                         SHADER_TYPE                          ShaderStages,
                                                                         bool                                 bIsDeviceInternal) :
    TPipelineResourceSignatureBase{pRefCounters, pDevice, Desc, ShaderStages, bIsDeviceInternal}
{
    try
    {
        Initialize(
            GetRawAllocator(), DecoupleCombinedSamplers(Desc), /*CreateImmutableSamplers = */ true,
            [this]() //
            {
                CreateBindGroupLayouts(/*IsSerialized*/ false);
            },
            [this]() //
            {
                return ShaderResourceCacheWebGPU::GetRequiredMemorySize(GetNumBindGroups(), m_BindGroupSizes.data());
            });
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

struct PipelineResourceSignatureWebGPUImpl::WGPUBindGroupLayoutsCreateInfo
{
    using EntriesArrayType = std::array<std::vector<WGPUBindGroupLayoutEntry>, BIND_GROUP_ID_NUM_GROUPS>;
    EntriesArrayType wgpuBGLayoutEntries;
};

void PipelineResourceSignatureWebGPUImpl::CreateBindGroupLayouts(const bool IsSerialized)
{
    // Binding count in each cache group.
    BindingCountType BindingCount = {};
    // Required cache size for each cache group
    CacheOffsetsType CacheGroupSizes = {};

    // Resource bindings as well as cache offsets are ordered by CACHE_GROUP in each bind group:
    //
    //      static/mutable vars group: |  Dynamic UBs  |  Dynamic SBs  |   The rest    |
    //      dynamic vars group:        |  Dynamic UBs  |  Dynamic SBs  |   The rest    |
    //
    // Note also that resources in m_Desc.Resources are sorted by variable type

    auto ReserveBindings = [&BindingCount, &CacheGroupSizes](CACHE_GROUP GroupId, Uint32 Count) {
        // NB: since WebGPU does not support resource arrays, binding count is the same as the cache group size
        BindingCount[GroupId] += Count;
        CacheGroupSizes[GroupId] += Count;
    };

    // The total number of static resources in all stages accounting for array sizes.
    Uint32 StaticResourceCount = 0;

    // Index of the immutable sampler for every sampler in m_Desc.Resources, or InvalidImmutableSamplerIndex.
    std::vector<Uint32> ResourceToImmutableSamplerInd(m_Desc.NumResources, InvalidImmutableSamplerIndex);
    for (Uint32 i = 0; i < m_Desc.NumResources; ++i)
    {
        const PipelineResourceDesc& ResDesc    = m_Desc.Resources[i];
        const CACHE_GROUP           CacheGroup = GetResourceCacheGroup(ResDesc);

        bool IsImmutableSampler = false;
        if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER)
        {
            // We only need to search for immutable samplers for SHADER_RESOURCE_TYPE_SAMPLER.
            // For SHADER_RESOURCE_TYPE_TEXTURE_SRV, we will look for the assigned sampler and check if it is immutable.

            // Note that FindImmutableSampler() below will work properly both when combined texture samplers are used and when not.
            const Uint32 SrcImmutableSamplerInd = FindImmutableSampler(ResDesc.ShaderStages, ResDesc.Name);
            if (SrcImmutableSamplerInd != InvalidImmutableSamplerIndex)
            {
                ResourceToImmutableSamplerInd[i] = SrcImmutableSamplerInd;
                // Set the immutable sampler array size to match the resource array size
                ImmutableSamplerAttribsWebGPU& DstImtblSampAttribs = m_pImmutableSamplerAttribs[SrcImmutableSamplerInd];
                // One immutable sampler may be used by different arrays in different shader stages - use the maximum array size
                DstImtblSampAttribs.ArraySize = std::max(DstImtblSampAttribs.ArraySize, ResDesc.ArraySize);

                IsImmutableSampler = true;
            }
        }

        // We allocate bindings for immutable samplers separately
        if (!IsImmutableSampler)
        {
            ReserveBindings(CacheGroup, ResDesc.ArraySize);
            if (ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
                StaticResourceCount += ResDesc.ArraySize;
        }
    }

    // Reserve bindings for immutable samplers
    static constexpr BIND_GROUP_ID ImmutableSamplerBindGroupId = BIND_GROUP_ID_STATIC_MUTABLE;
    static constexpr CACHE_GROUP   ImmutableSamplerCacheGroup  = CACHE_GROUP_OTHER_STAT_VAR;
    for (Uint32 i = 0; i < m_Desc.NumImmutableSamplers; ++i)
    {
        const ImmutableSamplerAttribsWebGPU& ImtblSampAttribs = m_pImmutableSamplerAttribs[i];
        ReserveBindings(ImmutableSamplerCacheGroup, ImtblSampAttribs.ArraySize);
    }

    VERIFY_EXPR((StaticResourceCount != 0) == (GetNumStaticResStages() != 0));
    // Initialize static resource cache
    if (StaticResourceCount != 0)
    {
        VERIFY_EXPR(m_pStaticResCache != nullptr);
        m_pStaticResCache->InitializeGroups(GetRawAllocator(), 1, &StaticResourceCount);
    }

    // Bind group mapping (static/mutable (0) or dynamic (1) -> bind group index).
    // If all resources are dynamic, then the signature will contain a single group with index 0.
    std::array<Uint32, BIND_GROUP_ID_NUM_GROUPS> BindGroupIdToIndex = {};
    {
        const Uint32 TotalStaticBindings =
            BindingCount[CACHE_GROUP_DYN_UB_STAT_VAR] +
            BindingCount[CACHE_GROUP_DYN_SB_STAT_VAR] +
            BindingCount[CACHE_GROUP_OTHER_STAT_VAR];
        const Uint32 TotalDynamicBindings =
            BindingCount[CACHE_GROUP_DYN_UB_DYN_VAR] +
            BindingCount[CACHE_GROUP_DYN_SB_DYN_VAR] +
            BindingCount[CACHE_GROUP_OTHER_DYN_VAR];

        Uint32 Idx = 0;

        BindGroupIdToIndex[BIND_GROUP_ID_STATIC_MUTABLE] = (TotalStaticBindings != 0 ? Idx++ : 0xFF);
        BindGroupIdToIndex[BIND_GROUP_ID_DYNAMIC]        = (TotalDynamicBindings != 0 ? Idx++ : 0xFF);
        VERIFY_EXPR(Idx <= MAX_BIND_GROUPS);
    }

    // Get starting offsets and binding indices for each cache group
    CacheOffsetsType CacheGroupOffsets =
        {
            // static/mutable group
            0,
            CacheGroupSizes[CACHE_GROUP_DYN_UB_STAT_VAR],
            CacheGroupSizes[CACHE_GROUP_DYN_UB_STAT_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_SB_STAT_VAR],
            // dynamic group
            0,
            CacheGroupSizes[CACHE_GROUP_DYN_UB_DYN_VAR],
            CacheGroupSizes[CACHE_GROUP_DYN_UB_DYN_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_SB_DYN_VAR],
        };
    BindingCountType BindingIndices =
        {
            // static/mutable group
            0,
            BindingCount[CACHE_GROUP_DYN_UB_STAT_VAR],
            BindingCount[CACHE_GROUP_DYN_UB_STAT_VAR] + BindingCount[CACHE_GROUP_DYN_SB_STAT_VAR],
            // dynamic group
            0,
            BindingCount[CACHE_GROUP_DYN_UB_DYN_VAR],
            BindingCount[CACHE_GROUP_DYN_UB_DYN_VAR] + BindingCount[CACHE_GROUP_DYN_SB_DYN_VAR],
        };

    auto AllocateBindings = [&](BIND_GROUP_ID BindGroupId,
                                CACHE_GROUP   CacheGroup,
                                Uint32        Count,
                                Uint32&       BindGroupIndex,
                                Uint32&       BindingIndex,
                                Uint32&       CacheOffset) {
        // Remap GroupId to the actual bind group index.
        BindGroupIndex = BindGroupIdToIndex[BindGroupId];
        VERIFY_EXPR(BindGroupIndex < MAX_BIND_GROUPS);

        BindingIndex = BindingIndices[CacheGroup];
        CacheOffset  = CacheGroupOffsets[CacheGroup];

        BindingIndices[CacheGroup] += Count;
        CacheGroupOffsets[CacheGroup] += Count;
    };

    m_BindGroupLayoutsCreateInfo                                          = std::make_unique<WGPUBindGroupLayoutsCreateInfo>();
    WGPUBindGroupLayoutsCreateInfo::EntriesArrayType& wgpuBGLayoutEntries = m_BindGroupLayoutsCreateInfo->wgpuBGLayoutEntries;

    // Allocate bindings for immutable samplers first
    for (Uint32 i = 0; i < m_Desc.NumImmutableSamplers; ++i)
    {
        const ImmutableSamplerDesc&    ImtblSamp        = GetImmutableSamplerDesc(i);
        ImmutableSamplerAttribsWebGPU& ImtblSampAttribs = m_pImmutableSamplerAttribs[i];
        AllocateBindings(ImmutableSamplerBindGroupId, ImmutableSamplerCacheGroup, ImtblSampAttribs.ArraySize,
                         ImtblSampAttribs.BindGroup, ImtblSampAttribs.BindingIndex, ImtblSampAttribs.CacheOffset);

        // Add layout entries
        for (Uint32 elem = 0; elem < ImtblSampAttribs.ArraySize; ++elem)
        {
            WGPUBindGroupLayoutEntry wgpuBGLayoutEntry{};
            wgpuBGLayoutEntry.binding      = ImtblSampAttribs.BindingIndex + elem;
            wgpuBGLayoutEntry.visibility   = ShaderStagesToWGPUShaderStageFlags(ImtblSamp.ShaderStages & WEB_GPU_SUPPORTED_SHADER_STAGES);
            wgpuBGLayoutEntry.sampler.type = SamplerDescToWGPUSamplerBindingType(ImtblSamp.Desc);
            wgpuBGLayoutEntries[ImtblSampAttribs.BindGroup].push_back(wgpuBGLayoutEntry);
        }
    }

    // Current offset in the static resource cache
    Uint32 StaticCacheOffset = 0;

    for (Uint32 i = 0; i < m_Desc.NumResources; ++i)
    {
        const PipelineResourceDesc& ResDesc = m_Desc.Resources[i];
        VERIFY(i == 0 || ResDesc.VarType >= m_Desc.Resources[i - 1].VarType, "Resources must be sorted by variable type");

        const BindGroupEntryType EntryType = GetBindGroupEntryType(ResDesc);

        Uint32 AssignedSamplerInd     = ResourceAttribs::InvalidSamplerInd;
        Uint32 SrcImmutableSamplerInd = ResourceToImmutableSamplerInd[i];
        if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV)
        {
            VERIFY_EXPR(SrcImmutableSamplerInd == InvalidImmutableSamplerIndex);
            AssignedSamplerInd = FindAssignedSampler(ResDesc, ResourceAttribs::InvalidSamplerInd);
            if (AssignedSamplerInd != ResourceAttribs::InvalidSamplerInd)
            {
                SrcImmutableSamplerInd = ResourceToImmutableSamplerInd[AssignedSamplerInd];
            }
        }

        const bool IsImmutableSampler = (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER && SrcImmutableSamplerInd != InvalidImmutableSamplerIndex);

        Uint32 BindGroupIndex = ~0u;
        Uint32 BindingIndex   = ~0u;
        Uint32 CacheOffset    = ~0u;
        if (IsImmutableSampler)
        {
            // Note that the same immutable sampler may be assigned to multiple sampler resources in different shader stages
            const ImmutableSamplerAttribsWebGPU& ImtblSampAttribs = m_pImmutableSamplerAttribs[SrcImmutableSamplerInd];

            BindGroupIndex = ImtblSampAttribs.BindGroup;
            BindingIndex   = ImtblSampAttribs.BindingIndex;
            CacheOffset    = ImtblSampAttribs.CacheOffset;
        }
        else
        {
            const BIND_GROUP_ID GroupId = VarTypeToBindGroupId(ResDesc.VarType);
            AllocateBindings(GroupId, GetResourceCacheGroup(ResDesc), ResDesc.ArraySize, BindGroupIndex, BindingIndex, CacheOffset);
        }
        VERIFY_EXPR(BindGroupIndex != ~0u && BindingIndex != ~0u && CacheOffset != ~0u);

        auto* const pAttribs = m_pResourceAttribs + i;
        if (!IsSerialized)
        {
            new (pAttribs) ResourceAttribs{
                BindingIndex,
                AssignedSamplerInd,
                ResDesc.ArraySize,
                EntryType,
                BindGroupIndex,
                SrcImmutableSamplerInd != InvalidImmutableSamplerIndex,
                CacheOffset,
                (ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC && !IsImmutableSampler) ? StaticCacheOffset : ~0u,
            };
        }
        else
        {
            DEV_CHECK_ERR(pAttribs->BindingIndex == BindingIndex,
                          "Deserialized binding index (", pAttribs->BindingIndex, ") is invalid: ", BindingIndex, " is expected.");
            DEV_CHECK_ERR(pAttribs->SamplerInd == AssignedSamplerInd,
                          "Deserialized sampler index (", pAttribs->SamplerInd, ") is invalid: ", AssignedSamplerInd, " is expected.");
            DEV_CHECK_ERR(pAttribs->ArraySize == ResDesc.ArraySize,
                          "Deserialized array size (", pAttribs->ArraySize, ") is invalid: ", ResDesc.ArraySize, " is expected.");
            DEV_CHECK_ERR(pAttribs->GetBindGroupEntryType() == EntryType, "Deserialized bind group entry type is invalid");
            DEV_CHECK_ERR(pAttribs->BindGroup == BindGroupIndex,
                          "Deserialized bind group index (", pAttribs->BindGroup, ") is invalid: ", BindGroupIndex, " is expected.");
            DEV_CHECK_ERR(pAttribs->IsImmutableSamplerAssigned() == (SrcImmutableSamplerInd != InvalidImmutableSamplerIndex), "Deserialized immutable sampler flag is invalid");
            DEV_CHECK_ERR(pAttribs->SRBCacheOffset == CacheOffset,
                          "Deserialized SRB cache offset (", pAttribs->SRBCacheOffset, ") is invalid: ", CacheOffset, " is expected.");
            DEV_CHECK_ERR(pAttribs->StaticCacheOffset == ((ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC && !IsImmutableSampler) ? StaticCacheOffset : ~0u),
                          "Deserialized static cache offset is invalid.");
        }

        if (!IsImmutableSampler)
        {
            for (Uint32 elem = 0; elem < ResDesc.ArraySize; ++elem)
            {
                WGPUBindGroupLayoutEntry wgpuBGLayoutEntry = GetWGPUBindGroupLayoutEntry(*pAttribs, ResDesc, elem);
                wgpuBGLayoutEntries[BindGroupIndex].push_back(wgpuBGLayoutEntry);
            }

            if (ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
            {
                VERIFY(pAttribs->BindGroup == 0, "Static resources must always be allocated in bind group 0");
                m_pStaticResCache->InitializeResources(pAttribs->BindGroup, StaticCacheOffset, ResDesc.ArraySize,
                                                       pAttribs->GetBindGroupEntryType(), pAttribs->IsImmutableSamplerAssigned());
                StaticCacheOffset += ResDesc.ArraySize;
            }
        }
    }

    VERIFY_EXPR(StaticCacheOffset == StaticResourceCount);

#ifdef DILIGENT_DEBUG
    if (m_pStaticResCache != nullptr)
    {
        m_pStaticResCache->DbgVerifyResourceInitialization();
    }
#endif

    m_DynamicUniformBufferCount = static_cast<Uint16>(CacheGroupSizes[CACHE_GROUP_DYN_UB_STAT_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_UB_DYN_VAR]);
    m_DynamicStorageBufferCount = static_cast<Uint16>(CacheGroupSizes[CACHE_GROUP_DYN_SB_STAT_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_SB_DYN_VAR]);
    VERIFY_EXPR(m_DynamicUniformBufferCount == CacheGroupSizes[CACHE_GROUP_DYN_UB_STAT_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_UB_DYN_VAR]);
    VERIFY_EXPR(m_DynamicStorageBufferCount == CacheGroupSizes[CACHE_GROUP_DYN_SB_STAT_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_SB_DYN_VAR]);

    VERIFY_EXPR(m_pStaticResCache == nullptr || const_cast<const ShaderResourceCacheWebGPU*>(m_pStaticResCache)->GetBindGroup(0).GetSize() == StaticCacheOffset);
    VERIFY_EXPR(CacheGroupOffsets[CACHE_GROUP_DYN_UB_STAT_VAR] == CacheGroupSizes[CACHE_GROUP_DYN_UB_STAT_VAR]);
    VERIFY_EXPR(CacheGroupOffsets[CACHE_GROUP_DYN_SB_STAT_VAR] == CacheGroupSizes[CACHE_GROUP_DYN_UB_STAT_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_SB_STAT_VAR]);
    VERIFY_EXPR(CacheGroupOffsets[CACHE_GROUP_OTHER_STAT_VAR] == CacheGroupSizes[CACHE_GROUP_DYN_UB_STAT_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_SB_STAT_VAR] + CacheGroupSizes[CACHE_GROUP_OTHER_STAT_VAR]);
    VERIFY_EXPR(CacheGroupOffsets[CACHE_GROUP_DYN_UB_DYN_VAR] == CacheGroupSizes[CACHE_GROUP_DYN_UB_DYN_VAR]);
    VERIFY_EXPR(CacheGroupOffsets[CACHE_GROUP_DYN_SB_DYN_VAR] == CacheGroupSizes[CACHE_GROUP_DYN_UB_DYN_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_SB_DYN_VAR]);
    VERIFY_EXPR(CacheGroupOffsets[CACHE_GROUP_OTHER_DYN_VAR] == CacheGroupSizes[CACHE_GROUP_DYN_UB_DYN_VAR] + CacheGroupSizes[CACHE_GROUP_DYN_SB_DYN_VAR] + CacheGroupSizes[CACHE_GROUP_OTHER_DYN_VAR]);
    VERIFY_EXPR(BindingIndices[CACHE_GROUP_DYN_UB_STAT_VAR] == BindingCount[CACHE_GROUP_DYN_UB_STAT_VAR]);
    VERIFY_EXPR(BindingIndices[CACHE_GROUP_DYN_SB_STAT_VAR] == BindingCount[CACHE_GROUP_DYN_UB_STAT_VAR] + BindingCount[CACHE_GROUP_DYN_SB_STAT_VAR]);
    VERIFY_EXPR(BindingIndices[CACHE_GROUP_OTHER_STAT_VAR] == BindingCount[CACHE_GROUP_DYN_UB_STAT_VAR] + BindingCount[CACHE_GROUP_DYN_SB_STAT_VAR] + BindingCount[CACHE_GROUP_OTHER_STAT_VAR]);
    VERIFY_EXPR(BindingIndices[CACHE_GROUP_DYN_UB_DYN_VAR] == BindingCount[CACHE_GROUP_DYN_UB_DYN_VAR]);
    VERIFY_EXPR(BindingIndices[CACHE_GROUP_DYN_SB_DYN_VAR] == BindingCount[CACHE_GROUP_DYN_UB_DYN_VAR] + BindingCount[CACHE_GROUP_DYN_SB_DYN_VAR]);
    VERIFY_EXPR(BindingIndices[CACHE_GROUP_OTHER_DYN_VAR] == BindingCount[CACHE_GROUP_DYN_UB_DYN_VAR] + BindingCount[CACHE_GROUP_DYN_SB_DYN_VAR] + BindingCount[CACHE_GROUP_OTHER_DYN_VAR]);

    Uint32 NumGroups = 0;
    if (BindGroupIdToIndex[BIND_GROUP_ID_STATIC_MUTABLE] < MAX_BIND_GROUPS)
    {
        const Uint32 BindGroupIndex = BindGroupIdToIndex[BIND_GROUP_ID_STATIC_MUTABLE];
        m_BindGroupSizes[BindGroupIndex] =
            CacheGroupSizes[CACHE_GROUP_DYN_UB_STAT_VAR] +
            CacheGroupSizes[CACHE_GROUP_DYN_SB_STAT_VAR] +
            CacheGroupSizes[CACHE_GROUP_OTHER_STAT_VAR];
        m_DynamicOffsetCounts[BindGroupIndex] =
            CacheGroupSizes[CACHE_GROUP_DYN_UB_STAT_VAR] +
            CacheGroupSizes[CACHE_GROUP_DYN_SB_STAT_VAR];
        ++NumGroups;
    }

    if (BindGroupIdToIndex[BIND_GROUP_ID_DYNAMIC] < MAX_BIND_GROUPS)
    {
        const Uint32 BindGroupIndex = BindGroupIdToIndex[BIND_GROUP_ID_DYNAMIC];
        m_BindGroupSizes[BindGroupIndex] =
            CacheGroupSizes[CACHE_GROUP_DYN_UB_DYN_VAR] +
            CacheGroupSizes[CACHE_GROUP_DYN_SB_DYN_VAR] +
            CacheGroupSizes[CACHE_GROUP_OTHER_DYN_VAR];
        m_DynamicOffsetCounts[BindGroupIndex] =
            CacheGroupSizes[CACHE_GROUP_DYN_UB_DYN_VAR] +
            CacheGroupSizes[CACHE_GROUP_DYN_SB_DYN_VAR];
        ++NumGroups;
    }

#ifdef DILIGENT_DEBUG
    for (Uint32 i = 0; i < NumGroups; ++i)
        VERIFY_EXPR(m_BindGroupSizes[i] != ~0U && m_BindGroupSizes[i] > 0);
#else
    (void)NumGroups;
#endif
    VERIFY_EXPR(NumGroups == GetNumBindGroups());

    // Since WebGPU does not support multithreading, we can't create the bind group layouts
    // here as the signature may be created in a worker thread.
}

WGPUBindGroupLayout PipelineResourceSignatureWebGPUImpl::GetWGPUBindGroupLayout(BIND_GROUP_ID GroupId)
{
    if (m_BindGroupLayoutsCreateInfo)
    {
        const WGPUBindGroupLayoutsCreateInfo::EntriesArrayType& wgpuBGLayoutEntries = m_BindGroupLayoutsCreateInfo->wgpuBGLayoutEntries;

        WGPUDevice wgpuDevice = GetDevice()->GetWebGPUDevice();

        for (size_t i = 0; i < wgpuBGLayoutEntries.size(); ++i)
        {
            const auto& wgpuEntries = wgpuBGLayoutEntries[i];
            if (wgpuEntries.empty())
                continue;

            const std::string Label = std::string{m_Desc.Name} + " - bind group " + std::to_string(i);

            WGPUBindGroupLayoutDescriptor BGLayoutDesc{};
            BGLayoutDesc.label      = Label.c_str();
            BGLayoutDesc.entryCount = wgpuEntries.size();
            BGLayoutDesc.entries    = wgpuEntries.data();

            m_wgpuBindGroupLayouts[i].Reset(wgpuDeviceCreateBindGroupLayout(wgpuDevice, &BGLayoutDesc));
            VERIFY_EXPR(m_wgpuBindGroupLayouts[i]);
        }

        m_BindGroupLayoutsCreateInfo.reset();
    }
    return m_wgpuBindGroupLayouts[GroupId];
}

PipelineResourceSignatureWebGPUImpl::~PipelineResourceSignatureWebGPUImpl()
{
    Destruct();
}

void PipelineResourceSignatureWebGPUImpl::InitSRBResourceCache(ShaderResourceCacheWebGPU& ResourceCache)
{
    const Uint32 NumGroups = GetNumBindGroups();
#ifdef DILIGENT_DEBUG
    for (Uint32 i = 0; i < NumGroups; ++i)
        VERIFY_EXPR(m_BindGroupSizes[i] != ~0U);
#endif

    auto& CacheMemAllocator = m_SRBMemAllocator.GetResourceCacheDataAllocator(0);
    ResourceCache.InitializeGroups(CacheMemAllocator, NumGroups, m_BindGroupSizes.data());

    const Uint32                   TotalResources = GetTotalResourceCount();
    const ResourceCacheContentType CacheType      = ResourceCache.GetContentType();
    for (Uint32 r = 0; r < TotalResources; ++r)
    {
        const PipelineResourceDesc& ResDesc = GetResourceDesc(r);
        const ResourceAttribs&      Attr    = GetResourceAttribs(r);
        if (Attr.GetBindGroupEntryType() == BindGroupEntryType::Sampler && Attr.IsImmutableSamplerAssigned())
        {
            // Skip immutable samplers
            VERIFY_EXPR(ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER);
            continue;
        }

        ResourceCache.InitializeResources(Attr.BindGroup, Attr.CacheOffset(CacheType), ResDesc.ArraySize,
                                          Attr.GetBindGroupEntryType(), Attr.IsImmutableSamplerAssigned());
    }

    // Initialize immutable samplers
    for (Uint32 i = 0; i < m_Desc.NumImmutableSamplers; ++i)
    {
        const ImmutableSamplerAttribsWebGPU& ImtblSampAttr = m_pImmutableSamplerAttribs[i];
        VERIFY_EXPR(ImtblSampAttr.IsAllocated());
        VERIFY_EXPR(ImtblSampAttr.ArraySize > 0);
        ResourceCache.InitializeResources(ImtblSampAttr.BindGroup, ImtblSampAttr.CacheOffset, ImtblSampAttr.ArraySize,
                                          BindGroupEntryType::Sampler, /*HasImmutableSampler = */ true);

        if (const RefCntAutoPtr<SamplerWebGPUImpl>& pSampler = m_pImmutableSamplers[i])
        {
            for (Uint32 elem = 0; elem < ImtblSampAttr.ArraySize; ++elem)
            {
                ResourceCache.SetResource(ImtblSampAttr.BindGroup, ImtblSampAttr.CacheOffset + elem, pSampler);
            }
        }
    }

#ifdef DILIGENT_DEBUG
    ResourceCache.DbgVerifyResourceInitialization();
#endif
}

void PipelineResourceSignatureWebGPUImpl::CopyStaticResources(ShaderResourceCacheWebGPU& DstResourceCache) const
{
    if (!HasBindGroup(BIND_GROUP_ID_STATIC_MUTABLE) || m_pStaticResCache == nullptr)
        return;

    // SrcResourceCache contains only static resources.
    // In case of SRB, DstResourceCache contains static, mutable and dynamic resources.
    // In case of Signature, DstResourceCache contains only static resources.
    const ShaderResourceCacheWebGPU&            SrcResourceCache = *m_pStaticResCache;
    const Uint32                                StaticGroupIdx   = GetBindGroupIndex<BIND_GROUP_ID_STATIC_MUTABLE>();
    const ShaderResourceCacheWebGPU::BindGroup& SrcBindGroup     = SrcResourceCache.GetBindGroup(StaticGroupIdx);
    const ShaderResourceCacheWebGPU::BindGroup& DstBindGroup     = const_cast<const ShaderResourceCacheWebGPU&>(DstResourceCache).GetBindGroup(StaticGroupIdx);
    const std::pair<Uint32, Uint32>             ResIdxRange      = GetResourceIndexRange(SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    const ResourceCacheContentType              SrcCacheType     = SrcResourceCache.GetContentType();
    const ResourceCacheContentType              DstCacheType     = DstResourceCache.GetContentType();

    for (Uint32 r = ResIdxRange.first; r < ResIdxRange.second; ++r)
    {
        const PipelineResourceDesc& ResDesc = GetResourceDesc(r);
        const ResourceAttribs&      Attr    = GetResourceAttribs(r);
        VERIFY_EXPR(ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

        if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER && Attr.IsImmutableSamplerAssigned())
        {
            // Skip immutable samplers as they are initialized in InitSRBResourceCache()
            if (DstCacheType == ResourceCacheContentType::SRB)
            {
                for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd)
                {
                    VERIFY(DstBindGroup.GetResource(Attr.CacheOffset(DstCacheType) + ArrInd).pObject,
                           "Immutable sampler must have been initialized in InitSRBResourceCache(). This is likely a bug.");
                }
            }
            continue;
        }

        for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd)
        {
            const Uint32                               SrcCacheOffset = Attr.CacheOffset(SrcCacheType) + ArrInd;
            const ShaderResourceCacheWebGPU::Resource& SrcCachedRes   = SrcBindGroup.GetResource(SrcCacheOffset);
            IDeviceObject*                             pObject        = SrcCachedRes.pObject;
            if (pObject == nullptr)
            {
                if (DstCacheType == ResourceCacheContentType::SRB)
                    LOG_ERROR_MESSAGE("No resource is assigned to static shader variable '", GetShaderResourcePrintName(ResDesc, ArrInd), "' in pipeline resource signature '", m_Desc.Name, "'.");
                continue;
            }

            const Uint32                               DstCacheOffset = Attr.CacheOffset(DstCacheType) + ArrInd;
            const ShaderResourceCacheWebGPU::Resource& DstCachedRes   = DstBindGroup.GetResource(DstCacheOffset);
            VERIFY_EXPR(SrcCachedRes.Type == DstCachedRes.Type);

            const IDeviceObject* pCachedResource = DstCachedRes.pObject;
            if (pCachedResource != pObject)
            {
                DEV_CHECK_ERR(pCachedResource == nullptr, "Static resource has already been initialized, and the new resource does not match previously assigned resource");
                DstResourceCache.SetResource(StaticGroupIdx,
                                             DstCacheOffset,
                                             SrcCachedRes.pObject,
                                             SrcCachedRes.BufferBaseOffset,
                                             SrcCachedRes.BufferRangeSize);
            }
        }
    }

#ifdef DILIGENT_DEBUG
    DstResourceCache.DbgVerifyDynamicBuffersCounter();
#endif
}

template <>
Uint32 PipelineResourceSignatureWebGPUImpl::GetBindGroupIndex<PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_STATIC_MUTABLE>() const
{
    VERIFY(HasBindGroup(BIND_GROUP_ID_STATIC_MUTABLE), "This signature does not have static/mutable descriptor set");
    return 0;
}

template <>
Uint32 PipelineResourceSignatureWebGPUImpl::GetBindGroupIndex<PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_DYNAMIC>() const
{
    VERIFY(HasBindGroup(BIND_GROUP_ID_DYNAMIC), "This signature does not have dynamic descriptor set");
    return HasBindGroup(BIND_GROUP_ID_STATIC_MUTABLE) ? 1 : 0;
}

PipelineResourceSignatureWebGPUImpl::PipelineResourceSignatureWebGPUImpl(IReferenceCounters*                                pRefCounters,
                                                                         RenderDeviceWebGPUImpl*                            pDevice,
                                                                         const PipelineResourceSignatureDesc&               Desc,
                                                                         const PipelineResourceSignatureInternalDataWebGPU& InternalData) :
    TPipelineResourceSignatureBase{pRefCounters, pDevice, Desc, InternalData}
{
    try
    {
        Deserialize(
            GetRawAllocator(), DecoupleCombinedSamplers(Desc), InternalData, /*CreateImmutableSamplers = */ true,
            [this]() //
            {
                CreateBindGroupLayouts(/*IsSerialized*/ true);
            },
            [this]() //
            {
                return ShaderResourceCacheWebGPU::GetRequiredMemorySize(GetNumBindGroups(), m_BindGroupSizes.data());
            });
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

#ifdef DILIGENT_DEVELOPMENT
bool PipelineResourceSignatureWebGPUImpl::DvpValidateCommittedResource(const DeviceContextWebGPUImpl*   pDeviceCtx,
                                                                       const WGSLShaderResourceAttribs& WGSLAttribs,
                                                                       Uint32                           ResIndex,
                                                                       const ShaderResourceCacheWebGPU& ResourceCache,
                                                                       const char*                      ShaderName,
                                                                       const char*                      PSOName) const
{
    VERIFY_EXPR(ResIndex < m_Desc.NumResources);
    const PipelineResourceDesc& ResDesc    = m_Desc.Resources[ResIndex];
    const ResourceAttribs&      ResAttribs = m_pResourceAttribs[ResIndex];
    VERIFY(strcmp(ResDesc.Name, WGSLAttribs.Name) == 0, "Inconsistent resource names");

    const ShaderResourceCacheWebGPU::BindGroup& BindGroupResources = ResourceCache.GetBindGroup(ResAttribs.BindGroup);
    const ResourceCacheContentType              CacheType          = ResourceCache.GetContentType();
    const Uint32                                CacheOffset        = ResAttribs.CacheOffset(CacheType);

    VERIFY_EXPR(WGSLAttribs.ArraySize <= ResAttribs.ArraySize);

    bool BindingsOK = true;
    for (Uint32 ArrIndex = 0; ArrIndex < WGSLAttribs.ArraySize; ++ArrIndex)
    {
        const ShaderResourceCacheWebGPU::Resource& Res = BindGroupResources.GetResource(CacheOffset + ArrIndex);
        if (!Res)
        {
            LOG_ERROR_MESSAGE("No resource is bound to variable '", GetShaderResourcePrintName(WGSLAttribs, ArrIndex),
                              "' in shader '", ShaderName, "' of PSO '", PSOName, "'");
            BindingsOK = false;
            continue;
        }

        if (ResAttribs.IsCombinedWithSampler())
        {
            const PipelineResourceDesc& SamplerResDesc = GetResourceDesc(ResAttribs.SamplerInd);
            const ResourceAttribs&      SamplerAttribs = GetResourceAttribs(ResAttribs.SamplerInd);
            VERIFY_EXPR(SamplerResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER);
            VERIFY_EXPR(SamplerResDesc.ArraySize == 1 || SamplerResDesc.ArraySize == ResDesc.ArraySize);
            if (ArrIndex < SamplerResDesc.ArraySize)
            {
                const ShaderResourceCacheWebGPU::BindGroup& SamBindGroupResources = ResourceCache.GetBindGroup(SamplerAttribs.BindGroup);
                const Uint32                                SamCacheOffset        = SamplerAttribs.CacheOffset(CacheType);
                const ShaderResourceCacheWebGPU::Resource&  Sam                   = SamBindGroupResources.GetResource(SamCacheOffset + ArrIndex);
                if (!Sam)
                {
                    if (!SamplerAttribs.IsImmutableSamplerAssigned())
                    {
                        LOG_ERROR_MESSAGE("No sampler is bound to sampler variable '", GetShaderResourcePrintName(SamplerResDesc, ArrIndex),
                                          "' combined with texture '", WGSLAttribs.Name, "' in shader '", ShaderName, "' of PSO '", PSOName, "'.");
                        BindingsOK = false;
                    }
                    else
                    {
                        UNEXPECTED("Immutable sampler must have been initialized in InitSRBResourceCache(). This is likely a bug.");
                        BindingsOK = false;
                    }
                }
            }
        }

        static_assert(static_cast<size_t>(BindGroupEntryType::Count) == 12, "Please update the switch below to handle the new bind group entry type");
        switch (ResAttribs.GetBindGroupEntryType())
        {
            case BindGroupEntryType::UniformBuffer:
            case BindGroupEntryType::UniformBufferDynamic:
                VERIFY_EXPR(ResDesc.ResourceType == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER);
                // When can use raw cast here because the dynamic type is verified when the resource
                // is bound. It will be null if the type is incorrect.
                if (const BufferWebGPUImpl* pBufferWebGPU = Res.pObject.RawPtr<BufferWebGPUImpl>())
                {
                    pBufferWebGPU->DvpVerifyDynamicAllocation(pDeviceCtx);

                    if ((WGSLAttribs.BufferStaticSize != 0) &&
                        (GetDevice()->GetValidationFlags() & VALIDATION_FLAG_CHECK_SHADER_BUFFER_SIZE) != 0 &&
                        (pBufferWebGPU->GetDesc().Size < WGSLAttribs.BufferStaticSize))
                    {
                        // It is OK if robustBufferAccess feature is enabled, otherwise access outside of buffer range may lead to crash or undefined behavior.
                        LOG_WARNING_MESSAGE("The size of uniform buffer '",
                                            pBufferWebGPU->GetDesc().Name, "' bound to shader variable '",
                                            GetShaderResourcePrintName(WGSLAttribs, ArrIndex), "' is ", pBufferWebGPU->GetDesc().Size,
                                            " bytes, but the shader expects at least ", WGSLAttribs.BufferStaticSize,
                                            " bytes.");
                    }
                }
                break;

            case BindGroupEntryType::StorageBuffer:
            case BindGroupEntryType::StorageBuffer_ReadOnly:
            case BindGroupEntryType::StorageBufferDynamic:
            case BindGroupEntryType::StorageBufferDynamic_ReadOnly:
                VERIFY_EXPR(ResDesc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV || ResDesc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV);
                // When can use raw cast here because the dynamic type is verified when the resource
                // is bound. It will be null if the type is incorrect.
                if (const BufferViewWebGPUImpl* pBufferViewWebGPU = Res.pObject.RawPtr<BufferViewWebGPUImpl>())
                {
                    const BufferWebGPUImpl* pBufferWebGPU = ClassPtrCast<BufferWebGPUImpl>(pBufferViewWebGPU->GetBuffer());
                    const BufferViewDesc&   ViewDesc      = pBufferViewWebGPU->GetDesc();
                    const BufferDesc&       BuffDesc      = pBufferWebGPU->GetDesc();

                    pBufferWebGPU->DvpVerifyDynamicAllocation(pDeviceCtx);

                    if (BuffDesc.ElementByteStride == 0)
                    {
                        if ((ViewDesc.ByteWidth < WGSLAttribs.BufferStaticSize) &&
                            (GetDevice()->GetValidationFlags() & VALIDATION_FLAG_CHECK_SHADER_BUFFER_SIZE) != 0)
                        {
                            // It is OK if robustBufferAccess feature is enabled, otherwise access outside of buffer range may lead to crash or undefined behavior.
                            LOG_WARNING_MESSAGE("The size of buffer view '", ViewDesc.Name, "' of buffer '", BuffDesc.Name, "' bound to shader variable '",
                                                GetShaderResourcePrintName(WGSLAttribs, ArrIndex), "' is ", ViewDesc.ByteWidth, " bytes, but the shader expects at least ",
                                                WGSLAttribs.BufferStaticSize, " bytes.");
                        }
                    }
                    else
                    {
                        if ((ViewDesc.ByteWidth < WGSLAttribs.BufferStaticSize || (ViewDesc.ByteWidth - WGSLAttribs.BufferStaticSize) % BuffDesc.ElementByteStride != 0) &&
                            (GetDevice()->GetValidationFlags() & VALIDATION_FLAG_CHECK_SHADER_BUFFER_SIZE) != 0)
                        {
                            // For buffers with dynamic arrays we know only static part size and array element stride.
                            // Element stride in the shader may be differ than in the code. Here we check that the buffer size is exactly the same as the array with N elements.
                            LOG_WARNING_MESSAGE("The size (", ViewDesc.ByteWidth, ") and stride (", BuffDesc.ElementByteStride, ") of buffer view '",
                                                ViewDesc.Name, "' of buffer '", BuffDesc.Name, "' bound to shader variable '",
                                                GetShaderResourcePrintName(WGSLAttribs, ArrIndex), "' are incompatible with what the shader expects. This may be the result of the array element size mismatch.");
                        }
                    }
                }
                break;

            case BindGroupEntryType::Texture:
            case BindGroupEntryType::StorageTexture_WriteOnly:
            case BindGroupEntryType::StorageTexture_ReadOnly:
            case BindGroupEntryType::StorageTexture_ReadWrite:
                VERIFY_EXPR(ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV ||
                            ResDesc.ResourceType == SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT ||
                            ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_UAV);
                // When can use raw cast here because the dynamic type is verified when the resource
                // is bound. It will be null if the type is incorrect.
                if (const TextureViewWebGPUImpl* pTexViewWebGPU = Res.pObject.RawPtr<TextureViewWebGPUImpl>())
                {
                    if (!ValidateResourceViewDimension(WGSLAttribs.Name, WGSLAttribs.ArraySize, ArrIndex, pTexViewWebGPU, WGSLAttribs.GetResourceDimension(), WGSLAttribs.IsMultisample()))
                        BindingsOK = false;
                }
                break;

            case BindGroupEntryType::Sampler:
                // Nothing else to check
                break;

            case BindGroupEntryType::ExternalTexture:
                break;

            default:
                break;
                // Nothing to do
        }
    }

    return BindingsOK;
}

bool PipelineResourceSignatureWebGPUImpl::DvpValidateImmutableSampler(const WGSLShaderResourceAttribs& WGSLAttribs,
                                                                      Uint32                           ImtblSamIndex,
                                                                      const ShaderResourceCacheWebGPU& ResourceCache,
                                                                      const char*                      ShaderName,
                                                                      const char*                      PSOName) const
{
    VERIFY_EXPR(ImtblSamIndex < m_Desc.NumImmutableSamplers);
    const ImmutableSamplerAttribsWebGPU& ImtblSamAttribs = m_pImmutableSamplerAttribs[ImtblSamIndex];

    const ShaderResourceCacheWebGPU::BindGroup& BindGroupResources = ResourceCache.GetBindGroup(ImtblSamAttribs.BindGroup);
    VERIFY_EXPR(WGSLAttribs.ArraySize <= ImtblSamAttribs.ArraySize);

    bool BindingsOK = true;
    for (Uint32 ArrIndex = 0; ArrIndex < WGSLAttribs.ArraySize; ++ArrIndex)
    {
        const ShaderResourceCacheWebGPU::Resource& Res = BindGroupResources.GetResource(ImtblSamAttribs.CacheOffset + ArrIndex);
        DEV_CHECK_ERR(Res.HasImmutableSampler, "Resource must have immutable sampler assigned");
        if (!Res)
        {
            DEV_ERROR("Immutable sampler is not initialized for variable '", GetShaderResourcePrintName(WGSLAttribs, ArrIndex),
                      "' in shader '", ShaderName, "' of PSO '", PSOName, "'. This is likely a bug as immutable samplers should be initialized by InitSRBResourceCache.");
            BindingsOK = false;
        }
    }
    return BindingsOK;
}
#endif

} // namespace Diligent
