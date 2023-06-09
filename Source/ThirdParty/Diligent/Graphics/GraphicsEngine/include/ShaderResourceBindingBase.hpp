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
/// Implementation of the Diligent::ShaderResourceBindingBase template class

#include <array>
#include <functional>

#include "PrivateConstants.h"
#include "ShaderResourceBinding.h"
#include "ObjectBase.hpp"
#include "GraphicsTypes.h"
#include "Constants.h"
#include "RefCntAutoPtr.hpp"
#include "GraphicsAccessories.hpp"
#include "ShaderResourceCacheCommon.hpp"
#include "FixedLinearAllocator.hpp"
#include "EngineMemory.h"

namespace Diligent
{

/// Template class implementing base functionality of the shader resource binding

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class ShaderResourceBindingBase : public ObjectBase<typename EngineImplTraits::ShaderResourceBindingInterface>
{
public:
    // Base interface this class inherits (IShaderResourceBindingD3D12, IShaderResourceBindingVk, etc.)
    using BaseInterface = typename EngineImplTraits::ShaderResourceBindingInterface;

    // The type of the pipeline resource signature implementation (PipelineResourceSignatureD3D12Impl, PipelineResourceSignatureVkImpl, etc.).
    using ResourceSignatureType = typename EngineImplTraits::PipelineResourceSignatureImplType;

    // The type of the shader resource cache implementation (ShaderResourceCacheD3D12Impl, ShaderResourceCacheVkImpl, etc.)
    using ShaderResourceCacheImplType = typename EngineImplTraits::ShaderResourceCacheImplType;

    // The type of the shader resource variable manager (ShaderVariableManagerD3D12Impl, ShaderVariableManagerVkImpl, etc.)
    using ShaderVariableManagerImplType = typename EngineImplTraits::ShaderVariableManagerImplType;

    using TObjectBase = ObjectBase<BaseInterface>;

    /// \param pRefCounters - Reference counters object that controls the lifetime of this SRB.
    /// \param pPRS         - Pipeline resource signature that this SRB belongs to.
    ShaderResourceBindingBase(IReferenceCounters* pRefCounters, ResourceSignatureType* pPRS) :
        TObjectBase{pRefCounters},
        m_pPRS{pPRS},
        m_ShaderResourceCache{ResourceCacheContentType::SRB}
    {
        try
        {
            m_ActiveShaderStageIndex.fill(-1);

            const auto NumShaders   = GetNumShaders();
            const auto PipelineType = GetPipelineType();
            for (Uint32 s = 0; s < NumShaders; ++s)
            {
                const auto ShaderType = pPRS->GetActiveShaderStageType(s);
                const auto ShaderInd  = GetShaderTypePipelineIndex(ShaderType, PipelineType);

                m_ActiveShaderStageIndex[ShaderInd] = static_cast<Int8>(s);
            }


            FixedLinearAllocator MemPool{GetRawAllocator()};
            MemPool.AddSpace<ShaderVariableManagerImplType>(NumShaders);
            MemPool.Reserve();
            static_assert(std::is_nothrow_constructible<ShaderVariableManagerImplType, decltype(*this), ShaderResourceCacheImplType&>::value,
                          "Constructor of ShaderVariableManagerImplType must be noexcept, so we can safely construct all managers");
            m_pShaderVarMgrs = MemPool.ConstructArray<ShaderVariableManagerImplType>(NumShaders, std::ref(*this), std::ref(m_ShaderResourceCache));

            // The memory is now owned by ShaderResourceBindingBase and will be freed by Destruct().
            auto* Ptr = MemPool.ReleaseOwnership();
            VERIFY_EXPR(Ptr == m_pShaderVarMgrs);
            (void)Ptr;

            // It is important to construct all objects before initializing them because if an exception is thrown,
            // Destruct() will call destructors for all non-null objects.

            pPRS->InitSRBResourceCache(m_ShaderResourceCache);

            auto& SRBMemAllocator = pPRS->GetSRBMemoryAllocator();
            for (Uint32 s = 0; s < NumShaders; ++s)
            {
                const auto ShaderType = pPRS->GetActiveShaderStageType(s);
                const auto ShaderInd  = GetShaderTypePipelineIndex(ShaderType, pPRS->GetPipelineType());
                const auto MgrInd     = m_ActiveShaderStageIndex[ShaderInd];
                VERIFY_EXPR(MgrInd >= 0 && MgrInd < static_cast<int>(NumShaders));

                auto& VarDataAllocator = SRBMemAllocator.GetShaderVariableDataAllocator(s);

                // Initialize vars manager to reference mutable and dynamic variables
                // Note that the cache has space for all variable types
                const SHADER_RESOURCE_VARIABLE_TYPE VarTypes[] = {SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};
                m_pShaderVarMgrs[MgrInd].Initialize(*pPRS, VarDataAllocator, VarTypes, _countof(VarTypes), ShaderType);
            }
        }
        catch (...)
        {
            // We must release objects manually as destructor will not be called.
            Destruct();
            throw;
        }
    }

    ~ShaderResourceBindingBase()
    {
        Destruct();
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_ShaderResourceBinding, TObjectBase)

    Uint32 GetBindingIndex() const
    {
        return m_pPRS->GetDesc().BindingIndex;
    }

    PIPELINE_TYPE GetPipelineType() const
    {
        return m_pPRS->GetPipelineType();
    }

    Uint32 GetNumShaders() const
    {
        return m_pPRS->GetNumActiveShaderStages();
    }

    /// Implementation of IShaderResourceBinding::GetPipelineResourceSignature().
    virtual IPipelineResourceSignature* DILIGENT_CALL_TYPE GetPipelineResourceSignature() const override final
    {
        return GetSignature();
    }

    virtual bool DILIGENT_CALL_TYPE StaticResourcesInitialized() const override final
    {
        return m_bStaticResourcesInitialized;
    }

    ResourceSignatureType* GetSignature() const
    {
        return m_pPRS;
    }

    void SetStaticResourcesInitialized()
    {
        VERIFY_EXPR(!m_bStaticResourcesInitialized);
        m_bStaticResourcesInitialized = true;
    }

    /// Implementation of IShaderResourceBinding::GetVariableByName().
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetVariableByName(SHADER_TYPE ShaderType, const char* Name) override final
    {
        const auto PipelineType = GetPipelineType();
        if (!IsConsistentShaderType(ShaderType, PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to find mutable/dynamic variable '", Name, "' in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(PipelineType), " pipeline resource signature '", m_pPRS->GetDesc().Name, "'.");
            return nullptr;
        }

        const auto ShaderInd = GetShaderTypePipelineIndex(ShaderType, PipelineType);
        const auto MgrInd    = m_ActiveShaderStageIndex[ShaderInd];
        if (MgrInd < 0)
            return nullptr;

        VERIFY_EXPR(static_cast<Uint32>(MgrInd) < GetNumShaders());
        return m_pShaderVarMgrs[MgrInd].GetVariable(Name);
    }

    /// Implementation of IShaderResourceBinding::GetVariableCount().
    virtual Uint32 DILIGENT_CALL_TYPE GetVariableCount(SHADER_TYPE ShaderType) const override final
    {
        const auto PipelineType = GetPipelineType();
        if (!IsConsistentShaderType(ShaderType, PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to get the number of mutable/dynamic variables in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(PipelineType), " pipeline resource signature '", m_pPRS->GetDesc().Name, "'.");
            return 0;
        }

        const auto ShaderInd = GetShaderTypePipelineIndex(ShaderType, PipelineType);
        const auto MgrInd    = m_ActiveShaderStageIndex[ShaderInd];
        if (MgrInd < 0)
            return 0;

        VERIFY_EXPR(static_cast<Uint32>(MgrInd) < GetNumShaders());
        return m_pShaderVarMgrs[MgrInd].GetVariableCount();
    }

    /// Implementation of IShaderResourceBinding::GetVariableByIndex().
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index) override final
    {
        const auto PipelineType = GetPipelineType();
        if (!IsConsistentShaderType(ShaderType, PipelineType))
        {
            LOG_WARNING_MESSAGE("Unable to get mutable/dynamic variable at index ", Index, " in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is invalid for ", GetPipelineTypeString(PipelineType), " pipeline resource signature '", m_pPRS->GetDesc().Name, "'.");
            return nullptr;
        }

        const auto ShaderInd = GetShaderTypePipelineIndex(ShaderType, PipelineType);
        const auto MgrInd    = m_ActiveShaderStageIndex[ShaderInd];
        if (MgrInd < 0)
            return nullptr;

        VERIFY_EXPR(static_cast<Uint32>(MgrInd) < GetNumShaders());
        return m_pShaderVarMgrs[MgrInd].GetVariable(Index);
    }

