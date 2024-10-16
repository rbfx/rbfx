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

#include "ReloadablePipelineState.hpp"

#include "RenderStateCacheImpl.hpp"
#include "ReloadableShader.hpp"
#include "GraphicsTypesX.hpp"

namespace Diligent
{

constexpr INTERFACE_ID ReloadablePipelineState::IID_InternalImpl;

struct ReloadablePipelineState::CreateInfoWrapperBase
{
    virtual ~CreateInfoWrapperBase() {}
};

template <typename CreateInfoType>
struct ReloadablePipelineState::CreateInfoWrapper : CreateInfoWrapperBase
{
    CreateInfoWrapper(const CreateInfoType& CI) :
        m_CI{CI}
    {
        ProcessPipelineStateCreateInfoShaders(static_cast<const CreateInfoType&>(m_CI), [](IShader* pShader) {
            if (pShader == nullptr)
                return;

            if (!RefCntAutoPtr<IShader>{pShader, ReloadableShader::IID_InternalImpl})
            {
                const auto* Name = pShader->GetDesc().Name;
                LOG_WARNING_MESSAGE("Shader '", (Name ? Name : "<unnamed>"),
                                    "' is not a reloadable shader. To enable hot pipeline state reload, all shaders must be created through the render state cache.");
            }
        });
    }

    const CreateInfoType& Get() const
    {
        return m_CI;
    }

    operator const CreateInfoType&() const
    {
        return m_CI;
    }

    operator CreateInfoType&()
    {
        return m_CI;
    }

protected:
    typename PipelineStateCreateInfoXTraits<CreateInfoType>::CreateInfoXType m_CI;
};

ReloadablePipelineState::ReloadablePipelineState(IReferenceCounters*            pRefCounters,
                                                 RenderStateCacheImpl*          pStateCache,
                                                 IPipelineState*                pPipeline,
                                                 const PipelineStateCreateInfo& CreateInfo) :
    TBase{pRefCounters},
    m_pStateCache{pStateCache},
    m_Type{CreateInfo.PSODesc.PipelineType}
{
    m_pPipeline = pPipeline;

    static_assert(PIPELINE_TYPE_COUNT == 5, "Did you add a new pipeline type? You may need to handle it here.");
    switch (CreateInfo.PSODesc.PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
        case PIPELINE_TYPE_MESH:
            m_pCreateInfo = std::make_unique<CreateInfoWrapper<GraphicsPipelineStateCreateInfo>>(static_cast<const GraphicsPipelineStateCreateInfo&>(CreateInfo));
            break;

        case PIPELINE_TYPE_COMPUTE:
            m_pCreateInfo = std::make_unique<CreateInfoWrapper<ComputePipelineStateCreateInfo>>(static_cast<const ComputePipelineStateCreateInfo&>(CreateInfo));
            break;

        case PIPELINE_TYPE_RAY_TRACING:
            m_pCreateInfo = std::make_unique<CreateInfoWrapper<RayTracingPipelineStateCreateInfo>>(static_cast<const RayTracingPipelineStateCreateInfo&>(CreateInfo));
            break;

        case PIPELINE_TYPE_TILE:
            m_pCreateInfo = std::make_unique<CreateInfoWrapper<TilePipelineStateCreateInfo>>(static_cast<const TilePipelineStateCreateInfo&>(CreateInfo));
            break;

        default:
            UNEXPECTED("Unexpected pipeline type");
    }
}


ReloadablePipelineState::~ReloadablePipelineState()
{
}

void ReloadablePipelineState::QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)
{
    if (ppInterface == nullptr)
        return;
    DEV_CHECK_ERR(*ppInterface == nullptr, "Overwriting reference to an existing object may result in memory leaks");
    *ppInterface = nullptr;

    if (IID == IID_InternalImpl || IID == IID_PipelineState || IID == IID_DeviceObject || IID == IID_Unknown)
    {
        *ppInterface = this;
        (*ppInterface)->AddRef();
    }
    else
    {
        // This will handle implementation-specific interfaces such as PipelineStateD3D11Impl::IID_InternalImpl,
        // PipelineStateD3D12Impl::IID_InternalImpl, etc. requested by e.g. device context implementations
        // (DeviceContextD3D11Impl::SetPipelineState, DeviceContextD3D12Impl::SetPipelineState, etc.)
        m_pPipeline->QueryInterface(IID, ppInterface);
    }
}

template <typename CreateInfoType>
void ModifyPsoCreateInfo(CreateInfoType& PsoCreateInfo, ReloadGraphicsPipelineCallbackType ReloadGraphicsPipeline, void* pUserData)
{
}

template <>
void ModifyPsoCreateInfo(GraphicsPipelineStateCreateInfo& PsoCreateInfo, ReloadGraphicsPipelineCallbackType ReloadGraphicsPipeline, void* pUserData)
{
    if (ReloadGraphicsPipeline != nullptr)
    {
        ReloadGraphicsPipeline(PsoCreateInfo.PSODesc.Name, PsoCreateInfo.GraphicsPipeline, pUserData);
    }
}

