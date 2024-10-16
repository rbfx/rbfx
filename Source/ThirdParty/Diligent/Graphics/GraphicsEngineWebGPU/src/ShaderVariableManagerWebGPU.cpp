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

#include "ShaderVariableManagerWebGPU.hpp"
#include "RenderDeviceWebGPUImpl.hpp"
#include "PipelineResourceSignatureWebGPUImpl.hpp"
#include "SamplerWebGPUImpl.hpp"
#include "TextureViewWebGPUImpl.hpp"
#include "TextureWebGPUImpl.hpp"
#include "BufferViewWebGPUImpl.hpp"
#include "BufferWebGPUImpl.hpp"

namespace Diligent
{

template <typename HandlerType>
void ProcessSignatureResources(const PipelineResourceSignatureWebGPUImpl& Signature,
                               const SHADER_RESOURCE_VARIABLE_TYPE*       AllowedVarTypes,
                               Uint32                                     NumAllowedTypes,
                               SHADER_TYPE                                ShaderStages,
                               HandlerType&&                              Handler)
{
    const bool UsingSeparateSamplers = Signature.IsUsingSeparateSamplers();
    Signature.ProcessResources(AllowedVarTypes, NumAllowedTypes, ShaderStages,
                               [&](const PipelineResourceDesc& ResDesc, Uint32 Index) //
                               {
                                   const PipelineResourceAttribsWebGPU& ResAttr = Signature.GetResourceAttribs(Index);

                                   // When using HLSL-style combined image samplers, we need to skip separate samplers.
                                   // Also always skip immutable separate samplers.
                                   if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER &&
                                       (!UsingSeparateSamplers || ResAttr.IsImmutableSamplerAssigned()))
                                       return;

                                   Handler(Index);
                               });
}

size_t ShaderVariableManagerWebGPU::GetRequiredMemorySize(const PipelineResourceSignatureWebGPUImpl& Signature,
                                                          const SHADER_RESOURCE_VARIABLE_TYPE*       AllowedVarTypes,
                                                          Uint32                                     NumAllowedTypes,
                                                          SHADER_TYPE                                ShaderStages,
                                                          Uint32*                                    pNumVariables)
{
    Uint32 NumVariables = 0;
    if (pNumVariables == nullptr)
        pNumVariables = &NumVariables;
    *pNumVariables = 0;
    ProcessSignatureResources(Signature, AllowedVarTypes, NumAllowedTypes, ShaderStages,
                              [pNumVariables](Uint32) //
                              {
                                  ++(*pNumVariables);
                              });

    return (*pNumVariables) * sizeof(ShaderVariableWebGPUImpl);
}

// Creates shader variable for every resource whose type is one of AllowedVarTypes
void ShaderVariableManagerWebGPU::Initialize(const PipelineResourceSignatureWebGPUImpl& Signature,
                                             IMemoryAllocator&                          Allocator,
                                             const SHADER_RESOURCE_VARIABLE_TYPE*       AllowedVarTypes,
                                             Uint32                                     NumAllowedTypes,
                                             SHADER_TYPE                                ShaderType)
{
    VERIFY_EXPR(m_NumVariables == 0);
    const size_t MemSize = GetRequiredMemorySize(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType, &m_NumVariables);

    if (m_NumVariables == 0)
        return;

    TBase::Initialize(Signature, Allocator, MemSize);

    Uint32 VarInd = 0;
    ProcessSignatureResources(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType,
                              [this, &VarInd](Uint32 ResIndex) //
                              {
                                  ::new (m_pVariables + VarInd) ShaderVariableWebGPUImpl{*this, ResIndex};
                                  ++VarInd;
                              });
    VERIFY_EXPR(VarInd == m_NumVariables);
}

void ShaderVariableManagerWebGPU::Destroy(IMemoryAllocator& Allocator)
{
    if (m_pVariables != nullptr)
    {
        for (Uint32 v = 0; v < m_NumVariables; ++v)
            m_pVariables[v].~ShaderVariableWebGPUImpl();
    }
    TBase::Destroy(Allocator);
}

