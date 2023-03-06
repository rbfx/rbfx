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

#include "WinHPreface.h"
#include <d3dcompiler.h>
#include "WinHPostface.h"

#include "ShaderVariableManagerD3D11.hpp"
#include "ShaderResourceCacheD3D11.hpp"
#include "BufferD3D11Impl.hpp"
#include "BufferViewD3D11Impl.hpp"
#include "TextureBaseD3D11.hpp"
#include "TextureViewD3D11.h"
#include "SamplerD3D11Impl.hpp"
#include "ShaderD3D11Impl.hpp"
#include "ShaderResourceVariableBase.hpp"
#include "PipelineResourceSignatureD3D11Impl.hpp"

namespace Diligent
{
namespace
{
template <typename HandlerType>
void ProcessSignatureResources(const PipelineResourceSignatureD3D11Impl& Signature,
                               const SHADER_RESOURCE_VARIABLE_TYPE*      AllowedVarTypes,
                               Uint32                                    NumAllowedTypes,
                               SHADER_TYPE                               ShaderStages,
                               HandlerType&&                             Handler)
{
    const bool UsingCombinedSamplers = Signature.IsUsingCombinedSamplers();
    Signature.ProcessResources(AllowedVarTypes, NumAllowedTypes, ShaderStages,
                               [&](const PipelineResourceDesc& ResDesc, Uint32 Index) //
                               {
                                   const auto& ResAttr = Signature.GetResourceAttribs(Index);

                                   // Skip samplers combined with textures and immutable samplers
                                   if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER &&
                                       (UsingCombinedSamplers || ResAttr.IsImmutableSamplerAssigned()))
                                       return;

                                   Handler(Index);
                               });
}
} // namespace

void ShaderVariableManagerD3D11::Destroy(IMemoryAllocator& Allocator)
{
    if (m_pVariables != nullptr)
    {
        HandleResources(
            [&](ConstBuffBindInfo& cb) {
                cb.~ConstBuffBindInfo();
            },
            [&](TexSRVBindInfo& ts) {
                ts.~TexSRVBindInfo();
            },
            [&](TexUAVBindInfo& uav) {
                uav.~TexUAVBindInfo();
            },
            [&](BuffSRVBindInfo& srv) {
                srv.~BuffSRVBindInfo();
            },
            [&](BuffUAVBindInfo& uav) {
                uav.~BuffUAVBindInfo();
            },
            [&](SamplerBindInfo& sam) {
                sam.~SamplerBindInfo();
            });
    }

    TBase::Destroy(Allocator);
}

const PipelineResourceDesc& ShaderVariableManagerD3D11::GetResourceDesc(Uint32 Index) const
{
    VERIFY_EXPR(m_pSignature);
    return m_pSignature->GetResourceDesc(Index);
}

const PipelineResourceAttribsD3D11& ShaderVariableManagerD3D11::GetResourceAttribs(Uint32 Index) const
{
    VERIFY_EXPR(m_pSignature);
    return m_pSignature->GetResourceAttribs(Index);
}

D3DShaderResourceCounters ShaderVariableManagerD3D11::CountResources(
    const PipelineResourceSignatureD3D11Impl& Signature,
    const SHADER_RESOURCE_VARIABLE_TYPE*      AllowedVarTypes,
    Uint32                                    NumAllowedTypes,
    const SHADER_TYPE                         ShaderType)
{
    D3DShaderResourceCounters Counters;
    ProcessSignatureResources(
        Signature, AllowedVarTypes, NumAllowedTypes, ShaderType,
        [&](Uint32 Index) //
        {
            const auto& ResDesc = Signature.GetResourceDesc(Index);
            static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update the switch below to handle the new shader resource range");
            switch (ResDesc.ResourceType)
            {
                // clang-format off
                case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:  ++Counters.NumCBs;      break;
                case SHADER_RESOURCE_TYPE_TEXTURE_SRV:      ++Counters.NumTexSRVs;  break;
                case SHADER_RESOURCE_TYPE_BUFFER_SRV:       ++Counters.NumBufSRVs;  break;
                case SHADER_RESOURCE_TYPE_TEXTURE_UAV:      ++Counters.NumTexUAVs;  break;
                case SHADER_RESOURCE_TYPE_BUFFER_UAV:       ++Counters.NumBufUAVs;  break;
                case SHADER_RESOURCE_TYPE_SAMPLER:          ++Counters.NumSamplers; break;
                case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT: ++Counters.NumTexSRVs;  break;
                // clang-format on
                default:
                    UNEXPECTED("Unsupported resource type.");
            }
        });
    return Counters;
}

