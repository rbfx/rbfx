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
/// Implementation of the Diligent::PipelineStateBase template class

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <vector>

#include "PrivateConstants.h"
#include "PipelineState.h"
#include "DeviceObjectBase.hpp"
#include "STDAllocator.hpp"
#include "EngineMemory.h"
#include "GraphicsAccessories.hpp"
#include "FixedLinearAllocator.hpp"
#include "HashUtils.hpp"
#include "PipelineResourceSignatureBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "AsyncInitializer.hpp"
#include "GraphicsTypesX.hpp"

namespace Diligent
{

/// Internal PSO create flags.
enum PSO_CREATE_INTERNAL_FLAGS : Uint32
{
    PSO_CREATE_INTERNAL_FLAG_NONE = 0u,

    /// Pipeline resource signature 0 is the implicit signature
    /// created from the resource layout.
    ///
    /// \note This flag is used for PSO deserialization.
    PSO_CREATE_INTERNAL_FLAG_IMPLICIT_SIGNATURE0 = 1u << 0u,

    /// Compiled shaders do not contain reflection information.
    ///
    /// \note This flag is used for PSO deserialization.
    PSO_CREATE_INTERNAL_FLAG_NO_SHADER_REFLECTION = 1u << 1u,

    PSO_CREATE_INTERNAL_FLAG_LAST = PSO_CREATE_INTERNAL_FLAG_NO_SHADER_REFLECTION
};
DEFINE_FLAG_ENUM_OPERATORS(PSO_CREATE_INTERNAL_FLAGS);

struct PSOCreateInternalInfo
{
    PSO_CREATE_INTERNAL_FLAGS Flags = PSO_CREATE_INTERNAL_FLAG_NONE;
};

template <typename PSOCreateInfoType>
void ValidatePSOCreateInfo(const IRenderDevice*     pDevice,
                           const PSOCreateInfoType& CreateInfo) noexcept(false);

// Validates graphics pipeline create attributes and throws an exception in case of an error.
template <>
void ValidatePSOCreateInfo<GraphicsPipelineStateCreateInfo>(const IRenderDevice*                   pDevice,
                                                            const GraphicsPipelineStateCreateInfo& CreateInfo) noexcept(false);

// Validates compute pipeline create attributes and throws an exception in case of an error.
template <>
void ValidatePSOCreateInfo<ComputePipelineStateCreateInfo>(const IRenderDevice*                  pDevice,
                                                           const ComputePipelineStateCreateInfo& CreateInfo) noexcept(false);

// Validates ray-tracing pipeline create attributes and throws an exception in case of an error.
template <>
void ValidatePSOCreateInfo<RayTracingPipelineStateCreateInfo>(const IRenderDevice*                     pDevice,
                                                              const RayTracingPipelineStateCreateInfo& CreateInfo) noexcept(false);

// Validates tile pipeline create attributes and throws an exception in case of an error.
template <>
void ValidatePSOCreateInfo<TilePipelineStateCreateInfo>(const IRenderDevice*               pDevice,
                                                        const TilePipelineStateCreateInfo& CreateInfo) noexcept(false);

/// Validates that pipeline resource description 'ResDesc' is compatible with the actual resource
/// attributes and throws an exception in case of an error.
void ValidatePipelineResourceCompatibility(const PipelineResourceDesc& ResDesc,
                                           SHADER_RESOURCE_TYPE        Type,
                                           PIPELINE_RESOURCE_FLAGS     ResourceFlags,
                                           Uint32                      ArraySize,
                                           const char*                 ShaderName,
                                           const char*                 SignatureName) noexcept(false);


/// Copies ray tracing shader group names and also initializes the mapping from the group name to its index.
void CopyRTShaderGroupNames(std::unordered_map<HashMapStringKey, Uint32>& NameToGroupIndex,
                            const RayTracingPipelineStateCreateInfo&      CreateInfo,
                            FixedLinearAllocator&                         MemPool) noexcept;

void CorrectGraphicsPipelineDesc(GraphicsPipelineDesc& GraphicsPipeline, const DeviceFeatures& Features) noexcept;


/// Finds a pipeline resource layout variable with the name 'Name' in shader stage 'ShaderStage'
/// in the list of variables of 'LayoutDesc'. If CombinedSamplerSuffix != null, the
/// variable is treated as a combined sampler and the suffix is added to the names of
/// variables from 'LayoutDesc' when comparing with 'Name'.
/// If the variable is not found, returns default variable
/// {ShaderStage, Name, LayoutDesc.DefaultVariableType}.
ShaderResourceVariableDesc FindPipelineResourceLayoutVariable(
    const PipelineResourceLayoutDesc& LayoutDesc,
    const char*                       Name,
    SHADER_TYPE                       ShaderStage,
    const char*                       CombinedSamplerSuffix);


/// Hash map key that identifies shader resource by its name and shader stages
struct ShaderResourceHashKey : public HashMapStringKey
{
    template <typename... ArgsType>
    ShaderResourceHashKey(const SHADER_TYPE _ShaderStages,
                          ArgsType&&... Args) noexcept :
        // clang-format off
        HashMapStringKey{std::forward<ArgsType>(Args)...},
        ShaderStages    {_ShaderStages}
    // clang-format on
    {
        Ownership_Hash = ComputeHash(GetHash(), Uint32{ShaderStages}) & HashMask;
    }

    ShaderResourceHashKey(ShaderResourceHashKey&& Key) noexcept :
        // clang-format off
        HashMapStringKey{std::move(Key)},
        ShaderStages    {Key.ShaderStages}
    // clang-format on
    {
        Key.ShaderStages = SHADER_TYPE_UNKNOWN;
    }

    bool operator==(const ShaderResourceHashKey& rhs) const noexcept
    {
        return ShaderStages == rhs.ShaderStages &&
            static_cast<const HashMapStringKey&>(*this) == static_cast<const HashMapStringKey&>(rhs);
    }

