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

#include "pch.h"

#include "ShaderVariableManagerGL.hpp"

#include <array>

#include "RenderDeviceGLImpl.hpp"
#include "PipelineResourceSignatureGLImpl.hpp"
#include "Align.hpp"
#include "PlatformMisc.hpp"

namespace Diligent
{

ShaderVariableManagerGL::ResourceCounters ShaderVariableManagerGL::CountResources(
    const PipelineResourceSignatureGLImpl& Signature,
    const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
    Uint32                                 NumAllowedTypes,
    const SHADER_TYPE                      ShaderType)
{
    ResourceCounters Counters;

    Signature.ProcessResources(
        AllowedVarTypes, NumAllowedTypes, ShaderType,
        [&](const PipelineResourceDesc& ResDesc, Uint32) //
        {
            if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER)
                return;

            static_assert(BINDING_RANGE_COUNT == 4, "Please update the switch below to handle the new shader resource range");
            switch (PipelineResourceToBindingRange(ResDesc))
            {
                // clang-format off
                case BINDING_RANGE_UNIFORM_BUFFER: ++Counters.NumUBs;           break;
                case BINDING_RANGE_TEXTURE:        ++Counters.NumTextures;      break;
                case BINDING_RANGE_IMAGE:          ++Counters.NumImages;        break;
                case BINDING_RANGE_STORAGE_BUFFER: ++Counters.NumStorageBlocks; break;
                // clang-format on
                default:
                    UNEXPECTED("Unsupported resource type.");
            }
        });

    return Counters;
}

size_t ShaderVariableManagerGL::GetRequiredMemorySize(const PipelineResourceSignatureGLImpl& Signature,
                                                      const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                                                      Uint32                                 NumAllowedTypes,
                                                      SHADER_TYPE                            ShaderType)
{
    auto Counters = CountResources(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType);

    // clang-format off
    size_t RequiredSize = Counters.NumUBs           * sizeof(UniformBuffBindInfo)   +
                          Counters.NumTextures      * sizeof(TextureBindInfo)       +
                          Counters.NumImages        * sizeof(ImageBindInfo)         +
                          Counters.NumStorageBlocks * sizeof(StorageBufferBindInfo);
    // clang-format on
    return RequiredSize;
}

