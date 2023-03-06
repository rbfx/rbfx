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
/// Implementation of the Diligent::PipelineResourceSignatureBase template class

#include <array>
#include <limits>
#include <algorithm>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_set>

#include "PrivateConstants.h"
#include "PipelineResourceSignature.h"
#include "DeviceObjectBase.hpp"
#include "RenderDeviceBase.hpp"
#include "FixedLinearAllocator.hpp"
#include "BasicMath.hpp"
#include "StringTools.hpp"
#include "PlatformMisc.hpp"
#include "SRBMemoryAllocator.hpp"
#include "ShaderResourceCacheCommon.hpp"
#include "HashUtils.hpp"

#if defined(_MSC_VER) && defined(FindResource)
#    error One of Windows headers leaks FindResource macro, which may result in odd errors. You need to undef the macro.
#endif

namespace Diligent
{

/// Validates pipeline resource signature description and throws an exception in case of an error.
/// \note  pDevice can be null if PRS is used for serialization.
void ValidatePipelineResourceSignatureDesc(const PipelineResourceSignatureDesc& Desc,
                                           const IRenderDevice*                 pDevice,
                                           RENDER_DEVICE_TYPE                   DeviceType) noexcept(false);

static constexpr Uint32 InvalidImmutableSamplerIndex = ~0u;
/// Finds an immutable sampler for the resource name 'ResourceName' that is defined in shader stages 'ShaderStages'.
/// If 'SamplerSuffix' is not null, it will be appended to the 'ResourceName'.
/// Returns an index of the sampler in ImtblSamplers array, or InvalidImmutableSamplerIndex if there is no suitable sampler.
Uint32 FindImmutableSampler(const ImmutableSamplerDesc ImtblSamplers[],
                            Uint32                     NumImtblSamplers,
                            SHADER_TYPE                ShaderStages,
                            const char*                ResourceName,
                            const char*                SamplerSuffix);

static constexpr Uint32 InvalidPipelineResourceIndex = ~0u;
/// Finds a resource with the given name in the specified shader stage and returns its
/// index in Resources[], or InvalidPipelineResourceIndex if the resource is not found.
Uint32 FindResource(const PipelineResourceDesc Resources[],
                    Uint32                     NumResources,
                    SHADER_TYPE                ShaderStage,
                    const char*                ResourceName);

/// Returns true if two pipeline resource signature descriptions are compatible, and false otherwise
bool PipelineResourceSignaturesCompatible(const PipelineResourceSignatureDesc& Desc0,
                                          const PipelineResourceSignatureDesc& Desc1,
                                          bool                                 IgnoreSamplerDescriptions = false) noexcept;

/// Calculates hash of the pipeline resource signature description.
size_t CalculatePipelineResourceSignatureDescHash(const PipelineResourceSignatureDesc& Desc) noexcept;


/// Reserves space for pipeline resource signature description in the Allocator
void ReserveSpaceForPipelineResourceSignatureDesc(FixedLinearAllocator&                Allocator,
                                                  const PipelineResourceSignatureDesc& Desc);

/// Copies pipeline resource signature description using the allocator
void CopyPipelineResourceSignatureDesc(FixedLinearAllocator&                                            Allocator,
                                       const PipelineResourceSignatureDesc&                             SrcDesc,
                                       PipelineResourceSignatureDesc&                                   DstDesc,
                                       std::array<Uint16, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES + 1>& ResourceOffsets);

/// Pipeline resource signature internal data required for serialization/deserialization.
struct PipelineResourceSignatureInternalData
{
    SHADER_TYPE                               ShaderStages          = SHADER_TYPE_UNKNOWN;
    SHADER_TYPE                               StaticResShaderStages = SHADER_TYPE_UNKNOWN;
    PIPELINE_TYPE                             PipelineType          = PIPELINE_TYPE_INVALID;
    std::array<Int8, MAX_SHADERS_IN_PIPELINE> StaticResStageIndex   = {};

    Uint8 _Padding = 0;

    constexpr bool operator==(const PipelineResourceSignatureInternalData& Rhs) const
    {
        // clang-format off
        return ShaderStages          == Rhs.ShaderStages          &&
               StaticResShaderStages == Rhs.StaticResShaderStages &&
               PipelineType          == Rhs.PipelineType          &&
               StaticResStageIndex   == Rhs.StaticResStageIndex;
        // clang-format on
    }
};
ASSERT_SIZEOF(PipelineResourceSignatureInternalData, 16, "The struct is used in serialization and must be tightly packed");


/// Helper class that wraps the pipeline resource signature description.
class PipelineResourceSignatureDescWrapper
{
public:
    PipelineResourceSignatureDescWrapper() = default;

    // Note: any copy or move requires updating string pointers
    PipelineResourceSignatureDescWrapper(const PipelineResourceSignatureDescWrapper&) = delete;
    PipelineResourceSignatureDescWrapper& operator=(const PipelineResourceSignatureDescWrapper&) = delete;
    PipelineResourceSignatureDescWrapper& operator=(PipelineResourceSignatureDescWrapper&&) = delete;

    PipelineResourceSignatureDescWrapper(const char*                       PSOName,
                                         const PipelineResourceLayoutDesc& ResourceLayout,
                                         Uint32                            SRBAllocationGranularity)
    {
        if (PSOName != nullptr)
        {
            m_Name = "Implicit signature of PSO '";
            m_Name += PSOName;
            m_Name += '\'';
            m_Desc.Name = m_Name.c_str();
        }

        m_ImmutableSamplers.reserve(ResourceLayout.NumImmutableSamplers);
        for (Uint32 i = 0; i < ResourceLayout.NumImmutableSamplers; ++i)
            AddImmutableSampler(ResourceLayout.ImmutableSamplers[i]);

        m_Desc.SRBAllocationGranularity = SRBAllocationGranularity;
    }

