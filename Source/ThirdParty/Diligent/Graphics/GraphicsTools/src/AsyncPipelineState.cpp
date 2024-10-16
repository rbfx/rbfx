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

#include "AsyncPipelineState.hpp"

#include "RenderStateCacheImpl.hpp"
#include "ReloadableShader.hpp"
#include "GraphicsTypesX.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

constexpr INTERFACE_ID AsyncPipelineState::IID_InternalImpl;

struct AsyncPipelineState::CreateInfoWrapperBase
{
    virtual ~CreateInfoWrapperBase() {}

    virtual SHADER_STATUS GetShadersStatus(bool WaitForCompletion) const = 0;
};

template <typename CreateInfoType>
struct AsyncPipelineState::CreateInfoWrapper : CreateInfoWrapperBase
{
    CreateInfoWrapper(const CreateInfoType& CI) :
        m_CI{CI}
    {
    }

    const CreateInfoType& Get() const
    {
        return m_CI;
    }

    operator const CreateInfoType&() const
    {
        return m_CI;
    }

    virtual SHADER_STATUS GetShadersStatus(bool WaitForCompletion) const override final
    {
        return GetPipelineStateCreateInfoShadersStatus<CreateInfoType>(m_CI, WaitForCompletion);
    }

protected:
    typename PipelineStateCreateInfoXTraits<CreateInfoType>::CreateInfoXType m_CI;
};

AsyncPipelineState::AsyncPipelineState(IReferenceCounters*            pRefCounters,
                                       RenderStateCacheImpl*          pStateCache,
                                       const PipelineStateCreateInfo& CreateInfo) :
    TBase{pRefCounters},
    m_pStateCache{pStateCache},
    m_Type{CreateInfo.PSODesc.PipelineType},
    m_UniqueID{}
{
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


AsyncPipelineState::~AsyncPipelineState()
{
}

void AsyncPipelineState::QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)
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
    else if (m_pPipeline)
    {
        // This will handle implementation-specific interfaces such as PipelineStateD3D11Impl::IID_InternalImpl,
        // PipelineStateD3D12Impl::IID_InternalImpl, etc. requested by e.g. device context implementations
        // (DeviceContextD3D11Impl::SetPipelineState, DeviceContextD3D12Impl::SetPipelineState, etc.)
        m_pPipeline->QueryInterface(IID, ppInterface);
    }
}

void AsyncPipelineState::InitInternalPipeline()
{
    static_assert(PIPELINE_TYPE_COUNT == 5, "Did you add a new pipeline type? You may need to handle it here.");
    switch (m_Type)
    {
        case PIPELINE_TYPE_GRAPHICS:
        case PIPELINE_TYPE_MESH:
            m_pStateCache->CreatePipelineStateInternal<GraphicsPipelineStateCreateInfo>(static_cast<const CreateInfoWrapper<GraphicsPipelineStateCreateInfo>&>(*m_pCreateInfo), &m_pPipeline);
            break;

        case PIPELINE_TYPE_COMPUTE:
            m_pStateCache->CreatePipelineStateInternal<ComputePipelineStateCreateInfo>(static_cast<const CreateInfoWrapper<ComputePipelineStateCreateInfo>&>(*m_pCreateInfo), &m_pPipeline);
            break;

        case PIPELINE_TYPE_RAY_TRACING:
            m_pStateCache->CreatePipelineStateInternal<RayTracingPipelineStateCreateInfo>(static_cast<const CreateInfoWrapper<RayTracingPipelineStateCreateInfo>&>(*m_pCreateInfo), &m_pPipeline);
            break;

        case PIPELINE_TYPE_TILE:
            m_pStateCache->CreatePipelineStateInternal<TilePipelineStateCreateInfo>(static_cast<const CreateInfoWrapper<TilePipelineStateCreateInfo>&>(*m_pCreateInfo), &m_pPipeline);
            break;

        default:
            UNEXPECTED("Unexpected pipeline type");
    }

    DEV_CHECK_ERR(!RefCntAutoPtr<IPipelineState>(m_pPipeline, IID_InternalImpl),
                  "Asynchronous pipeline must not be created here as we checked that all shaders are ready and we "
                  "don't want to wrap async pipeline into another async pipeline.");
}

PIPELINE_STATE_STATUS AsyncPipelineState::GetStatus(bool WaitForCompletion)
{
    if (m_pPipeline)
        return m_pPipeline->GetStatus(WaitForCompletion);

    const SHADER_STATUS ShadersStatus = m_pCreateInfo->GetShadersStatus(WaitForCompletion);
    switch (ShadersStatus)
    {
        case SHADER_STATUS_UNINITIALIZED:
            UNEXPECTED("Shader status must not be uninitialized");
            return PIPELINE_STATE_STATUS_FAILED;

        case SHADER_STATUS_COMPILING:
            return PIPELINE_STATE_STATUS_COMPILING;

        case SHADER_STATUS_READY:
            InitInternalPipeline();
            return m_pPipeline ? m_pPipeline->GetStatus(WaitForCompletion) : PIPELINE_STATE_STATUS_FAILED;

        case SHADER_STATUS_FAILED:
            return PIPELINE_STATE_STATUS_FAILED;

        default:
            UNEXPECTED("Unexpected shader status");
            return PIPELINE_STATE_STATUS_FAILED;
    }
}

void AsyncPipelineState::Create(RenderStateCacheImpl*          pStateCache,
                                const PipelineStateCreateInfo& CreateInfo,
                                IPipelineState**               ppReloadablePipeline)
{
    try
    {
        RefCntAutoPtr<AsyncPipelineState> pReloadablePipeline{MakeNewRCObj<AsyncPipelineState>()(pStateCache, CreateInfo)};
        *ppReloadablePipeline = pReloadablePipeline.Detach();
    }
    catch (...)
    {
        LOG_ERROR("Failed to create reloadable pipeline state '", (CreateInfo.PSODesc.Name ? CreateInfo.PSODesc.Name : "<unnamed>"), "'.");
    }
}

} // namespace Diligent