size_t ShaderVariableManagerD3D11::GetRequiredMemorySize(const PipelineResourceSignatureD3D11Impl& Signature,
                                                         const SHADER_RESOURCE_VARIABLE_TYPE*      AllowedVarTypes,
                                                         Uint32                                    NumAllowedTypes,
                                                         SHADER_TYPE                               ShaderType)
{
    const auto ResCounters = CountResources(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType);
    // clang-format off
    auto MemSize = ResCounters.NumCBs      * sizeof(ConstBuffBindInfo) +
                   ResCounters.NumTexSRVs  * sizeof(TexSRVBindInfo)    +
                   ResCounters.NumTexUAVs  * sizeof(TexUAVBindInfo)    +
                   ResCounters.NumBufSRVs  * sizeof(BuffSRVBindInfo)   +
                   ResCounters.NumBufUAVs  * sizeof(BuffUAVBindInfo)   +
                   ResCounters.NumSamplers * sizeof(SamplerBindInfo);
    // clang-format on
    return MemSize;
}


void ShaderVariableManagerD3D11::Initialize(const PipelineResourceSignatureD3D11Impl& Signature,
                                            IMemoryAllocator&                         Allocator,
                                            const SHADER_RESOURCE_VARIABLE_TYPE*      AllowedVarTypes,
                                            Uint32                                    NumAllowedTypes,
                                            SHADER_TYPE                               ShaderType)
{
    const auto ResCounters = CountResources(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType);

    m_ShaderTypeIndex = static_cast<Uint8>(GetShaderTypeIndex(ShaderType));

    // Initialize offsets
    size_t CurrentOffset = 0;

    auto AdvanceOffset = [&CurrentOffset](size_t NumBytes) //
    {
        constexpr size_t MaxOffset = std::numeric_limits<OffsetType>::max();
        VERIFY(CurrentOffset <= MaxOffset, "Current offset (", CurrentOffset, ") exceeds max allowed value (", MaxOffset, ")");
        auto Offset = static_cast<OffsetType>(CurrentOffset);
        CurrentOffset += NumBytes;
        return Offset;
    };

    // clang-format off
    auto CBOffset    = AdvanceOffset(ResCounters.NumCBs      * sizeof(ConstBuffBindInfo));  (void)CBOffset; // To suppress warning
    m_TexSRVsOffset  = AdvanceOffset(ResCounters.NumTexSRVs  * sizeof(TexSRVBindInfo)   );
    m_TexUAVsOffset  = AdvanceOffset(ResCounters.NumTexUAVs  * sizeof(TexUAVBindInfo)   );
    m_BuffSRVsOffset = AdvanceOffset(ResCounters.NumBufSRVs  * sizeof(BuffSRVBindInfo)  );
    m_BuffUAVsOffset = AdvanceOffset(ResCounters.NumBufUAVs  * sizeof(BuffUAVBindInfo)  );
    m_SamplerOffset  = AdvanceOffset(ResCounters.NumSamplers * sizeof(SamplerBindInfo)  );
    m_MemorySize     = AdvanceOffset(0);
    // clang-format on

    VERIFY_EXPR(m_MemorySize == GetRequiredMemorySize(Signature, AllowedVarTypes, NumAllowedTypes, ShaderType));
    TBase::Initialize(Signature, Allocator, m_MemorySize);

    // clang-format off
    VERIFY_EXPR(ResCounters.NumCBs     == GetNumCBs()     );
    VERIFY_EXPR(ResCounters.NumTexSRVs == GetNumTexSRVs() );
    VERIFY_EXPR(ResCounters.NumTexUAVs == GetNumTexUAVs() );
    VERIFY_EXPR(ResCounters.NumBufSRVs == GetNumBufSRVs() );
    VERIFY_EXPR(ResCounters.NumBufUAVs == GetNumBufUAVs() );
    VERIFY_EXPR(ResCounters.NumSamplers== GetNumSamplers());
    // clang-format on

    // Current resource index for every resource type
    Uint32 cb     = 0;
    Uint32 texSrv = 0;
    Uint32 texUav = 0;
    Uint32 bufSrv = 0;
    Uint32 bufUav = 0;
    Uint32 sam    = 0;

    ProcessSignatureResources(
        Signature, AllowedVarTypes, NumAllowedTypes, ShaderType,
        [&](Uint32 Index) //
        {
            const auto& ResDesc = Signature.GetResourceDesc(Index);
            static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update the switch below to handle the new shader resource range");
            switch (ResDesc.ResourceType)
            {
                case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
                    // Initialize current CB in place, increment CB counter
                    new (&GetResource<ConstBuffBindInfo>(cb++)) ConstBuffBindInfo{*this, Index};
                    break;

                case SHADER_RESOURCE_TYPE_TEXTURE_SRV:
                case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT:
                    // Initialize tex SRV in place, increment counter of tex SRVs
                    new (&GetResource<TexSRVBindInfo>(texSrv++)) TexSRVBindInfo{*this, Index};
                    break;

                case SHADER_RESOURCE_TYPE_BUFFER_SRV:
                    // Initialize buff SRV in place, increment counter of buff SRVs
                    new (&GetResource<BuffSRVBindInfo>(bufSrv++)) BuffSRVBindInfo{*this, Index};
                    break;

                case SHADER_RESOURCE_TYPE_TEXTURE_UAV:
                    // Initialize tex UAV in place, increment counter of tex UAVs
                    new (&GetResource<TexUAVBindInfo>(texUav++)) TexUAVBindInfo{*this, Index};
                    break;

                case SHADER_RESOURCE_TYPE_BUFFER_UAV:
                    // Initialize buff UAV in place, increment counter of buff UAVs
                    new (&GetResource<BuffUAVBindInfo>(bufUav++)) BuffUAVBindInfo{*this, Index};
                    break;

                case SHADER_RESOURCE_TYPE_SAMPLER:
                    // Initialize current sampler in place, increment sampler counter
                    new (&GetResource<SamplerBindInfo>(sam++)) SamplerBindInfo{*this, Index};
                    break;

                default:
                    UNEXPECTED("Unsupported resource type.");
            }
        });

    // clang-format off
    VERIFY(cb     == GetNumCBs(),      "Not all CBs are initialized which will cause a crash when dtor is called");
    VERIFY(texSrv == GetNumTexSRVs(),  "Not all Tex SRVs are initialized which will cause a crash when dtor is called");
    VERIFY(texUav == GetNumTexUAVs(),  "Not all Tex UAVs are initialized which will cause a crash when dtor is called");
    VERIFY(bufSrv == GetNumBufSRVs(),  "Not all Buf SRVs are initialized which will cause a crash when dtor is called");
    VERIFY(bufUav == GetNumBufUAVs(),  "Not all Buf UAVs are initialized which will cause a crash when dtor is called");
    VERIFY(sam    == GetNumSamplers(), "Not all samplers are initialized which will cause a crash when dtor is called");
    // clang-format on
}