void ShaderVariableManagerGL::Initialize(const PipelineResourceSignatureGLImpl& Signature,
                                         IMemoryAllocator&                      Allocator,
                                         const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                                         Uint32                                 NumAllowedTypes,
                                         SHADER_TYPE                            ShaderType)
{
    const auto Counters = CountResources(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType);

    // Initialize offsets
    size_t CurrentOffset = 0;

    auto AdvanceOffset = [&CurrentOffset](size_t NumBytes) //
    {
        constexpr size_t MaxOffset = std::numeric_limits<OffsetType>::max();
        VERIFY(CurrentOffset <= MaxOffset, "Current offset (", CurrentOffset, ") exceeds max allowed value (", MaxOffset, ")");
        (void)MaxOffset;
        auto Offset = static_cast<OffsetType>(CurrentOffset);
        CurrentOffset += NumBytes;
        return Offset;
    };

    // clang-format off
    auto UBOffset         = AdvanceOffset(Counters.NumUBs           * sizeof(UniformBuffBindInfo)  ); (void)UBOffset; // To suppress warning
    m_TextureOffset       = AdvanceOffset(Counters.NumTextures      * sizeof(TextureBindInfo)      );
    m_ImageOffset         = AdvanceOffset(Counters.NumImages        * sizeof(ImageBindInfo)        );
    m_StorageBufferOffset = AdvanceOffset(Counters.NumStorageBlocks * sizeof(StorageBufferBindInfo));
    m_VariableEndOffset   = AdvanceOffset(0);
    // clang-format off
    auto TotalMemorySize = m_VariableEndOffset;
    VERIFY_EXPR(TotalMemorySize == GetRequiredMemorySize(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType));
    TBase::Initialize(Signature, Allocator, TotalMemorySize);

    // clang-format off
    VERIFY_EXPR(Counters.NumUBs           == GetNumUBs()           );
    VERIFY_EXPR(Counters.NumTextures      == GetNumTextures()      );
    VERIFY_EXPR(Counters.NumImages        == GetNumImages()        );
    VERIFY_EXPR(Counters.NumStorageBlocks == GetNumStorageBuffers());
    // clang-format on

    // Current resource index for every resource type
    ResourceCounters VarCounters = {};

    Signature.ProcessResources(
        AllowedVarTypes, NumAllowedTypes, ShaderType,
        [&](const PipelineResourceDesc& ResDesc, Uint32 Index) //
        {
            if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER)
                return;

            static_assert(BINDING_RANGE_COUNT == 4, "Please update the switch below to handle the new shader resource range");
            switch (PipelineResourceToBindingRange(ResDesc))
            {
                case BINDING_RANGE_UNIFORM_BUFFER:
                    new (&GetResource<UniformBuffBindInfo>(VarCounters.NumUBs++)) UniformBuffBindInfo{*this, Index};
                    break;
                case BINDING_RANGE_TEXTURE:
                    new (&GetResource<TextureBindInfo>(VarCounters.NumTextures++)) TextureBindInfo{*this, Index};
                    break;
                case BINDING_RANGE_IMAGE:
                    new (&GetResource<ImageBindInfo>(VarCounters.NumImages++)) ImageBindInfo{*this, Index};
                    break;
                case BINDING_RANGE_STORAGE_BUFFER:
                    new (&GetResource<StorageBufferBindInfo>(VarCounters.NumStorageBlocks++)) StorageBufferBindInfo{*this, Index};
                    break;
                default:
                    UNEXPECTED("Unsupported resource type.");
            }
        });

    // clang-format off
    VERIFY(VarCounters.NumUBs           == GetNumUBs(),             "Not all UBs are initialized which will cause a crash when dtor is called");
    VERIFY(VarCounters.NumTextures      == GetNumTextures(),        "Not all Textures are initialized which will cause a crash when dtor is called");
    VERIFY(VarCounters.NumImages        == GetNumImages(),          "Not all Images are initialized which will cause a crash when dtor is called");
    VERIFY(VarCounters.NumStorageBlocks == GetNumStorageBuffers(),  "Not all SSBOs are initialized which will cause a crash when dtor is called");
    // clang-format on
}

void ShaderVariableManagerGL::Destroy(IMemoryAllocator& Allocator)
{
    if (m_pVariables != nullptr)
    {
        HandleResources(
            [&](UniformBuffBindInfo& ub) {
                ub.~UniformBuffBindInfo();
            },
            [&](TextureBindInfo& tex) {
                tex.~TextureBindInfo();
            },
            [&](ImageBindInfo& img) {
                img.~ImageBindInfo();
            },
            [&](StorageBufferBindInfo& ssbo) {
                ssbo.~StorageBufferBindInfo();
            });
    }
    TBase::Destroy(Allocator);
}

void ShaderVariableManagerGL::UniformBuffBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();

    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Index is out of range, but it should've been corrected by ShaderVariableBase::SetArray()");
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER);

    auto& ResourceCache = m_ParentManager.m_ResourceCache;

    // We cannot use ClassPtrCast<> here as the resource can be of wrong type
    RefCntAutoPtr<BufferGLImpl> pBuffGLImpl{BindInfo.pObject, IID_BufferGL};
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedUB = ResourceCache.GetConstUB(Attr.CacheOffset + BindInfo.ArrayIndex);
        VerifyConstantBufferBinding(Desc, BindInfo, pBuffGLImpl.RawPtr(), CachedUB.pBuffer.RawPtr(),
                                    CachedUB.BaseOffset, CachedUB.RangeSize, m_ParentManager.m_pSignature->GetDesc().Name);
    }
#endif

    ResourceCache.SetUniformBuffer(Attr.CacheOffset + BindInfo.ArrayIndex, std::move(pBuffGLImpl),
                                   BindInfo.BufferBaseOffset, BindInfo.BufferRangeSize);
}

