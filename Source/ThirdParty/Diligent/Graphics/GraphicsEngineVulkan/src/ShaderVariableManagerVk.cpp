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

#include "ShaderVariableManagerVk.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "PipelineResourceSignatureVkImpl.hpp"
#include "SamplerVkImpl.hpp"
#include "TextureViewVkImpl.hpp"
#include "TopLevelASVkImpl.hpp"

namespace Diligent
{

template <typename HandlerType>
void ProcessSignatureResources(const PipelineResourceSignatureVkImpl& Signature,
                               const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                               Uint32                                 NumAllowedTypes,
                               SHADER_TYPE                            ShaderStages,
                               HandlerType&&                          Handler)
{
    const bool UsingSeparateSamplers = Signature.IsUsingSeparateSamplers();
    Signature.ProcessResources(AllowedVarTypes, NumAllowedTypes, ShaderStages,
                               [&](const PipelineResourceDesc& ResDesc, Uint32 Index) //
                               {
                                   const PipelineResourceAttribsVk& ResAttr = Signature.GetResourceAttribs(Index);

                                   // When using HLSL-style combined image samplers, we need to skip separate samplers.
                                   // Also always skip immutable separate samplers.
                                   if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER &&
                                       (!UsingSeparateSamplers || ResAttr.IsImmutableSamplerAssigned()))
                                       return;

                                   Handler(Index);
                               });
}

size_t ShaderVariableManagerVk::GetRequiredMemorySize(const PipelineResourceSignatureVkImpl& Signature,
                                                      const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                                                      Uint32                                 NumAllowedTypes,
                                                      SHADER_TYPE                            ShaderStages,
                                                      Uint32*                                pNumVariables)
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

    return (*pNumVariables) * sizeof(ShaderVariableVkImpl);
}

// Creates shader variable for every resource from SrcLayout whose type is one AllowedVarTypes
void ShaderVariableManagerVk::Initialize(const PipelineResourceSignatureVkImpl& Signature,
                                         IMemoryAllocator&                      Allocator,
                                         const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                                         Uint32                                 NumAllowedTypes,
                                         SHADER_TYPE                            ShaderType)
{
    VERIFY_EXPR(m_NumVariables == 0);
    const auto MemSize = GetRequiredMemorySize(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType, &m_NumVariables);

    if (m_NumVariables == 0)
        return;

    TBase::Initialize(Signature, Allocator, MemSize);

    Uint32 VarInd = 0;
    ProcessSignatureResources(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType,
                              [this, &VarInd](Uint32 ResIndex) //
                              {
                                  ::new (m_pVariables + VarInd) ShaderVariableVkImpl{*this, ResIndex};
                                  ++VarInd;
                              });
    VERIFY_EXPR(VarInd == m_NumVariables);
}

void ShaderVariableManagerVk::Destroy(IMemoryAllocator& Allocator)
{
    if (m_pVariables != nullptr)
    {
        for (Uint32 v = 0; v < m_NumVariables; ++v)
            m_pVariables[v].~ShaderVariableVkImpl();
    }
    TBase::Destroy(Allocator);
}

ShaderVariableVkImpl* ShaderVariableManagerVk::GetVariable(const Char* Name) const
{
    for (Uint32 v = 0; v < m_NumVariables; ++v)
    {
        ShaderVariableVkImpl& Var = m_pVariables[v];
        if (strcmp(Var.GetDesc().Name, Name) == 0)
            return &Var;
    }
    return nullptr;
}


ShaderVariableVkImpl* ShaderVariableManagerVk::GetVariable(Uint32 Index) const
{
    if (Index >= m_NumVariables)
    {
        LOG_ERROR("Index ", Index, " is out of range");
        return nullptr;
    }

    return m_pVariables + Index;
}