template <typename CreateInfoType>
bool ReloadablePipelineState::Reload(ReloadGraphicsPipelineCallbackType ReloadGraphicsPipeline, void* pUserData)
{
    auto& CreateInfo = static_cast<CreateInfoWrapper<CreateInfoType>&>(*m_pCreateInfo);
    ModifyPsoCreateInfo<CreateInfoType>(static_cast<CreateInfoType&>(CreateInfo), ReloadGraphicsPipeline, pUserData);

    RefCntAutoPtr<IPipelineState> pNewPSO;

    // Note that the create info struct references reloadable shaders, so that the pipeline will use the updated shaders
    const auto FoundInCache = m_pStateCache->CreatePipelineStateInternal(static_cast<const CreateInfoType&>(CreateInfo), &pNewPSO);

    if (pNewPSO)
    {
        if (m_pPipeline != pNewPSO)
        {
            // Do not update old pipeline if it is not null.
            // If multiple reloads are requested, we need to keep the original pipeline that keeps the original resources.
            if (!m_pOldPipeline)
            {
                m_pOldPipeline = m_pPipeline;
            }
            m_pPipeline = pNewPSO;

            // If any of the pipelines is not ready, we will copy static resources when both are ready in GetStatus()
            if (m_pPipeline->GetStatus() == PIPELINE_STATE_STATUS_READY && m_pOldPipeline->GetStatus() == PIPELINE_STATE_STATUS_READY)
            {
                CopyStaticResources();
            }
        }
    }
    else
    {
        const auto* Name = CreateInfo.Get().PSODesc.Name;
        LOG_ERROR_MESSAGE("Failed to reload pipeline state '", (Name ? Name : "<unnamed>"), "'.");
    }
    return !FoundInCache;
}

void ReloadablePipelineState::CopyStaticResources()
{
    const Uint32 SrcSignCount = m_pOldPipeline->GetResourceSignatureCount();
    const Uint32 DstSignCount = m_pPipeline->GetResourceSignatureCount();
    if (SrcSignCount == DstSignCount)
    {
        for (Uint32 s = 0; s < SrcSignCount; ++s)
        {
            IPipelineResourceSignature* pSrcSign = m_pOldPipeline->GetResourceSignature(s);
            IPipelineResourceSignature* pDstSign = m_pPipeline->GetResourceSignature(s);
            if (pSrcSign != pDstSign)
            {
                pSrcSign->CopyStaticResources(pDstSign);
            }
        }
    }
    else
    {
        UNEXPECTED("The number of resource signatures in old pipeline (", SrcSignCount, ") does not match the number of signatures in new pipeline (", DstSignCount, ")");
    }
    m_pOldPipeline.Release();
}


PIPELINE_STATE_STATUS ReloadablePipelineState::GetStatus(bool WaitForCompletion)
{
    const PIPELINE_STATE_STATUS Status = m_pPipeline ? m_pPipeline->GetStatus(WaitForCompletion) : PIPELINE_STATE_STATUS_FAILED;
    if (Status != PIPELINE_STATE_STATUS_READY)
        return Status;

    if (m_pOldPipeline)
    {
        const PIPELINE_STATE_STATUS OldPsoStatus = m_pOldPipeline->GetStatus(WaitForCompletion);
        if (OldPsoStatus == PIPELINE_STATE_STATUS_READY && Status == PIPELINE_STATE_STATUS_READY)
        {
            CopyStaticResources();
        }
        else if (OldPsoStatus == PIPELINE_STATE_STATUS_COMPILING)
        {
            // Wait until the old pipeline is ready
            return PIPELINE_STATE_STATUS_COMPILING;
        }
        else if (OldPsoStatus == PIPELINE_STATE_STATUS_FAILED)
        {
            LOG_ERROR_MESSAGE("Failed to copy static resources from the old pipeline after reloading pipeline state '", m_pPipeline->GetDesc().Name,
                              "'. Old pipeline is in the failed state.");
            m_pOldPipeline.Release();
        }
    }

    return Status;
}

bool ReloadablePipelineState::Reload(ReloadGraphicsPipelineCallbackType ReloadGraphicsPipeline, void* pUserData)
{
    static_assert(PIPELINE_TYPE_COUNT == 5, "Did you add a new pipeline type? You may need to handle it here.");
    // Note that all shaders in Create Info are reloadable shaders, so they will automatically redirect all calls
    // to the updated internal shader
    switch (m_Type)
    {
        case PIPELINE_TYPE_GRAPHICS:
        case PIPELINE_TYPE_MESH:
            return Reload<GraphicsPipelineStateCreateInfo>(ReloadGraphicsPipeline, pUserData);

        case PIPELINE_TYPE_COMPUTE:
            return Reload<ComputePipelineStateCreateInfo>(ReloadGraphicsPipeline, pUserData);

        case PIPELINE_TYPE_RAY_TRACING:
            return Reload<RayTracingPipelineStateCreateInfo>(ReloadGraphicsPipeline, pUserData);

        case PIPELINE_TYPE_TILE:
            return Reload<TilePipelineStateCreateInfo>(ReloadGraphicsPipeline, pUserData);

        default:
            UNEXPECTED("Unexpected pipeline type");
            return false;
    }
}

void ReloadablePipelineState::Create(RenderStateCacheImpl*          pStateCache,
                                     IPipelineState*                pPipeline,
                                     const PipelineStateCreateInfo& CreateInfo,
                                     IPipelineState**               ppReloadablePipeline)
{
    try
    {
        RefCntAutoPtr<ReloadablePipelineState> pReloadablePipeline{MakeNewRCObj<ReloadablePipelineState>()(pStateCache, pPipeline, CreateInfo)};
        *ppReloadablePipeline = pReloadablePipeline.Detach();
    }
    catch (...)
    {
        LOG_ERROR("Failed to create reloadable pipeline state '", (CreateInfo.PSODesc.Name ? CreateInfo.PSODesc.Name : "<unnamed>"), "'.");
    }
}

} // namespace Diligent