    explicit PipelineResourceSignatureDescWrapper(const PipelineResourceSignatureDesc& Desc) :
        m_Name{Desc.Name != nullptr ? Desc.Name : ""},
        m_CombinedSamplerSuffix{Desc.CombinedSamplerSuffix != nullptr ? Desc.CombinedSamplerSuffix : ""},
        m_Desc{Desc}
    {
        m_Desc.Name = m_Name.c_str();
        if (Desc.CombinedSamplerSuffix != nullptr)
            m_Desc.CombinedSamplerSuffix = m_CombinedSamplerSuffix.c_str();

        m_Resources.reserve(Desc.NumResources);
        for (Uint32 i = 0; i < Desc.NumResources; ++i)
            AddResource(Desc.Resources[i]);

        m_ImmutableSamplers.reserve(Desc.NumImmutableSamplers);
        for (Uint32 i = 0; i < Desc.NumImmutableSamplers; ++i)
            AddImmutableSampler(Desc.ImmutableSamplers[i]);
    }

    // We need to update string pointers as they may change
    PipelineResourceSignatureDescWrapper(PipelineResourceSignatureDescWrapper&& rhs) noexcept :
        // clang-format off
        m_Name                 {std::move(rhs.m_Name)},
        m_CombinedSamplerSuffix{std::move(rhs.m_CombinedSamplerSuffix)},
        m_Resources            {std::move(rhs.m_Resources)},
        m_ImmutableSamplers    {std::move(rhs.m_ImmutableSamplers)},
        m_StringPool           {std::move(rhs.m_StringPool)},
        m_Desc                 {rhs.m_Desc}
    // clang-format on
    {
        static_assert(sizeof(*this) == sizeof(m_Name) + sizeof(m_CombinedSamplerSuffix) + sizeof(m_Resources) + sizeof(m_ImmutableSamplers) + sizeof(m_StringPool) + sizeof(m_Desc), "Please update move ctor");
        UpdatePointers();
    }

    template <typename... ArgsType>
    void AddResource(ArgsType&&... Args)
    {
        m_Resources.emplace_back(std::forward<ArgsType>(Args)...);
        m_Desc.NumResources = StaticCast<Uint32>(m_Resources.size());
        m_Desc.Resources    = m_Resources.data();

        auto& Res{m_Resources.back()};
        Res.Name = m_StringPool.insert(Res.Name).first->c_str();
    }

    template <typename... ArgsType>
    void AddImmutableSampler(ArgsType&&... Args)
    {
        m_ImmutableSamplers.emplace_back(std::forward<ArgsType>(Args)...);
        m_Desc.NumImmutableSamplers = StaticCast<Uint32>(m_ImmutableSamplers.size());
        m_Desc.ImmutableSamplers    = m_ImmutableSamplers.data();

        auto& ImtblSam{m_ImmutableSamplers.back()};
        ImtblSam.SamplerOrTextureName = m_StringPool.insert(ImtblSam.SamplerOrTextureName).first->c_str();
    }

    template <class HandlerType>
    void ProcessImmutableSamplers(HandlerType&& Handler)
    {
        for (auto& ImtblSam : m_ImmutableSamplers)
        {
            const char* OrigName = ImtblSam.SamplerOrTextureName;
            Handler(ImtblSam);
            if (ImtblSam.SamplerOrTextureName != OrigName) // Compare pointers, not strings!
                ImtblSam.SamplerOrTextureName = m_StringPool.insert(ImtblSam.SamplerOrTextureName).first->c_str();
        }
    }

    template <class HandlerType>
    void ProcessResources(HandlerType&& Handler)
    {
        for (auto& Res : m_Resources)
        {
            const char* OrigName = Res.Name;
            Handler(Res);
            if (Res.Name != OrigName) // Compare pointers, not strings!
                Res.Name = m_StringPool.insert(Res.Name).first->c_str();
        }
    }

    void SetCombinedSamplerSuffix(const char* Suffix)
    {
        VERIFY_EXPR(Suffix != nullptr);
        if (m_Desc.UseCombinedTextureSamplers)
        {
            if (strcmp(m_Desc.CombinedSamplerSuffix, Suffix) != 0)
            {
                LOG_ERROR_AND_THROW("New Combined Sampler Suffix (", Suffix, ") does not match the current suffix (", m_Desc.CombinedSamplerSuffix,
                                    "). This indicates that the pipeline state uses shaders with different sampler suffixes, which is not allowed.");
            }
        }
        else
        {
            m_CombinedSamplerSuffix           = Suffix;
            m_Desc.CombinedSamplerSuffix      = m_CombinedSamplerSuffix.c_str();
            m_Desc.UseCombinedTextureSamplers = true;
        }
    }

    void SetName(const char* Name)
    {
        m_Name      = Name;
        m_Desc.Name = m_Name.c_str();
    }

    const PipelineResourceSignatureDesc& Get() const
    {
        return m_Desc;
    }
    operator const PipelineResourceSignatureDesc&() const
    {
        return Get();
    }

private:
    void UpdatePointers()
    {
        m_Desc.Name                  = m_Name.c_str();
        m_Desc.CombinedSamplerSuffix = m_CombinedSamplerSuffix.c_str();

        m_Desc.NumResources = StaticCast<Uint32>(m_Resources.size());
        m_Desc.Resources    = m_Resources.data();

        m_Desc.NumImmutableSamplers = StaticCast<Uint32>(m_ImmutableSamplers.size());
        m_Desc.ImmutableSamplers    = m_ImmutableSamplers.data();
    }

private:
    String m_Name;
    String m_CombinedSamplerSuffix;