void ShaderVariableManagerD3D11::ConstBuffBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER);
    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Array index (", BindInfo.ArrayIndex, ") is out of range. This error should've been caught by ShaderVariableBase::SetArray()");

    auto& ResourceCache = m_ParentManager.m_ResourceCache;

    // We cannot use ClassPtrCast<> here as the resource can be of wrong type
    RefCntAutoPtr<BufferD3D11Impl> pBuffD3D11Impl{BindInfo.pObject, IID_BufferD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedCB = ResourceCache.GetResource<D3D11_RESOURCE_RANGE_CBV>(Attr.BindPoints + BindInfo.ArrayIndex);
        VerifyConstantBufferBinding(Desc, BindInfo, pBuffD3D11Impl.RawPtr(), CachedCB.pBuff.RawPtr(),
                                    CachedCB.BaseOffset, CachedCB.RangeSize,
                                    m_ParentManager.m_pSignature->GetDesc().Name);
    }
#endif
    ResourceCache.SetResource<D3D11_RESOURCE_RANGE_CBV>(Attr.BindPoints + BindInfo.ArrayIndex, std::move(pBuffD3D11Impl), BindInfo.BufferBaseOffset, BindInfo.BufferRangeSize);
}

void ShaderVariableManagerD3D11::ConstBuffBindInfo::SetDynamicOffset(Uint32 ArrayIndex, Uint32 Offset)
{
    const auto& Attr = GetAttribs();
    const auto& Desc = GetDesc();
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER);
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedCB = m_ParentManager.m_ResourceCache.GetResource<D3D11_RESOURCE_RANGE_CBV>(Attr.BindPoints + ArrayIndex);
        VerifyDynamicBufferOffset<BufferD3D11Impl, BufferViewD3D11Impl>(Desc, CachedCB.pBuff, CachedCB.BaseOffset, CachedCB.RangeSize, Offset);
    }