    struct Hasher
    {
        size_t operator()(const ShaderResourceHashKey& Key) const noexcept
        {
            return Key.GetHash();
        }
    };

protected:
    SHADER_TYPE ShaderStages = SHADER_TYPE_UNKNOWN;
};

namespace PipelineStateUtils
{

template <typename ShaderImplType>
void WaitUntilShaderReadyIfRequested(RefCntAutoPtr<ShaderImplType>& pShader, bool WaitForCompletion)
{
    if (WaitForCompletion)
    {
        const SHADER_STATUS ShaderStatus = pShader->GetStatus(/*WaitForCompletion = */ true);
        if (ShaderStatus != SHADER_STATUS_READY)
        {
            LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name, "' is in ", GetShaderStatusString(ShaderStatus),
                                " status and cannot be used to create a pipeline state. Use GetStatus() to check the shader status.");
        }
    }
}

template <typename ShaderImplType, typename TShaderStages>
void ExtractShaders(const GraphicsPipelineStateCreateInfo& CreateInfo,
                    TShaderStages&                         ShaderStages,
                    bool                                   WaitUntilShadersReady,
                    SHADER_TYPE&                           ActiveShaderStages)
{
    VERIFY_EXPR(CreateInfo.PSODesc.IsAnyGraphicsPipeline());

    ShaderStages.clear();
    ActiveShaderStages = SHADER_TYPE_UNKNOWN;

    auto AddShaderStage = [&](IShader* pShader) {
        if (pShader != nullptr)
        {
            RefCntAutoPtr<ShaderImplType> pShaderImpl{pShader, ShaderImplType::IID_InternalImpl};
            VERIFY(pShaderImpl, "Unexpected shader object implementation");
            WaitUntilShaderReadyIfRequested(pShaderImpl, WaitUntilShadersReady);
            ShaderStages.emplace_back(pShaderImpl);
            const SHADER_TYPE ShaderType = pShader->GetDesc().ShaderType;
            VERIFY((ActiveShaderStages & ShaderType) == 0,
                   "Shader stage ", GetShaderTypeLiteralName(ShaderType), " has already been initialized in PSO.");
            ActiveShaderStages |= ShaderType;
#ifdef DILIGENT_DEBUG
            for (size_t i = 0; i + 1 < ShaderStages.size(); ++i)
                VERIFY_EXPR(GetShaderStageType(ShaderStages[i]) != ShaderType);
#endif
        }
    };

    switch (CreateInfo.PSODesc.PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
        {
            AddShaderStage(CreateInfo.pVS);
            AddShaderStage(CreateInfo.pHS);
            AddShaderStage(CreateInfo.pDS);
            AddShaderStage(CreateInfo.pGS);
            AddShaderStage(CreateInfo.pPS);
            VERIFY(CreateInfo.pVS != nullptr, "Vertex shader must not be null");
            break;
        }

        case PIPELINE_TYPE_MESH:
        {
            AddShaderStage(CreateInfo.pAS);
            AddShaderStage(CreateInfo.pMS);
            AddShaderStage(CreateInfo.pPS);
            VERIFY(CreateInfo.pMS != nullptr, "Mesh shader must not be null");
            break;
        }

        default:
            UNEXPECTED("unknown pipeline type");
    }

    VERIFY_EXPR(!ShaderStages.empty());
}

template <typename ShaderImplType, typename TShaderStages>
void ExtractShaders(const ComputePipelineStateCreateInfo& CreateInfo,
                    TShaderStages&                        ShaderStages,
                    bool                                  WaitUntilShadersReady,
                    SHADER_TYPE&                          ActiveShaderStages)
{
    VERIFY_EXPR(CreateInfo.PSODesc.IsComputePipeline());

    ShaderStages.clear();

    VERIFY_EXPR(CreateInfo.PSODesc.PipelineType == PIPELINE_TYPE_COMPUTE);
    VERIFY_EXPR(CreateInfo.pCS != nullptr);
    VERIFY_EXPR(CreateInfo.pCS->GetDesc().ShaderType == SHADER_TYPE_COMPUTE);

    RefCntAutoPtr<ShaderImplType> pShaderImpl{CreateInfo.pCS, ShaderImplType::IID_InternalImpl};
    VERIFY(pShaderImpl, "Unexpected shader object implementation");
    WaitUntilShaderReadyIfRequested(pShaderImpl, WaitUntilShadersReady);
    ShaderStages.emplace_back(pShaderImpl);
    ActiveShaderStages = SHADER_TYPE_COMPUTE;

    VERIFY_EXPR(!ShaderStages.empty());
}

template <typename ShaderImplType, typename TShaderStages>
void ExtractShaders(const RayTracingPipelineStateCreateInfo& CreateInfo,
                    TShaderStages&                           ShaderStages,
                    bool                                     WaitUntilShadersReady,
                    SHADER_TYPE&                             ActiveShaderStages)
{
    VERIFY_EXPR(CreateInfo.PSODesc.IsRayTracingPipeline());

    std::unordered_set<IShader*> UniqueShaders;

    auto AddShader = [&ShaderStages,
                      &UniqueShaders,
                      &ActiveShaderStages,
                      WaitUntilShadersReady](IShader* pShader) {
        if (pShader != nullptr && UniqueShaders.insert(pShader).second)
        {
            const SHADER_TYPE ShaderType = pShader->GetDesc().ShaderType;
            const Int32       StageInd   = GetShaderTypePipelineIndex(ShaderType, PIPELINE_TYPE_RAY_TRACING);
            auto&             Stage      = ShaderStages[StageInd];
            ActiveShaderStages |= ShaderType;
            RefCntAutoPtr<ShaderImplType> pShaderImpl{pShader, ShaderImplType::IID_InternalImpl};
            VERIFY(pShaderImpl, "Unexpected shader object implementation");
            WaitUntilShaderReadyIfRequested(pShaderImpl, WaitUntilShadersReady);
            Stage.Append(pShaderImpl);
        }
    };

    ShaderStages.clear();
    ShaderStages.resize(MAX_SHADERS_IN_PIPELINE);
    ActiveShaderStages = SHADER_TYPE_UNKNOWN;

    for (Uint32 i = 0; i < CreateInfo.GeneralShaderCount; ++i)
    {
        AddShader(CreateInfo.pGeneralShaders[i].pShader);
    }
    for (Uint32 i = 0; i < CreateInfo.TriangleHitShaderCount; ++i)
    {
        AddShader(CreateInfo.pTriangleHitShaders[i].pClosestHitShader);
        AddShader(CreateInfo.pTriangleHitShaders[i].pAnyHitShader);
    }
    for (Uint32 i = 0; i < CreateInfo.ProceduralHitShaderCount; ++i)
    {
        AddShader(CreateInfo.pProceduralHitShaders[i].pIntersectionShader);
        AddShader(CreateInfo.pProceduralHitShaders[i].pClosestHitShader);
        AddShader(CreateInfo.pProceduralHitShaders[i].pAnyHitShader);
    }

    if (ShaderStages[GetShaderTypePipelineIndex(SHADER_TYPE_RAY_GEN, PIPELINE_TYPE_RAY_TRACING)].Count() == 0)
        LOG_ERROR_AND_THROW("At least one shader with type SHADER_TYPE_RAY_GEN must be provided.");

    // Remove empty stages
    for (auto iter = ShaderStages.begin(); iter != ShaderStages.end();)
    {
        if (iter->Count() == 0)
        {
            iter = ShaderStages.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    VERIFY_EXPR(!ShaderStages.empty());
}

template <typename ShaderImplType, typename TShaderStages>
void ExtractShaders(const TilePipelineStateCreateInfo& CreateInfo,
                    TShaderStages&                     ShaderStages,
                    bool                               WaitUntilShadersReady,
                    SHADER_TYPE&                       ActiveShaderStages)
{
    VERIFY_EXPR(CreateInfo.PSODesc.IsTilePipeline());

    ShaderStages.clear();

    VERIFY_EXPR(CreateInfo.PSODesc.PipelineType == PIPELINE_TYPE_TILE);
    VERIFY_EXPR(CreateInfo.pTS != nullptr);
    VERIFY_EXPR(CreateInfo.pTS->GetDesc().ShaderType == SHADER_TYPE_TILE);

    RefCntAutoPtr<ShaderImplType> pShaderImpl{CreateInfo.pTS, ShaderImplType::IID_InternalImpl};
    VERIFY(pShaderImpl, "Unexpected shader object implementation");
    WaitUntilShaderReadyIfRequested(pShaderImpl, WaitUntilShadersReady);
    ShaderStages.emplace_back(pShaderImpl);
    ActiveShaderStages = SHADER_TYPE_TILE;

    VERIFY_EXPR(!ShaderStages.empty());
}

} // namespace PipelineStateUtils

/// Template class implementing base functionality of the pipeline state object.

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class PipelineStateBase : public DeviceObjectBase<typename EngineImplTraits::PipelineStateInterface, typename EngineImplTraits::RenderDeviceImplType, PipelineStateDesc>
{
protected:
    // Base interface this class inherits (IPipelineStateD3D12, IPipelineStateVk, etc.)
    using BaseInterface = typename EngineImplTraits::PipelineStateInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    // Pipeline state implementation type (PipelineStateD3D12Impl, PipelineStateVkImpl, etc.).
    using PipelineStateImplType = typename EngineImplTraits::PipelineStateImplType;

    // Pipeline resource signature implementation type (PipelineResourceSignatureD3D12Impl, PipelineResourceSignatureVkImpl, etc.).
    using PipelineResourceSignatureImplType = typename EngineImplTraits::PipelineResourceSignatureImplType;

    // Render pass implementation type (RenderPassD3D12Impl, RenderPassVkImpl, etc.).
    using RenderPassImplType = typename EngineImplTraits::RenderPassImplType;

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, PipelineStateDesc>;

public:
    /// Initializes the object as a specific pipeline

    /// \tparam PSOCreateInfoType - Pipeline state create info type (GraphicsPipelineStateCreateInfo,
    ///                             ComputePipelineStateCreateInfo, etc.)
    /// \param pRefCounters      - Reference counters object that controls the lifetime of this PSO
    /// \param pDevice           - Pointer to the device.
    /// \param CreateInfo        - Pipeline state create info.
    /// \param bIsDeviceInternal - Flag indicating if the pipeline state is an internal device object and
    ///							   must not keep a strong reference to the device.
    template <typename PSOCreateInfoType>
    PipelineStateBase(IReferenceCounters*      pRefCounters,
                      RenderDeviceImplType*    pDevice,
                      const PSOCreateInfoType& CreateInfo,
                      bool                     bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, CreateInfo.PSODesc, bIsDeviceInternal},
        m_UsingImplicitSignature{CreateInfo.ppResourceSignatures == nullptr ||
                                 CreateInfo.ResourceSignaturesCount == 0 ||
                                 (GetInternalCreateFlags(CreateInfo) & PSO_CREATE_INTERNAL_FLAG_IMPLICIT_SIGNATURE0) != 0}
    {
        try
        {
            ValidatePSOCreateInfo(this->GetDevice(), CreateInfo);

            Uint64 DeviceQueuesMask = this->GetDevice()->GetCommandQueueMask();
            DEV_CHECK_ERR((this->m_Desc.ImmediateContextMask & DeviceQueuesMask) != 0,
                          "No bits in the immediate mask (0x", std::hex, this->m_Desc.ImmediateContextMask,
                          ") correspond to one of ", this->GetDevice()->GetCommandQueueCount(), " available software command queues.");
            this->m_Desc.ImmediateContextMask &= DeviceQueuesMask;
        }
        catch (...)
        {
            Destruct();
            throw;
        }
    }


    ~PipelineStateBase()
    {
        VERIFY(!AsyncInitializer::GetAsyncTask(m_AsyncInitializer), "Initialize task is still running. This may result in a crash if the task accesses resources owned by the pipeline state object.");

        /*
        /// \note Destructor cannot directly remove the object from the registry as this may cause a
        ///       deadlock at the point where StateObjectsRegistry::Find() locks the weak pointer: if we
        ///       are in dtor, the object is locked by Diligent::RefCountedObject::Release() and
        ///       StateObjectsRegistry::Find() will wait for that lock to be released.
        ///       A the same time this thread will be waiting for the other thread to unlock the registry.\n
        ///       Thus destructor only notifies the registry that there is a deleted object.
        ///       The reference to the object will be removed later.
        auto &PipelineStateRegistry = static_cast<TRenderDeviceBase*>(this->GetDevice())->GetBSRegistry();
        auto &RasterizerStateRegistry = static_cast<TRenderDeviceBase*>(this->GetDevice())->GetRSRegistry();
        auto &DSSRegistry = static_cast<TRenderDeviceBase*>(this->GetDevice())->GetDSSRegistry();
        // StateObjectsRegistry::ReportDeletedObject() does not lock the registry, but only
        // atomically increments the outstanding deleted objects counter.
        PipelineStateRegistry.ReportDeletedObject();
        RasterizerStateRegistry.ReportDeletedObject();
        DSSRegistry.ReportDeletedObject();
        */
        VERIFY(m_IsDestructed, "This object must be explicitly destructed with Destruct()");
    }

    void Destruct()
    {
        VERIFY(!m_IsDestructed, "This object has already been destructed");

        if (this->m_Desc.IsAnyGraphicsPipeline() && m_pGraphicsPipelineData != nullptr)
        {
            m_pGraphicsPipelineData->~GraphicsPipelineData();
        }
        else if (this->m_Desc.IsRayTracingPipeline() && m_pRayTracingPipelineData != nullptr)
        {
            m_pRayTracingPipelineData->~RayTracingPipelineData();
        }
        else if (this->m_Desc.IsTilePipeline() && m_pTilePipelineData != nullptr)
        {
            m_pTilePipelineData->~TilePipelineData();
        }

        if (m_Signatures != nullptr)
        {
            for (Uint32 i = 0; i < m_SignatureCount; ++i)
                m_Signatures[i].~SignatureAutoPtrType();
            m_Signatures = nullptr;
        }

        if (m_pPipelineDataRawMem)
        {
            GetRawAllocator().Free(m_pPipelineDataRawMem);
            m_pPipelineDataRawMem = nullptr;
        }
#if DILIGENT_DEBUG
        m_IsDestructed = true;
#endif
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_PipelineState, TDeviceObjectBase)

    virtual const PipelineStateDesc& DILIGENT_CALL_TYPE GetDesc() const override final
    {
        CheckPipelineReady();
        return this->m_Desc;
    }

    Uint32 GetBufferStride(Uint32 BufferSlot) const
    {
        CheckPipelineReady();
        VERIFY_EXPR(this->m_Desc.IsAnyGraphicsPipeline());
        return BufferSlot < m_pGraphicsPipelineData->BufferSlotsUsed ? m_pGraphicsPipelineData->pStrides[BufferSlot] : 0;
    }

    Uint32 GetNumBufferSlotsUsed() const
    {
        CheckPipelineReady();
        VERIFY_EXPR(this->m_Desc.IsAnyGraphicsPipeline());
        return m_pGraphicsPipelineData->BufferSlotsUsed;
    }

    RefCntAutoPtr<IRenderPass> const& GetRenderPassPtr() const
    {
        VERIFY_EXPR(this->m_Desc.IsAnyGraphicsPipeline());
        return m_pGraphicsPipelineData->pRenderPass;
    }

    RefCntAutoPtr<IRenderPass>& GetRenderPassPtr()
    {
        VERIFY_EXPR(this->m_Desc.IsAnyGraphicsPipeline());
        return m_pGraphicsPipelineData->pRenderPass;
    }

    virtual const GraphicsPipelineDesc& DILIGENT_CALL_TYPE GetGraphicsPipelineDesc() const override final
    {
        CheckPipelineReady();
        VERIFY_EXPR(this->m_Desc.IsAnyGraphicsPipeline());
        VERIFY_EXPR(m_pGraphicsPipelineData != nullptr);
        return m_pGraphicsPipelineData->Desc;
    }

    virtual const RayTracingPipelineDesc& DILIGENT_CALL_TYPE GetRayTracingPipelineDesc() const override final
    {
        CheckPipelineReady();
        VERIFY_EXPR(this->m_Desc.IsRayTracingPipeline());
        VERIFY_EXPR(m_pRayTracingPipelineData != nullptr);
        return m_pRayTracingPipelineData->Desc;
    }

    virtual const TilePipelineDesc& DILIGENT_CALL_TYPE GetTilePipelineDesc() const override final
    {
        CheckPipelineReady();
        VERIFY_EXPR(this->m_Desc.IsTilePipeline());
        VERIFY_EXPR(m_pTilePipelineData != nullptr);
        return m_pTilePipelineData->Desc;
    }

    inline void CopyShaderHandle(const char* Name, void* pData, size_t DataSize) const
    {
        VERIFY_EXPR(this->m_Desc.IsRayTracingPipeline());
        VERIFY_EXPR(m_pRayTracingPipelineData != nullptr);

        const auto ShaderHandleSize = m_pRayTracingPipelineData->ShaderHandleSize;
        VERIFY(ShaderHandleSize <= DataSize, "DataSize (", DataSize, ") must be at least as large as the shader handle size (", ShaderHandleSize, ").");

        if (Name == nullptr || Name[0] == '\0')
        {
            // set shader binding to zero to skip shader execution
            std::memset(pData, 0, ShaderHandleSize);
            return;
        }

        auto iter = m_pRayTracingPipelineData->NameToGroupIndex.find(Name);
        if (iter != m_pRayTracingPipelineData->NameToGroupIndex.end())
        {
            VERIFY_EXPR(ShaderHandleSize * (iter->second + 1) <= m_pRayTracingPipelineData->ShaderDataSize);
            std::memcpy(pData, &m_pRayTracingPipelineData->ShaderHandles[ShaderHandleSize * iter->second], ShaderHandleSize);
            return;
        }
        UNEXPECTED("Can't find shader group '", Name, "'.");
    }

    virtual void DILIGENT_CALL_TYPE CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding,
                                                                bool                     InitStaticResources) override final
    {
        CheckPipelineReady();

        *ppShaderResourceBinding = nullptr;

        if (!m_UsingImplicitSignature)
        {
            LOG_ERROR_MESSAGE("IPipelineState::CreateShaderResourceBinding is not allowed for pipelines that use explicit "
                              "resource signatures. Use IPipelineResourceSignature::CreateShaderResourceBinding instead.");
            return;
        }

        return this->GetResourceSignature(0)->CreateShaderResourceBinding(ppShaderResourceBinding, InitStaticResources);
    }

    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByName(SHADER_TYPE ShaderType,
                                                                                const Char* Name) override final
    {
        CheckPipelineReady();

        if (!m_UsingImplicitSignature)
        {
            LOG_ERROR_MESSAGE("IPipelineState::GetStaticVariableByName is not allowed for pipelines that use explicit "
                              "resource signatures. Use IPipelineResourceSignature::GetStaticVariableByName instead.");
            return nullptr;
        }

        if ((m_ActiveShaderStages & ShaderType) == 0)
        {
            LOG_WARNING_MESSAGE("Unable to find static variable '", Name, "' in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is inactive in PSO '", this->m_Desc.Name, "'.");
            return nullptr;
        }

        return this->GetResourceSignature(0)->GetStaticVariableByName(ShaderType, Name);
    }

    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByIndex(SHADER_TYPE ShaderType,
                                                                                 Uint32      Index) override final
    {
        CheckPipelineReady();

        if (!m_UsingImplicitSignature)
        {
            LOG_ERROR_MESSAGE("IPipelineState::GetStaticVariableByIndex is not allowed for pipelines that use explicit "
                              "resource signatures. Use IPipelineResourceSignature::GetStaticVariableByIndex instead.");
            return nullptr;
        }

        if ((m_ActiveShaderStages & ShaderType) == 0)
        {
            LOG_WARNING_MESSAGE("Unable to get static variable at index ", Index, " in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is inactive in PSO '", this->m_Desc.Name, "'.");
            return nullptr;
        }

        return this->GetResourceSignature(0)->GetStaticVariableByIndex(ShaderType, Index);
    }

    virtual Uint32 DILIGENT_CALL_TYPE GetStaticVariableCount(SHADER_TYPE ShaderType) const override final
    {
        CheckPipelineReady();

        if (!m_UsingImplicitSignature)
        {
            LOG_ERROR_MESSAGE("IPipelineState::GetStaticVariableCount is not allowed for pipelines that use explicit "
                              "resource signatures. Use IPipelineResourceSignature::GetStaticVariableCount instead.");
            return 0;
        }

        if ((m_ActiveShaderStages & ShaderType) == 0)
        {
            LOG_WARNING_MESSAGE("Unable to get the number of static variables in shader stage ", GetShaderTypeLiteralName(ShaderType),
                                " as the stage is inactive in PSO '", this->m_Desc.Name, "'.");
            return 0;
        }

        return this->GetResourceSignature(0)->GetStaticVariableCount(ShaderType);
    }

    virtual void DILIGENT_CALL_TYPE BindStaticResources(SHADER_TYPE                 ShaderStages,
                                                        IResourceMapping*           pResourceMapping,
                                                        BIND_SHADER_RESOURCES_FLAGS Flags) override final
    {
        CheckPipelineReady();

        if (!m_UsingImplicitSignature)
        {
            LOG_ERROR_MESSAGE("IPipelineState::BindStaticResources is not allowed for pipelines that use explicit "
                              "resource signatures. Use IPipelineResourceSignature::BindStaticResources instead.");
            return;
        }

        return this->GetResourceSignature(0)->BindStaticResources(ShaderStages, pResourceMapping, Flags);
    }

    virtual void DILIGENT_CALL_TYPE InitializeStaticSRBResources(IShaderResourceBinding* pSRB) const override final
    {
        CheckPipelineReady();

        if (!m_UsingImplicitSignature)
        {
            LOG_ERROR_MESSAGE("IPipelineState::InitializeStaticSRBResources is not allowed for pipelines that use explicit "
                              "resource signatures. Use IPipelineResourceSignature::InitializeStaticSRBResources instead.");
            return;
        }

        return this->GetResourceSignature(0)->InitializeStaticSRBResources(pSRB);
    }

    virtual void DILIGENT_CALL_TYPE CopyStaticResources(IPipelineState* pDstPipeline) const override final
    {
        CheckPipelineReady();

        if (pDstPipeline == nullptr)
        {
            DEV_ERROR("Destination pipeline must not be null");
            return;
        }

        if (pDstPipeline == this)
        {
            DEV_ERROR("Source and destination pipelines must be different");
            return;
        }

        if (!m_UsingImplicitSignature)
        {
            LOG_ERROR_MESSAGE("IPipelineState::CopyStaticResources is not allowed for pipelines that use explicit "
                              "resource signatures. Use IPipelineResourceSignature::CopyStaticResources instead.");
            return;
        }

        auto* pDstSign = static_cast<PipelineStateImplType*>(pDstPipeline)->GetResourceSignature(0);
        return this->GetResourceSignature(0)->CopyStaticResources(pDstSign);
    }

    /// Implementation of IPipelineState::GetResourceSignatureCount().
    virtual Uint32 DILIGENT_CALL_TYPE GetResourceSignatureCount() const override final
    {
        CheckPipelineReady();
        return m_SignatureCount;
    }

    /// Implementation of IPipelineState::GetResourceSignature().
    virtual PipelineResourceSignatureImplType* DILIGENT_CALL_TYPE GetResourceSignature(Uint32 Index) const override final
    {
        CheckPipelineReady();
        VERIFY_EXPR(Index < m_SignatureCount);
        return m_Signatures[Index];
    }

    /// Implementation of IPipelineState::IsCompatibleWith().
    virtual bool DILIGENT_CALL_TYPE IsCompatibleWith(const IPipelineState* pPSO) const override // May be overridden
    {
        CheckPipelineReady();
        DEV_CHECK_ERR(pPSO != nullptr, "pPSO must not be null");

        if (pPSO == this)
            return true;

        RefCntAutoPtr<PipelineStateImplType> pPSOImpl{const_cast<IPipelineState*>(pPSO), PipelineStateImplType::IID_InternalImpl};
        VERIFY(pPSOImpl, "Unknown PSO implementation type");

        const auto& lhs = *static_cast<const PipelineStateImplType*>(this);
        const auto& rhs = *pPSOImpl;

        const auto SignCount = lhs.GetResourceSignatureCount();
        if (SignCount != rhs.GetResourceSignatureCount())
            return false;

        for (Uint32 s = 0; s < SignCount; ++s)
        {
            const auto* pLhsSign = GetResourceSignature(s);
            const auto* pRhsSign = rhs.GetResourceSignature(s);
            if (!PipelineResourceSignatureImplType::SignaturesCompatible(pLhsSign, pRhsSign))
                return false;
        }

        return true;
    }

    virtual PIPELINE_STATE_STATUS DILIGENT_CALL_TYPE GetStatus(bool WaitForCompletion = false) override
    {
        VERIFY_EXPR(m_Status.load() != PIPELINE_STATE_STATUS_UNINITIALIZED);
        ASYNC_TASK_STATUS InitTaskStatus = AsyncInitializer::Update(m_AsyncInitializer, WaitForCompletion);
        if (InitTaskStatus == ASYNC_TASK_STATUS_COMPLETE)
        {
            VERIFY(m_Status.load() > PIPELINE_STATE_STATUS_COMPILING, "Pipeline state status must be atomically set by the initialization task before it finishes");
        }
        return m_Status.load();
    }

    SHADER_TYPE GetActiveShaderStages() const
    {
        return m_ActiveShaderStages;
    }

protected:
    using TNameToGroupIndexMap = std::unordered_map<HashMapStringKey, Uint32>;

    void ReserveSpaceForPipelineDesc(const GraphicsPipelineStateCreateInfo& CreateInfo,
                                     FixedLinearAllocator&                  MemPool) noexcept
    {
        MemPool.AddSpace<GraphicsPipelineData>();
        ReserveResourceLayout(CreateInfo.PSODesc.ResourceLayout, MemPool);
        ReserveResourceSignatures(CreateInfo, MemPool);

        const auto& InputLayout = CreateInfo.GraphicsPipeline.InputLayout;
        if (InputLayout.NumElements > 0)
        {
            Uint32 BufferSlotsUsed = 0;
            MemPool.AddSpace<LayoutElement>(InputLayout.NumElements);
            for (Uint32 i = 0; i < InputLayout.NumElements; ++i)
            {
                auto& LayoutElem = InputLayout.LayoutElements[i];
                MemPool.AddSpaceForString(LayoutElem.HLSLSemantic);
                BufferSlotsUsed = std::max(BufferSlotsUsed, LayoutElem.BufferSlot + 1);
            }

            MemPool.AddSpace<Uint32>(BufferSlotsUsed);
        }

        static_assert(std::is_trivially_destructible<decltype(*InputLayout.LayoutElements)>::value, "Add destructor for this object to Destruct()");
    }

    void ReserveSpaceForPipelineDesc(const ComputePipelineStateCreateInfo& CreateInfo,
                                     FixedLinearAllocator&                 MemPool) noexcept
    {
        ReserveResourceLayout(CreateInfo.PSODesc.ResourceLayout, MemPool);
        ReserveResourceSignatures(CreateInfo, MemPool);
    }

    void ReserveSpaceForPipelineDesc(const RayTracingPipelineStateCreateInfo& CreateInfo,
                                     FixedLinearAllocator&                    MemPool) noexcept
    {
        size_t RTDataSize = sizeof(RayTracingPipelineData);
        // Reserve space for shader handles
        const auto ShaderHandleSize = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        RTDataSize += size_t{ShaderHandleSize} *
            (size_t{CreateInfo.GeneralShaderCount} +
             size_t{CreateInfo.TriangleHitShaderCount} +
             size_t{CreateInfo.ProceduralHitShaderCount});
        // Extra bytes were reserved to avoid compiler errors on zero-sized arrays
        RTDataSize -= sizeof(RayTracingPipelineData::ShaderHandles);
        MemPool.AddSpace(RTDataSize, alignof(RayTracingPipelineData));

        for (Uint32 i = 0; i < CreateInfo.GeneralShaderCount; ++i)
        {
            MemPool.AddSpaceForString(CreateInfo.pGeneralShaders[i].Name);
        }
        for (Uint32 i = 0; i < CreateInfo.TriangleHitShaderCount; ++i)
        {
            MemPool.AddSpaceForString(CreateInfo.pTriangleHitShaders[i].Name);
        }
        for (Uint32 i = 0; i < CreateInfo.ProceduralHitShaderCount; ++i)
        {
            MemPool.AddSpaceForString(CreateInfo.pProceduralHitShaders[i].Name);
        }

        ReserveResourceLayout(CreateInfo.PSODesc.ResourceLayout, MemPool);
        ReserveResourceSignatures(CreateInfo, MemPool);
    }

    void ReserveSpaceForPipelineDesc(const TilePipelineStateCreateInfo& CreateInfo,
                                     FixedLinearAllocator&              MemPool) noexcept
    {
        MemPool.AddSpace<TilePipelineData>();
        ReserveResourceLayout(CreateInfo.PSODesc.ResourceLayout, MemPool);
        ReserveResourceSignatures(CreateInfo, MemPool);
    }

public:
protected:
    template <typename ShaderImplType, typename PSOCreateInfoType>
    void Construct(const PSOCreateInfoType& CreateInfo)
    {
        auto* const pThisImpl = static_cast<PipelineStateImplType*>(this);

        m_Status.store(PIPELINE_STATE_STATUS_COMPILING);
        if ((CreateInfo.Flags & PSO_CREATE_FLAG_ASYNCHRONOUS) != 0 && this->m_pDevice->GetShaderCompilationThreadPool() != nullptr)
        {
            // Collect all asynchronous shader compile tasks
            typename PipelineStateImplType::TShaderStages ShaderStages;
            SHADER_TYPE                                   ActiveShaderStages    = SHADER_TYPE_UNKNOWN;
            constexpr bool                                WaitUntilShadersReady = false;
            PipelineStateUtils::ExtractShaders<ShaderImplType>(CreateInfo, ShaderStages, WaitUntilShadersReady, ActiveShaderStages);

            std::vector<const ShaderImplType*> Shaders;
            for (const auto& Stage : ShaderStages)
            {
                std::vector<const ShaderImplType*> StageShaders = GetStageShaders(Stage);
                Shaders.insert(Shaders.end(), StageShaders.begin(), StageShaders.end());
            }

            std::vector<RefCntAutoPtr<IAsyncTask>> ShaderCompileTasks;
            for (const ShaderImplType* pShader : Shaders)
            {
                if (RefCntAutoPtr<IAsyncTask> pCompileTask = pShader->GetCompileTask())
                    ShaderCompileTasks.emplace_back(std::move(pCompileTask));
            }

            m_AsyncInitializer = AsyncInitializer::Start(
                this->m_pDevice->GetShaderCompilationThreadPool(),
                ShaderCompileTasks, // Make sure that all asynchronous shader compile tasks are completed first
                [pThisImpl,
#ifdef DILIGENT_DEBUG
                 Shaders,
#endif
                 CreateInfo = typename PipelineStateCreateInfoXTraits<PSOCreateInfoType>::CreateInfoXType{CreateInfo}](Uint32 ThreadId) mutable //
                {
#ifdef DILIGENT_DEBUG
                    for (const ShaderImplType* pShader : Shaders)
                    {
                        VERIFY(!pShader->IsCompiling(), "All shader compile tasks must have been completed since we used them as "
                                                        "prerequisites for the pipeline initialization task. This appears to be a bug.");
                    }
#endif
                    try
                    {
                        pThisImpl->InitializePipeline(CreateInfo);
                        pThisImpl->m_Status.store(PIPELINE_STATE_STATUS_READY);
                    }
                    catch (...)
                    {
                        pThisImpl->m_Status.store(PIPELINE_STATE_STATUS_FAILED);
                    }

                    // Release create info objects
                    CreateInfo.Clear();
                });
        }
        else
        {
            try
            {
                pThisImpl->InitializePipeline(CreateInfo);
                m_Status.store(PIPELINE_STATE_STATUS_READY);
            }
            catch (...)
            {
                pThisImpl->Destruct();
                throw;
            }
        }
    }

    template <typename ShaderImplType, typename PSOCreateInfoType, typename TShaderStages>
    void ExtractShaders(const PSOCreateInfoType& PSOCreateInfo,
                        TShaderStages&           ShaderStages,
                        bool                     WaitUntilShadersReady = false)
    {
        VERIFY_EXPR(this->m_Desc.PipelineType == PSOCreateInfo.PSODesc.PipelineType);
        PipelineStateUtils::ExtractShaders<ShaderImplType>(PSOCreateInfo, ShaderStages, WaitUntilShadersReady, m_ActiveShaderStages);
    }

    void InitializePipelineDesc(const GraphicsPipelineStateCreateInfo& CreateInfo,
                                FixedLinearAllocator&                  MemPool)
    {
        this->m_pGraphicsPipelineData = MemPool.Construct<GraphicsPipelineData>();
        void* Ptr                     = MemPool.ReleaseOwnership();
        VERIFY_EXPR(Ptr == m_pPipelineDataRawMem);

        auto& GraphicsPipeline = this->m_pGraphicsPipelineData->Desc;
        auto& pRenderPass      = this->m_pGraphicsPipelineData->pRenderPass;
        auto& BufferSlotsUsed  = this->m_pGraphicsPipelineData->BufferSlotsUsed;
        auto& pStrides         = this->m_pGraphicsPipelineData->pStrides;

        GraphicsPipeline = CreateInfo.GraphicsPipeline;
        CorrectGraphicsPipelineDesc(GraphicsPipeline, this->GetDevice()->GetDeviceInfo().Features);

        CopyResourceLayout(CreateInfo.PSODesc.ResourceLayout, this->m_Desc.ResourceLayout, MemPool);
        CopyResourceSignatures(CreateInfo, MemPool);

        pRenderPass = GraphicsPipeline.pRenderPass;
        if (pRenderPass)
        {
            const auto& RPDesc = pRenderPass->GetDesc();
            VERIFY_EXPR(GraphicsPipeline.SubpassIndex < RPDesc.SubpassCount);
            const auto& Subpass = pRenderPass.template RawPtr<RenderPassImplType>()->GetSubpass(GraphicsPipeline.SubpassIndex);

            GraphicsPipeline.NumRenderTargets = static_cast<Uint8>(Subpass.RenderTargetAttachmentCount);
            for (Uint32 rt = 0; rt < Subpass.RenderTargetAttachmentCount; ++rt)
            {
                const auto& RTAttachmentRef = Subpass.pRenderTargetAttachments[rt];
                if (RTAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
                {
                    VERIFY_EXPR(RTAttachmentRef.AttachmentIndex < RPDesc.AttachmentCount);
                    GraphicsPipeline.RTVFormats[rt] = RPDesc.pAttachments[RTAttachmentRef.AttachmentIndex].Format;
                }
            }

            if (Subpass.pDepthStencilAttachment != nullptr)
            {
                const auto& DSAttachmentRef = *Subpass.pDepthStencilAttachment;
                if (DSAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
                {
                    VERIFY_EXPR(DSAttachmentRef.AttachmentIndex < RPDesc.AttachmentCount);
                    GraphicsPipeline.DSVFormat = RPDesc.pAttachments[DSAttachmentRef.AttachmentIndex].Format;
                }
            }
        }

        const auto&    InputLayout     = GraphicsPipeline.InputLayout;
        LayoutElement* pLayoutElements = nullptr;
        if (InputLayout.NumElements > 0)
        {
            pLayoutElements = MemPool.ConstructArray<LayoutElement>(InputLayout.NumElements);
            for (size_t Elem = 0; Elem < InputLayout.NumElements; ++Elem)
            {
                const auto& SrcElem   = InputLayout.LayoutElements[Elem];
                pLayoutElements[Elem] = SrcElem;
                VERIFY_EXPR(SrcElem.HLSLSemantic != nullptr);
                pLayoutElements[Elem].HLSLSemantic = MemPool.CopyString(SrcElem.HLSLSemantic);
            }

            // Correct description and compute offsets and tight strides
            const auto Strides = ResolveInputLayoutAutoOffsetsAndStrides(pLayoutElements, InputLayout.NumElements);
            BufferSlotsUsed    = static_cast<Uint8>(Strides.size());

            pStrides = MemPool.CopyConstructArray<Uint32>(Strides.data(), BufferSlotsUsed);
        }
        GraphicsPipeline.InputLayout.LayoutElements = pLayoutElements;
    }

    void InitializePipelineDesc(const ComputePipelineStateCreateInfo& CreateInfo,
                                FixedLinearAllocator&                 MemPool)
    {
        m_pPipelineDataRawMem = MemPool.ReleaseOwnership();

        CopyResourceLayout(CreateInfo.PSODesc.ResourceLayout, this->m_Desc.ResourceLayout, MemPool);
        CopyResourceSignatures(CreateInfo, MemPool);
    }

    void InitializePipelineDesc(const RayTracingPipelineStateCreateInfo& CreateInfo,
                                FixedLinearAllocator&                    MemPool) noexcept
    {
        size_t RTDataSize = sizeof(RayTracingPipelineData);
        // Allocate space for shader handles
        const auto ShaderHandleSize = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        const auto ShaderDataSize   = ShaderHandleSize * (CreateInfo.GeneralShaderCount + CreateInfo.TriangleHitShaderCount + CreateInfo.ProceduralHitShaderCount);
        RTDataSize += ShaderDataSize;
        // Extra bytes were reserved to avoid compiler errors on zero-sized arrays
        RTDataSize -= sizeof(RayTracingPipelineData::ShaderHandles);

        this->m_pRayTracingPipelineData = static_cast<RayTracingPipelineData*>(MemPool.Allocate(RTDataSize, alignof(RayTracingPipelineData)));
        new (this->m_pRayTracingPipelineData) RayTracingPipelineData{};
        this->m_pRayTracingPipelineData->ShaderHandleSize = ShaderHandleSize;
        this->m_pRayTracingPipelineData->Desc             = CreateInfo.RayTracingPipeline;
        this->m_pRayTracingPipelineData->ShaderDataSize   = ShaderDataSize;

        void* Ptr = MemPool.ReleaseOwnership();
        VERIFY_EXPR(Ptr == m_pPipelineDataRawMem);

        TNameToGroupIndexMap& NameToGroupIndex = this->m_pRayTracingPipelineData->NameToGroupIndex;
        CopyRTShaderGroupNames(NameToGroupIndex, CreateInfo, MemPool);

        CopyResourceLayout(CreateInfo.PSODesc.ResourceLayout, this->m_Desc.ResourceLayout, MemPool);
        CopyResourceSignatures(CreateInfo, MemPool);
    }

    void InitializePipelineDesc(const TilePipelineStateCreateInfo& CreateInfo,
                                FixedLinearAllocator&              MemPool)
    {
        this->m_pTilePipelineData = MemPool.Construct<TilePipelineData>();
        void* Ptr                 = MemPool.ReleaseOwnership();
        VERIFY_EXPR(Ptr == m_pPipelineDataRawMem);

        this->m_pTilePipelineData->Desc = CreateInfo.TilePipeline;

        CopyResourceLayout(CreateInfo.PSODesc.ResourceLayout, this->m_Desc.ResourceLayout, MemPool);
        CopyResourceSignatures(CreateInfo, MemPool);
    }

    // Resource attribution properties
    struct ResourceAttribution
    {
        static constexpr Uint32 InvalidSignatureIndex = ~0u;
        static constexpr Uint32 InvalidResourceIndex  = InvalidPipelineResourceIndex;
        static constexpr Uint32 InvalidSamplerIndex   = InvalidImmutableSamplerIndex;

        const PipelineResourceSignatureImplType* pSignature = nullptr;

        Uint32 SignatureIndex        = InvalidSignatureIndex;
        Uint32 ResourceIndex         = InvalidResourceIndex;
        Uint32 ImmutableSamplerIndex = InvalidSamplerIndex;

        ResourceAttribution() noexcept {}
        ResourceAttribution(const PipelineResourceSignatureImplType* _pSignature,
                            Uint32                                   _SignatureIndex,
                            Uint32                                   _ResourceIndex,
                            Uint32                                   _ImmutableSamplerIndex = InvalidResourceIndex) noexcept :
            pSignature{_pSignature},
            SignatureIndex{_SignatureIndex},
            ResourceIndex{_ResourceIndex},
            ImmutableSamplerIndex{_ImmutableSamplerIndex}
        {
            VERIFY_EXPR(pSignature == nullptr || pSignature->GetDesc().BindingIndex == SignatureIndex);
            VERIFY_EXPR((ResourceIndex == InvalidResourceIndex) || (ImmutableSamplerIndex == InvalidSamplerIndex));
        }

        explicit operator bool() const
        {
            return ((SignatureIndex != InvalidSignatureIndex) &&
                    (ResourceIndex != InvalidResourceIndex || ImmutableSamplerIndex != InvalidSamplerIndex));
        }

        bool IsImmutableSampler() const
        {
            return operator bool() && ImmutableSamplerIndex != InvalidSamplerIndex;
        }
    };

    template <typename PipelineResourceSignatureImplPtrType>
    static ResourceAttribution GetResourceAttribution(const char*                                Name,
                                                      SHADER_TYPE                                Stage,
                                                      const PipelineResourceSignatureImplPtrType pSignatures[],
                                                      Uint32                                     SignCount)
    {
        VERIFY_EXPR(Name != nullptr && Name[0] != '\0');
        for (Uint32 sign = 0; sign < SignCount; ++sign)
        {
            const PipelineResourceSignatureImplType* const pSignature = pSignatures[sign];
            if (pSignature == nullptr)
                continue;

            const auto ResIndex = pSignature->FindResource(Stage, Name);
            if (ResIndex != ResourceAttribution::InvalidResourceIndex)
                return ResourceAttribution{pSignature, sign, ResIndex};
            else
            {
                const auto ImtblSamIndex = pSignature->FindImmutableSampler(Stage, Name);
                if (ImtblSamIndex != ResourceAttribution::InvalidSamplerIndex)
                    return ResourceAttribution{pSignature, sign, ResourceAttribution::InvalidResourceIndex, ImtblSamIndex};
            }
        }
        return ResourceAttribution{};
    }

    ResourceAttribution GetResourceAttribution(const char* Name, SHADER_TYPE Stage) const
    {
        const auto* const pThis = static_cast<const PipelineStateImplType*>(this);
        return GetResourceAttribution(Name, Stage, pThis->m_Signatures, pThis->m_SignatureCount);
    }

    template <typename... ExtraArgsType>
    void InitDefaultSignature(const PipelineResourceSignatureDesc& SignDesc,
                              const ExtraArgsType&... ExtraArgs)
    {
        VERIFY_EXPR(m_SignatureCount == 1 && m_UsingImplicitSignature);

        RefCntAutoPtr<PipelineResourceSignatureImplType> pImplicitSignature;
        this->GetDevice()->CreatePipelineResourceSignature(SignDesc, pImplicitSignature.template DblPtr<IPipelineResourceSignature>(), ExtraArgs...);

        if (!pImplicitSignature)
            LOG_ERROR_AND_THROW("Failed to create implicit resource signature for pipeline state '", this->m_Desc.Name, "'.");

        VERIFY_EXPR(pImplicitSignature->GetDesc().BindingIndex == 0);
        VERIFY(!m_Signatures[0], "Signature 0 has already been initialized.");
        m_Signatures[0] = std::move(pImplicitSignature);
    }

    static PSO_CREATE_INTERNAL_FLAGS GetInternalCreateFlags(const PipelineStateCreateInfo& CreateInfo)
    {
        const auto* pInternalCI = static_cast<PSOCreateInternalInfo*>(CreateInfo.pInternalData);
        return pInternalCI != nullptr ? pInternalCI->Flags : PSO_CREATE_INTERNAL_FLAG_NONE;
    }

private:
    void CheckPipelineReady() const
    {
        // It is OK to use m_Desc.Name as it is initialized by DeviceObjectBase
        DEV_CHECK_ERR(m_Status.load() == PIPELINE_STATE_STATUS_READY, "Pipeline state '", this->m_Desc.Name,
                      "' is expected to be Ready, but its actual status is ", GetPipelineStateStatusString(m_Status.load()),
                      ". Use GetStatus() to check the pipeline state status.");
    }

    static void ReserveResourceLayout(const PipelineResourceLayoutDesc& SrcLayout, FixedLinearAllocator& MemPool) noexcept
    {
        if (SrcLayout.Variables != nullptr)
        {
            MemPool.AddSpace<ShaderResourceVariableDesc>(SrcLayout.NumVariables);
            for (Uint32 i = 0; i < SrcLayout.NumVariables; ++i)
            {
                VERIFY(SrcLayout.Variables[i].Name != nullptr, "Variable name can't be null");
                MemPool.AddSpaceForString(SrcLayout.Variables[i].Name);
            }
        }

        if (SrcLayout.ImmutableSamplers != nullptr)
        {
            MemPool.AddSpace<ImmutableSamplerDesc>(SrcLayout.NumImmutableSamplers);
            for (Uint32 i = 0; i < SrcLayout.NumImmutableSamplers; ++i)
            {
                VERIFY(SrcLayout.ImmutableSamplers[i].SamplerOrTextureName != nullptr, "Immutable sampler or texture name can't be null");
                MemPool.AddSpaceForString(SrcLayout.ImmutableSamplers[i].SamplerOrTextureName);
            }
        }

        static_assert(std::is_trivially_destructible<decltype(*SrcLayout.Variables)>::value, "Add destructor for this object to Destruct()");
        static_assert(std::is_trivially_destructible<decltype(*SrcLayout.ImmutableSamplers)>::value, "Add destructor for this object to Destruct()");
    }

    static void CopyResourceLayout(const PipelineResourceLayoutDesc& SrcLayout, PipelineResourceLayoutDesc& DstLayout, FixedLinearAllocator& MemPool)
    {
        if (SrcLayout.Variables != nullptr)
        {
            auto* const Variables = MemPool.ConstructArray<ShaderResourceVariableDesc>(SrcLayout.NumVariables);
            DstLayout.Variables   = Variables;
            for (Uint32 i = 0; i < SrcLayout.NumVariables; ++i)
            {
                const auto& SrcVar = SrcLayout.Variables[i];
                Variables[i]       = SrcVar;
                Variables[i].Name  = MemPool.CopyString(SrcVar.Name);
            }
        }

        if (SrcLayout.ImmutableSamplers != nullptr)
        {
            auto* const ImmutableSamplers = MemPool.ConstructArray<ImmutableSamplerDesc>(SrcLayout.NumImmutableSamplers);
            DstLayout.ImmutableSamplers   = ImmutableSamplers;
            for (Uint32 i = 0; i < SrcLayout.NumImmutableSamplers; ++i)
            {
                const auto& SrcSmplr = SrcLayout.ImmutableSamplers[i];
#ifdef DILIGENT_DEVELOPMENT
                {
                    const auto& BorderColor = SrcSmplr.Desc.BorderColor;
                    if (!((BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 0) ||
                          (BorderColor[0] == 0 && BorderColor[1] == 0 && BorderColor[2] == 0 && BorderColor[3] == 1) ||
                          (BorderColor[0] == 1 && BorderColor[1] == 1 && BorderColor[2] == 1 && BorderColor[3] == 1)))
                    {
                        LOG_WARNING_MESSAGE("Immutable sampler for variable \"", SrcSmplr.SamplerOrTextureName, "\" specifies border color (",
                                            BorderColor[0], ", ", BorderColor[1], ", ", BorderColor[2], ", ", BorderColor[3],
                                            "). D3D12 static samplers only allow transparent black (0,0,0,0), opaque black (0,0,0,1) or opaque white (1,1,1,1) as border colors");
                    }
                }
#endif

                ImmutableSamplers[i]                      = SrcSmplr;
                ImmutableSamplers[i].SamplerOrTextureName = MemPool.CopyString(SrcSmplr.SamplerOrTextureName);
            }
        }
    }

    void ReserveResourceSignatures(const PipelineStateCreateInfo& CreateInfo, FixedLinearAllocator& MemPool)
    {
        if (m_UsingImplicitSignature && (GetInternalCreateFlags(CreateInfo) & PSO_CREATE_INTERNAL_FLAG_IMPLICIT_SIGNATURE0) == 0)
        {
            VERIFY_EXPR(CreateInfo.ResourceSignaturesCount == 0 || CreateInfo.ppResourceSignatures == nullptr);
            m_SignatureCount = 1;
        }
        else
        {
            VERIFY_EXPR(CreateInfo.ResourceSignaturesCount > 0 && CreateInfo.ppResourceSignatures != nullptr);
            Uint32 MaxSignatureBindingIndex = 0;
            for (Uint32 i = 0; i < CreateInfo.ResourceSignaturesCount; ++i)
            {
                const auto* pSignature = ClassPtrCast<PipelineResourceSignatureImplType>(CreateInfo.ppResourceSignatures[i]);
                VERIFY(pSignature != nullptr, "Pipeline resource signature at index ", i, " is null. This error should've been caught by ValidatePipelineResourceSignatures.");

                Uint32 Index = pSignature->GetDesc().BindingIndex;
                VERIFY(Index < MAX_RESOURCE_SIGNATURES,
                       "Pipeline resource signature specifies binding index ", Uint32{Index}, " that exceeds the limit (", MAX_RESOURCE_SIGNATURES - 1,
                       "). This error should've been caught by ValidatePipelineResourceSignatureDesc.");

                MaxSignatureBindingIndex = std::max(MaxSignatureBindingIndex, Uint32{Index});
            }
            VERIFY_EXPR(MaxSignatureBindingIndex < MAX_RESOURCE_SIGNATURES);
            m_SignatureCount = static_cast<decltype(m_SignatureCount)>(MaxSignatureBindingIndex + 1);
            VERIFY_EXPR(m_SignatureCount == MaxSignatureBindingIndex + 1);
        }

        MemPool.AddSpace<SignatureAutoPtrType>(m_SignatureCount);
    }

    void CopyResourceSignatures(const PipelineStateCreateInfo& CreateInfo, FixedLinearAllocator& MemPool)
    {
        m_Signatures = MemPool.ConstructArray<SignatureAutoPtrType>(m_SignatureCount);
        if (!m_UsingImplicitSignature || (GetInternalCreateFlags(CreateInfo) & PSO_CREATE_INTERNAL_FLAG_IMPLICIT_SIGNATURE0) != 0)
        {
            VERIFY_EXPR(CreateInfo.ResourceSignaturesCount != 0 && CreateInfo.ppResourceSignatures != nullptr);
            for (Uint32 i = 0; i < CreateInfo.ResourceSignaturesCount; ++i)
            {
                auto* pSignature = ClassPtrCast<PipelineResourceSignatureImplType>(CreateInfo.ppResourceSignatures[i]);
                VERIFY_EXPR(pSignature != nullptr);

                const Uint32 Index = pSignature->GetDesc().BindingIndex;

#ifdef DILIGENT_DEBUG
                VERIFY_EXPR(Index < m_SignatureCount);

                VERIFY(m_Signatures[Index] == nullptr,
                       "Pipeline resource signature '", pSignature->GetDesc().Name, "' at index ", Uint32{Index},
                       " conflicts with another resource signature '", m_Signatures[Index]->GetDesc().Name,
                       "' that uses the same index. This error should've been caught by ValidatePipelineResourceSignatures.");

                for (Uint32 s = 0, StageCount = pSignature->GetNumActiveShaderStages(); s < StageCount; ++s)
                {
                    const auto ShaderType = pSignature->GetActiveShaderStageType(s);
                    VERIFY(IsConsistentShaderType(ShaderType, CreateInfo.PSODesc.PipelineType),
                           "Pipeline resource signature '", pSignature->GetDesc().Name, "' at index ", Uint32{Index},
                           " has shader stage '", GetShaderTypeLiteralName(ShaderType), "' that is not compatible with pipeline type '",
                           GetPipelineTypeString(CreateInfo.PSODesc.PipelineType), "'.");
                }
#endif

                m_Signatures[Index] = pSignature;
            }
        }
    }

protected:
    std::unique_ptr<AsyncInitializer> m_AsyncInitializer;

    /// Shader stages that are active in this PSO.
    SHADER_TYPE m_ActiveShaderStages = SHADER_TYPE_UNKNOWN;

    /// True if the pipeline was created using implicit root signature.
    const bool m_UsingImplicitSignature;

    std::atomic<PIPELINE_STATE_STATUS> m_Status{PIPELINE_STATE_STATUS_UNINITIALIZED};

    /// The number of signatures in m_Signatures array.
    /// Note that this is not necessarily the same as the number of signatures
    /// that were used to create the pipeline, because signatures are arranged
    /// by their binding index.
    Uint8 m_SignatureCount = 0;

    /// Resource signatures arranged by their binding indices
    using SignatureAutoPtrType         = RefCntAutoPtr<PipelineResourceSignatureImplType>;
    SignatureAutoPtrType* m_Signatures = nullptr; // [m_SignatureCount]

    struct GraphicsPipelineData
    {
        GraphicsPipelineDesc Desc;

        RefCntAutoPtr<IRenderPass> pRenderPass; ///< Strong reference to the render pass object

        Uint32* pStrides        = nullptr;
        Uint8   BufferSlotsUsed = 0;
    };

    struct RayTracingPipelineData
    {
        RayTracingPipelineDesc Desc;

        // Mapping from the shader group name to its index in the pipeline.
        // It is used to find the shader handle in ShaderHandles array.
        TNameToGroupIndexMap NameToGroupIndex;

        Uint32 ShaderHandleSize = 0;
        Uint32 ShaderDataSize   = 0;

        // Array of shader handles for every group in the pipeline.
        // The handles will be copied to the SBT using NameToGroupIndex to find
        // handles by group name.
        // The actual array size will be determined at run time and will be stored
        // in ShaderDataSize.
        Uint8 ShaderHandles[sizeof(void*)] = {};
    };
    static_assert(offsetof(RayTracingPipelineData, ShaderHandles) % sizeof(void*) == 0, "ShaderHandles member is expected to be sizeof(void*)-aligned");

    struct TilePipelineData
    {
        TilePipelineDesc Desc;
    };

    union
    {
        GraphicsPipelineData*   m_pGraphicsPipelineData;
        RayTracingPipelineData* m_pRayTracingPipelineData;
        TilePipelineData*       m_pTilePipelineData;
        void*                   m_pPipelineDataRawMem = nullptr;
    };

#ifdef DILIGENT_DEBUG
    bool m_IsDestructed = false;
#endif
};

} // namespace Diligent