void ShaderVariableManagerGL::UniformBuffBindInfo::SetDynamicOffset(Uint32 ArrayIndex, Uint32 Offset)
{
    const auto& Attr = GetAttribs();
    const auto& Desc = GetDesc();
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER);
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedUB = m_ParentManager.m_ResourceCache.GetConstUB(Attr.CacheOffset + ArrayIndex);
        VerifyDynamicBufferOffset<BufferGLImpl, BufferViewGLImpl>(Desc, CachedUB.pBuffer, CachedUB.BaseOffset, CachedUB.RangeSize, Offset);
    }
#endif
    m_ParentManager.m_ResourceCache.SetDynamicUBOffset(Attr.CacheOffset + ArrayIndex, Offset);
}


void ShaderVariableManagerGL::TextureBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();

    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Index is out of range, but it should've been corrected by ShaderVariableBase::SetArray()");
    auto& ResourceCache = m_ParentManager.m_ResourceCache;

    if (Desc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV ||
        Desc.ResourceType == SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT)
    {
        // We cannot use ClassPtrCast<> here as the resource can be of wrong type
        RefCntAutoPtr<TextureViewGLImpl> pViewGL{BindInfo.pObject, IID_TextureViewGL};

        const auto ImmutableSamplerAssigned = (m_ParentManager.m_pSignature->GetImmutableSamplerIdx(Attr) != InvalidImmutableSamplerIndex);
#ifdef DILIGENT_DEVELOPMENT
        {
            const auto& CachedTexSampler = ResourceCache.GetConstTexture(Attr.CacheOffset + BindInfo.ArrayIndex);
            VerifyResourceViewBinding(Desc, BindInfo, pViewGL.RawPtr(),
                                      {TEXTURE_VIEW_SHADER_RESOURCE}, // Expected view type
                                      RESOURCE_DIM_UNDEFINED,         // Expected resource dimension - unknown at this point
                                      false,                          // IsMultisample (ignored when resource dim is undefined)
                                      CachedTexSampler.pView.RawPtr(),
                                      m_ParentManager.m_pSignature->GetDesc().Name);
            if (ImmutableSamplerAssigned && ResourceCache.GetContentType() == ResourceCacheContentType::SRB)
            {
                VERIFY(CachedTexSampler.pSampler != nullptr, "Immutable samplers must be initialized in the SRB cache by PipelineResourceSignatureGLImpl::InitSRBResourceCache!");
            }
            if (Desc.ResourceType == SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT)
            {
                DEV_CHECK_ERR(!ImmutableSamplerAssigned, "Input attachment must not have assigned sampler.");
            }
        }
#endif
        ResourceCache.SetTexture(Attr.CacheOffset + BindInfo.ArrayIndex, std::move(pViewGL), !ImmutableSamplerAssigned);
    }
    else if (Desc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV)
    {
        // We cannot use ClassPtrCast<> here as the resource can be of wrong type
        RefCntAutoPtr<BufferViewGLImpl> pViewGL{BindInfo.pObject, IID_BufferViewGL};
#ifdef DILIGENT_DEVELOPMENT
        {
            const auto& CachedBuffSampler = ResourceCache.GetConstTexture(Attr.CacheOffset + BindInfo.ArrayIndex);
            VerifyResourceViewBinding(Desc, BindInfo, pViewGL.RawPtr(),
                                      {BUFFER_VIEW_SHADER_RESOURCE}, // Expected view type
                                      RESOURCE_DIM_BUFFER,           // Expected resource dimension
                                      false,                         // IsMultisample (ignored when resource dim is buffer)
                                      CachedBuffSampler.pView.RawPtr(),
                                      m_ParentManager.m_pSignature->GetDesc().Name);

            VERIFY((Desc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) != 0,
                   "FORMATTED_BUFFER resource flag is not set for a texel buffer - this should've not happened.");
            ValidateBufferMode(Desc, BindInfo.ArrayIndex, pViewGL.RawPtr());
        }
#endif
        ResourceCache.SetTexelBuffer(Attr.CacheOffset + BindInfo.ArrayIndex, std::move(pViewGL));
    }
    else
    {
        UNEXPECTED("Unexpected resource type ", GetShaderResourceTypeLiteralName(Desc.ResourceType), ". Texture SRV or buffer SRV is expected.");
    }
}