#endif
    m_ParentManager.m_ResourceCache.SetDynamicCBOffset(Attr.BindPoints + ArrayIndex, Offset);
}

void ShaderVariableManagerD3D11::TexSRVBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV ||
                Desc.ResourceType == SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT);
    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Array index (", BindInfo.ArrayIndex, ") is out of range. This error should've been caught by ShaderVariableBase::SetArray()");

    auto& ResourceCache = m_ParentManager.m_ResourceCache;

    // We cannot use ClassPtrCast<> here as the resource can be of wrong type
    RefCntAutoPtr<TextureViewD3D11Impl> pViewD3D11{BindInfo.pObject, IID_TextureViewD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        auto& CachedSRV = ResourceCache.GetResource<D3D11_RESOURCE_RANGE_SRV>(Attr.BindPoints + BindInfo.ArrayIndex);
        VerifyResourceViewBinding(Desc, BindInfo, pViewD3D11.RawPtr(), {TEXTURE_VIEW_SHADER_RESOURCE},
                                  RESOURCE_DIM_UNDEFINED, false, CachedSRV.pView.RawPtr(),
                                  m_ParentManager.m_pSignature->GetDesc().Name);
    }
#endif

    if (Attr.IsSamplerAssigned() && !Attr.IsImmutableSamplerAssigned())
    {
        const auto& SampAttr = m_ParentManager.GetResourceAttribs(Attr.SamplerInd);
        const auto& SampDesc = m_ParentManager.GetResourceDesc(Attr.SamplerInd);
        VERIFY_EXPR(SampDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER);
        VERIFY(!SampAttr.IsImmutableSamplerAssigned(),
               "When an immutable sampler is assigned to a texture, the texture's ImtblSamplerAssigned flag must also be set by "
               "PipelineResourceSignatureD3D11Impl::CreateLayout(). This mismatch is a bug.");
        VERIFY_EXPR((Desc.ShaderStages & SampDesc.ShaderStages) == Desc.ShaderStages);
        VERIFY_EXPR(SampDesc.ArraySize == Desc.ArraySize || SampDesc.ArraySize == 1);
        const auto SampArrayIndex = (SampDesc.ArraySize != 1 ? BindInfo.ArrayIndex : 0);

        if (pViewD3D11)
        {
            auto* const pSamplerD3D11Impl = pViewD3D11->GetSampler<SamplerD3D11Impl>();
            if (pSamplerD3D11Impl != nullptr)
            {
#ifdef DILIGENT_DEVELOPMENT
                {
                    const auto& CachedSampler = ResourceCache.GetResource<D3D11_RESOURCE_RANGE_SAMPLER>(SampAttr.BindPoints + SampArrayIndex);
                    VerifySamplerBinding(SampDesc, BindResourceInfo{SampArrayIndex, pSamplerD3D11Impl, BindInfo.Flags}, pSamplerD3D11Impl, CachedSampler.pSampler,
                                         m_ParentManager.m_pSignature->GetDesc().Name);
                }
#endif
                ResourceCache.SetResource<D3D11_RESOURCE_RANGE_SAMPLER>(SampAttr.BindPoints + SampArrayIndex, pSamplerD3D11Impl);
            }
            else
            {
                LOG_ERROR_MESSAGE("Failed to bind sampler to variable '", GetShaderResourcePrintName(SampDesc, BindInfo.ArrayIndex),
                                  "'. Sampler is not set in the texture view '", pViewD3D11->GetDesc().Name, "'");
            }
        }
    }
    ResourceCache.SetResource<D3D11_RESOURCE_RANGE_SRV>(Attr.BindPoints + BindInfo.ArrayIndex, std::move(pViewD3D11));
}