    std::vector<PipelineResourceDesc> m_Resources;
    std::vector<ImmutableSamplerDesc> m_ImmutableSamplers;
    std::unordered_set<String>        m_StringPool;

    PipelineResourceSignatureDesc m_Desc;

    // NB: when adding new members, don't forget to update move ctor!
};

/// Template class implementing base functionality of the pipeline resource signature object.

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class PipelineResourceSignatureBase : public DeviceObjectBase<typename EngineImplTraits::PipelineResourceSignatureInterface, typename EngineImplTraits::RenderDeviceImplType, PipelineResourceSignatureDesc>
{
public:
    // Base interface this class inherits (IPipelineResourceSignatureD3D12, PipelineResourceSignatureVk, etc.)
    using BaseInterface = typename EngineImplTraits::PipelineResourceSignatureInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    // Shader resource cache implementation type (ShaderResourceCacheD3D12, ShaderResourceCacheVk, etc.)
    using ShaderResourceCacheImplType = typename EngineImplTraits::ShaderResourceCacheImplType;

    // Shader variable manager implementation type (ShaderVariableManagerD3D12, ShaderVariableManagerVk, etc.)
    using ShaderVariableManagerImplType = typename EngineImplTraits::ShaderVariableManagerImplType;

    // Shader resource binding implementation type (ShaderResourceBindingD3D12Impl, ShaderResourceBindingVkImpl, etc.)
    using ShaderResourceBindingImplType = typename EngineImplTraits::ShaderResourceBindingImplType;

    // Pipeline resource signature implementation type (PipelineResourceSignatureD3D12Impl, PipelineResourceSignatureVkImpl, etc.)
    using PipelineResourceSignatureImplType = typename EngineImplTraits::PipelineResourceSignatureImplType;

    // Pipeline resource attribs type (PipelineResourceAttribsD3D12, PipelineResourceAttribsVk, etc.)
    using PipelineResourceAttribsType = typename EngineImplTraits::PipelineResourceAttribsType;

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, PipelineResourceSignatureDesc>;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this resource signature.
    /// \param pDevice           - Pointer to the device.
    /// \param Desc              - Resource signature description.
    /// \param ShaderStages      - Active shader stages. This parameter is only provided for default resource signatures
    ///                            created by PSOs and matches the active shader stages in the PSO.
    /// \param bIsDeviceInternal - Flag indicating if this resource signature is an internal device object and
    ///                            must not keep a strong reference to the device.
    PipelineResourceSignatureBase(IReferenceCounters*                  pRefCounters,
                                  RenderDeviceImplType*                pDevice,
                                  const PipelineResourceSignatureDesc& Desc,
                                  SHADER_TYPE                          ShaderStages      = SHADER_TYPE_UNKNOWN,
                                  bool                                 bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal},
        m_ShaderStages{ShaderStages},
        m_SRBMemAllocator{GetRawAllocator()}
    {
        // Don't read from m_Desc until it was allocated and copied in CopyPipelineResourceSignatureDesc()
        this->m_Desc.Resources             = nullptr;
        this->m_Desc.ImmutableSamplers     = nullptr;
        this->m_Desc.CombinedSamplerSuffix = nullptr;

        ValidatePipelineResourceSignatureDesc(Desc, pDevice, EngineImplTraits::DeviceType);

        // Determine shader stages that have any resources as well as
        // shader stages that have static resources.
        for (Uint32 i = 0; i < Desc.NumResources; ++i)
        {
            const auto& ResDesc = Desc.Resources[i];

            m_ShaderStages |= ResDesc.ShaderStages;
            if (ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
                m_StaticResShaderStages |= ResDesc.ShaderStages;
        }

        if (m_ShaderStages != SHADER_TYPE_UNKNOWN)
        {
            m_PipelineType = PipelineTypeFromShaderStages(m_ShaderStages);
            DEV_CHECK_ERR(m_PipelineType != PIPELINE_TYPE_INVALID, "Failed to deduce pipeline type from shader stages");
        }

        {
            Uint32 StaticVarStageIdx = 0;
            for (auto StaticResStages = m_StaticResShaderStages; StaticResStages != SHADER_TYPE_UNKNOWN; ++StaticVarStageIdx)
            {
                const auto StageBit                  = ExtractLSB(StaticResStages);
                const auto ShaderTypeInd             = GetShaderTypePipelineIndex(StageBit, m_PipelineType);
                m_StaticResStageIndex[ShaderTypeInd] = static_cast<Int8>(StaticVarStageIdx);
            }
            VERIFY_EXPR(StaticVarStageIdx == GetNumStaticResStages());
        }
    }

    PipelineResourceSignatureBase(IReferenceCounters*                          pRefCounters,
                                  RenderDeviceImplType*                        pDevice,
                                  const PipelineResourceSignatureDesc&         Desc,
                                  const PipelineResourceSignatureInternalData& InternalData) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, false /*bIsDeviceInternal*/},
        // clang-format off
        m_ShaderStages         {InternalData.ShaderStages},
        m_StaticResShaderStages{InternalData.StaticResShaderStages},
        m_PipelineType         {InternalData.PipelineType},
        m_StaticResStageIndex  {InternalData.StaticResStageIndex},
        m_SRBMemAllocator      {GetRawAllocator()}
    // clang-format on
    {
        // Don't read from m_Desc until it was allocated and copied in CopyPipelineResourceSignatureDesc()
        this->m_Desc.Resources             = nullptr;
        this->m_Desc.ImmutableSamplers     = nullptr;
        this->m_Desc.CombinedSamplerSuffix = nullptr;

#ifdef DILIGENT_DEVELOPMENT
        ValidatePipelineResourceSignatureDesc(Desc, pDevice, EngineImplTraits::DeviceType);
#endif
    }

    ~PipelineResourceSignatureBase()
    {
        VERIFY(m_IsDestructed, "This object must be explicitly destructed with Destruct()");
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_PipelineResourceSignature, TDeviceObjectBase)

    /// Implementation of IPipelineResourceSignature::GetStaticVariableCount.
    virtual Uint32 DILIGENT_CALL_TYPE GetStaticVariableCount(SHADER_TYPE ShaderType) const override final
    {
        if (!IsConsistentShaderType(ShaderType, m_PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to get the number of static variables in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(m_PipelineType), " pipeline resource signature '", this->m_Desc.Name, "'.");
            return 0;
        }

        const auto ShaderTypeInd = GetShaderTypePipelineIndex(ShaderType, m_PipelineType);
        const auto VarMngrInd    = m_StaticResStageIndex[ShaderTypeInd];
        if (VarMngrInd < 0)
            return 0;

        VERIFY_EXPR(static_cast<Uint32>(VarMngrInd) < GetNumStaticResStages());
        return m_StaticVarsMgrs[VarMngrInd].GetVariableCount();
    }

    /// Implementation of IPipelineResourceSignature::GetStaticVariableByName.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByName(SHADER_TYPE ShaderType,
                                                                                const Char* Name) override final
    {
        if (!IsConsistentShaderType(ShaderType, m_PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to find static variable '", Name, "' in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(m_PipelineType), " pipeline resource signature '", this->m_Desc.Name, "'.");
            return nullptr;
        }

        const auto ShaderTypeInd = GetShaderTypePipelineIndex(ShaderType, m_PipelineType);
        const auto VarMngrInd    = m_StaticResStageIndex[ShaderTypeInd];
        if (VarMngrInd < 0)
            return nullptr;

        VERIFY_EXPR(static_cast<Uint32>(VarMngrInd) < GetNumStaticResStages());
        return m_StaticVarsMgrs[VarMngrInd].GetVariable(Name);
    }