ShaderVariableWebGPUImpl* ShaderVariableManagerWebGPU::GetVariable(const Char* Name) const
{
    for (Uint32 v = 0; v < m_NumVariables; ++v)
    {
        ShaderVariableWebGPUImpl& Var = m_pVariables[v];
        if (strcmp(Var.GetDesc().Name, Name) == 0)
            return &Var;
    }
    return nullptr;
}

ShaderVariableWebGPUImpl* ShaderVariableManagerWebGPU::GetVariable(Uint32 Index) const
{
    if (Index >= m_NumVariables)
    {
        LOG_ERROR("Index ", Index, " is out of range");
        return nullptr;
    }

    return m_pVariables + Index;
}

Uint32 ShaderVariableManagerWebGPU::GetVariableIndex(const ShaderVariableWebGPUImpl& Variable)
{
    if (m_pVariables == nullptr)
    {
        LOG_ERROR("This shader variable manager has no variables");
        return ~0u;
    }

    const auto Offset = reinterpret_cast<const Uint8*>(&Variable) - reinterpret_cast<Uint8*>(m_pVariables);
    DEV_CHECK_ERR(Offset % sizeof(ShaderVariableWebGPUImpl) == 0, "Offset is not multiple of ShaderVariableWebGPUImpl class size");
    const Uint32 Index = static_cast<Uint32>(Offset / sizeof(ShaderVariableWebGPUImpl));
    if (Index < m_NumVariables)
        return Index;
    else
    {
        LOG_ERROR("Failed to get variable index. The variable ", &Variable, " does not belong to this shader variable manager");
        return ~0u;
    }
}

const PipelineResourceDesc& ShaderVariableManagerWebGPU::GetResourceDesc(Uint32 Index) const
{
    VERIFY_EXPR(m_pSignature);
    return m_pSignature->GetResourceDesc(Index);
}

const ShaderVariableManagerWebGPU::ResourceAttribs& ShaderVariableManagerWebGPU::GetResourceAttribs(Uint32 Index) const
{
    VERIFY_EXPR(m_pSignature);
    return m_pSignature->GetResourceAttribs(Index);
}

void ShaderVariableManagerWebGPU::BindResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags)
{
    TBase::BindResources(pResourceMapping, Flags);
}

void ShaderVariableManagerWebGPU::CheckResources(IResourceMapping*                    pResourceMapping,
                                                 BIND_SHADER_RESOURCES_FLAGS          Flags,
                                                 SHADER_RESOURCE_VARIABLE_TYPE_FLAGS& StaleVarTypes) const
{
    TBase::CheckResources(pResourceMapping, Flags, StaleVarTypes);
}

namespace
{

#ifdef DILIGENT_DEVELOPMENT
inline BUFFER_VIEW_TYPE DvpBindGroupEntryTypeToBufferView(BindGroupEntryType Type)
{
    static_assert(static_cast<Uint32>(BindGroupEntryType::Count) == 12, "Please update the switch below to handle the new bind group entry type");
    switch (Type)
    {
        case BindGroupEntryType::StorageBuffer_ReadOnly:
        case BindGroupEntryType::StorageBufferDynamic_ReadOnly:
            return BUFFER_VIEW_SHADER_RESOURCE;

        case BindGroupEntryType::StorageBuffer:
        case BindGroupEntryType::StorageBufferDynamic:
            return BUFFER_VIEW_UNORDERED_ACCESS;

        default:
            UNEXPECTED("Unsupported descriptor type for buffer view");
            return BUFFER_VIEW_UNDEFINED;
    }
}

inline TEXTURE_VIEW_TYPE DvpBindGroupEntryTypeToTextureView(BindGroupEntryType Type)
{
    static_assert(static_cast<Uint32>(BindGroupEntryType::Count) == 12, "Please update the switch below to handle the new bind group entry type");
    switch (Type)
    {
        case BindGroupEntryType::StorageTexture_WriteOnly:
        case BindGroupEntryType::StorageTexture_ReadOnly:
        case BindGroupEntryType::StorageTexture_ReadWrite:
            return TEXTURE_VIEW_UNORDERED_ACCESS;

        case BindGroupEntryType::Texture:
            return TEXTURE_VIEW_SHADER_RESOURCE;

        default:
            UNEXPECTED("Unsupported descriptor type for texture view");
            return TEXTURE_VIEW_UNDEFINED;
    }
}
#endif

struct BindResourceHelper
{
    BindResourceHelper(const PipelineResourceSignatureWebGPUImpl& Signature,
                       ShaderResourceCacheWebGPU&                 ResourceCache,
                       Uint32                                     ResIndex,
                       Uint32                                     ArrayIndex);

