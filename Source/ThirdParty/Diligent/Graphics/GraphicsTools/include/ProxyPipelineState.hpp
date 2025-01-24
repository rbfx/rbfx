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

#pragma once

/// \file
/// Definition of the Diligent::ProxyPipelineState class

#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Proxy pipeline state delegates all calls to the internal pipeline object.

template <typename Base>
class ProxyPipelineState : public Base
{
public:
    template <typename... Args>
    ProxyPipelineState(Args&&... args) :
        Base{std::forward<Args>(args)...}
    {}

    virtual const PipelineStateDesc& DILIGENT_CALL_TYPE GetDesc() const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        static constexpr PipelineStateDesc NullDesc;
        return m_pPipeline ? m_pPipeline->GetDesc() : NullDesc;
    }

    virtual Int32 DILIGENT_CALL_TYPE GetUniqueID() const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        return m_pPipeline ? m_pPipeline->GetUniqueID() : -1;
    }

    virtual void DILIGENT_CALL_TYPE SetUserData(IObject* pUserData) override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        m_pPipeline->SetUserData(pUserData);
    }

    virtual IObject* DILIGENT_CALL_TYPE GetUserData() const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        return m_pPipeline ? m_pPipeline->GetUserData() : nullptr;
    }

    virtual const GraphicsPipelineDesc& DILIGENT_CALL_TYPE GetGraphicsPipelineDesc() const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        static constexpr GraphicsPipelineDesc NullDesc;
        return m_pPipeline ? m_pPipeline->GetGraphicsPipelineDesc() : NullDesc;
    }

    virtual const RayTracingPipelineDesc& DILIGENT_CALL_TYPE GetRayTracingPipelineDesc() const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        static constexpr RayTracingPipelineDesc NullDesc;
        return m_pPipeline ? m_pPipeline->GetRayTracingPipelineDesc() : NullDesc;
    }

    virtual const TilePipelineDesc& DILIGENT_CALL_TYPE GetTilePipelineDesc() const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        static constexpr TilePipelineDesc NullDesc;
        return m_pPipeline ? m_pPipeline->GetTilePipelineDesc() : NullDesc;
    }

    virtual void DILIGENT_CALL_TYPE BindStaticResources(SHADER_TYPE ShaderStages, IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags) override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        if (m_pPipeline)
        {
            m_pPipeline->BindStaticResources(ShaderStages, pResourceMapping, Flags);
        }
    }

    virtual Uint32 DILIGENT_CALL_TYPE GetStaticVariableCount(SHADER_TYPE ShaderType) const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        return m_pPipeline->GetStaticVariableCount(ShaderType);
    }

    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByName(SHADER_TYPE ShaderType, const Char* Name) override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        return m_pPipeline ? m_pPipeline->GetStaticVariableByName(ShaderType, Name) : nullptr;
    }

    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index) override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        return m_pPipeline ? m_pPipeline->GetStaticVariableByIndex(ShaderType, Index) : nullptr;
    }

    virtual void DILIGENT_CALL_TYPE CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding, bool InitStaticResources) override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        m_pPipeline->CreateShaderResourceBinding(ppShaderResourceBinding, InitStaticResources);
    }

    virtual void DILIGENT_CALL_TYPE InitializeStaticSRBResources(IShaderResourceBinding* pShaderResourceBinding) const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        if (m_pPipeline)
        {
            m_pPipeline->InitializeStaticSRBResources(pShaderResourceBinding);
        }
    }

    virtual void DILIGENT_CALL_TYPE CopyStaticResources(IPipelineState* pPSO) const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        if (m_pPipeline)
        {
            m_pPipeline->CopyStaticResources(pPSO);
        }
    }

    virtual bool DILIGENT_CALL_TYPE IsCompatibleWith(const IPipelineState* pPSO) const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        return m_pPipeline ? m_pPipeline->IsCompatibleWith(pPSO) : false;
    }

    virtual Uint32 DILIGENT_CALL_TYPE GetResourceSignatureCount() const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        return m_pPipeline ? m_pPipeline->GetResourceSignatureCount() : 0;
    }

    virtual IPipelineResourceSignature* DILIGENT_CALL_TYPE GetResourceSignature(Uint32 Index) const override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        return m_pPipeline ? m_pPipeline->GetResourceSignature(Index) : nullptr;
    }

    virtual PIPELINE_STATE_STATUS DILIGENT_CALL_TYPE GetStatus(bool WaitForCompletion) override
    {
        DEV_CHECK_ERR(m_pPipeline, "Internal pipeline is null");
        return m_pPipeline ? m_pPipeline->GetStatus(WaitForCompletion) : PIPELINE_STATE_STATUS_UNINITIALIZED;
    }

protected:
    RefCntAutoPtr<IPipelineState> m_pPipeline;
};

} // namespace Diligent