void ShaderVariableManagerD3D11::SamplerBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER);
    VERIFY(!Attr.IsImmutableSamplerAssigned(), "Sampler must not be assigned to an immutable sampler.");
    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Array index (", BindInfo.ArrayIndex,
           ") is out of range. This error should've been caught by ShaderVariableBase::SetArray()");

    auto& ResourceCache = m_ParentManager.m_ResourceCache;

    // We cannot use ClassPtrCast<> here as the resource can be of wrong type
    RefCntAutoPtr<SamplerD3D11Impl> pSamplerD3D11{BindInfo.pObject, IID_SamplerD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedSampler = ResourceCache.GetResource<D3D11_RESOURCE_RANGE_SAMPLER>(Attr.BindPoints + BindInfo.ArrayIndex);
        VerifySamplerBinding(Desc, BindInfo, pSamplerD3D11.RawPtr(), CachedSampler.pSampler, m_ParentManager.m_pSignature->GetDesc().Name);
    }
#endif

    ResourceCache.SetResource<D3D11_RESOURCE_RANGE_SAMPLER>(Attr.BindPoints + BindInfo.ArrayIndex, std::move(pSamplerD3D11));
}

void ShaderVariableManagerD3D11::BuffSRVBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV);
    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Array index (", BindInfo.ArrayIndex,
           ") is out of range. This error should've been caught by ShaderVariableBase::SetArray()");

    auto& ResourceCache = m_ParentManager.m_ResourceCache;

    // We cannot use ClassPtrCast<> here as the resource can be of wrong type
    RefCntAutoPtr<BufferViewD3D11Impl> pViewD3D11{BindInfo.pObject, IID_BufferViewD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedSRV = ResourceCache.GetResource<D3D11_RESOURCE_RANGE_SRV>(Attr.BindPoints + BindInfo.ArrayIndex);
        VerifyResourceViewBinding(Desc, BindInfo, pViewD3D11.RawPtr(), {BUFFER_VIEW_SHADER_RESOURCE},
                                  RESOURCE_DIM_BUFFER, false, CachedSRV.pView.RawPtr(),
                                  m_ParentManager.m_pSignature->GetDesc().Name);
        ValidateBufferMode(Desc, BindInfo.ArrayIndex, pViewD3D11.RawPtr());
    }
#endif
    ResourceCache.SetResource<D3D11_RESOURCE_RANGE_SRV>(Attr.BindPoints + BindInfo.ArrayIndex, std::move(pViewD3D11));
}