    void operator()(const BindResourceInfo& BindInfo) const;

private:
    // clang-format off
    void CacheUniformBuffer(const BindResourceInfo& BindInfo) const;
    void CacheStorageBuffer(const BindResourceInfo& BindInfo) const;
    void CacheTexture      (const BindResourceInfo& BindInfo) const;
    void CacheSampler      (const BindResourceInfo& BindInfo) const;
    // clang-format on

    template <typename ObjectType>
    bool UpdateCachedResource(RefCntAutoPtr<ObjectType>&& pObject,
                              SET_SHADER_RESOURCE_FLAGS   Flags,
                              Uint64                      BufferBaseOffset = 0,
                              Uint64                      BufferRangeSize  = 0) const;

private:
    using ResourceAttribs = PipelineResourceSignatureWebGPUImpl::ResourceAttribs;
    using BindGroup       = ShaderResourceCacheWebGPU::BindGroup;

    const PipelineResourceSignatureWebGPUImpl& m_Signature;
    ShaderResourceCacheWebGPU&                 m_ResourceCache;
    const Uint32                               m_ArrayIndex;
    const ResourceCacheContentType             m_CacheType;
    const PipelineResourceDesc&                m_ResDesc;
    const ResourceAttribs&                     m_Attribs;
    const Uint32                               m_DstResCacheOffset;
    const BindGroup&                           m_BindGroup;
    const ShaderResourceCacheWebGPU::Resource& m_DstRes;
};

BindResourceHelper::BindResourceHelper(const PipelineResourceSignatureWebGPUImpl& Signature,
                                       ShaderResourceCacheWebGPU&                 ResourceCache,
                                       Uint32                                     ResIndex,
                                       Uint32                                     ArrayIndex) :
    // clang-format off
    m_Signature         {Signature},
    m_ResourceCache     {ResourceCache},
    m_ArrayIndex        {ArrayIndex},
    m_CacheType         {ResourceCache.GetContentType()},
    m_ResDesc           {Signature.GetResourceDesc(ResIndex)},
    m_Attribs           {Signature.GetResourceAttribs(ResIndex)},
    m_DstResCacheOffset {m_Attribs.CacheOffset(m_CacheType) + ArrayIndex},
    m_BindGroup         {const_cast<const ShaderResourceCacheWebGPU&>(ResourceCache).GetBindGroup(m_Attribs.BindGroup)},
    m_DstRes            {m_BindGroup.GetResource(m_DstResCacheOffset)}
// clang-format on
{
    VERIFY(ArrayIndex < m_ResDesc.ArraySize, "Array index is out of range, but it should've been corrected by ShaderVariableBase::SetArray()");
    VERIFY(m_DstRes.Type == m_Attribs.GetBindGroupEntryType(), "Inconsistent types");
}

void BindResourceHelper::operator()(const BindResourceInfo& BindInfo) const
{
    VERIFY_EXPR(m_ArrayIndex == BindInfo.ArrayIndex);

    if (BindInfo.pObject != nullptr)
    {
        static_assert(static_cast<Uint32>(BindGroupEntryType::Count) == 12, "Please update the switch below to handle the new bind group entry type");
        switch (m_DstRes.Type)
        {
            case BindGroupEntryType::UniformBuffer:
            case BindGroupEntryType::UniformBufferDynamic:
                CacheUniformBuffer(BindInfo);
                break;

            case BindGroupEntryType::StorageBuffer:
            case BindGroupEntryType::StorageBufferDynamic:
            case BindGroupEntryType::StorageBuffer_ReadOnly:
            case BindGroupEntryType::StorageBufferDynamic_ReadOnly:
                CacheStorageBuffer(BindInfo);
                break;

            case BindGroupEntryType::Texture:
            case BindGroupEntryType::StorageTexture_WriteOnly:
            case BindGroupEntryType::StorageTexture_ReadOnly:
            case BindGroupEntryType::StorageTexture_ReadWrite:
                CacheTexture(BindInfo);
                break;

            case BindGroupEntryType::ExternalTexture:
                UNSUPPORTED("External textures are not yet supported");
                break;

            case BindGroupEntryType::Sampler:
                if (!m_Attribs.IsImmutableSamplerAssigned())
                {
                    CacheSampler(BindInfo);
                }
                else
                {
                    UNEXPECTED("Attempting to assign a sampler to an immutable sampler '", m_ResDesc.Name, '\'');
                }
                break;

            default:
                UNEXPECTED("Unknown resource type ", static_cast<Uint32>(m_DstRes.Type));
        }
    }
    else
    {
        DEV_CHECK_ERR((m_DstRes.pObject == nullptr ||
                       m_ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC ||
                       (BindInfo.Flags & SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE) != 0),
                      "Shader variable '", m_ResDesc.Name, "' is not dynamic, but is being reset to null. This is an error and may cause unpredicted behavior. ",
                      "If this is intended and you ensured proper synchronization, use the SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE flag. "
                      "Otherwise, use another shader resource binding instance or label the variable as dynamic.");

        m_ResourceCache.ResetResource(m_Attribs.BindGroup, m_DstResCacheOffset);
    }
}

template <typename ObjectType>
bool BindResourceHelper::UpdateCachedResource(RefCntAutoPtr<ObjectType>&& pObject,
                                              SET_SHADER_RESOURCE_FLAGS   Flags,
                                              Uint64                      BufferBaseOffset,
                                              Uint64                      BufferRangeSize) const
{
    if (pObject)
    {
        if (m_DstRes.pObject != nullptr &&
            m_ResDesc.VarType != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC &&
            (Flags & SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE) == 0)
        {
            DEV_CHECK_ERR(m_DstRes.pObject == pObject, "Binding another object to a non-dynamic variable is not allowed");
            // Do not update resource if one is already bound unless it is dynamic. This may be
            // dangerous as writing descriptors while they are used by the GPU is an undefined behavior
            return false;
        }

        m_ResourceCache.SetResource(m_Attribs.BindGroup,
                                    m_DstResCacheOffset,
                                    std::move(pObject),
                                    BufferBaseOffset,
                                    BufferRangeSize);
        return true;
    }
    else
    {
        return false;
    }
}


void BindResourceHelper::CacheUniformBuffer(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting uniform buffer to null is handled by BindResourceHelper::operator()");
    VERIFY((m_DstRes.Type == BindGroupEntryType::UniformBuffer ||
            m_DstRes.Type == BindGroupEntryType::UniformBufferDynamic),
           "Uniform buffer resource is expected");

    // We cannot use ClassPtrCast<> here as the resource can have wrong type
    RefCntAutoPtr<BufferWebGPUImpl> pBufferWebGPU{BindInfo.pObject, IID_BufferWebGPU};
#ifdef DILIGENT_DEVELOPMENT
    VerifyConstantBufferBinding(m_ResDesc, BindInfo, pBufferWebGPU.RawPtr(), m_DstRes.pObject.RawPtr(),
                                m_DstRes.BufferBaseOffset, m_DstRes.BufferRangeSize, m_Signature.GetDesc().Name);
#endif

    UpdateCachedResource(std::move(pBufferWebGPU), BindInfo.Flags, BindInfo.BufferBaseOffset, BindInfo.BufferRangeSize);
}

void BindResourceHelper::CacheStorageBuffer(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting storage buffer to null is handled by BindResourceHelper::operator()");
    VERIFY((m_DstRes.Type == BindGroupEntryType::StorageBuffer ||
            m_DstRes.Type == BindGroupEntryType::StorageBufferDynamic ||
            m_DstRes.Type == BindGroupEntryType::StorageBuffer_ReadOnly ||
            m_DstRes.Type == BindGroupEntryType::StorageBufferDynamic_ReadOnly),
           "Storage buffer resource is expected");

    RefCntAutoPtr<BufferViewWebGPUImpl> pBufferViewWebGPU{BindInfo.pObject, IID_BufferViewWebGPU};
#ifdef DILIGENT_DEVELOPMENT
    {
        // HLSL buffer SRVs are mapped to storage buffers in GLSL
        const BUFFER_VIEW_TYPE RequiredViewType = DvpBindGroupEntryTypeToBufferView(m_DstRes.Type);
        VerifyResourceViewBinding(m_ResDesc, BindInfo, pBufferViewWebGPU.RawPtr(),
                                  {RequiredViewType},
                                  RESOURCE_DIM_BUFFER, // Expected resource dim
                                  false,               // IsMultisample (ignored when resource dim is buffer)
                                  m_DstRes.pObject.RawPtr(),
                                  m_Signature.GetDesc().Name);

        VERIFY((m_ResDesc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) == 0,
               "FORMATTED_BUFFER resource flag is set for a storage buffer - this should've not happened.");
        ValidateBufferMode(m_ResDesc, m_ArrayIndex, pBufferViewWebGPU.RawPtr());
    }
#endif

    UpdateCachedResource(std::move(pBufferViewWebGPU), BindInfo.Flags);
}

void BindResourceHelper::CacheTexture(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting image to null is handled by BindResourceHelper::operator()");
    VERIFY((m_DstRes.Type == BindGroupEntryType::Texture ||
            m_DstRes.Type == BindGroupEntryType::StorageTexture_ReadOnly ||
            m_DstRes.Type == BindGroupEntryType::StorageTexture_WriteOnly ||
            m_DstRes.Type == BindGroupEntryType::StorageTexture_ReadWrite),
           "Texture or storage texture resource is expected");

    RefCntAutoPtr<TextureViewWebGPUImpl> pTexViewWebGPU0{BindInfo.pObject, IID_TextureViewWebGPU};
#ifdef DILIGENT_DEVELOPMENT
    {
        // HLSL buffer SRVs are mapped to storage buffers in GLSL
        TEXTURE_VIEW_TYPE RequiredViewType = DvpBindGroupEntryTypeToTextureView(m_DstRes.Type);
        VerifyResourceViewBinding(m_ResDesc, BindInfo, pTexViewWebGPU0.RawPtr(),
                                  {RequiredViewType},
                                  RESOURCE_DIM_UNDEFINED, // Required resource dimension is not known
                                  false,                  // IsMultisample (ignored when resource dim is unknown)
                                  m_DstRes.pObject.RawPtr(),
                                  m_Signature.GetDesc().Name);
    }
#endif

    TextureViewWebGPUImpl* pTexViewWebGPU = pTexViewWebGPU0;
    if (UpdateCachedResource(std::move(pTexViewWebGPU0), BindInfo.Flags))
    {
        if (m_Attribs.IsCombinedWithSampler())
        {
            VERIFY(m_DstRes.Type == BindGroupEntryType::Texture, "Only textures can be combined with samplers.");

            const PipelineResourceDesc&          SamplerResDesc = m_Signature.GetResourceDesc(m_Attribs.SamplerInd);
            const PipelineResourceAttribsWebGPU& SamplerAttribs = m_Signature.GetResourceAttribs(m_Attribs.SamplerInd);
            VERIFY_EXPR(SamplerResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER);

            if (!SamplerAttribs.IsImmutableSamplerAssigned())
            {
                if (ISampler* pSampler = pTexViewWebGPU->GetSampler())
                {
                    DEV_CHECK_ERR(SamplerResDesc.ArraySize == 1 || SamplerResDesc.ArraySize == m_ResDesc.ArraySize,
                                  "Array size (", SamplerResDesc.ArraySize,
                                  ") of texture variable '",
                                  SamplerResDesc.Name,
                                  "' must be one or the same as the array size (", m_ResDesc.ArraySize,
                                  ") of texture variable '", m_ResDesc.Name, "' it is assigned to");

                    BindResourceHelper BindSeparateSampler{
                        m_Signature,
                        m_ResourceCache,
                        m_Attribs.SamplerInd,
                        SamplerResDesc.ArraySize == 1 ? 0 : m_ArrayIndex};
                    BindSeparateSampler(BindResourceInfo{BindSeparateSampler.m_ArrayIndex, pSampler, BindInfo.Flags});
                }
                else
                {
                    LOG_ERROR_MESSAGE("Failed to bind sampler to sampler variable '", SamplerResDesc.Name,
                                      "' assigned to texture '", GetShaderResourcePrintName(m_ResDesc, m_ArrayIndex),
                                      "': no sampler is set in texture view '", pTexViewWebGPU->GetDesc().Name, '\'');
                }
            }
        }
    }
}

void BindResourceHelper::CacheSampler(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting separate sampler to null is handled by BindResourceHelper::operator()");
    VERIFY(m_DstRes.Type == BindGroupEntryType::Sampler, "Separate sampler resource is expected");
    VERIFY(!m_Attribs.IsImmutableSamplerAssigned(), "This separate sampler is assigned an immutable sampler");

    RefCntAutoPtr<SamplerWebGPUImpl> pSamplerWebGPU{BindInfo.pObject, IID_Sampler};
#ifdef DILIGENT_DEVELOPMENT
    VerifySamplerBinding(m_ResDesc, BindInfo, pSamplerWebGPU.RawPtr(), m_DstRes.pObject, m_Signature.GetDesc().Name);
#endif

    UpdateCachedResource(std::move(pSamplerWebGPU), BindInfo.Flags);
}

} // namespace