    /// Implementation of IPipelineResourceSignature::GetStaticVariableByIndex.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByIndex(SHADER_TYPE ShaderType,
                                                                                 Uint32      Index) override final
    {
        if (!IsConsistentShaderType(ShaderType, m_PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to get static variable at index ", Index, " in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(m_PipelineType), " pipeline resource signature '", this->m_Desc.Name, "'.");
            return nullptr;
        }

        const auto ShaderTypeInd = GetShaderTypePipelineIndex(ShaderType, m_PipelineType);
        const auto VarMngrInd    = m_StaticResStageIndex[ShaderTypeInd];
        if (VarMngrInd < 0)
            return nullptr;

        VERIFY_EXPR(static_cast<Uint32>(VarMngrInd) < GetNumStaticResStages());
        return m_StaticVarsMgrs[VarMngrInd].GetVariable(Index);
    }

    /// Implementation of IPipelineResourceSignature::BindStaticResources.
    virtual void DILIGENT_CALL_TYPE BindStaticResources(SHADER_TYPE                 ShaderStages,
                                                        IResourceMapping*           pResourceMapping,
                                                        BIND_SHADER_RESOURCES_FLAGS Flags) override final
    {
        const auto PipelineType = GetPipelineType();
        for (Uint32 ShaderInd = 0; ShaderInd < m_StaticResStageIndex.size(); ++ShaderInd)
        {
            const auto VarMngrInd = m_StaticResStageIndex[ShaderInd];
            if (VarMngrInd >= 0)
            {
                VERIFY_EXPR(static_cast<Uint32>(VarMngrInd) < GetNumStaticResStages());
                // ShaderInd is the shader type pipeline index here
                const auto ShaderType = GetShaderTypeFromPipelineIndex(ShaderInd, PipelineType);
                if (ShaderStages & ShaderType)
                {
                    m_StaticVarsMgrs[VarMngrInd].BindResources(pResourceMapping, Flags);
                }
            }
        }
    }

    /// Implementation of IPipelineResourceSignature::CreateShaderResourceBinding.
    virtual void DILIGENT_CALL_TYPE CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding,
                                                                bool                     InitStaticResources) override final
    {
        auto* pThisImpl{static_cast<PipelineResourceSignatureImplType*>(this)};
        auto& SRBAllocator{pThisImpl->GetDevice()->GetSRBAllocator()};
        auto* pResBindingImpl{NEW_RC_OBJ(SRBAllocator, "ShaderResourceBinding instance", ShaderResourceBindingImplType)(pThisImpl)};
        if (InitStaticResources)
            pThisImpl->InitializeStaticSRBResources(pResBindingImpl);
        pResBindingImpl->QueryInterface(IID_ShaderResourceBinding, reinterpret_cast<IObject**>(ppShaderResourceBinding));
    }

    /// Implementation of IPipelineResourceSignature::InitializeStaticSRBResources.
    virtual void DILIGENT_CALL_TYPE InitializeStaticSRBResources(IShaderResourceBinding* pSRB) const override final
    {
        DEV_CHECK_ERR(pSRB != nullptr, "SRB must not be null");

        auto* const pSRBImpl = ClassPtrCast<ShaderResourceBindingImplType>(pSRB);
        if (pSRBImpl->StaticResourcesInitialized())
        {
            LOG_WARNING_MESSAGE("Static resources have already been initialized in this shader resource binding object.");
            return;
        }

        const auto* const pThisImpl = static_cast<const PipelineResourceSignatureImplType*>(this);
#ifdef DILIGENT_DEVELOPMENT
        {
            const auto* pSRBSignature = pSRBImpl->GetPipelineResourceSignature();
            DEV_CHECK_ERR(pSRBSignature->IsCompatibleWith(pThisImpl), "Shader resource binding is not compatible with resource signature '", pThisImpl->m_Desc.Name, "'.");
        }
#endif

        auto& ResourceCache = pSRBImpl->GetResourceCache();
        pThisImpl->CopyStaticResources(ResourceCache);

        pSRBImpl->SetStaticResourcesInitialized();
    }

    /// Implementation of IPipelineResourceSignature::CopyStaticResources.
    virtual void DILIGENT_CALL_TYPE CopyStaticResources(IPipelineResourceSignature* pDstSignature) const override final
    {
        if (pDstSignature == nullptr)
        {
            DEV_ERROR("Destination signature must not be null");
            return;
        }

        if (pDstSignature == this)
        {
            DEV_ERROR("Source and destination signatures must be different");
            return;
        }

        const auto* const pThisImpl    = static_cast<const PipelineResourceSignatureImplType*>(this);
        auto* const       pDstSignImpl = static_cast<const PipelineResourceSignatureImplType*>(pDstSignature);
        if (!pDstSignImpl->IsCompatibleWith(pThisImpl))
        {
            LOG_ERROR_MESSAGE("Can't copy static resources: destination pipeline resource signature '", pDstSignImpl->m_Desc.Name,
                              "' is not compatible with the source signature '", pThisImpl->m_Desc.Name, "'.");
            return;
        }

        pThisImpl->CopyStaticResources(*pDstSignImpl->m_pStaticResCache);
    }

    /// Implementation of IPipelineResourceSignature::IsCompatibleWith.
    virtual bool DILIGENT_CALL_TYPE IsCompatibleWith(const IPipelineResourceSignature* pPRS) const override final
    {
        if (pPRS == nullptr)
            return IsEmpty();

        if (this == pPRS)
            return true;

        const auto& This  = *static_cast<const PipelineResourceSignatureImplType*>(this);
        const auto& Other = *ClassPtrCast<const PipelineResourceSignatureImplType>(pPRS);

        if (This.GetHash() != Other.GetHash())
            return false;

        if (!PipelineResourceSignaturesCompatible(This.GetDesc(), Other.GetDesc()))
            return false;

        const auto ResCount = This.GetTotalResourceCount();
        VERIFY_EXPR(ResCount == Other.GetTotalResourceCount());
        for (Uint32 r = 0; r < ResCount; ++r)
        {
            const auto& Res      = This.GetResourceAttribs(r);
            const auto& OtherRes = Other.GetResourceAttribs(r);
            if (!Res.IsCompatibleWith(OtherRes))
                return false;
        }

        return true;
    }

    bool IsIncompatibleWith(const PipelineResourceSignatureImplType& Other) const
    {
        return GetHash() != Other.GetHash();
    }

    size_t GetHash() const { return m_Hash; }

    PIPELINE_TYPE GetPipelineType() const { return m_PipelineType; }

    const char* GetCombinedSamplerSuffix() const { return this->m_Desc.CombinedSamplerSuffix; }

    bool IsUsingCombinedSamplers() const { return this->m_Desc.CombinedSamplerSuffix != nullptr; }
    bool IsUsingSeparateSamplers() const { return !IsUsingCombinedSamplers(); }

    Uint32 GetTotalResourceCount() const { return this->m_Desc.NumResources; }
    Uint32 GetImmutableSamplerCount() const { return this->m_Desc.NumImmutableSamplers; }

    std::pair<Uint32, Uint32> GetResourceIndexRange(SHADER_RESOURCE_VARIABLE_TYPE VarType) const
    {
        return std::pair<Uint32, Uint32>{m_ResourceOffsets[VarType], m_ResourceOffsets[size_t{VarType} + 1]};
    }

    // Returns the number of shader stages that have resources.
    Uint32 GetNumActiveShaderStages() const
    {
        return PlatformMisc::CountOneBits(Uint32{m_ShaderStages});
    }

    // Returns the number of shader stages that have static resources.
    Uint32 GetNumStaticResStages() const
    {
        return PlatformMisc::CountOneBits(Uint32{m_StaticResShaderStages});
    }

    // Returns the type of the active shader stage with the given index.
    SHADER_TYPE GetActiveShaderStageType(Uint32 StageIndex) const
    {
        VERIFY_EXPR(StageIndex < GetNumActiveShaderStages());

        SHADER_TYPE Stages = m_ShaderStages;
        for (Uint32 Index = 0; Stages != SHADER_TYPE_UNKNOWN; ++Index)
        {
            auto StageBit = ExtractLSB(Stages);

            if (Index == StageIndex)
                return StageBit;
        }

        UNEXPECTED("Index is out of range");
        return SHADER_TYPE_UNKNOWN;
    }

    /// Finds a resource with the given name in the specified shader stage and returns its
    /// index in m_Desc.Resources[], or InvalidPipelineResourceIndex if the resource is not found.
    Uint32 FindResource(SHADER_TYPE ShaderStage, const char* ResourceName) const
    {
        return Diligent::FindResource(this->m_Desc.Resources, this->m_Desc.NumResources, ShaderStage, ResourceName);
    }

    /// Finds an immutable with the given name in the specified shader stage and returns its
    /// index in m_Desc.ImmutableSamplers[], or InvalidImmutableSamplerIndex if the sampler is not found.
    Uint32 FindImmutableSampler(SHADER_TYPE ShaderStage, const char* ResourceName) const
    {
        return Diligent::FindImmutableSampler(this->m_Desc.ImmutableSamplers, this->m_Desc.NumImmutableSamplers,
                                              ShaderStage, ResourceName, GetCombinedSamplerSuffix());
    }

    const PipelineResourceDesc& GetResourceDesc(Uint32 ResIndex) const
    {
        VERIFY_EXPR(ResIndex < this->m_Desc.NumResources);
        return this->m_Desc.Resources[ResIndex];
    }

    const ImmutableSamplerDesc& GetImmutableSamplerDesc(Uint32 SampIndex) const
    {
        VERIFY_EXPR(SampIndex < this->m_Desc.NumImmutableSamplers);
        return this->m_Desc.ImmutableSamplers[SampIndex];
    }

    const PipelineResourceAttribsType& GetResourceAttribs(Uint32 ResIndex) const
    {
        VERIFY_EXPR(ResIndex < this->m_Desc.NumResources);
        return m_pResourceAttribs[ResIndex];
    }

    static bool SignaturesCompatible(const PipelineResourceSignatureImplType* pSign0,
                                     const PipelineResourceSignatureImplType* pSign1)
    {
        if (pSign0 == pSign1)
            return true;

        bool IsNull0 = pSign0 == nullptr || pSign0->IsEmpty();
        bool IsNull1 = pSign1 == nullptr || pSign1->IsEmpty();
        if (IsNull0 && IsNull1)
            return true;

        if (IsNull0 != IsNull1)
            return false;

        VERIFY_EXPR(pSign0 != nullptr && pSign1 != nullptr);
        return pSign0->IsCompatibleWith(pSign1);
    }

    SRBMemoryAllocator& GetSRBMemoryAllocator()
    {
        return m_SRBMemAllocator;
    }

    // Processes resources with the allowed variable types in the allowed shader stages
    // and calls user-provided handler for each resource.
    template <typename HandlerType>
    void ProcessResources(const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes,
                          Uint32                               NumAllowedTypes,
                          SHADER_TYPE                          AllowedStages,
                          HandlerType&&                        Handler) const
    {
        if (AllowedVarTypes == nullptr)
            NumAllowedTypes = 1;

        for (Uint32 TypeIdx = 0; TypeIdx < NumAllowedTypes; ++TypeIdx)
        {
            const auto IdxRange = AllowedVarTypes != nullptr ?
                GetResourceIndexRange(AllowedVarTypes[TypeIdx]) :
                std::make_pair<Uint32, Uint32>(0, GetTotalResourceCount());
            for (Uint32 ResIdx = IdxRange.first; ResIdx < IdxRange.second; ++ResIdx)
            {
                const auto& ResDesc = GetResourceDesc(ResIdx);
                VERIFY_EXPR(AllowedVarTypes == nullptr || ResDesc.VarType == AllowedVarTypes[TypeIdx]);

                if ((ResDesc.ShaderStages & AllowedStages) != 0)
                {
                    Handler(ResDesc, ResIdx);
                }
            }
        }
    }

    bool IsEmpty() const
    {
        return GetTotalResourceCount() == 0 && GetImmutableSamplerCount() == 0;
    }