void ShaderVariableManagerGL::ImageBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();

    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Index is out of range, but it should've been corrected by ShaderVariableBase::SetArray()");
    auto& ResourceCache = m_ParentManager.m_ResourceCache;

    if (Desc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_UAV)
    {
        // We cannot use ClassPtrCast<> here as the resource can be of wrong type
        RefCntAutoPtr<TextureViewGLImpl> pViewGL{BindInfo.pObject, IID_TextureViewGL};
#ifdef DILIGENT_DEVELOPMENT
        {
            const auto& CachedUAV = ResourceCache.GetConstImage(Attr.CacheOffset + BindInfo.ArrayIndex);
            VerifyResourceViewBinding(Desc, BindInfo, pViewGL.RawPtr(),
                                      {TEXTURE_VIEW_UNORDERED_ACCESS}, // Expected view type
                                      RESOURCE_DIM_UNDEFINED,          // Expected resource dimension - unknown at this point
                                      false,                           // IsMultisample (ignored when resource dim is unknown)
                                      CachedUAV.pView.RawPtr(),
                                      m_ParentManager.m_pSignature->GetDesc().Name);
        }
#endif
        ResourceCache.SetTexImage(Attr.CacheOffset + BindInfo.ArrayIndex, std::move(pViewGL));
    }
    else if (Desc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV)
    {
        // We cannot use ClassPtrCast<> here as the resource can be of wrong type
        RefCntAutoPtr<BufferViewGLImpl> pViewGL{BindInfo.pObject, IID_BufferViewGL};
#ifdef DILIGENT_DEVELOPMENT
        {
            const auto& CachedUAV = ResourceCache.GetConstImage(Attr.CacheOffset + BindInfo.ArrayIndex);
            VerifyResourceViewBinding(Desc, BindInfo, pViewGL.RawPtr(),
                                      {BUFFER_VIEW_UNORDERED_ACCESS}, // Expected view type
                                      RESOURCE_DIM_BUFFER,            // Expected resource dimension
                                      false,                          // IsMultisample (ignored when resource dim is buffer)
                                      CachedUAV.pView.RawPtr(),
                                      m_ParentManager.m_pSignature->GetDesc().Name);

            VERIFY((Desc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) != 0,
                   "FORMATTED_BUFFER resource flag is not set for an image buffer - this should've not happened.");
            ValidateBufferMode(Desc, BindInfo.ArrayIndex, pViewGL.RawPtr());
        }
#endif
        ResourceCache.SetBufImage(Attr.CacheOffset + BindInfo.ArrayIndex, std::move(pViewGL));
    }
    else
    {
        UNEXPECTED("Unexpected resource type ", GetShaderResourceTypeLiteralName(Desc.ResourceType), ". Texture UAV or buffer UAV is expected.");
    }
}



void ShaderVariableManagerGL::StorageBufferBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();

    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Index is out of range, but it should've been corrected by ShaderVariableBase::SetArray()");
    auto& ResourceCache = m_ParentManager.m_ResourceCache;
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV ||
                Desc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV);

    // We cannot use ClassPtrCast<> here as the resource can be of wrong type
    RefCntAutoPtr<BufferViewGLImpl> pViewGL{BindInfo.pObject, IID_BufferViewGL};
#ifdef DILIGENT_DEVELOPMENT
    {
        auto& CachedSSBO = ResourceCache.GetConstSSBO(Attr.CacheOffset + BindInfo.ArrayIndex);
        // HLSL structured buffers are mapped to SSBOs in GLSL
        VerifyResourceViewBinding(Desc, BindInfo, pViewGL.RawPtr(),
                                  {BUFFER_VIEW_SHADER_RESOURCE, BUFFER_VIEW_UNORDERED_ACCESS}, // Expected view types
                                  RESOURCE_DIM_BUFFER,                                         // Expected resource dimension
                                  false,                                                       // IsMultisample (ignored when resource dim is buffer)
                                  CachedSSBO.pBufferView.RawPtr(),
                                  m_ParentManager.m_pSignature->GetDesc().Name);

        VERIFY((Desc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) == 0,
               "FORMATTED_BUFFER resource flag is set for a storage buffer - this should've not happened.");
        ValidateBufferMode(Desc, BindInfo.ArrayIndex, pViewGL.RawPtr());
    }
