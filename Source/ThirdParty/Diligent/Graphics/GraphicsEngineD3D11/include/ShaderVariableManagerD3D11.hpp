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

#pragma once

/// \file
/// Declaration of Diligent::ShaderVariableManagerD3D11 class

#include "EngineD3D11ImplTraits.hpp"
#include "ShaderResources.hpp"
#include "ShaderResourceVariableBase.hpp"
#include "ShaderResourceVariableD3D.h"
#include "PipelineResourceAttribsD3D11.hpp"
#include "ShaderResourceCacheD3D11.hpp"

namespace Diligent
{

/// Diligent::ShaderVariableManagerD3D11 class
// sizeof(ShaderVariableManagerD3D11) == 48, (Release, x64)
class ShaderVariableManagerD3D11 : ShaderVariableManagerBase<EngineD3D11ImplTraits, void>
{
public:
    using TBase = ShaderVariableManagerBase<EngineD3D11ImplTraits, void>;
    ShaderVariableManagerD3D11(IObject&                  Owner,
                               ShaderResourceCacheD3D11& ResourceCache) noexcept :
        TBase{Owner, ResourceCache}
    {
    }

    void Destroy(IMemoryAllocator& Allocator);

    // clang-format off
    // No copies, only moves are allowed
    ShaderVariableManagerD3D11             (const ShaderVariableManagerD3D11&)  = delete;
    ShaderVariableManagerD3D11& operator = (const ShaderVariableManagerD3D11&)  = delete;
    ShaderVariableManagerD3D11             (      ShaderVariableManagerD3D11&&) = default;
    ShaderVariableManagerD3D11& operator = (      ShaderVariableManagerD3D11&&) = delete;
    // clang-format on

    void Initialize(const PipelineResourceSignatureD3D11Impl& Signature,
                    IMemoryAllocator&                         Allocator,
                    const SHADER_RESOURCE_VARIABLE_TYPE*      AllowedVarTypes,
                    Uint32                                    NumAllowedTypes,
                    SHADER_TYPE                               ShaderType);

    static size_t GetRequiredMemorySize(const PipelineResourceSignatureD3D11Impl& Signature,
                                        const SHADER_RESOURCE_VARIABLE_TYPE*      AllowedVarTypes,
                                        Uint32                                    NumAllowedTypes,
                                        SHADER_TYPE                               ShaderType);

    using ResourceAttribs = PipelineResourceAttribsD3D11;

    const PipelineResourceDesc& GetResourceDesc(Uint32 Index) const;
    const ResourceAttribs&      GetResourceAttribs(Uint32 Index) const;


    template <typename ThisImplType, D3D11_RESOURCE_RANGE ResRange>
    struct ShaderVariableD3D11Base : ShaderVariableBase<ThisImplType, ShaderVariableManagerD3D11, IShaderResourceVariableD3D>
    {
    public:
        ShaderVariableD3D11Base(ShaderVariableManagerD3D11& ParentLayout, Uint32 ResIndex) :
            ShaderVariableBase<ThisImplType, ShaderVariableManagerD3D11, IShaderResourceVariableD3D>{ParentLayout, ResIndex}
        {}

        // clang-format off
        ShaderVariableD3D11Base            (const ShaderVariableD3D11Base&)  = delete;
        ShaderVariableD3D11Base            (      ShaderVariableD3D11Base&&) = delete;
        ShaderVariableD3D11Base& operator= (const ShaderVariableD3D11Base&)  = delete;
        ShaderVariableD3D11Base& operator= (      ShaderVariableD3D11Base&&) = delete;
        // clang-format on

        const ResourceAttribs& GetAttribs() const { return m_ParentManager.GetResourceAttribs(m_ResIndex); }

        virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final
        {
            if (ppInterface == nullptr)
                return;

            *ppInterface = nullptr;
            if (IID == IID_ShaderResourceVariableD3D || IID == IID_ShaderResourceVariable || IID == IID_Unknown)
            {
                *ppInterface = this;
                (*ppInterface)->AddRef();
            }
        }

