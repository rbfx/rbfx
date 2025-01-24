/*
 *  Copyright 2023-2024 Diligent Graphics LLC
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
/// Declaration of Diligent::PipelineStateWebGPUImpl class

#include "EngineWebGPUImplTraits.hpp"
#include "PipelineStateBase.hpp"
#include "PipelineResourceSignatureWebGPUImpl.hpp"
#include "ShaderWebGPUImpl.hpp"
#include "WebGPUObjectWrappers.hpp"
#include "PipelineLayoutWebGPU.hpp"

namespace Diligent
{

class DeviceContextWebGPUImpl;

/// Pipeline state object implementation in WebGPU backend.
class PipelineStateWebGPUImpl final : public PipelineStateBase<EngineWebGPUImplTraits>
{
public:
    using TPipelineStateBase = PipelineStateBase<EngineWebGPUImplTraits>;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0xA76F7092, 0xFD19, 0x4C08, {0xA8, 0xCD, 0x08, 0x0C, 0x24, 0x47, 0x4F, 0x7B}};

    PipelineStateWebGPUImpl(IReferenceCounters*                    pRefCounters,
                            RenderDeviceWebGPUImpl*                pDevice,
                            const GraphicsPipelineStateCreateInfo& CreateInfo);

    PipelineStateWebGPUImpl(IReferenceCounters*                   pRefCounters,
                            RenderDeviceWebGPUImpl*               pDevice,
                            const ComputePipelineStateCreateInfo& CreateInfo);

    ~PipelineStateWebGPUImpl() override;

    IMPLEMENT_QUERY_INTERFACE2_IN_PLACE(IID_PipelineStateWebGPU, IID_InternalImpl, TPipelineStateBase)

    /// Implementation of IPipelineState::GetStatus().
    virtual PIPELINE_STATE_STATUS DILIGENT_CALL_TYPE GetStatus(bool WaitForCompletion = false) override final;

    WGPURenderPipeline DILIGENT_CALL_TYPE GetWebGPURenderPipeline() const override final;

    WGPUComputePipeline DILIGENT_CALL_TYPE GetWebGPUComputePipeline() const override final;

    void Destruct();

    static constexpr Uint32 MaxBindGroupsInPipeline = MAX_RESOURCE_SIGNATURES * PipelineResourceSignatureWebGPUImpl::MAX_BIND_GROUPS;

    const PipelineLayoutWebGPU& GetPipelineLayout() const { return m_PipelineLayout; }

    struct ShaderStageInfo
    {
        const SHADER_TYPE       Type;
        ShaderWebGPUImpl* const pShader;
        std::string             PatchedWGSL;

        ShaderStageInfo(ShaderWebGPUImpl* _pShader) :
            Type{_pShader->GetDesc().ShaderType},
            pShader{_pShader}
        {}

        const std::string& GetWGSL() const
        {
            return PatchedWGSL.empty() ? pShader->GetWGSL() : PatchedWGSL;
        }

        friend SHADER_TYPE GetShaderStageType(const ShaderStageInfo& Stage) { return Stage.Type; }

        friend std::vector<const ShaderWebGPUImpl*> GetStageShaders(const ShaderStageInfo& Stage) { return {Stage.pShader}; }
    };
    using TShaderStages = std::vector<ShaderStageInfo>;


    using TBindIndexToBindGroupIndex = std::array<Uint32, MAX_RESOURCE_SIGNATURES>;
    using TShaderResources           = std::vector<std::shared_ptr<const WGSLShaderResources>>;
    using TResourceAttibutions       = std::vector<ResourceAttribution>;
    static void RemapOrVerifyShaderResources(
        TShaderStages&                                           ShaderStages,
        const RefCntAutoPtr<PipelineResourceSignatureWebGPUImpl> pSignatures[],
        const Uint32                                             SignatureCount,
        const TBindIndexToBindGroupIndex&                        BindIndexToBindGroupIndex,
        bool                                                     bVerifyOnly,
        const char*                                              PipelineName,
        TShaderResources*                                        pShaderResources     = nullptr,
        TResourceAttibutions*                                    pResourceAttibutions = nullptr) noexcept(false);

    static PipelineResourceSignatureDescWrapper GetDefaultResourceSignatureDesc(
        const TShaderStages&              ShaderStages,
        const char*                       PSOName,
        const PipelineResourceLayoutDesc& ResourceLayout,
        Uint32                            SRBAllocationGranularity);

#ifdef DILIGENT_DEVELOPMENT
    // Performs validation of SRB resource parameters that are not possible to validate
    // when resource is bound.
    using ShaderResourceCacheArrayType = std::array<ShaderResourceCacheWebGPU*, MAX_RESOURCE_SIGNATURES>;
    void DvpVerifySRBResources(const DeviceContextWebGPUImpl* pDeviceCtx, const ShaderResourceCacheArrayType& ResourceCaches) const;
#endif

private:
    friend TPipelineStateBase; // TPipelineStateBase::Construct needs access to InitializePipeline

    template <typename PSOCreateInfoType>
    std::vector<ShaderStageInfo> InitInternalObjects(const PSOCreateInfoType& CreateInfo);

    void InitPipelineLayout(const PipelineStateCreateInfo& CreateInfo, TShaderStages& ShaderStages);

    void InitializePipeline(const GraphicsPipelineStateCreateInfo& CreateInfo);
    void InitializePipeline(const ComputePipelineStateCreateInfo& CreateInfo);

    struct AsyncPipelineBuilder;

    void InitializeWebGPURenderPipeline(const TShaderStages&  ShaderStages,
                                        AsyncPipelineBuilder* AsyncBuilder = nullptr);
    void InitializeWebGPUComputePipeline(const TShaderStages&  ShaderStages,
                                         AsyncPipelineBuilder* AsyncBuilder = nullptr);

private:
    WebGPURenderPipelineWrapper  m_wgpuRenderPipeline;
    WebGPUComputePipelineWrapper m_wgpuComputePipeline;
    PipelineLayoutWebGPU         m_PipelineLayout;

    RefCntAutoPtr<AsyncPipelineBuilder> m_AsyncBuilder;

#ifdef DILIGENT_DEVELOPMENT
    // Shader resources for all shaders in all shader stages
    TShaderResources m_ShaderResources;
    // Resource attributions for every resource in m_ShaderResources, in the same order
    TResourceAttibutions m_ResourceAttibutions;
#endif
};

} // namespace Diligent