    /// Implementation of IShaderResourceBinding::BindResources().
    virtual void DILIGENT_CALL_TYPE BindResources(SHADER_TYPE                 ShaderStages,
                                                  IResourceMapping*           pResMapping,
                                                  BIND_SHADER_RESOURCES_FLAGS Flags) override final
    {
        ProcessVariables(ShaderStages,
                         [pResMapping, Flags](ShaderVariableManagerImplType& Mgr) //
                         {
                             Mgr.BindResources(pResMapping, Flags);
                             return true;
                         });
    }

    /// Implementation of IShaderResourceBinding::CheckResources().
    virtual SHADER_RESOURCE_VARIABLE_TYPE_FLAGS DILIGENT_CALL_TYPE CheckResources(
        SHADER_TYPE                 ShaderStages,
        IResourceMapping*           pResMapping,
        BIND_SHADER_RESOURCES_FLAGS Flags) const override final
    {
        SHADER_RESOURCE_VARIABLE_TYPE_FLAGS StaleVarTypes = SHADER_RESOURCE_VARIABLE_TYPE_FLAG_NONE;
        ProcessVariables(ShaderStages,
                         [&](const ShaderVariableManagerImplType& Mgr) //
                         {
                             Mgr.CheckResources(pResMapping, Flags, StaleVarTypes);
                             // Stop when both mutable and dynamic variables are stale as there is no reason to check further.
                             return (StaleVarTypes & SHADER_RESOURCE_VARIABLE_TYPE_FLAG_MUT_DYN) != SHADER_RESOURCE_VARIABLE_TYPE_FLAG_MUT_DYN;
                         });
        return StaleVarTypes;
    }