        virtual void DILIGENT_CALL_TYPE GetHLSLResourceDesc(HLSLShaderResourceDesc& HLSLResDesc) const override final
        {
            GetResourceDesc(HLSLResDesc);
            HLSLResDesc.ShaderRegister = GetAttribs().BindPoints[m_ParentManager.m_ShaderTypeIndex];
        }

        virtual IDeviceObject* DILIGENT_CALL_TYPE Get(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < GetDesc().ArraySize);
            return m_ParentManager.m_ResourceCache.GetResource<ResRange>(GetAttribs().BindPoints + ArrayIndex).Get();
        }

        void SetDynamicOffset(Uint32 ArrayIndex, Uint32 DynamicOffset)
        {
            UNSUPPORTED("Dynamic offset may only be set for constant buffers.");
        }
    };

    struct ConstBuffBindInfo final : ShaderVariableD3D11Base<ConstBuffBindInfo, D3D11_RESOURCE_RANGE_CBV>
    {
        ConstBuffBindInfo(ShaderVariableManagerD3D11& ParentLayout, Uint32 ResIndex) :
            ShaderVariableD3D11Base<ConstBuffBindInfo, D3D11_RESOURCE_RANGE_CBV>{ParentLayout, ResIndex}
        {}

        __forceinline void BindResource(const BindResourceInfo& BindInfo);

        void SetDynamicOffset(Uint32 ArrayIndex, Uint32 Offset);
    };

    struct TexSRVBindInfo final : ShaderVariableD3D11Base<TexSRVBindInfo, D3D11_RESOURCE_RANGE_SRV>
    {
        TexSRVBindInfo(ShaderVariableManagerD3D11& ParentLayout, Uint32 ResIndex) :
            ShaderVariableD3D11Base<TexSRVBindInfo, D3D11_RESOURCE_RANGE_SRV>{ParentLayout, ResIndex}
        {}

        __forceinline void BindResource(const BindResourceInfo& BindInfo);
    };

    struct TexUAVBindInfo final : ShaderVariableD3D11Base<TexUAVBindInfo, D3D11_RESOURCE_RANGE_UAV>
    {
        TexUAVBindInfo(ShaderVariableManagerD3D11& ParentLayout, Uint32 ResIndex) :
            ShaderVariableD3D11Base<TexUAVBindInfo, D3D11_RESOURCE_RANGE_UAV>{ParentLayout, ResIndex}
        {}

        __forceinline void BindResource(const BindResourceInfo& BindInfo);
    };

    struct BuffUAVBindInfo final : ShaderVariableD3D11Base<BuffUAVBindInfo, D3D11_RESOURCE_RANGE_UAV>
    {
        BuffUAVBindInfo(ShaderVariableManagerD3D11& ParentLayout, Uint32 ResIndex) :
            ShaderVariableD3D11Base<BuffUAVBindInfo, D3D11_RESOURCE_RANGE_UAV>{ParentLayout, ResIndex}
        {}

        __forceinline void BindResource(const BindResourceInfo& BindInfo);
    };

    struct BuffSRVBindInfo final : ShaderVariableD3D11Base<BuffSRVBindInfo, D3D11_RESOURCE_RANGE_SRV>
    {
        BuffSRVBindInfo(ShaderVariableManagerD3D11& ParentLayout, Uint32 ResIndex) :
            ShaderVariableD3D11Base<BuffSRVBindInfo, D3D11_RESOURCE_RANGE_SRV>{ParentLayout, ResIndex}
        {}

        __forceinline void BindResource(const BindResourceInfo& BindInfo);
    };

    struct SamplerBindInfo final : ShaderVariableD3D11Base<SamplerBindInfo, D3D11_RESOURCE_RANGE_SAMPLER>
    {
        SamplerBindInfo(ShaderVariableManagerD3D11& ParentLayout, Uint32 ResIndex) :
            ShaderVariableD3D11Base<SamplerBindInfo, D3D11_RESOURCE_RANGE_SAMPLER>{ParentLayout, ResIndex}
        {}

        __forceinline void BindResource(const BindResourceInfo& BindInfo);
    };