protected:
    using AllocResourceAttribsCallbackType         = std::function<PipelineResourceAttribsType*(FixedLinearAllocator&)>;
    using AllocImmutableSamplerAttribsCallbackType = std::function<void*(FixedLinearAllocator&)>;

    template <typename ImmutableSamplerAttribsType>
    void Initialize(IMemoryAllocator&                               RawAllocator,
                    const PipelineResourceSignatureDesc&            Desc,
                    ImmutableSamplerAttribsType*&                   ImmutableSamAttribs,
                    const std::function<void()>&                    InitResourceLayout,
                    const std::function<size_t()>&                  GetRequiredResourceCacheMemorySize,
                    const AllocResourceAttribsCallbackType&         AllocResourceAttribs  = nullptr,
                    const AllocImmutableSamplerAttribsCallbackType& AllocImmutableSampler = nullptr) noexcept(false)
    {
        FixedLinearAllocator Allocator{RawAllocator};

        ReserveSpaceForPipelineResourceSignatureDesc(Allocator, Desc);

        Allocator.AddSpace<PipelineResourceAttribsType>(Desc.NumResources);

        const auto NumStaticResStages = GetNumStaticResStages();
        if (NumStaticResStages > 0)
        {
            Allocator.AddSpace<ShaderResourceCacheImplType>(1);
            Allocator.AddSpace<ShaderVariableManagerImplType>(NumStaticResStages);
        }

        Allocator.AddSpace<ImmutableSamplerAttribsType>(Desc.NumImmutableSamplers);

        Allocator.Reserve();
        // The memory is now owned by PipelineResourceSignatureBase and will be freed by Destruct().
        m_pRawMemory = decltype(m_pRawMemory){Allocator.ReleaseOwnership(), STDDeleterRawMem<void>{RawAllocator}};

        CopyPipelineResourceSignatureDesc(Allocator, Desc, this->m_Desc, m_ResourceOffsets);

#ifdef DILIGENT_DEBUG
        VERIFY_EXPR(m_ResourceOffsets[SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES] == this->m_Desc.NumResources);
        for (Uint32 VarType = 0; VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES; ++VarType)
        {
            auto IdxRange = GetResourceIndexRange(static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType));
            for (Uint32 idx = IdxRange.first; idx < IdxRange.second; ++idx)
                VERIFY(this->m_Desc.Resources[idx].VarType == VarType, "Unexpected resource var type");
        }