Uint32 ShaderVariableManagerVk::GetVariableIndex(const ShaderVariableVkImpl& Variable)
{
    if (m_pVariables == nullptr)
    {
        LOG_ERROR("This shader variable manager has no variables");
        return ~0u;
    }

    const auto Offset = reinterpret_cast<const Uint8*>(&Variable) - reinterpret_cast<Uint8*>(m_pVariables);
    DEV_CHECK_ERR(Offset % sizeof(ShaderVariableVkImpl) == 0, "Offset is not multiple of ShaderVariableVkImpl class size");
    const Uint32 Index = static_cast<Uint32>(Offset / sizeof(ShaderVariableVkImpl));
    if (Index < m_NumVariables)
        return Index;
    else
    {
        LOG_ERROR("Failed to get variable index. The variable ", &Variable, " does not belong to this shader variable manager");
        return ~0u;
    }
}

const PipelineResourceDesc& ShaderVariableManagerVk::GetResourceDesc(Uint32 Index) const
{
    VERIFY_EXPR(m_pSignature);
    return m_pSignature->GetResourceDesc(Index);
}

const ShaderVariableManagerVk::ResourceAttribs& ShaderVariableManagerVk::GetResourceAttribs(Uint32 Index) const
{
    VERIFY_EXPR(m_pSignature);
    return m_pSignature->GetResourceAttribs(Index);
}

void ShaderVariableManagerVk::BindResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags)
{
    TBase::BindResources(pResourceMapping, Flags);
}

void ShaderVariableManagerVk::CheckResources(IResourceMapping*                    pResourceMapping,
                                             BIND_SHADER_RESOURCES_FLAGS          Flags,
                                             SHADER_RESOURCE_VARIABLE_TYPE_FLAGS& StaleVarTypes) const
{
    TBase::CheckResources(pResourceMapping, Flags, StaleVarTypes);
}


namespace
{

#ifdef DILIGENT_DEVELOPMENT
inline BUFFER_VIEW_TYPE DvpDescriptorTypeToBufferView(DescriptorType Type)
{
    static_assert(static_cast<Uint32>(DescriptorType::Count) == 16, "Please update the switch below to handle the new descriptor type");
    switch (Type)
    {
        case DescriptorType::UniformTexelBuffer:
        case DescriptorType::StorageTexelBuffer_ReadOnly:
        case DescriptorType::StorageBuffer_ReadOnly:
        case DescriptorType::StorageBufferDynamic_ReadOnly:
            return BUFFER_VIEW_SHADER_RESOURCE;

        case DescriptorType::StorageTexelBuffer:
        case DescriptorType::StorageBuffer:
        case DescriptorType::StorageBufferDynamic:
            return BUFFER_VIEW_UNORDERED_ACCESS;

        default:
            UNEXPECTED("Unsupported descriptor type for buffer view");
            return BUFFER_VIEW_UNDEFINED;
    }
}

inline TEXTURE_VIEW_TYPE DvpDescriptorTypeToTextureView(DescriptorType Type)
{
    static_assert(static_cast<Uint32>(DescriptorType::Count) == 16, "Please update the switch below to handle the new descriptor type");
    switch (Type)
    {
        case DescriptorType::StorageImage:
            return TEXTURE_VIEW_UNORDERED_ACCESS;

        case DescriptorType::CombinedImageSampler:
        case DescriptorType::SeparateImage:
        case DescriptorType::InputAttachment:
        case DescriptorType::InputAttachment_General:
            return TEXTURE_VIEW_SHADER_RESOURCE;

        default:
            UNEXPECTED("Unsupported descriptor type for texture view");
            return TEXTURE_VIEW_UNDEFINED;
    }
}
#endif

struct BindResourceHelper
{
    BindResourceHelper(const PipelineResourceSignatureVkImpl& Signature,
                       ShaderResourceCacheVk&                 ResourceCache,
                       Uint32                                 ResIndex,
                       Uint32                                 ArrayIndex);