    void BindResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags);

    void CheckResources(IResourceMapping*                    pResourceMapping,
                        BIND_SHADER_RESOURCES_FLAGS          Flags,
                        SHADER_RESOURCE_VARIABLE_TYPE_FLAGS& StaleVarTypes) const;

    IShaderResourceVariable* GetVariable(const Char* Name) const;
    IShaderResourceVariable* GetVariable(Uint32 Index) const;

    IObject& GetOwner() { return m_Owner; }

    Uint32 GetVariableCount() const;

    Uint32 GetVariableIndex(const IShaderResourceVariable& Variable) const;

    // clang-format off
    Uint32 GetNumCBs()      const { return (m_TexSRVsOffset  - 0               ) / sizeof(ConstBuffBindInfo);}
    Uint32 GetNumTexSRVs()  const { return (m_TexUAVsOffset  - m_TexSRVsOffset ) / sizeof(TexSRVBindInfo);   }
    Uint32 GetNumTexUAVs()  const { return (m_BuffSRVsOffset - m_TexUAVsOffset ) / sizeof(TexUAVBindInfo) ;  }
    Uint32 GetNumBufSRVs()  const { return (m_BuffUAVsOffset - m_BuffSRVsOffset) / sizeof(BuffSRVBindInfo);  }
    Uint32 GetNumBufUAVs()  const { return (m_SamplerOffset  - m_BuffUAVsOffset) / sizeof(BuffUAVBindInfo);  }
    Uint32 GetNumSamplers() const { return (m_MemorySize     - m_SamplerOffset ) / sizeof(SamplerBindInfo);  }

    template<typename ResourceType> Uint32 GetNumResources()const;
    template<> Uint32 GetNumResources<ConstBuffBindInfo>() const { return GetNumCBs();      }
    template<> Uint32 GetNumResources<TexSRVBindInfo>   () const { return GetNumTexSRVs();  }
    template<> Uint32 GetNumResources<TexUAVBindInfo>   () const { return GetNumTexUAVs();  }
    template<> Uint32 GetNumResources<BuffSRVBindInfo>  () const { return GetNumBufSRVs();  }
    template<> Uint32 GetNumResources<BuffUAVBindInfo>  () const { return GetNumBufUAVs();  }
    template<> Uint32 GetNumResources<SamplerBindInfo>  () const { return GetNumSamplers(); }
    // clang-format on