void ShaderVariableManagerD3D11::TexUAVBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_UAV);
    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Array index (", BindInfo.ArrayIndex,
           ") is out of range. This error should've been caught by ShaderVariableBase::SetArray()");

    auto& ResourceCache = m_ParentManager.m_ResourceCache;

    // We cannot use ClassPtrCast<> here as the resource can be of wrong type
    RefCntAutoPtr<TextureViewD3D11Impl> pViewD3D11{BindInfo.pObject, IID_TextureViewD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedUAV = ResourceCache.GetResource<D3D11_RESOURCE_RANGE_UAV>(Attr.BindPoints + BindInfo.ArrayIndex);
        VerifyResourceViewBinding(Desc, BindInfo, pViewD3D11.RawPtr(), {TEXTURE_VIEW_UNORDERED_ACCESS},
                                  RESOURCE_DIM_UNDEFINED, false, CachedUAV.pView.RawPtr(),
                                  m_ParentManager.m_pSignature->GetDesc().Name);
    }
#endif
    ResourceCache.SetResource<D3D11_RESOURCE_RANGE_UAV>(Attr.BindPoints + BindInfo.ArrayIndex, std::move(pViewD3D11));
}


void ShaderVariableManagerD3D11::BuffUAVBindInfo::BindResource(const BindResourceInfo& BindInfo)
{
    const auto& Desc = GetDesc();
    const auto& Attr = GetAttribs();
    VERIFY_EXPR(Desc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV);
    VERIFY(BindInfo.ArrayIndex < Desc.ArraySize, "Array index (", BindInfo.ArrayIndex,
           ") is out of range. This error should've been caught by ShaderVariableBase::SetArray()");

    auto& ResourceCache = m_ParentManager.m_ResourceCache;

    // We cannot use ClassPtrCast<> here as the resource can be of wrong type
    RefCntAutoPtr<BufferViewD3D11Impl> pViewD3D11{BindInfo.pObject, IID_BufferViewD3D11};
#ifdef DILIGENT_DEVELOPMENT
    {
        const auto& CachedUAV = ResourceCache.GetResource<D3D11_RESOURCE_RANGE_UAV>(Attr.BindPoints + BindInfo.ArrayIndex);
        VerifyResourceViewBinding(Desc, BindInfo, pViewD3D11.RawPtr(), {BUFFER_VIEW_UNORDERED_ACCESS},
                                  RESOURCE_DIM_BUFFER, false, CachedUAV.pView.RawPtr(),
                                  m_ParentManager.m_pSignature->GetDesc().Name);
        ValidateBufferMode(Desc, BindInfo.ArrayIndex, pViewD3D11.RawPtr());
    }
#endif
    ResourceCache.SetResource<D3D11_RESOURCE_RANGE_UAV>(Attr.BindPoints + BindInfo.ArrayIndex, std::move(pViewD3D11));
}


void ShaderVariableManagerD3D11::CheckResources(IResourceMapping*                    pResourceMapping,
                                                BIND_SHADER_RESOURCES_FLAGS          Flags,
                                                SHADER_RESOURCE_VARIABLE_TYPE_FLAGS& StaleVarTypes) const
{
    if ((Flags & BIND_SHADER_RESOURCES_UPDATE_ALL) == 0)
        Flags |= BIND_SHADER_RESOURCES_UPDATE_ALL;

    const auto AllowedTypes = m_ResourceCache.GetContentType() == ResourceCacheContentType::SRB ?
        SHADER_RESOURCE_VARIABLE_TYPE_FLAG_MUT_DYN :
        SHADER_RESOURCE_VARIABLE_TYPE_FLAG_STATIC;

    HandleConstResources(
        [&](const ConstBuffBindInfo& cb) {
            cb.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        },
        [&](const TexSRVBindInfo& ts) {
            ts.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        },
        [&](const TexUAVBindInfo& uav) {
            uav.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        },
        [&](const BuffSRVBindInfo& srv) {
            srv.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        },
        [&](const BuffUAVBindInfo& uav) {
            uav.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        },
        [&](const SamplerBindInfo& sam) {
            sam.CheckResources(pResourceMapping, Flags, StaleVarTypes);
            return (StaleVarTypes & AllowedTypes) != AllowedTypes;
        });
}