#endif

        // Objects will be constructed by the specific implementation
        static_assert(std::is_trivially_destructible<PipelineResourceAttribsType>::value,
                      "PipelineResourceAttribsType objects must be constructed to be properly destructed in case an exception is thrown");
        m_pResourceAttribs = AllocResourceAttribs ?
            AllocResourceAttribs(Allocator) :
            Allocator.Allocate<PipelineResourceAttribsType>(Desc.NumResources);

        if (NumStaticResStages > 0)
        {
            m_pStaticResCache = Allocator.Construct<ShaderResourceCacheImplType>(ResourceCacheContentType::Signature);

            static_assert(std::is_nothrow_constructible<ShaderVariableManagerImplType, decltype(*this), ShaderResourceCacheImplType&>::value,
                          "Constructor of ShaderVariableManagerImplType must be noexcept, so we can safely construct all manager objects");
            m_StaticVarsMgrs = Allocator.ConstructArray<ShaderVariableManagerImplType>(NumStaticResStages, std::ref(*this), std::ref(*m_pStaticResCache));
        }

        ImmutableSamAttribs = AllocImmutableSampler ?
            static_cast<ImmutableSamplerAttribsType*>(AllocImmutableSampler(Allocator)) :
            Allocator.ConstructArray<ImmutableSamplerAttribsType>(Desc.NumImmutableSamplers);

        InitResourceLayout();

        auto* const pThisImpl = static_cast<PipelineResourceSignatureImplType*>(this);

        if (NumStaticResStages > 0)
        {
            constexpr SHADER_RESOURCE_VARIABLE_TYPE AllowedVarTypes[] = {SHADER_RESOURCE_VARIABLE_TYPE_STATIC};
            for (Uint32 i = 0; i < m_StaticResStageIndex.size(); ++i)
            {
                Int8 Idx = m_StaticResStageIndex[i];
                if (Idx >= 0)
                {
                    VERIFY_EXPR(static_cast<Uint32>(Idx) < NumStaticResStages);
                    const auto ShaderType = GetShaderTypeFromPipelineIndex(i, GetPipelineType());
                    m_StaticVarsMgrs[Idx].Initialize(*pThisImpl, RawAllocator, AllowedVarTypes, _countof(AllowedVarTypes), ShaderType);
                }
            }
        }

        if (Desc.SRBAllocationGranularity > 1)
        {
            std::array<size_t, MAX_SHADERS_IN_PIPELINE> ShaderVariableDataSizes = {};
            for (Uint32 s = 0; s < GetNumActiveShaderStages(); ++s)
            {
                constexpr SHADER_RESOURCE_VARIABLE_TYPE AllowedVarTypes[]{SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};
                ShaderVariableDataSizes[s] = ShaderVariableManagerImplType::GetRequiredMemorySize(*pThisImpl, AllowedVarTypes, _countof(AllowedVarTypes), GetActiveShaderStageType(s));
            }

            const size_t CacheMemorySize = GetRequiredResourceCacheMemorySize();
            m_SRBMemAllocator.Initialize(Desc.SRBAllocationGranularity, GetNumActiveShaderStages(), ShaderVariableDataSizes.data(), 1, &CacheMemorySize);
        }

        pThisImpl->CalculateHash();
    }

