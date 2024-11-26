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
/// Declaration of Diligent::PipelineStateD3D12Impl class

#include <vector>

#include "EngineD3D12ImplTraits.hpp"
#include "PipelineStateBase.hpp"
#include "PipelineResourceSignatureD3D12Impl.hpp" // Required by PipelineStateBase
#include "RootSignature.hpp"

namespace Diligent
{

class ShaderResourcesD3D12;

/// Pipeline state object implementation in Direct3D12 backend.
class PipelineStateD3D12Impl final : public PipelineStateBase<EngineD3D12ImplTraits>
{
public:
    using TPipelineStateBase = PipelineStateBase<EngineD3D12ImplTraits>;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0x9007f2a7, 0x3852, 0x4718, {0x84, 0xce, 0xfd, 0xc, 0xe1, 0xe9, 0xd3, 0x65}};

    PipelineStateD3D12Impl(IReferenceCounters* pRefCounters, RenderDeviceD3D12Impl* pDeviceD3D12, const GraphicsPipelineStateCreateInfo& CreateInfo);
    PipelineStateD3D12Impl(IReferenceCounters* pRefCounters, RenderDeviceD3D12Impl* pDeviceD3D12, const ComputePipelineStateCreateInfo& CreateInfo);
    PipelineStateD3D12Impl(IReferenceCounters* pRefCounters, RenderDeviceD3D12Impl* pDeviceD3D12, const RayTracingPipelineStateCreateInfo& CreateInfo);
    ~PipelineStateD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE2_IN_PLACE(IID_PipelineStateD3D12, IID_InternalImpl, TPipelineStateBase)

    /// Implementation of IPipelineState::IsCompatibleWith() in Direct3D12 backend.
    virtual bool DILIGENT_CALL_TYPE IsCompatibleWith(const IPipelineState* pPSO) const override final;

    /// Implementation of IPipelineStateD3D12::GetD3D12PipelineState().
    virtual ID3D12PipelineState* DILIGENT_CALL_TYPE GetD3D12PipelineState() const override final { return static_cast<ID3D12PipelineState*>(m_pd3d12PSO.p); }

    /// Implementation of IPipelineStateD3D12::GetD3D12StateObject().
    virtual ID3D12StateObject* DILIGENT_CALL_TYPE GetD3D12StateObject() const override final { return static_cast<ID3D12StateObject*>(m_pd3d12PSO.p); }

    /// Implementation of IPipelineStateD3D12::GetD3D12RootSignature().
    virtual ID3D12RootSignature* DILIGENT_CALL_TYPE GetD3D12RootSignature() const override final { return m_RootSig->GetD3D12RootSignature(); }

    const RootSignatureD3D12& GetRootSignature() const { return *m_RootSig; }

#ifdef DILIGENT_DEVELOPMENT
    using ShaderResourceCacheArrayType = std::array<ShaderResourceCacheD3D12*, MAX_RESOURCE_SIGNATURES>;
    void DvpVerifySRBResources(const DeviceContextD3D12Impl*       pDeviceCtx,
                               const ShaderResourceCacheArrayType& ResourceCaches) const;
#endif

    struct ShaderStageInfo
    {
        ShaderStageInfo() {}
        explicit ShaderStageInfo(const ShaderD3D12Impl* pShader);

        void   Append(const ShaderD3D12Impl* pShader);
        size_t Count() const;

        SHADER_TYPE                           Type = SHADER_TYPE_UNKNOWN;
        std::vector<const ShaderD3D12Impl*>   Shaders;
        std::vector<RefCntAutoPtr<IDataBlob>> ByteCodes;

        friend SHADER_TYPE GetShaderStageType(const ShaderStageInfo& Stage) { return Stage.Type; }
    };
    using TShaderStages = std::vector<ShaderStageInfo>;

    using TValidateShaderResourcesFn = std::function<void(const ShaderD3D12Impl* pShader, const LocalRootSignatureD3D12* pLocalRootSig)>;
    using TValidateShaderBindingsFn  = std::function<void(const ShaderD3D12Impl* pShader, const ResourceBinding::TMap& BindingsMap)>;

    static void RemapOrVerifyShaderResources(
        TShaderStages&                                           ShaderStages,
        const RefCntAutoPtr<PipelineResourceSignatureD3D12Impl>* pSignatures,
        Uint32                                                   SignatureCount,
        const RootSignatureD3D12&                                RootSig,
        class IDXCompiler*                                       pDxCompiler,
        LocalRootSignatureD3D12*                                 pLocalRootSig             = nullptr,
        const TValidateShaderResourcesFn&                        ValidateShaderResourcesFn = {},
        const TValidateShaderBindingsFn&                         VlidateBindingsFn         = {}) noexcept(false);

    static PipelineResourceSignatureDescWrapper GetDefaultResourceSignatureDesc(
        const TShaderStages&              ShaderStages,
        const char*                       PSOName,
        const PipelineResourceLayoutDesc& ResourceLayout,
        Uint32                            SRBAllocationGranularity,
        const LocalRootSignatureD3D12*    pLocalRootSig) noexcept(false);

private:
    template <typename PSOCreateInfoType>
    void InitInternalObjects(const PSOCreateInfoType& CreateInfo,
                             TShaderStages&           ShaderStages,
                             LocalRootSignatureD3D12* pLocalRootSig = nullptr) noexcept(false);

    void InitRootSignature(const PipelineStateCreateInfo& CreateInfo,
                           TShaderStages&                 ShaderStages,
                           LocalRootSignatureD3D12*       pLocalRootSig) noexcept(false);

    void InitializePipeline(const GraphicsPipelineStateCreateInfo& CreateInfo);
    void InitializePipeline(const ComputePipelineStateCreateInfo& CreateInfo);
    void InitializePipeline(const RayTracingPipelineStateCreateInfo& CreateInfo);

    // TPipelineStateBase::Construct needs access to InitializePipeline
    friend TPipelineStateBase;

    void Destruct();

    void ValidateShaderResources(const ShaderD3D12Impl* pShader, const LocalRootSignatureD3D12* pLocalRootSig);

private:
    CComPtr<ID3D12DeviceChild>        m_pd3d12PSO;
    RefCntAutoPtr<RootSignatureD3D12> m_RootSig;

    // NB:  Pipeline resource signatures used to create the PSO may NOT be the same as
    //      pipeline resource signatures in m_RootSig, because the latter may be used from the
    //      cache. While the two signatures may be compatible, they resource names may not be identical.

#ifdef DILIGENT_DEVELOPMENT
    // Shader resources for all shaders in all shader stages in the pipeline.
    std::vector<std::shared_ptr<const ShaderResourcesD3D12>> m_ShaderResources;

    // Shader resource attributions for every resource in m_ShaderResources, in the same order.
    std::vector<ResourceAttribution> m_ResourceAttibutions;
#endif
};

} // namespace Diligent