#endif
    ResourceCache.SetSSBO(Attr.CacheOffset + BindInfo.ArrayIndex, std::move(pViewGL));
}

void ShaderVariableManagerGL::StorageBufferBindInfo::SetDynamicOffset(Uint32 ArrayIndex, Uint32 Offset)
{
    const auto& Attr = GetAttribs();
    const auto& Desc = GetDesc();
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV ||
                Desc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV);
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedSSBO = m_ParentManager.m_ResourceCache.GetConstSSBO(Attr.CacheOffset + ArrayIndex);
        VerifyDynamicBufferOffset<BufferGLImpl, BufferViewGLImpl>(Desc, CachedSSBO.pBufferView, 0, 0, Offset);
    }
#endif

    m_ParentManager.m_ResourceCache.SetDynamicSSBOOffset(Attr.CacheOffset + ArrayIndex, Offset);
}

void ShaderVariableManagerGL::BindResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags)
{
    if (pResourceMapping == nullptr)
    {
        LOG_ERROR_MESSAGE("Failed to bind resources: resource mapping is null");
        return;
    }

    if ((Flags & BIND_SHADER_RESOURCES_UPDATE_ALL) == 0)
        Flags |= BIND_SHADER_RESOURCES_UPDATE_ALL;

    HandleResources(
        [&](UniformBuffBindInfo& ub) {
            ub.BindResources(pResourceMapping, Flags);
        },
        [&](TextureBindInfo& tex) {
            tex.BindResources(pResourceMapping, Flags);
        },
        [&](ImageBindInfo& img) {
            img.BindResources(pResourceMapping, Flags);
        },
        [&](StorageBufferBindInfo& ssbo) {
            ssbo.BindResources(pResourceMapping, Flags);
        });
}

void ShaderVariableManagerGL::CheckResources(IResourceMapping*                    pResourceMapping,
                                             BIND_SHADER_RESOURCES_FLAGS          Flags,
                                             SHADER_RESOURCE_VARIABLE_TYPE_FLAGS& StaleVarTypes) const
{
    if ((Flags & BIND_SHADER_RESOURCES_UPDATE_ALL) == 0)
        Flags |= BIND_SHADER_RESOURCES_UPDATE_ALL;

    const auto AllowedTypes = m_ResourceCache.GetContentType() == ResourceCacheContentType::SRB ?
        SHADER_RESOURCE_VARIABLE_TYPE_FLAG_MUT_DYN :
        SHADER_RESOURCE_VARIABLE_TYPE_FLAG_STATIC;

    HandleConstResources(
        [&](const UniformBuffBindInfo& ub) {
            ub.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        },
        [&](const TextureBindInfo& tex) {
            tex.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        },
        [&](const ImageBindInfo& img) {
            img.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        },
        [&](const StorageBufferBindInfo& ssbo) {
            ssbo.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        });
}

template <typename ResourceType>
IShaderResourceVariable* ShaderVariableManagerGL::GetResourceByName(const Char* Name) const
{
    auto NumResources = GetNumResources<ResourceType>();
    for (Uint32 res = 0; res < NumResources; ++res)
    {
        auto&       Resource = GetResource<ResourceType>(res);
        const auto& ResDesc  = Resource.GetDesc();
        if (strcmp(ResDesc.Name, Name) == 0)
            return &Resource;
    }

    return nullptr;
}


IShaderResourceVariable* ShaderVariableManagerGL::GetVariable(const Char* Name) const
{
    if (auto* pUB = GetResourceByName<UniformBuffBindInfo>(Name))
        return pUB;

    if (auto* pTexture = GetResourceByName<TextureBindInfo>(Name))
        return pTexture;

    if (auto* pImage = GetResourceByName<ImageBindInfo>(Name))
        return pImage;

    if (auto* pSSBO = GetResourceByName<StorageBufferBindInfo>(Name))
        return pSSBO;

    return nullptr;
}