    void operator()(const BindResourceInfo& BindInfo) const;

private:
    // clang-format off
    void CacheUniformBuffer        (const BindResourceInfo& BindInfo) const;
    void CacheStorageBuffer        (const BindResourceInfo& BindInfo) const;
    void CacheTexelBuffer          (const BindResourceInfo& BindInfo) const;
    void CacheImage                (const BindResourceInfo& BindInfo) const;
    void CacheSeparateSampler      (const BindResourceInfo& BindInfo) const;
    void CacheInputAttachment      (const BindResourceInfo& BindInfo) const;
    void CacheAccelerationStructure(const BindResourceInfo& BindInfo) const;
    // clang-format on

    template <typename ObjectType>
    bool UpdateCachedResource(RefCntAutoPtr<ObjectType>&& pObject,
                              SET_SHADER_RESOURCE_FLAGS   Flags,
                              Uint64                      BufferBaseOffset = 0,
                              Uint64                      BufferRangeSize  = 0) const;

private:
    using ResourceAttribs = PipelineResourceSignatureVkImpl::ResourceAttribs;
    using CachedSet       = ShaderResourceCacheVk::DescriptorSet;

    const PipelineResourceSignatureVkImpl& m_Signature;
    ShaderResourceCacheVk&                 m_ResourceCache;
    const Uint32                           m_ArrayIndex;
    const ResourceCacheContentType         m_CacheType;
    const PipelineResourceDesc&            m_ResDesc;
    const ResourceAttribs&                 m_Attribs;
    const Uint32                           m_DstResCacheOffset;
    const CachedSet&                       m_CachedSet;
    const ShaderResourceCacheVk::Resource& m_DstRes;
};

BindResourceHelper::BindResourceHelper(const PipelineResourceSignatureVkImpl& Signature,
                                       ShaderResourceCacheVk&                 ResourceCache,
                                       Uint32                                 ResIndex,
                                       Uint32                                 ArrayIndex) :
    // clang-format off
    m_Signature         {Signature},
    m_ResourceCache     {ResourceCache},
    m_ArrayIndex        {ArrayIndex},
    m_CacheType         {ResourceCache.GetContentType()},
    m_ResDesc           {Signature.GetResourceDesc(ResIndex)},
    m_Attribs           {Signature.GetResourceAttribs(ResIndex)},
    m_DstResCacheOffset {m_Attribs.CacheOffset(m_CacheType) + ArrayIndex},
    m_CachedSet         {const_cast<const ShaderResourceCacheVk&>(ResourceCache).GetDescriptorSet(m_Attribs.DescrSet)},
    m_DstRes            {m_CachedSet.GetResource(m_DstResCacheOffset)}
// clang-format on
{
    VERIFY(ArrayIndex < m_ResDesc.ArraySize, "Array index is out of range, but it should've been corrected by ShaderVariableBase::SetArray()");
    VERIFY(m_DstRes.Type == m_Attribs.GetDescriptorType(), "Inconsistent types");

#ifdef DILIGENT_DEBUG
    {
        auto vkDescrSet = m_CachedSet.GetVkDescriptorSet();
        if (m_CacheType == ResourceCacheContentType::SRB)
        {
            if (m_ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC ||
                m_ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
            {
                VERIFY(vkDescrSet != VK_NULL_HANDLE, "Static and mutable variables must have a valid Vulkan descriptor set assigned");
            }
            else
            {
                VERIFY(vkDescrSet == VK_NULL_HANDLE, "Dynamic variables must never have valid Vulkan descriptor set assigned");
            }
        }
        else if (m_CacheType == ResourceCacheContentType::Signature)
        {
            VERIFY(vkDescrSet == VK_NULL_HANDLE, "Static shader resource cache should not have Vulkan descriptor set allocation");
        }
        else
        {
            UNEXPECTED("Unexpected shader resource cache content type");
        }
    }
#endif
}

void BindResourceHelper::operator()(const BindResourceInfo& BindInfo) const
{
    VERIFY_EXPR(m_ArrayIndex == BindInfo.ArrayIndex);

    if (BindInfo.pObject != nullptr)
    {
        static_assert(static_cast<Uint32>(DescriptorType::Count) == 16, "Please update the switch below to handle the new descriptor type");
        switch (m_DstRes.Type)
        {
            case DescriptorType::UniformBuffer:
            case DescriptorType::UniformBufferDynamic:
                CacheUniformBuffer(BindInfo);
                break;

            case DescriptorType::StorageBuffer:
            case DescriptorType::StorageBuffer_ReadOnly:
            case DescriptorType::StorageBufferDynamic:
            case DescriptorType::StorageBufferDynamic_ReadOnly:
                CacheStorageBuffer(BindInfo);
                break;

            case DescriptorType::UniformTexelBuffer:
            case DescriptorType::StorageTexelBuffer:
            case DescriptorType::StorageTexelBuffer_ReadOnly:
                CacheTexelBuffer(BindInfo);
                break;

            case DescriptorType::StorageImage:
            case DescriptorType::SeparateImage:
            case DescriptorType::CombinedImageSampler:
                CacheImage(BindInfo);
                break;

            case DescriptorType::Sampler:
                if (!m_Attribs.IsImmutableSamplerAssigned())
                {
                    CacheSeparateSampler(BindInfo);
                }
                else
                {
                    // Immutable samplers are permanently bound into the set layout; later binding a sampler
                    // into an immutable sampler slot in a descriptor set is not allowed (13.2.1)
                    UNEXPECTED("Attempting to assign a sampler to an immutable sampler '", m_ResDesc.Name, '\'');
                }
                break;

            case DescriptorType::InputAttachment:
            case DescriptorType::InputAttachment_General:
                CacheInputAttachment(BindInfo);
                break;

            case DescriptorType::AccelerationStructure:
                CacheAccelerationStructure(BindInfo);
                break;

            default: UNEXPECTED("Unknown resource type ", static_cast<Uint32>(m_DstRes.Type));
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

        m_ResourceCache.ResetResource(m_Attribs.DescrSet, m_DstResCacheOffset);
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

        m_ResourceCache.SetResource(&m_Signature.GetDevice()->GetLogicalDevice(),
                                    m_Attribs.DescrSet,
                                    m_DstResCacheOffset,
                                    {
                                        m_Attribs.BindingIndex,
                                        m_ArrayIndex,
                                        std::move(pObject),
                                        BufferBaseOffset,
                                        BufferRangeSize //
                                    });
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
    VERIFY((m_DstRes.Type == DescriptorType::UniformBuffer ||
            m_DstRes.Type == DescriptorType::UniformBufferDynamic),
           "Uniform buffer resource is expected");

    // We cannot use ClassPtrCast<> here as the resource can have wrong type
    RefCntAutoPtr<BufferVkImpl> pBufferVk{BindInfo.pObject, IID_BufferVk};
#ifdef DILIGENT_DEVELOPMENT
    VerifyConstantBufferBinding(m_ResDesc, BindInfo, pBufferVk.RawPtr(), m_DstRes.pObject.RawPtr(),
                                m_DstRes.BufferBaseOffset, m_DstRes.BufferRangeSize, m_Signature.GetDesc().Name);
#endif

    UpdateCachedResource(std::move(pBufferVk), BindInfo.Flags, BindInfo.BufferBaseOffset, BindInfo.BufferRangeSize);
}

void BindResourceHelper::CacheStorageBuffer(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting storage buffer to null is handled by BindResourceHelper::operator()");
    VERIFY((m_DstRes.Type == DescriptorType::StorageBuffer ||
            m_DstRes.Type == DescriptorType::StorageBuffer_ReadOnly ||
            m_DstRes.Type == DescriptorType::StorageBufferDynamic ||
            m_DstRes.Type == DescriptorType::StorageBufferDynamic_ReadOnly),
           "Storage buffer resource is expected");

    RefCntAutoPtr<BufferViewVkImpl> pBufferViewVk{BindInfo.pObject, IID_BufferViewVk};
#ifdef DILIGENT_DEVELOPMENT
    {
        // HLSL buffer SRVs are mapped to storage buffers in GLSL
        const BUFFER_VIEW_TYPE RequiredViewType = DvpDescriptorTypeToBufferView(m_DstRes.Type);
        VerifyResourceViewBinding(m_ResDesc, BindInfo, pBufferViewVk.RawPtr(),
                                  {RequiredViewType},
                                  RESOURCE_DIM_BUFFER, // Expected resource dim
                                  false,               // IsMultisample (ignored when resource dim is buffer)
                                  m_DstRes.pObject.RawPtr(),
                                  m_Signature.GetDesc().Name);

        VERIFY((m_ResDesc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) == 0,
               "FORMATTED_BUFFER resource flag is set for a storage buffer - this should've not happened.");
        ValidateBufferMode(m_ResDesc, m_ArrayIndex, pBufferViewVk.RawPtr());
    }
#endif

    UpdateCachedResource(std::move(pBufferViewVk), BindInfo.Flags);
}

void BindResourceHelper::CacheTexelBuffer(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting texel buffer to null is handled by BindResourceHelper::operator()");
    VERIFY((m_DstRes.Type == DescriptorType::UniformTexelBuffer ||
            m_DstRes.Type == DescriptorType::StorageTexelBuffer ||
            m_DstRes.Type == DescriptorType::StorageTexelBuffer_ReadOnly),
           "Uniform or storage buffer resource is expected");

    RefCntAutoPtr<BufferViewVkImpl> pBufferViewVk{BindInfo.pObject, IID_BufferViewVk};
#ifdef DILIGENT_DEVELOPMENT
    {
        // HLSL buffer SRVs are mapped to storage buffers in GLSL
        const BUFFER_VIEW_TYPE RequiredViewType = DvpDescriptorTypeToBufferView(m_DstRes.Type);
        VerifyResourceViewBinding(m_ResDesc, BindInfo, pBufferViewVk.RawPtr(),
                                  {RequiredViewType},
                                  RESOURCE_DIM_BUFFER, // Expected resource dim
                                  false,               // IsMultisample (ignored when resource dim is buffer)
                                  m_DstRes.pObject.RawPtr(),
                                  m_Signature.GetDesc().Name);

        VERIFY((m_ResDesc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) != 0,
               "FORMATTED_BUFFER resource flag is not set for a texel buffer - this should've not happened.");
        ValidateBufferMode(m_ResDesc, m_ArrayIndex, pBufferViewVk.RawPtr());
    }
#endif

    UpdateCachedResource(std::move(pBufferViewVk), BindInfo.Flags);
}

void BindResourceHelper::CacheImage(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting image to null is handled by BindResourceHelper::operator()");
    VERIFY((m_DstRes.Type == DescriptorType::StorageImage ||
            m_DstRes.Type == DescriptorType::SeparateImage ||
            m_DstRes.Type == DescriptorType::CombinedImageSampler),
           "Storage image, separate image or sampled image resource is expected");

    RefCntAutoPtr<TextureViewVkImpl> pTexViewVk0{BindInfo.pObject, IID_TextureViewVk};
#ifdef DILIGENT_DEVELOPMENT
    {
        // HLSL buffer SRVs are mapped to storage buffers in GLSL
        TEXTURE_VIEW_TYPE RequiredViewType = DvpDescriptorTypeToTextureView(m_DstRes.Type);
        VerifyResourceViewBinding(m_ResDesc, BindInfo, pTexViewVk0.RawPtr(),
                                  {RequiredViewType},
                                  RESOURCE_DIM_UNDEFINED, // Required resource dimension is not known
                                  false,                  // IsMultisample (ignored when resource dim is unknown)
                                  m_DstRes.pObject.RawPtr(),
                                  m_Signature.GetDesc().Name);
    }
#endif

    TextureViewVkImpl* pTexViewVk = pTexViewVk0;
    if (UpdateCachedResource(std::move(pTexViewVk0), BindInfo.Flags))
    {
#ifdef DILIGENT_DEVELOPMENT
        if (m_DstRes.Type == DescriptorType::CombinedImageSampler && !m_Attribs.IsImmutableSamplerAssigned())
        {
            if (pTexViewVk->GetSampler() == nullptr)
            {
                LOG_ERROR_MESSAGE("Error binding texture view '", pTexViewVk->GetDesc().Name, "' to variable '",
                                  GetShaderResourcePrintName(m_ResDesc, m_ArrayIndex), "'. No sampler is assigned to the view");
            }
        }
#endif

        if (m_Attribs.IsCombinedWithSampler())
        {
            VERIFY(m_DstRes.Type == DescriptorType::SeparateImage,
                   "Only separate images can be assigned separate samplers when using HLSL-style combined samplers.");
            VERIFY(!m_Attribs.IsImmutableSamplerAssigned(), "Separate image can't be assigned an immutable sampler.");

            const PipelineResourceDesc&      SamplerResDesc = m_Signature.GetResourceDesc(m_Attribs.SamplerInd);
            const PipelineResourceAttribsVk& SamplerAttribs = m_Signature.GetResourceAttribs(m_Attribs.SamplerInd);
            VERIFY_EXPR(SamplerResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER);

            if (!SamplerAttribs.IsImmutableSamplerAssigned())
            {
                if (ISampler* pSampler = pTexViewVk->GetSampler())
                {
                    DEV_CHECK_ERR(SamplerResDesc.ArraySize == 1 || SamplerResDesc.ArraySize == m_ResDesc.ArraySize,
                                  "Array size (", SamplerResDesc.ArraySize,
                                  ") of separate sampler variable '",
                                  SamplerResDesc.Name,
                                  "' must be one or the same as the array size (", m_ResDesc.ArraySize,
                                  ") of separate image variable '", m_ResDesc.Name, "' it is assigned to");

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
                                      "' assigned to separate image '", GetShaderResourcePrintName(m_ResDesc, m_ArrayIndex),
                                      "': no sampler is set in texture view '", pTexViewVk->GetDesc().Name, '\'');
                }
            }
        }
    }
}

void BindResourceHelper::CacheSeparateSampler(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting separate sampler to null is handled by BindResourceHelper::operator()");
    VERIFY(m_DstRes.Type == DescriptorType::Sampler, "Separate sampler resource is expected");
    VERIFY(!m_Attribs.IsImmutableSamplerAssigned(), "This separate sampler is assigned an immutable sampler");

    RefCntAutoPtr<SamplerVkImpl> pSamplerVk{BindInfo.pObject, IID_Sampler};
#ifdef DILIGENT_DEVELOPMENT
    VerifySamplerBinding(m_ResDesc, BindInfo, pSamplerVk.RawPtr(), m_DstRes.pObject, m_Signature.GetDesc().Name);
#endif

    UpdateCachedResource(std::move(pSamplerVk), BindInfo.Flags);
}

void BindResourceHelper::CacheInputAttachment(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting input attachment to null is handled by BindResourceHelper::operator()");
    VERIFY((m_DstRes.Type == DescriptorType::InputAttachment ||
            m_DstRes.Type == DescriptorType::InputAttachment_General),
           "Input attachment resource is expected");
    RefCntAutoPtr<TextureViewVkImpl> pTexViewVk{BindInfo.pObject, IID_TextureViewVk};
#ifdef DILIGENT_DEVELOPMENT
    VerifyResourceViewBinding(m_ResDesc, BindInfo, pTexViewVk.RawPtr(),
                              {TEXTURE_VIEW_SHADER_RESOURCE},
                              RESOURCE_DIM_UNDEFINED,
                              false, // IsMultisample
                              m_DstRes.pObject.RawPtr(),
                              m_Signature.GetDesc().Name);
#endif

    UpdateCachedResource(std::move(pTexViewVk), BindInfo.Flags);
}

void BindResourceHelper::CacheAccelerationStructure(const BindResourceInfo& BindInfo) const
{
    VERIFY(BindInfo.pObject != nullptr, "Setting acceleration structure to null is handled by BindResourceHelper::operator()");
    VERIFY(m_DstRes.Type == DescriptorType::AccelerationStructure, "Acceleration Structure resource is expected");
    RefCntAutoPtr<TopLevelASVkImpl> pTLASVk{BindInfo.pObject, IID_TopLevelASVk};
#ifdef DILIGENT_DEVELOPMENT
    VerifyTLASResourceBinding(m_ResDesc, BindInfo, pTLASVk.RawPtr(), m_DstRes.pObject.RawPtr(), m_Signature.GetDesc().Name);
#endif

    UpdateCachedResource(std::move(pTLASVk), BindInfo.Flags);
}

} // namespace


void ShaderVariableManagerVk::BindResource(Uint32 ResIndex, const BindResourceInfo& BindInfo)
{
    BindResourceHelper BindHelper{
        *m_pSignature,
        m_ResourceCache,
        ResIndex,
        BindInfo.ArrayIndex};

    BindHelper(BindInfo);
}

void ShaderVariableManagerVk::SetBufferDynamicOffset(Uint32 ResIndex,
                                                     Uint32 ArrayIndex,
                                                     Uint32 BufferDynamicOffset)
{
    const PipelineResourceAttribsVk& Attribs           = m_pSignature->GetResourceAttribs(ResIndex);
    const Uint32                     DstResCacheOffset = Attribs.CacheOffset(m_ResourceCache.GetContentType()) + ArrayIndex;
#ifdef DILIGENT_DEVELOPMENT
    {
        const PipelineResourceDesc&                 ResDesc = m_pSignature->GetResourceDesc(ResIndex);
        const ShaderResourceCacheVk::DescriptorSet& Set     = const_cast<const ShaderResourceCacheVk&>(m_ResourceCache).GetDescriptorSet(Attribs.DescrSet);
        const ShaderResourceCacheVk::Resource&      DstRes  = Set.GetResource(DstResCacheOffset);
        VerifyDynamicBufferOffset<BufferVkImpl, BufferViewVkImpl>(ResDesc, DstRes.pObject, DstRes.BufferBaseOffset, DstRes.BufferRangeSize, BufferDynamicOffset);
    }
#endif
    m_ResourceCache.SetDynamicBufferOffset(Attribs.DescrSet, DstResCacheOffset, BufferDynamicOffset);
}

IDeviceObject* ShaderVariableManagerVk::Get(Uint32 ArrayIndex, Uint32 ResIndex) const
{
    const PipelineResourceDesc&      ResDesc     = GetResourceDesc(ResIndex);
    const PipelineResourceAttribsVk& Attribs     = GetResourceAttribs(ResIndex);
    const Uint32                     CacheOffset = Attribs.CacheOffset(m_ResourceCache.GetContentType());

    VERIFY_EXPR(ArrayIndex < ResDesc.ArraySize);

    if (Attribs.DescrSet < m_ResourceCache.GetNumDescriptorSets())
    {
        const ShaderResourceCacheVk::DescriptorSet& Set = const_cast<const ShaderResourceCacheVk&>(m_ResourceCache).GetDescriptorSet(Attribs.DescrSet);
        if (CacheOffset + ArrayIndex < Set.GetSize())
        {
            const ShaderResourceCacheVk::Resource& CachedRes = Set.GetResource(CacheOffset + ArrayIndex);
            return CachedRes.pObject;
        }
    }

    return nullptr;
}

} // namespace Diligent