    ShaderResourceCacheImplType&       GetResourceCache() { return m_ShaderResourceCache; }
    const ShaderResourceCacheImplType& GetResourceCache() const { return m_ShaderResourceCache; }

private:
    void Destruct()
    {
        if (m_pShaderVarMgrs != nullptr)
        {
            auto& SRBMemAllocator = GetSignature()->GetSRBMemoryAllocator();
            for (Uint32 s = 0; s < GetNumShaders(); ++s)
            {
                auto& VarDataAllocator = SRBMemAllocator.GetShaderVariableDataAllocator(s);
                m_pShaderVarMgrs[s].Destroy(VarDataAllocator);
                m_pShaderVarMgrs[s].~ShaderVariableManagerImplType();
            }
            GetRawAllocator().Free(m_pShaderVarMgrs);
        }
    }

    template <typename HandlerType>
    void ProcessVariables(SHADER_TYPE   ShaderStages,
                          HandlerType&& Handler) const
    {
        const auto PipelineType = GetPipelineType();
        for (size_t ShaderInd = 0; ShaderInd < m_ActiveShaderStageIndex.size(); ++ShaderInd)
        {
            const auto VarMngrInd = m_ActiveShaderStageIndex[ShaderInd];
            if (VarMngrInd < 0)
                continue;

            // ShaderInd is the shader type pipeline index here
            const auto ShaderType = GetShaderTypeFromPipelineIndex(static_cast<Int32>(ShaderInd), PipelineType);
            if ((ShaderStages & ShaderType) == 0)
                continue;

            if (!Handler(m_pShaderVarMgrs[VarMngrInd]))
                break;
        }
    }

protected:
    /// Strong reference to pipeline resource signature. We must use strong reference, because
    /// shader resource binding uses pipeline resource signature's memory allocator to allocate
    /// memory for shader resource cache.
    RefCntAutoPtr<ResourceSignatureType> m_pPRS;

    // Index of the active shader stage that has resources, for every shader
    // type in the pipeline (given by GetShaderTypePipelineIndex(ShaderType, m_PipelineType)).
    std::array<Int8, MAX_SHADERS_IN_PIPELINE> m_ActiveShaderStageIndex = {-1, -1, -1, -1, -1, -1};
    static_assert(MAX_SHADERS_IN_PIPELINE == 6, "Please update the initializer list above");

    ShaderResourceCacheImplType    m_ShaderResourceCache;
    ShaderVariableManagerImplType* m_pShaderVarMgrs = nullptr; // [GetNumShaders()]

    bool m_bStaticResourcesInitialized = false;
};

} // namespace Diligent