void ShaderVariableManagerWebGPU::BindResource(Uint32 ResIndex, const BindResourceInfo& BindInfo)
{
    BindResourceHelper BindHelper{
        *m_pSignature,
        m_ResourceCache,
        ResIndex,
        BindInfo.ArrayIndex};

    BindHelper(BindInfo);
}

void ShaderVariableManagerWebGPU::SetBufferDynamicOffset(Uint32 ResIndex,
                                                         Uint32 ArrayIndex,
                                                         Uint32 BufferDynamicOffset)
{
    const PipelineResourceAttribsWebGPU& Attribs           = m_pSignature->GetResourceAttribs(ResIndex);
    const Uint32                         DstResCacheOffset = Attribs.CacheOffset(m_ResourceCache.GetContentType()) + ArrayIndex;
#ifdef DILIGENT_DEVELOPMENT
    {
        const PipelineResourceDesc&                 ResDesc = m_pSignature->GetResourceDesc(ResIndex);
        const ShaderResourceCacheWebGPU::BindGroup& Group   = const_cast<const ShaderResourceCacheWebGPU&>(m_ResourceCache).GetBindGroup(Attribs.BindGroup);
        const ShaderResourceCacheWebGPU::Resource&  DstRes  = Group.GetResource(DstResCacheOffset);
        VerifyDynamicBufferOffset<BufferWebGPUImpl, BufferViewWebGPUImpl>(ResDesc, DstRes.pObject, DstRes.BufferBaseOffset, DstRes.BufferRangeSize, BufferDynamicOffset);
    }
#endif
    m_ResourceCache.SetDynamicBufferOffset(Attribs.BindGroup, DstResCacheOffset, BufferDynamicOffset);
}


IDeviceObject* ShaderVariableManagerWebGPU::Get(Uint32 ArrayIndex, Uint32 ResIndex) const
{
    const PipelineResourceDesc&          ResDesc     = GetResourceDesc(ResIndex);
    const PipelineResourceAttribsWebGPU& Attribs     = GetResourceAttribs(ResIndex);
    const Uint32                         CacheOffset = Attribs.CacheOffset(m_ResourceCache.GetContentType());

    VERIFY_EXPR(ArrayIndex < ResDesc.ArraySize);

    if (Attribs.BindGroup < m_ResourceCache.GetNumBindGroups())
    {
        const ShaderResourceCacheWebGPU::BindGroup& Group = const_cast<const ShaderResourceCacheWebGPU&>(m_ResourceCache).GetBindGroup(Attribs.BindGroup);
        if (CacheOffset + ArrayIndex < Group.GetSize())
        {
            const ShaderResourceCacheWebGPU::Resource& CachedRes = Group.GetResource(CacheOffset + ArrayIndex);
            return CachedRes.pObject;
        }
    }

    return nullptr;
}

} // namespace Diligent