void ShaderVariableManagerD3D11::BindResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags)
{
    if (pResourceMapping == nullptr)
    {
        LOG_ERROR_MESSAGE("Failed to bind resources: resource mapping is null");
        return;
    }

    if ((Flags & BIND_SHADER_RESOURCES_UPDATE_ALL) == 0)
        Flags |= BIND_SHADER_RESOURCES_UPDATE_ALL;

    HandleResources(
        [&](ConstBuffBindInfo& cb) {
            cb.BindResources(pResourceMapping, Flags);
        },
        [&](TexSRVBindInfo& ts) {
            ts.BindResources(pResourceMapping, Flags);
        },
        [&](TexUAVBindInfo& uav) {
            uav.BindResources(pResourceMapping, Flags);
        },
        [&](BuffSRVBindInfo& srv) {
            srv.BindResources(pResourceMapping, Flags);
        },
        [&](BuffUAVBindInfo& uav) {
            uav.BindResources(pResourceMapping, Flags);
        },
        [&](SamplerBindInfo& sam) {
            sam.BindResources(pResourceMapping, Flags);
        });
}

template <typename ResourceType>
IShaderResourceVariable* ShaderVariableManagerD3D11::GetResourceByName(const Char* Name) const
{
    auto NumResources = GetNumResources<ResourceType>();
    for (Uint32 res = 0; res < NumResources; ++res)
    {
        auto& Resource = GetResource<ResourceType>(res);
        if (strcmp(Resource.GetDesc().Name, Name) == 0)
            return &Resource;
    }

    return nullptr;
}

IShaderResourceVariable* ShaderVariableManagerD3D11::GetVariable(const Char* Name) const
{
    if (auto* pCB = GetResourceByName<ConstBuffBindInfo>(Name))
        return pCB;

    if (auto* pTexSRV = GetResourceByName<TexSRVBindInfo>(Name))
        return pTexSRV;

    if (auto* pTexUAV = GetResourceByName<TexUAVBindInfo>(Name))
        return pTexUAV;

    if (auto* pBuffSRV = GetResourceByName<BuffSRVBindInfo>(Name))
        return pBuffSRV;

    if (auto* pBuffUAV = GetResourceByName<BuffUAVBindInfo>(Name))
        return pBuffUAV;

    if (!m_pSignature->IsUsingCombinedSamplers())
    {
        // Immutable samplers are never initialized as variables
        if (auto* pSampler = GetResourceByName<SamplerBindInfo>(Name))
            return pSampler;
    }

    return nullptr;
}

class ShaderVariableIndexLocator
{
public:
    ShaderVariableIndexLocator(const ShaderVariableManagerD3D11& _Mgr, const IShaderResourceVariable& Variable) :
        // clang-format off
        Mgr      {_Mgr},
        VarOffset(reinterpret_cast<const Uint8*>(&Variable) - reinterpret_cast<const Uint8*>(_Mgr.m_pVariables))
    // clang-format on
    {}

    template <typename ResourceType>
    bool TryResource(ShaderVariableManagerD3D11::OffsetType NextResourceTypeOffset)
    {
#ifdef DILIGENT_DEBUG
        {
            VERIFY(Mgr.GetResourceOffset<ResourceType>() >= dbgPreviousResourceOffset, "Resource types are processed out of order!");
            dbgPreviousResourceOffset = Mgr.GetResourceOffset<ResourceType>();
            VERIFY_EXPR(NextResourceTypeOffset >= Mgr.GetResourceOffset<ResourceType>());
        }
#endif
        if (VarOffset < NextResourceTypeOffset)
        {
            auto RelativeOffset = VarOffset - Mgr.GetResourceOffset<ResourceType>();
            DEV_CHECK_ERR(RelativeOffset % sizeof(ResourceType) == 0, "Offset is not multiple of resource type (", sizeof(ResourceType), ")");
            Index += static_cast<Uint32>(RelativeOffset / sizeof(ResourceType));
            return true;
        }
        else
        {
            Index += Mgr.GetNumResources<ResourceType>();
            return false;
        }
    }