protected:
    template <typename ImmutableSamplerAttribsType, typename SerializedData>
    void Deserialize(IMemoryAllocator&                    RawAllocator,
                     const PipelineResourceSignatureDesc& Desc,
                     const SerializedData&                Serialized,
                     ImmutableSamplerAttribsType*&        ImmutableSamAttribs,
                     const std::function<void()>&         InitResourceLayout,
                     const std::function<size_t()>&       GetRequiredResourceCacheMemorySize) noexcept(false)
    {
        VERIFY_EXPR(Desc.NumResources == Serialized.NumResources);

        Initialize(
            RawAllocator, Desc, ImmutableSamAttribs, InitResourceLayout, GetRequiredResourceCacheMemorySize,
            [&Serialized](FixedLinearAllocator& Allocator) {
                return Allocator.CopyArray<PipelineResourceAttribsType>(Serialized.pResourceAttribs, Serialized.NumResources);
            },
            [&Desc, &Serialized](FixedLinearAllocator& Allocator) {
                return Serialized.pImmutableSamplers ?
                    Allocator.CopyConstructArray<ImmutableSamplerAttribsType>(Serialized.pImmutableSamplers, Serialized.NumImmutableSamplers) :
                    Allocator.ConstructArray<ImmutableSamplerAttribsType>(Desc.NumImmutableSamplers);
            });
    }

    // Decouples combined image samplers into texture SRV + sampler pairs.
    PipelineResourceSignatureDescWrapper DecoupleCombinedSamplers(const PipelineResourceSignatureDesc& Desc)
    {
        PipelineResourceSignatureDescWrapper UpdatedDesc{Desc};

        bool HasCombinedSamplers = false;
        for (Uint32 r = 0; r < Desc.NumResources; ++r)
        {
            const auto& Res{Desc.Resources[r]};
            if ((Res.Flags & PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER) == 0)
                continue;

            VERIFY(Res.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV,
                   "COMBINED_SAMPLER flag is only valid for texture SRVs. This error should've been caught by ValidatePipelineResourceSignatureDesc().");
            VERIFY(Desc.UseCombinedTextureSamplers && Desc.CombinedSamplerSuffix != nullptr,
                   "UseCombinedTextureSamplers must be true and CombinedSamplerSuffix must not be null when COMBINED_SAMPLER flag is used. "
                   "This error should've been caught by ValidatePipelineResourceSignatureDesc().");

            HasCombinedSamplers = true;

            const auto SamplerName = std::string{Res.Name} + Desc.CombinedSamplerSuffix;
            // Check if the sampler is already defined.
            if (Diligent::FindResource(Desc.Resources, Desc.NumResources, Res.ShaderStages, SamplerName.c_str()) == InvalidPipelineResourceIndex)
            {
                //  Add sampler to the list of resources.
                UpdatedDesc.AddResource(Res.ShaderStages, SamplerName.c_str(), Res.ArraySize, SHADER_RESOURCE_TYPE_SAMPLER, Res.VarType);
            }
        }

        if (HasCombinedSamplers)
        {
            // Clear COMBINED_SAMPLER flag
            UpdatedDesc.ProcessResources([](PipelineResourceDesc& Res) {
                Res.Flags &= ~PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER;
            });

            this->m_Desc.NumResources = UpdatedDesc.Get().NumResources;
        }

        return UpdatedDesc;
    }

    void GetInternalData(PipelineResourceSignatureInternalData& InternalData) const
    {
        InternalData.ShaderStages          = m_ShaderStages;
        InternalData.StaticResShaderStages = m_StaticResShaderStages;
        InternalData.PipelineType          = m_PipelineType;
        InternalData.StaticResStageIndex   = m_StaticResStageIndex;
    }