private:
    static D3DShaderResourceCounters CountResources(
        const PipelineResourceSignatureD3D11Impl& Signature,
        const SHADER_RESOURCE_VARIABLE_TYPE*      AllowedVarTypes,
        Uint32                                    NumAllowedTypes,
        SHADER_TYPE                               ShaderType);

    // clang-format off
    using OffsetType = Uint16;
    template<typename ResourceType> OffsetType GetResourceOffset()const;
    template<> OffsetType GetResourceOffset<ConstBuffBindInfo>() const { return 0;                }
    template<> OffsetType GetResourceOffset<TexSRVBindInfo>   () const { return m_TexSRVsOffset;  }
    template<> OffsetType GetResourceOffset<TexUAVBindInfo>   () const { return m_TexUAVsOffset;  }
    template<> OffsetType GetResourceOffset<BuffSRVBindInfo>  () const { return m_BuffSRVsOffset; }
    template<> OffsetType GetResourceOffset<BuffUAVBindInfo>  () const { return m_BuffUAVsOffset; }
    template<> OffsetType GetResourceOffset<SamplerBindInfo>  () const { return m_SamplerOffset;  }
    // clang-format on

    template <typename ResourceType>
    ResourceType& GetResource(Uint32 ResIndex) const
    {
        VERIFY(ResIndex < GetNumResources<ResourceType>(), "Resource index (", ResIndex, ") must be less than (", GetNumResources<ResourceType>(), ")");
        auto Offset = GetResourceOffset<ResourceType>();
        return reinterpret_cast<ResourceType*>(reinterpret_cast<Uint8*>(m_pVariables) + Offset)[ResIndex];
    }

    template <typename ResourceType>
    const ResourceType& GetConstResource(Uint32 ResIndex) const
    {
        VERIFY(ResIndex < GetNumResources<ResourceType>(), "Resource index (", ResIndex, ") must be less than (", GetNumResources<ResourceType>(), ")");
        auto Offset = GetResourceOffset<ResourceType>();
        return reinterpret_cast<const ResourceType*>(reinterpret_cast<const Uint8*>(m_pVariables) + Offset)[ResIndex];
    }

    template <typename ResourceType>
    IShaderResourceVariable* GetResourceByName(const Char* Name) const;

    template <typename THandleCB,
              typename THandleTexSRV,
              typename THandleTexUAV,
              typename THandleBufSRV,
              typename THandleBufUAV,
              typename THandleSampler>
    void HandleResources(THandleCB      HandleCB,
                         THandleTexSRV  HandleTexSRV,
                         THandleTexUAV  HandleTexUAV,
                         THandleBufSRV  HandleBufSRV,
                         THandleBufUAV  HandleBufUAV,
                         THandleSampler HandleSampler)
    {
        for (Uint32 cb = 0; cb < GetNumResources<ConstBuffBindInfo>(); ++cb)
            HandleCB(GetResource<ConstBuffBindInfo>(cb));

        for (Uint32 t = 0; t < GetNumResources<TexSRVBindInfo>(); ++t)
            HandleTexSRV(GetResource<TexSRVBindInfo>(t));

        for (Uint32 u = 0; u < GetNumResources<TexUAVBindInfo>(); ++u)
            HandleTexUAV(GetResource<TexUAVBindInfo>(u));

        for (Uint32 s = 0; s < GetNumResources<BuffSRVBindInfo>(); ++s)
            HandleBufSRV(GetResource<BuffSRVBindInfo>(s));

        for (Uint32 u = 0; u < GetNumResources<BuffUAVBindInfo>(); ++u)
            HandleBufUAV(GetResource<BuffUAVBindInfo>(u));

        for (Uint32 s = 0; s < GetNumResources<SamplerBindInfo>(); ++s)
            HandleSampler(GetResource<SamplerBindInfo>(s));
    }

    template <typename THandleCB,
              typename THandleTexSRV,
              typename THandleTexUAV,
              typename THandleBufSRV,
              typename THandleBufUAV,
              typename THandleSampler>
    void HandleConstResources(THandleCB      HandleCB,
                              THandleTexSRV  HandleTexSRV,
                              THandleTexUAV  HandleTexUAV,
                              THandleBufSRV  HandleBufSRV,
                              THandleBufUAV  HandleBufUAV,
                              THandleSampler HandleSampler) const
    {
        for (Uint32 cb = 0; cb < GetNumResources<ConstBuffBindInfo>(); ++cb)
            if (!HandleCB(GetConstResource<ConstBuffBindInfo>(cb)))
                return;

        for (Uint32 t = 0; t < GetNumResources<TexSRVBindInfo>(); ++t)
            if (!HandleTexSRV(GetConstResource<TexSRVBindInfo>(t)))
                return;

        for (Uint32 u = 0; u < GetNumResources<TexUAVBindInfo>(); ++u)
            if (!HandleTexUAV(GetConstResource<TexUAVBindInfo>(u)))
                return;

        for (Uint32 s = 0; s < GetNumResources<BuffSRVBindInfo>(); ++s)
            if (!HandleBufSRV(GetConstResource<BuffSRVBindInfo>(s)))
                return;

        for (Uint32 u = 0; u < GetNumResources<BuffUAVBindInfo>(); ++u)
            if (!HandleBufUAV(GetConstResource<BuffUAVBindInfo>(u)))
                return;

        for (Uint32 s = 0; s < GetNumResources<SamplerBindInfo>(); ++s)
            if (!HandleSampler(GetConstResource<SamplerBindInfo>(s)))
                return;
    }

    friend class ShaderVariableIndexLocator;
    friend class ShaderVariableLocator;

private:
    // Offsets in bytes
    OffsetType m_TexSRVsOffset  = 0;
    OffsetType m_TexUAVsOffset  = 0;
    OffsetType m_BuffSRVsOffset = 0;
    OffsetType m_BuffUAVsOffset = 0;
    OffsetType m_SamplerOffset  = 0;
    OffsetType m_MemorySize     = 0;

    Uint8 m_ShaderTypeIndex = 0;
};

} // namespace Diligent
