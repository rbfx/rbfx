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

#include "pch.h"

#include "PipelineResourceSignatureD3D12Impl.hpp"

#include <unordered_map>

#include "RenderDeviceD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"
#include "BufferViewD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "TextureViewD3D12Impl.hpp"
#include "ShaderVariableD3D.hpp"

#include "D3D12TypeConversions.hpp"

namespace Diligent
{

namespace
{

void ValidatePipelineResourceSignatureDescD3D12(const PipelineResourceSignatureDesc& Desc) noexcept(false)
{
    {
        std::unordered_multimap<HashMapStringKey, SHADER_TYPE> ResNameToShaderStages;
        for (Uint32 i = 0; i < Desc.NumResources; ++i)
        {
            const auto& Res = Desc.Resources[i];

            ResNameToShaderStages.emplace(Res.Name, Res.ShaderStages);
            auto range          = ResNameToShaderStages.equal_range(Res.Name);
            auto multi_stage_it = ResNameToShaderStages.end();
            for (auto it = range.first; it != range.second; ++it)
            {
                if (!IsPowerOfTwo(it->second))
                {
                    if (multi_stage_it == ResNameToShaderStages.end())
                        multi_stage_it = it;
                    else
                    {
                        LOG_ERROR_AND_THROW("Pipeline resource signature '", (Desc.Name != nullptr ? Desc.Name : ""),
                                            "' defines separate resources with the name '", Res.Name, "' in shader stages ",
                                            GetShaderStagesString(multi_stage_it->second), " and ",
                                            GetShaderStagesString(it->second),
                                            ". In Direct3D12 backend, only one resource in the group of resources with the same name can be shared between more than "
                                            "one shader stages. To solve this problem, use single shader stage for all but one resource with the same name.");
                    }
                }
            }
        }
    }

    {
        std::unordered_multimap<HashMapStringKey, SHADER_TYPE> SamNameToShaderStages;
        for (Uint32 i = 0; i < Desc.NumImmutableSamplers; ++i)
        {
            const auto& Sam = Desc.ImmutableSamplers[i];

            const auto* Name = Sam.SamplerOrTextureName;
            SamNameToShaderStages.emplace(Name, Sam.ShaderStages);
            auto range          = SamNameToShaderStages.equal_range(Name);
            auto multi_stage_it = SamNameToShaderStages.end();
            for (auto it = range.first; it != range.second; ++it)
            {
                if (!IsPowerOfTwo(it->second))
                {
                    if (multi_stage_it == SamNameToShaderStages.end())
                        multi_stage_it = it;
                    else
                    {
                        LOG_ERROR_AND_THROW("Pipeline resource signature '", (Desc.Name != nullptr ? Desc.Name : ""),
                                            "' defines separate immutable samplers with the name '", Name, "' in shader stages ",
                                            GetShaderStagesString(multi_stage_it->second), " and ",
                                            GetShaderStagesString(it->second),
                                            ". In Direct3D12 backend, only one immutable sampler in the group of samplers with the same name can be shared between more than "
                                            "one shader stages. To solve this problem, use single shader stage for all but one immutable sampler with the same name.");
                    }
                }
            }
        }
    }
}

} // namespace


PipelineResourceSignatureD3D12Impl::PipelineResourceSignatureD3D12Impl(IReferenceCounters*                  pRefCounters,
                                                                       RenderDeviceD3D12Impl*               pDevice,
                                                                       const PipelineResourceSignatureDesc& Desc,
                                                                       SHADER_TYPE                          ShaderStages,
                                                                       bool                                 bIsDeviceInternal) :
    TPipelineResourceSignatureBase{pRefCounters, pDevice, Desc, ShaderStages, bIsDeviceInternal}
{
    try
    {
        ValidatePipelineResourceSignatureDescD3D12(Desc);

        Initialize(
            GetRawAllocator(), DecoupleCombinedSamplers(Desc), m_ImmutableSamplers,
            [this]() //
            {
                AllocateRootParameters(/*IsSerialized*/ false);
            },
            [this]() //
            {
                return ShaderResourceCacheD3D12::GetMemoryRequirements(m_RootParams).TotalSize;
            });
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

void PipelineResourceSignatureD3D12Impl::AllocateRootParameters(const bool IsSerialized)
{
    // Index of the assigned sampler, for every texture SRV in m_Desc.Resources, or InvalidSamplerInd.
    std::vector<Uint32> TextureSrvToAssignedSamplerInd(m_Desc.NumResources, ResourceAttribs::InvalidSamplerInd);
    // Index of the immutable sampler for every sampler in m_Desc.Resources, or InvalidImmutableSamplerIndex.
    std::vector<Uint32> ResourceToImmutableSamplerInd(m_Desc.NumResources, InvalidImmutableSamplerIndex);
    for (Uint32 i = 0; i < m_Desc.NumResources; ++i)
    {
        const auto& ResDesc = m_Desc.Resources[i];

        if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER)
        {
            // We only need to search for immutable samplers for SHADER_RESOURCE_TYPE_SAMPLER.
            // For SHADER_RESOURCE_TYPE_TEXTURE_SRV, we will look for the assigned sampler and check if it is immutable.

            // If there is an immutable sampler that is not defined as resource, e.g.:
            //
            //      PipelineResourceDesc Resources[] = {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, ...}
            //      ImmutableSamplerDesc ImtblSams[] = {SHADER_TYPE_PIXEL, "g_Texture", ...}
            //
            // the sampler will not be assigned to the texture. It will be defined as static sampler when D3D12 PSO is created,
            // will be added to bindings map by UpdateShaderResourceBindingMap and then properly mapped to the shader sampler register.

            // Note that FindImmutableSampler() below will work properly both when combined texture samplers are used and when not:
            //  - When combined texture samplers are used, sampler suffix will not be null,
            //    and we will be looking for the 'Texture_sampler' name.
            //  - When combined texture samplers are not used, sampler suffix will be null,
            //    and we will be looking for the sampler name itself.
            const auto SrcImmutableSamplerInd = FindImmutableSampler(ResDesc.ShaderStages, ResDesc.Name);
            if (SrcImmutableSamplerInd != InvalidImmutableSamplerIndex)
            {
                ResourceToImmutableSamplerInd[i] = SrcImmutableSamplerInd;
                // Set the immutable sampler array size to match the resource array size
                auto& DstImtblSampAttribs = m_ImmutableSamplers[SrcImmutableSamplerInd];
                // One immutable sampler may be used by different arrays in different shader stages - use the maximum array size
                DstImtblSampAttribs.ArraySize = std::max(DstImtblSampAttribs.ArraySize, ResDesc.ArraySize);
            }
        }

        if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV)
        {
            TextureSrvToAssignedSamplerInd[i] = FindAssignedSampler(ResDesc, ResourceAttribs::InvalidSamplerInd);
        }
    }

    // The total number of resources (counting array size), for every descriptor range type
    std::array<Uint32, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER + 1> NumResources = {};

    // Cache table sizes for static resources
    std::array<Uint32, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER + 1> StaticResCacheTblSizes{};

    // Allocate registers for immutable samplers first
    for (Uint32 i = 0; i < m_Desc.NumImmutableSamplers; ++i)
    {
        auto&          ImmutableSampler    = m_ImmutableSamplers[i];
        constexpr auto DescriptorRangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        if (!IsSerialized)
        {
            ImmutableSampler.RegisterSpace  = 0;
            ImmutableSampler.ShaderRegister = NumResources[DescriptorRangeType];
        }
        else
        {
            DEV_CHECK_ERR(ImmutableSampler.RegisterSpace == 0, "Deserialized register space (", ImmutableSampler.RegisterSpace, ") is invalid: 0 is expected.");
            DEV_CHECK_ERR(ImmutableSampler.ShaderRegister == NumResources[DescriptorRangeType],
                          "Deserialized shader register (", ImmutableSampler.ShaderRegister, ") is invalid: ", NumResources[DescriptorRangeType], " is expected.");
        }
        NumResources[DescriptorRangeType] += ImmutableSampler.ArraySize;
    }


    RootParamsBuilder ParamsBuilder;

    Uint32 NextRTSizedArraySpace = 1;
    for (Uint32 i = 0; i < m_Desc.NumResources; ++i)
    {
        const auto& ResDesc = m_Desc.Resources[i];
        VERIFY(i == 0 || ResDesc.VarType >= m_Desc.Resources[i - 1].VarType, "Resources must be sorted by variable type");

        auto AssignedSamplerInd     = TextureSrvToAssignedSamplerInd[i];
        auto SrcImmutableSamplerInd = ResourceToImmutableSamplerInd[i];
        if (AssignedSamplerInd != ResourceAttribs::InvalidSamplerInd)
        {
            VERIFY_EXPR(ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV);
            VERIFY_EXPR(SrcImmutableSamplerInd == InvalidImmutableSamplerIndex);
            SrcImmutableSamplerInd = ResourceToImmutableSamplerInd[AssignedSamplerInd];
        }

        const auto d3d12DescriptorRangeType = ResourceTypeToD3D12DescriptorRangeType(ResDesc.ResourceType);
        const bool IsRTSizedArray           = (ResDesc.Flags & PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY) != 0;
        Uint32     Register                 = 0;
        Uint32     Space                    = 0;
        Uint32     SRBRootIndex             = ResourceAttribs::InvalidSRBRootIndex;
        Uint32     SRBOffsetFromTableStart  = ResourceAttribs::InvalidOffset;
        Uint32     SigRootIndex             = ResourceAttribs::InvalidSigRootIndex;
        Uint32     SigOffsetFromTableStart  = ResourceAttribs::InvalidOffset;

        auto d3d12RootParamType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(D3D12_ROOT_PARAMETER_TYPE_UAV + 1);
        // Do not allocate resource slot for immutable samplers that are also defined as resource
        if (!(ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER && SrcImmutableSamplerInd != InvalidImmutableSamplerIndex))
        {
            if (ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
            {
                // Use artificial root signature:
                // SRVs at root index D3D12_DESCRIPTOR_RANGE_TYPE_SRV (0)
                // UAVs at root index D3D12_DESCRIPTOR_RANGE_TYPE_UAV (1)
                // CBVs at root index D3D12_DESCRIPTOR_RANGE_TYPE_CBV (2)
                // Samplers at root index D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER (3)
                SigRootIndex            = d3d12DescriptorRangeType;
                SigOffsetFromTableStart = StaticResCacheTblSizes[SigRootIndex];
                StaticResCacheTblSizes[SigRootIndex] += ResDesc.ArraySize;
            }

            if (IsRTSizedArray)
            {
                // All run-time sized arrays are allocated in separate spaces.
                Space    = NextRTSizedArraySpace++;
                Register = 0;
            }
            else
            {
                // Normal resources go into space 0.
                Space    = 0;
                Register = NumResources[d3d12DescriptorRangeType];
                NumResources[d3d12DescriptorRangeType] += ResDesc.ArraySize;
            }

            const auto dbgValidResourceFlags = GetValidPipelineResourceFlags(ResDesc.ResourceType);
            VERIFY((ResDesc.Flags & ~dbgValidResourceFlags) == 0, "Invalid resource flags. This error should've been caught by ValidatePipelineResourceSignatureDesc.");

            const auto UseDynamicOffset  = (ResDesc.Flags & PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS) == 0;
            const auto IsFormattedBuffer = (ResDesc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) != 0;
            const auto IsArray           = ResDesc.ArraySize != 1;

            d3d12RootParamType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            static_assert(SHADER_RESOURCE_TYPE_LAST == SHADER_RESOURCE_TYPE_ACCEL_STRUCT, "Please update the switch below to handle the new shader resource type");
            switch (ResDesc.ResourceType)
            {
                case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
                    VERIFY(!IsFormattedBuffer, "Constant buffers can't be labeled as formatted. This error should've been caught by ValidatePipelineResourceSignatureDesc().");
                    d3d12RootParamType = UseDynamicOffset && !IsArray ? D3D12_ROOT_PARAMETER_TYPE_CBV : D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    break;

                case SHADER_RESOURCE_TYPE_BUFFER_SRV:
                    d3d12RootParamType = UseDynamicOffset && !IsFormattedBuffer && !IsArray ? D3D12_ROOT_PARAMETER_TYPE_SRV : D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    break;

                case SHADER_RESOURCE_TYPE_BUFFER_UAV:
                    // Always allocate buffer UAVs in descriptor tables
                    d3d12RootParamType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    break;

                default:
                    d3d12RootParamType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            }

            ParamsBuilder.AllocateResourceSlot(ResDesc.ShaderStages, ResDesc.VarType, d3d12RootParamType,
                                               d3d12DescriptorRangeType, ResDesc.ArraySize, Register, Space,
                                               SRBRootIndex, SRBOffsetFromTableStart);
        }
        else
        {
            VERIFY_EXPR(AssignedSamplerInd == ResourceAttribs::InvalidSamplerInd);
            // Use register and space assigned to the immutable sampler
            const auto& ImtblSamAttribs = GetImmutableSamplerAttribs(SrcImmutableSamplerInd);
            VERIFY_EXPR(ImtblSamAttribs.IsValid());
            // Initialize space and register, which are required for register remapping
            Space    = ImtblSamAttribs.RegisterSpace;
            Register = ImtblSamAttribs.ShaderRegister;
        }

        auto* const pAttrib = m_pResourceAttribs + i;
        if (!IsSerialized)
        {
            new (pAttrib) ResourceAttribs //
                {
                    Register,
                    Space,
                    AssignedSamplerInd,
                    SRBRootIndex,
                    SRBOffsetFromTableStart,
                    SigRootIndex,
                    SigOffsetFromTableStart,
                    SrcImmutableSamplerInd != InvalidImmutableSamplerIndex,
                    d3d12RootParamType //
                };
        }
        else
        {
            DEV_CHECK_ERR(pAttrib->Register == Register,
                          "Deserialized shader register (", pAttrib->Register, ") is invalid: ", Register, " is expected.");
            DEV_CHECK_ERR(pAttrib->Space == Space,
                          "Deserialized shader space (", pAttrib->Space, ") is invalid: ", Space, " is expected.");
            DEV_CHECK_ERR(pAttrib->SamplerInd == AssignedSamplerInd,
                          "Deserialized sampler index (", pAttrib->SamplerInd, ") is invalid: ", AssignedSamplerInd, " is expected.");
            DEV_CHECK_ERR(pAttrib->SRBRootIndex == SRBRootIndex,
                          "Deserialized root index (", pAttrib->SRBRootIndex, ") is invalid: ", SRBRootIndex, " is expected.");
            DEV_CHECK_ERR(pAttrib->SRBOffsetFromTableStart == SRBOffsetFromTableStart,
                          "Deserialized offset from table start (", pAttrib->SRBOffsetFromTableStart, ") is invalid: ", SRBOffsetFromTableStart, " is expected.");
            DEV_CHECK_ERR(pAttrib->SigRootIndex == SigRootIndex,
                          "Deserialized signature root index (", pAttrib->SigRootIndex, ") is invalid: ", SigRootIndex, " is expected.");
            DEV_CHECK_ERR(pAttrib->SigOffsetFromTableStart == SigOffsetFromTableStart,
                          "Deserialized signature offset from table start (", pAttrib->SigOffsetFromTableStart, ") is invalid: ", SigOffsetFromTableStart, " is expected.");
            DEV_CHECK_ERR(pAttrib->IsImmutableSamplerAssigned() == (SrcImmutableSamplerInd != InvalidImmutableSamplerIndex),
                          "Deserialized immutable sampler flag is invalid");
            DEV_CHECK_ERR(pAttrib->GetD3D12RootParamType() == d3d12RootParamType,
                          "Deserailized root parameter type is invalid.");
        }
    }
    ParamsBuilder.InitializeMgr(GetRawAllocator(), m_RootParams);

    if (GetNumStaticResStages() > 0)
    {
        m_pStaticResCache->Initialize(GetRawAllocator(), static_cast<Uint32>(StaticResCacheTblSizes.size()), StaticResCacheTblSizes.data());
    }
    else
    {
#ifdef DILIGENT_DEBUG
        for (auto TblSize : StaticResCacheTblSizes)
            VERIFY(TblSize == 0, "The size of every static resource cache table must be zero because there are no static resources in the PRS.");
#endif
    }
}

PipelineResourceSignatureD3D12Impl::~PipelineResourceSignatureD3D12Impl()
{
    Destruct();
}

void PipelineResourceSignatureD3D12Impl::Destruct()
{
    if (m_ImmutableSamplers != nullptr)
    {
        for (Uint32 i = 0; i < m_Desc.NumImmutableSamplers; ++i)
        {
            m_ImmutableSamplers[i].~ImmutableSamplerAttribs();
        }
        m_ImmutableSamplers = nullptr;
    }

    TPipelineResourceSignatureBase::Destruct();
}

void PipelineResourceSignatureD3D12Impl::InitSRBResourceCache(ShaderResourceCacheD3D12& ResourceCache)
{
    ResourceCache.Initialize(m_SRBMemAllocator.GetResourceCacheDataAllocator(0), m_pDevice, m_RootParams);
}

void PipelineResourceSignatureD3D12Impl::CopyStaticResources(ShaderResourceCacheD3D12& DstResourceCache) const
{
    if (m_pStaticResCache == nullptr)
        return;

    // SrcResourceCache contains only static resources.
    // In case of SRB, DstResourceCache contains static, mutable and dynamic resources.
    // In case of Signature, DstResourceCache contains only static resources.
    const auto& SrcResourceCache = *m_pStaticResCache;
    const auto  ResIdxRange      = GetResourceIndexRange(SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    auto* const d3d12Device      = GetDevice()->GetD3D12Device();
    const auto  SrcCacheType     = SrcResourceCache.GetContentType();
    const auto  DstCacheType     = DstResourceCache.GetContentType();
    VERIFY_EXPR(SrcCacheType == ResourceCacheContentType::Signature);

    for (Uint32 r = ResIdxRange.first; r < ResIdxRange.second; ++r)
    {
        const auto& ResDesc   = GetResourceDesc(r);
        const auto& Attr      = GetResourceAttribs(r);
        const bool  IsSampler = (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER);
        VERIFY_EXPR(ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

        if (IsSampler && Attr.IsImmutableSamplerAssigned())
        {
            // Immutable samplers should not be assigned cache space
            VERIFY_EXPR(Attr.RootIndex(ResourceCacheContentType::Signature) == ResourceAttribs::InvalidSigRootIndex);
            VERIFY_EXPR(Attr.RootIndex(ResourceCacheContentType::SRB) == ResourceAttribs::InvalidSRBRootIndex);
            VERIFY_EXPR(Attr.SigOffsetFromTableStart == ResourceAttribs::InvalidOffset);
            VERIFY_EXPR(Attr.SRBOffsetFromTableStart == ResourceAttribs::InvalidOffset);
            continue;
        }

        const auto  DstRootIndex = Attr.RootIndex(DstCacheType);
        const auto  SrcRootIndex = Attr.RootIndex(SrcCacheType);
        const auto& SrcRootTable = SrcResourceCache.GetRootTable(SrcRootIndex);
        const auto& DstRootTable = const_cast<const ShaderResourceCacheD3D12&>(DstResourceCache).GetRootTable(DstRootIndex);

        auto SrcCacheOffset = Attr.OffsetFromTableStart(SrcCacheType);
        auto DstCacheOffset = Attr.OffsetFromTableStart(DstCacheType);
        for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd, ++SrcCacheOffset, ++DstCacheOffset)
        {
            const auto& SrcRes = SrcRootTable.GetResource(SrcCacheOffset);
            if (!SrcRes.pObject)
            {
                if (DstCacheType == ResourceCacheContentType::SRB)
                    LOG_ERROR_MESSAGE("No resource is assigned to static shader variable '", GetShaderResourcePrintName(ResDesc, ArrInd), "' in pipeline resource signature '", m_Desc.Name, "'.");
                continue;
            }

            const auto& DstRes = DstRootTable.GetResource(DstCacheOffset);
            if (DstRes.pObject != SrcRes.pObject)
            {
                DEV_CHECK_ERR(DstRes.pObject == nullptr, "Static resource has already been initialized, and the new resource does not match previously assigned resource.");
                DstResourceCache.CopyResource(d3d12Device, DstRootIndex, DstCacheOffset, SrcRes);
            }
            else
            {
                VERIFY_EXPR(DstRes.pObject == SrcRes.pObject);
                VERIFY_EXPR(DstRes.Type == SrcRes.Type);
                VERIFY_EXPR(DstRes.CPUDescriptorHandle.ptr == SrcRes.CPUDescriptorHandle.ptr);
            }
        }
    }
}

void PipelineResourceSignatureD3D12Impl::CommitRootViews(const CommitCacheResourcesAttribs& CommitAttribs,
                                                         Uint64                             BuffersMask) const
{
    VERIFY_EXPR(CommitAttribs.pResourceCache != nullptr);

    while (BuffersMask != 0)
    {
        const auto  BufferBit = ExtractLSB(BuffersMask);
        const auto  RootInd   = PlatformMisc::GetLSB(BufferBit);
        const auto& CacheTbl  = CommitAttribs.pResourceCache->GetRootTable(RootInd);
        VERIFY_EXPR(CacheTbl.IsRootView());
        const auto& BaseRootIndex = CommitAttribs.BaseRootIndex;

        VERIFY_EXPR(CacheTbl.GetSize() == 1);
        const auto& Res = CacheTbl.GetResource(0);
        if (Res.IsNull())
        {
            LOG_ERROR_MESSAGE("Failed to bind root view at index ", BaseRootIndex + RootInd, ": no resource is bound in the cache.");
            continue;
        }

        const BufferD3D12Impl* pBuffer = nullptr;
        if (Res.Type == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER)
        {
            // No need to QueryInterface() - the type is verified when a resource is bound
            pBuffer = Res.pObject.ConstPtr<BufferD3D12Impl>();
        }
        else if (Res.Type == SHADER_RESOURCE_TYPE_BUFFER_SRV ||
                 Res.Type == SHADER_RESOURCE_TYPE_BUFFER_UAV)
        {
            auto* pBuffView = Res.pObject.ConstPtr<BufferViewD3D12Impl>();
            pBuffer         = pBuffView->GetBuffer<const BufferD3D12Impl>();
        }
        else
        {
            UNEXPECTED("Unexpected root view resource type");
        }
        VERIFY_EXPR(pBuffer != nullptr);

        auto BufferGPUAddress = pBuffer->GetGPUAddress(CommitAttribs.DeviceCtxId, nullptr /* Do not verify dynamic allocation here*/);
        if (BufferGPUAddress == 0)
        {
            // GPU address may be null if a dynamic buffer that is not used by the PSO has not been mapped yet.
            // Dynamic allocations will be checked by DvpValidateCommittedResource()
            return;
        }

        BufferGPUAddress += UINT64{Res.BufferBaseOffset} + UINT64{Res.BufferDynamicOffset};

        auto* const pd3d12CmdList = CommitAttribs.Ctx.GetCommandList();
        static_assert(SHADER_RESOURCE_TYPE_LAST == SHADER_RESOURCE_TYPE_ACCEL_STRUCT, "Please update the switch below to handle the new shader resource type");
        switch (Res.Type)
        {
            case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
                if (CommitAttribs.IsCompute)
                    pd3d12CmdList->SetComputeRootConstantBufferView(BaseRootIndex + RootInd, BufferGPUAddress);
                else
                    pd3d12CmdList->SetGraphicsRootConstantBufferView(BaseRootIndex + RootInd, BufferGPUAddress);
                break;

            case SHADER_RESOURCE_TYPE_BUFFER_SRV:
                if (CommitAttribs.IsCompute)
                    pd3d12CmdList->SetComputeRootShaderResourceView(BaseRootIndex + RootInd, BufferGPUAddress);
                else
                    pd3d12CmdList->SetGraphicsRootShaderResourceView(BaseRootIndex + RootInd, BufferGPUAddress);
                break;

            case SHADER_RESOURCE_TYPE_BUFFER_UAV:
                if (CommitAttribs.IsCompute)
                    pd3d12CmdList->SetComputeRootUnorderedAccessView(BaseRootIndex + RootInd, BufferGPUAddress);
                else
                    pd3d12CmdList->SetGraphicsRootUnorderedAccessView(BaseRootIndex + RootInd, BufferGPUAddress);
                break;

            default:
                UNEXPECTED("Unexpected root view resource type");
        }
    }
}

void PipelineResourceSignatureD3D12Impl::CommitRootTables(const CommitCacheResourcesAttribs& CommitAttribs) const
{
    VERIFY_EXPR(CommitAttribs.pResourceCache != nullptr);
    const auto& ResourceCache = *CommitAttribs.pResourceCache;
    const auto& BaseRootIndex = CommitAttribs.BaseRootIndex;
    auto&       CmdCtx        = CommitAttribs.Ctx;
    auto* const pd3d12Device  = CommitAttribs.pd3d12Device;

    // Having an array of actual DescriptorHeapAllocation objects introduces unnecessary overhead when
    // there are no dynamic variables as constructors and destructors are always called. To avoid this
    // overhead we will construct DescriptorHeapAllocation in-place only when they are really needed.
    std::array<DescriptorHeapAllocation*, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER + 1> pDynamicDescriptorAllocations{};

    // Reserve space for DescriptorHeapAllocation objects (do NOT zero-out!)
    alignas(DescriptorHeapAllocation) uint8_t DynamicDescriptorAllocationsRawMem[sizeof(DescriptorHeapAllocation) * (D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER + 1)];

    for (Uint32 heap_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; heap_type < D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER + 1; ++heap_type)
    {
        const auto d3d12HeapType = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(heap_type);

        auto NumDynamicDescriptors = m_RootParams.GetParameterGroupSize(d3d12HeapType, ROOT_PARAMETER_GROUP_DYNAMIC);
        if (NumDynamicDescriptors > 0)
        {
            auto& pAllocation = pDynamicDescriptorAllocations[d3d12HeapType];

            // Create new DescriptorHeapAllocation in-place
            pAllocation = new (&DynamicDescriptorAllocationsRawMem[sizeof(DescriptorHeapAllocation) * heap_type])
                DescriptorHeapAllocation{CmdCtx.AllocateDynamicGPUVisibleDescriptor(d3d12HeapType, NumDynamicDescriptors)};

            DEV_CHECK_ERR(!pAllocation->IsNull(),
                          "Failed to allocate ", NumDynamicDescriptors, " dynamic GPU-visible ",
                          (d3d12HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? "CBV/SRV/UAV" : "Sampler"),
                          " descriptor(s). Consider increasing GPUDescriptorHeapDynamicSize[", heap_type,
                          "] in EngineD3D12CreateInfo or optimizing dynamic resource utilization by using static "
                          "or mutable shader resource variables instead.");

            // Copy all dynamic descriptors from the CPU-only cache allocation
            const auto& SrcDynamicAllocation = ResourceCache.GetDescriptorAllocation(d3d12HeapType, ROOT_PARAMETER_GROUP_DYNAMIC);
            VERIFY_EXPR(SrcDynamicAllocation.GetNumHandles() == NumDynamicDescriptors);
            pd3d12Device->CopyDescriptorsSimple(NumDynamicDescriptors, pAllocation->GetCpuHandle(), SrcDynamicAllocation.GetCpuHandle(), d3d12HeapType);
        }
    }

    auto* const pSrvCbvUavDynamicAllocation = pDynamicDescriptorAllocations[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    auto* const pSamplerDynamicAllocation   = pDynamicDescriptorAllocations[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER];

    CommandContext::ShaderDescriptorHeaps Heaps{
        ResourceCache.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, ROOT_PARAMETER_GROUP_STATIC_MUTABLE),
        ResourceCache.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, ROOT_PARAMETER_GROUP_STATIC_MUTABLE),
    };
    if (Heaps.pSrvCbvUavHeap == nullptr && pSrvCbvUavDynamicAllocation != nullptr)
        Heaps.pSrvCbvUavHeap = pSrvCbvUavDynamicAllocation->GetDescriptorHeap();
    if (Heaps.pSamplerHeap == nullptr && pSamplerDynamicAllocation != nullptr)
        Heaps.pSamplerHeap = pSamplerDynamicAllocation->GetDescriptorHeap();

    VERIFY(pSrvCbvUavDynamicAllocation == nullptr || pSrvCbvUavDynamicAllocation->GetDescriptorHeap() == Heaps.pSrvCbvUavHeap,
           "Inconsistent CBV/SRV/UAV descriptor heaps");
    VERIFY(pSamplerDynamicAllocation == nullptr || pSamplerDynamicAllocation->GetDescriptorHeap() == Heaps.pSamplerHeap,
           "Inconsistent Sampler descriptor heaps");

    if (Heaps)
        CmdCtx.SetDescriptorHeaps(Heaps);

    const auto NumRootTables = m_RootParams.GetNumRootTables();
    for (Uint32 rt = 0; rt < NumRootTables; ++rt)
    {
        const auto& RootTable = m_RootParams.GetRootTable(rt);

        const auto TableOffsetInGroupAllocation = RootTable.TableOffsetInGroupAllocation;
        VERIFY_EXPR(TableOffsetInGroupAllocation != RootParameter::InvalidTableOffsetInGroupAllocation);

        const auto& d3d12Param = RootTable.d3d12RootParam;
        VERIFY_EXPR(d3d12Param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
        const auto& d3d12Table = d3d12Param.DescriptorTable;

        const auto d3d12HeapType = d3d12Table.pDescriptorRanges[0].RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER ?
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER :
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        D3D12_GPU_DESCRIPTOR_HANDLE RootTableGPUDescriptorHandle{};
        if (RootTable.Group == ROOT_PARAMETER_GROUP_DYNAMIC)
        {
            auto& DynamicAllocation      = *pDynamicDescriptorAllocations[d3d12HeapType];
            RootTableGPUDescriptorHandle = DynamicAllocation.GetGpuHandle(TableOffsetInGroupAllocation);
        }
        else
        {
            RootTableGPUDescriptorHandle = ResourceCache.GetDescriptorTableHandle<D3D12_GPU_DESCRIPTOR_HANDLE>(
                d3d12HeapType, ROOT_PARAMETER_GROUP_STATIC_MUTABLE, RootTable.RootIndex);
            VERIFY(RootTableGPUDescriptorHandle.ptr != 0, "Unexpected null GPU descriptor handle");
        }

        if (CommitAttribs.IsCompute)
            CmdCtx.GetCommandList()->SetComputeRootDescriptorTable(BaseRootIndex + RootTable.RootIndex, RootTableGPUDescriptorHandle);
        else
            CmdCtx.GetCommandList()->SetGraphicsRootDescriptorTable(BaseRootIndex + RootTable.RootIndex, RootTableGPUDescriptorHandle);
    }

    // Commit non-dynamic root buffer views
    if (auto NonDynamicBuffersMask = ResourceCache.GetNonDynamicRootBuffersMask())
    {
        CommitRootViews(CommitAttribs, NonDynamicBuffersMask);
    }

    // Manually destroy DescriptorHeapAllocation objects we created.
    for (auto* pAllocation : pDynamicDescriptorAllocations)
    {
        if (pAllocation != nullptr)
            pAllocation->~DescriptorHeapAllocation();
    }
}


void PipelineResourceSignatureD3D12Impl::UpdateShaderResourceBindingMap(ResourceBinding::TMap& ResourceMap, SHADER_TYPE ShaderStage, Uint32 BaseRegisterSpace) const
{
    VERIFY(ShaderStage != SHADER_TYPE_UNKNOWN && IsPowerOfTwo(ShaderStage), "Only single shader stage must be provided.");

    for (Uint32 r = 0, ResCount = GetTotalResourceCount(); r < ResCount; ++r)
    {
        const auto& ResDesc = GetResourceDesc(r);
        const auto& Attribs = GetResourceAttribs(r);

        if ((ResDesc.ShaderStages & ShaderStage) != 0)
        {
            ResourceBinding::BindInfo BindInfo //
                {
                    Attribs.Register,
                    Attribs.Space + BaseRegisterSpace,
                    ResDesc.ArraySize,
                    ResDesc.ResourceType //
                };
            auto IsUnique = ResourceMap.emplace(HashMapStringKey{ResDesc.Name}, BindInfo).second;
            VERIFY(IsUnique, "Shader resource '", ResDesc.Name,
                   "' already present in the binding map. Every shader resource in PSO must be unambiguously defined by "
                   "only one resource signature. This error should've been caught by ValidatePipelineResourceSignatures().");
        }
    }

    // Add immutable samplers to the map as there may be immutable samplers that are not defined as resources, e.g.:
    //
    //      PipelineResourceDesc Resources[] = {SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, ...}
    //      ImmutableSamplerDesc ImtblSams[] = {SHADER_TYPE_PIXEL, "g_Texture", ...}
    //
    for (Uint32 samp = 0, SampCount = GetImmutableSamplerCount(); samp < SampCount; ++samp)
    {
        const auto& ImtblSam = GetImmutableSamplerDesc(samp);
        const auto& SampAttr = GetImmutableSamplerAttribs(samp);

        if ((ImtblSam.ShaderStages & ShaderStage) != 0)
        {
            String SampName{ImtblSam.SamplerOrTextureName};
            if (IsUsingCombinedSamplers())
                SampName += GetCombinedSamplerSuffix();

            ResourceBinding::BindInfo BindInfo //
                {
                    SampAttr.ShaderRegister,
                    SampAttr.RegisterSpace + BaseRegisterSpace,
                    SampAttr.ArraySize,
                    SHADER_RESOURCE_TYPE_SAMPLER //
                };

            auto it_inserted = ResourceMap.emplace(HashMapStringKey{SampName}, BindInfo);
#ifdef DILIGENT_DEBUG
            if (!it_inserted.second)
            {
                const auto& ExistingBindInfo = it_inserted.first->second;
                VERIFY(ExistingBindInfo.BindPoint == BindInfo.BindPoint,
                       "Bind point defined by the immutable sampler attribs is inconsistent with the bind point defined by the sampler resource.");
                VERIFY(ExistingBindInfo.Space == BindInfo.Space,
                       "Register space defined by the immutable sampler attribs is inconsistent with the space defined by the sampler resource.");
                VERIFY(ExistingBindInfo.ArraySize >= BindInfo.ArraySize,
                       "Array size defined by the immutable sampler attribs is smaller than the size defined by the sampler resource. "
                       "This may be a bug in AllocateRootParameters().");
            }
#endif
        }
    }
}

bool PipelineResourceSignatureD3D12Impl::HasImmutableSamplerArray(SHADER_TYPE ShaderStage) const
{
    for (Uint32 s = 0; s < GetImmutableSamplerCount(); ++s)
    {
        const auto& ImtblSam = GetImmutableSamplerDesc(s);
        const auto& SampAttr = GetImmutableSamplerAttribs(s);
        if ((ImtblSam.ShaderStages & ShaderStage) != 0 && SampAttr.ArraySize > 1)
            return true;
    }
    return false;
}

#ifdef DILIGENT_DEVELOPMENT
bool PipelineResourceSignatureD3D12Impl::DvpValidateCommittedResource(const DeviceContextD3D12Impl*   pCtx,
                                                                      const D3DShaderResourceAttribs& D3DAttribs,
                                                                      Uint32                          ResIndex,
                                                                      const ShaderResourceCacheD3D12& ResourceCache,
                                                                      const char*                     ShaderName,
                                                                      const char*                     PSOName) const
{
    const auto& ResDesc    = GetResourceDesc(ResIndex);
    const auto& ResAttribs = GetResourceAttribs(ResIndex);
    VERIFY_EXPR(strcmp(ResDesc.Name, D3DAttribs.Name) == 0);
    VERIFY_EXPR(D3DAttribs.BindCount <= ResDesc.ArraySize);

    if ((ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER) && ResAttribs.IsImmutableSamplerAssigned())
        return true;

    const auto CacheType = ResourceCache.GetContentType();
    VERIFY(CacheType == ResourceCacheContentType::SRB, "Only SRB resource cache can be committed");
    const auto  RootIndex            = ResAttribs.RootIndex(CacheType);
    const auto  OffsetFromTableStart = ResAttribs.OffsetFromTableStart(CacheType);
    const auto& RootTable            = ResourceCache.GetRootTable(RootIndex);

    bool BindingsOK = true;
    for (Uint32 ArrIndex = 0; ArrIndex < D3DAttribs.BindCount; ++ArrIndex)
    {
        const auto& CachedRes = RootTable.GetResource(OffsetFromTableStart + ArrIndex);
        if (CachedRes.IsNull())
        {
            LOG_ERROR_MESSAGE("No resource is bound to variable '", GetShaderResourcePrintName(D3DAttribs.Name, D3DAttribs.BindCount, ArrIndex),
                              "' in shader '", ShaderName, "' of PSO '", PSOName, "'.");
            BindingsOK = false;
            continue;
        }

        if (ResAttribs.IsCombinedWithSampler())
        {
            const auto& SamplerResDesc = GetResourceDesc(ResAttribs.SamplerInd);
            const auto& SamplerAttribs = GetResourceAttribs(ResAttribs.SamplerInd);
            VERIFY_EXPR(SamplerResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER);
            VERIFY_EXPR(SamplerResDesc.ArraySize == 1 || SamplerResDesc.ArraySize == ResDesc.ArraySize);
            if (!SamplerAttribs.IsImmutableSamplerAssigned())
            {
                if (ArrIndex < SamplerResDesc.ArraySize)
                {
                    const auto  SamRootIndex            = SamplerAttribs.RootIndex(CacheType);
                    const auto  SamOffsetFromTableStart = SamplerAttribs.OffsetFromTableStart(CacheType);
                    const auto& SamRootTable            = ResourceCache.GetRootTable(SamRootIndex);
                    const auto& CachedSam               = SamRootTable.GetResource(SamOffsetFromTableStart + ArrIndex);
                    VERIFY_EXPR(CachedSam.Type == SHADER_RESOURCE_TYPE_SAMPLER);
                    if (CachedSam.IsNull())
                    {
                        LOG_ERROR_MESSAGE("No sampler is bound to sampler variable '", GetShaderResourcePrintName(SamplerResDesc, ArrIndex),
                                          "' combined with texture '", D3DAttribs.Name, "' in shader '", ShaderName, "' of PSO '", PSOName, "'.");
                        BindingsOK = false;
                    }
                }
            }
        }

        static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update the switch below to handle the new shader resource type");
        switch (ResDesc.ResourceType)
        {
            case SHADER_RESOURCE_TYPE_TEXTURE_SRV:
            case SHADER_RESOURCE_TYPE_TEXTURE_UAV:
                // When can use raw cast here because the dynamic type is verified when the resource
                // is bound. It will be null if the type is incorrect.
                if (const auto* pTexViewD3D12 = CachedRes.pObject.RawPtr<TextureViewD3D12Impl>())
                {
                    if (!ValidateResourceViewDimension(D3DAttribs.Name, D3DAttribs.BindCount, ArrIndex, pTexViewD3D12, D3DAttribs.GetResourceDimension(), D3DAttribs.IsMultisample()))
                        BindingsOK = false;
                }
                break;

            case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
                if (ResAttribs.GetD3D12RootParamType() == D3D12_ROOT_PARAMETER_TYPE_CBV)
                {
                    if (const auto* pBuffD3D12 = CachedRes.pObject.RawPtr<BufferD3D12Impl>())
                    {
                        const auto& BuffDesc = pBuffD3D12->GetDesc();

                        if (BuffDesc.Usage == USAGE_DYNAMIC)
                            pBuffD3D12->DvpVerifyDynamicAllocation(pCtx);

                        if (BuffDesc.Usage == USAGE_DYNAMIC || CachedRes.BufferRangeSize != 0 && CachedRes.BufferRangeSize < BuffDesc.Size)
                            VERIFY_EXPR((ResourceCache.GetDynamicRootBuffersMask() & (Uint64{1} << RootIndex)) != 0);
                        else
                            VERIFY_EXPR((ResourceCache.GetNonDynamicRootBuffersMask() & (Uint64{1} << RootIndex)) != 0);
                    }
                }
                break;

            case SHADER_RESOURCE_TYPE_BUFFER_SRV:
            case SHADER_RESOURCE_TYPE_BUFFER_UAV:
                if (const auto* pBuffViewD3D12 = CachedRes.pObject.RawPtr<BufferViewD3D12Impl>())
                {
                    if (!VerifyBufferViewModeD3D(pBuffViewD3D12, D3DAttribs, ShaderName))
                        BindingsOK = false;

                    if (ResAttribs.GetD3D12RootParamType() == D3D12_ROOT_PARAMETER_TYPE_SRV ||
                        ResAttribs.GetD3D12RootParamType() == D3D12_ROOT_PARAMETER_TYPE_UAV)
                    {
                        const auto* pBuffD3D12 = pBuffViewD3D12->GetBuffer<BufferD3D12Impl>();
                        VERIFY_EXPR(pBuffD3D12 != nullptr);
                        const auto& BuffDesc = pBuffD3D12->GetDesc();

                        if (BuffDesc.Usage == USAGE_DYNAMIC)
                            pBuffD3D12->DvpVerifyDynamicAllocation(pCtx);

                        if (BuffDesc.Usage == USAGE_DYNAMIC || CachedRes.BufferRangeSize != 0 && CachedRes.BufferRangeSize < BuffDesc.Size)
                            VERIFY_EXPR((ResourceCache.GetDynamicRootBuffersMask() & (Uint64{1} << RootIndex)) != 0);
                        else
                            VERIFY_EXPR((ResourceCache.GetNonDynamicRootBuffersMask() & (Uint64{1} << RootIndex)) != 0);
                    }
                }
                break;

            default:
                //Do nothing
                break;
        }
    }
    return BindingsOK;
}
#endif


PipelineResourceSignatureD3D12Impl::PipelineResourceSignatureD3D12Impl(IReferenceCounters*                               pRefCounters,
                                                                       RenderDeviceD3D12Impl*                            pDevice,
                                                                       const PipelineResourceSignatureDesc&              Desc,
                                                                       const PipelineResourceSignatureInternalDataD3D12& InternalData) :
    TPipelineResourceSignatureBase{pRefCounters, pDevice, Desc, InternalData}
{
    try
    {
        ValidatePipelineResourceSignatureDescD3D12(Desc);

        Deserialize(
            GetRawAllocator(), DecoupleCombinedSamplers(Desc), InternalData, m_ImmutableSamplers,
            [this]() //
            {
                AllocateRootParameters(/*IsSerialized*/ true);
            },
            [this]() //
            {
                return ShaderResourceCacheD3D12::GetMemoryRequirements(m_RootParams).TotalSize;
            });
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineResourceSignatureInternalDataD3D12 PipelineResourceSignatureD3D12Impl::GetInternalData() const
{
    PipelineResourceSignatureInternalDataD3D12 InternalData;

    TPipelineResourceSignatureBase::GetInternalData(InternalData);

    InternalData.pResourceAttribs     = m_pResourceAttribs;
    InternalData.NumResources         = GetDesc().NumResources;
    InternalData.pImmutableSamplers   = m_ImmutableSamplers;
    InternalData.NumImmutableSamplers = GetDesc().NumImmutableSamplers;

    return InternalData;
}

} // namespace Diligent