    Uint32 GetIndex() const { return Index; }

private:
    const ShaderVariableManagerD3D11& Mgr;
    const size_t                      VarOffset;
    Uint32                            Index = 0;
#ifdef DILIGENT_DEBUG
    Uint32 dbgPreviousResourceOffset = 0;
#endif
};

Uint32 ShaderVariableManagerD3D11::GetVariableIndex(const IShaderResourceVariable& Variable) const
{
    if (m_pVariables == nullptr)
    {
        LOG_ERROR("This shader variable manager does not have any resources");
        return ~0u;
    }

    ShaderVariableIndexLocator IdxLocator(*this, Variable);
    if (IdxLocator.TryResource<ConstBuffBindInfo>(m_TexSRVsOffset))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<TexSRVBindInfo>(m_TexUAVsOffset))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<TexUAVBindInfo>(m_BuffSRVsOffset))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<BuffSRVBindInfo>(m_BuffUAVsOffset))
        return IdxLocator.GetIndex();

    if (IdxLocator.TryResource<BuffUAVBindInfo>(m_SamplerOffset))
        return IdxLocator.GetIndex();

    if (!m_pSignature->IsUsingCombinedSamplers())
    {
        if (IdxLocator.TryResource<SamplerBindInfo>(m_MemorySize))
            return IdxLocator.GetIndex();
    }

    LOG_ERROR("Failed to get variable index. The variable ", &Variable, " does not belong to this shader variable manager");
    return ~0U;
}

class ShaderVariableLocator
{
public:
    ShaderVariableLocator(const ShaderVariableManagerD3D11& _Mgr, Uint32 _Index) :
        // clang-format off
        Mgr   {_Mgr},
        Index {_Index }
    // clang-format on
    {
    }

    template <typename ResourceType>
    IShaderResourceVariable* TryResource()
    {
#ifdef DILIGENT_DEBUG
        {
            VERIFY(Mgr.GetResourceOffset<ResourceType>() >= dbgPreviousResourceOffset, "Resource types are processed out of order!");
            dbgPreviousResourceOffset = Mgr.GetResourceOffset<ResourceType>();
        }
#endif
        auto NumResources = Mgr.GetNumResources<ResourceType>();
        if (Index < NumResources)
            return &Mgr.GetResource<ResourceType>(Index);
        else
        {
            Index -= NumResources;
            return nullptr;
        }
    }

private:
    ShaderVariableManagerD3D11 const& Mgr;
    Uint32                            Index = 0;
#ifdef DILIGENT_DEBUG
    Uint32 dbgPreviousResourceOffset = 0;
#endif
};

IShaderResourceVariable* ShaderVariableManagerD3D11::GetVariable(Uint32 Index) const
{
    ShaderVariableLocator VarLocator(*this, Index);

    if (auto* pCB = VarLocator.TryResource<ConstBuffBindInfo>())
        return pCB;

    if (auto* pTexSRV = VarLocator.TryResource<TexSRVBindInfo>())
        return pTexSRV;

    if (auto* pTexUAV = VarLocator.TryResource<TexUAVBindInfo>())
        return pTexUAV;

    if (auto* pBuffSRV = VarLocator.TryResource<BuffSRVBindInfo>())
        return pBuffSRV;

    if (auto* pBuffUAV = VarLocator.TryResource<BuffUAVBindInfo>())
        return pBuffUAV;

    if (!m_pSignature->IsUsingCombinedSamplers())
    {
        if (auto* pSampler = VarLocator.TryResource<SamplerBindInfo>())
            return pSampler;
    }

    LOG_ERROR(Index, " is not a valid variable index.");
    return nullptr;
}

Uint32 ShaderVariableManagerD3D11::GetVariableCount() const
{
    return GetNumCBs() + GetNumTexSRVs() + GetNumTexUAVs() + GetNumBufSRVs() + GetNumBufUAVs() + GetNumSamplers();
}

} // namespace Diligent