protected:
    void Destruct()
    {
        VERIFY(!m_IsDestructed, "This object has already been destructed");

        this->m_Desc.Resources             = nullptr;
        this->m_Desc.ImmutableSamplers     = nullptr;
        this->m_Desc.CombinedSamplerSuffix = nullptr;

        auto& RawAllocator = GetRawAllocator();

        if (m_StaticVarsMgrs != nullptr)
        {
            for (auto Idx : m_StaticResStageIndex)
            {
                if (Idx >= 0)
                {
                    m_StaticVarsMgrs[Idx].Destroy(RawAllocator);
                    m_StaticVarsMgrs[Idx].~ShaderVariableManagerImplType();
                }
            }
            m_StaticVarsMgrs = nullptr;
        }

        if (m_pStaticResCache != nullptr)
        {
            m_pStaticResCache->~ShaderResourceCacheImplType();
            m_pStaticResCache = nullptr;
        }

        m_StaticResStageIndex.fill(-1);

        static_assert(std::is_trivially_destructible<PipelineResourceAttribsType>::value, "Destructors for m_pResourceAttribs[] are required");
        m_pResourceAttribs = nullptr;

        m_pRawMemory.reset();

#if DILIGENT_DEBUG
        m_IsDestructed = true;
#endif
    }

    // Finds a sampler that is assigned to texture Tex, when combined texture samplers are used.
    // Returns an index of the sampler in m_Desc.Resources array, or InvalidSamplerValue if there is
    // no such sampler, or if combined samplers are not used.
    Uint32 FindAssignedSampler(const PipelineResourceDesc& Tex, Uint32 InvalidSamplerValue) const
    {
        VERIFY_EXPR(Tex.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV);
        Uint32 SamplerInd = InvalidSamplerValue;
        if (IsUsingCombinedSamplers())
        {
            const auto IdxRange = GetResourceIndexRange(Tex.VarType);

            for (Uint32 i = IdxRange.first; i < IdxRange.second; ++i)
            {
                const auto& Res = this->m_Desc.Resources[i];
                VERIFY_EXPR(Tex.VarType == Res.VarType);

                if (Res.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER &&
                    (Tex.ShaderStages & Res.ShaderStages) != 0 &&
                    StreqSuff(Res.Name, Tex.Name, GetCombinedSamplerSuffix()))
                {
                    VERIFY_EXPR((Res.ShaderStages & Tex.ShaderStages) == Tex.ShaderStages);
                    SamplerInd = i;
                    break;
                }
            }
        }
        return SamplerInd;
    }

    void CalculateHash()
    {
        const auto* const pThisImpl = static_cast<const PipelineResourceSignatureImplType*>(this);

        m_Hash = CalculatePipelineResourceSignatureDescHash(this->m_Desc);
        for (Uint32 i = 0; i < this->m_Desc.NumResources; ++i)
        {
            const auto& Attr = pThisImpl->GetResourceAttribs(i);
            HashCombine(m_Hash, Attr.GetHash());
        }
    }

protected:
    std::unique_ptr<void, STDDeleterRawMem<void>> m_pRawMemory;

    // Pipeline resource attributes
    PipelineResourceAttribsType* m_pResourceAttribs = nullptr; // [m_Desc.NumResources]

    // Static resource cache for all static resources
    ShaderResourceCacheImplType* m_pStaticResCache = nullptr;

    // Static variables manager for every shader stage
    ShaderVariableManagerImplType* m_StaticVarsMgrs = nullptr; // [GetNumStaticResStages()]

    size_t m_Hash = 0;

    // Resource offsets (e.g. index of the first resource), for each variable type.
    std::array<Uint16, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES + 1> m_ResourceOffsets = {};

    // Shader stages that have resources.
    SHADER_TYPE m_ShaderStages = SHADER_TYPE_UNKNOWN;

    // Shader stages that have static resources.
    SHADER_TYPE m_StaticResShaderStages = SHADER_TYPE_UNKNOWN;

    PIPELINE_TYPE m_PipelineType = PIPELINE_TYPE_INVALID;

    // Index of the shader stage that has static resources, for every shader
    // type in the pipeline (given by GetShaderTypePipelineIndex(ShaderType, m_PipelineType)).
    std::array<Int8, MAX_SHADERS_IN_PIPELINE> m_StaticResStageIndex = {-1, -1, -1, -1, -1, -1};
    static_assert(MAX_SHADERS_IN_PIPELINE == 6, "Please update the initializer list above");

    // Allocator for shader resource binding object instances.
    SRBMemoryAllocator m_SRBMemAllocator;

#ifdef DILIGENT_DEBUG
    bool m_IsDestructed = false;
#endif
};

} // namespace Diligent