class ShaderVariableLocator
{
public:
    ShaderVariableLocator(const ShaderVariableManagerGL& _Mgr,
                          Uint32                         _Index) :
        // clang-format off
        Mgr   {_Mgr},
        Index {_Index}
    // clang-format on
    {
    }

    template <typename ResourceType>
    IShaderResourceVariable* TryResource(Uint32 NumResources)
    {
        if (Index < NumResources)
            return &Mgr.GetResource<ResourceType>(Index);
        else
        {
            Index -= NumResources;
            return nullptr;
        }
    }

private:
    ShaderVariableManagerGL const& Mgr;
    Uint32                         Index;
};


IShaderResourceVariable* ShaderVariableManagerGL::GetVariable(Uint32 Index) const
{
    ShaderVariableLocator VarLocator(*this, Index);

    if (auto* pUB = VarLocator.TryResource<UniformBuffBindInfo>(GetNumUBs()))
        return pUB;

    if (auto* pTexture = VarLocator.TryResource<TextureBindInfo>(GetNumTextures()))
        return pTexture;

    if (auto* pImage = VarLocator.TryResource<ImageBindInfo>(GetNumImages()))
        return pImage;

    if (auto* pSSBO = VarLocator.TryResource<StorageBufferBindInfo>(GetNumStorageBuffers()))
        return pSSBO;

    LOG_ERROR(Index, " is not a valid variable index.");
    return nullptr;
}



class ShaderVariableIndexLocator
{
public:
    ShaderVariableIndexLocator(const ShaderVariableManagerGL& _Mgr, const IShaderResourceVariable& Variable) :
        // clang-format off
        Mgr      {_Mgr},
        VarOffset(reinterpret_cast<const Uint8*>(&Variable) - reinterpret_cast<const Uint8*>(_Mgr.m_pVariables))
    // clang-format on
    {}

    template <typename ResourceType>
    bool TryResource(Uint32 NextResourceTypeOffset, Uint32 VarCount)
    {
        if (VarOffset < NextResourceTypeOffset)
        {
            auto RelativeOffset = VarOffset - Mgr.GetResourceOffset<ResourceType>();
            DEV_CHECK_ERR(RelativeOffset % sizeof(ResourceType) == 0, "Offset is not multiple of resource type (", sizeof(ResourceType), ")");
            RelativeOffset /= sizeof(ResourceType);
            VERIFY(RelativeOffset >= 0 && RelativeOffset < VarCount,
                   "Relative offset is out of bounds which either means the variable does not belong to this SRB or "
                   "there is a bug in variable offsets");
            Index += static_cast<Uint32>(RelativeOffset);
            return true;
        }
        else
        {
            Index += VarCount;
            return false;
        }
    }

    Uint32 GetIndex() const { return Index; }

private:
    const ShaderVariableManagerGL& Mgr;
    const size_t                   VarOffset;
    Uint32                         Index = 0;
};


Uint32 ShaderVariableManagerGL::GetVariableIndex(const IShaderResourceVariable& Var) const
{
    if (m_pVariables == nullptr)
    {
        LOG_ERROR("This shader variable manager does not have any resources");
        return ~0u;
    }

    ShaderVariableIndexLocator IdxLocator{*this, Var};

    if (IdxLocator.TryResource<UniformBuffBindInfo>(m_TextureOffset, GetNumUBs()))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<TextureBindInfo>(m_ImageOffset, GetNumTextures()))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<ImageBindInfo>(m_StorageBufferOffset, GetNumImages()))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<StorageBufferBindInfo>(m_VariableEndOffset, GetNumStorageBuffers()))
        return IdxLocator.GetIndex();

    LOG_ERROR("Failed to get variable index. The variable ", &Var, " does not belong to this shader variable manager");
    return ~0u;
}

const PipelineResourceDesc& ShaderVariableManagerGL::GetResourceDesc(Uint32 Index) const
{
    VERIFY_EXPR(m_pSignature);
    return m_pSignature->GetResourceDesc(Index);
}

const ShaderVariableManagerGL::ResourceAttribs& ShaderVariableManagerGL::GetResourceAttribs(Uint32 Index) const
{
    VERIFY_EXPR(m_pSignature);
    return m_pSignature->GetResourceAttribs(Index);
}

} // namespace Diligent
