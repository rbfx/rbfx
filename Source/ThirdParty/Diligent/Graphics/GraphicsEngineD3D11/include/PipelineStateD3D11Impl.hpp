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
/// Declaration of Diligent::PipelineStateD3D11Impl class

#include "EngineD3D11ImplTraits.hpp"
#include "PipelineStateBase.hpp"

#include "PipelineResourceSignatureD3D11Impl.hpp" // Required by PipelineStateBase
#include "ShaderD3D11Impl.hpp"

namespace Diligent
{

/// Pipeline state object implementation in Direct3D11 backend.
class PipelineStateD3D11Impl final : public PipelineStateBase<EngineD3D11ImplTraits>
{
public:
    using TPipelineStateBase = PipelineStateBase<EngineD3D11ImplTraits>;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0xe432f9ec, 0xe60e, 0x4e14, {0xbc, 0xe0, 0x18, 0x81, 0x2f, 0x52, 0x32, 0x43}};

    PipelineStateD3D11Impl(IReferenceCounters*                    pRefCounters,
                           class RenderDeviceD3D11Impl*           pDeviceD3D11,
                           const GraphicsPipelineStateCreateInfo& CreateInfo);
    PipelineStateD3D11Impl(IReferenceCounters*                   pRefCounters,
                           class RenderDeviceD3D11Impl*          pDeviceD3D11,
                           const ComputePipelineStateCreateInfo& CreateInfo);
    ~PipelineStateD3D11Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IPipelineState::IsCompatibleWith() in Direct3D11 backend.
    virtual bool DILIGENT_CALL_TYPE IsCompatibleWith(const IPipelineState* pPSO) const override final;

    /// Implementation of IPipelineStateD3D11::GetD3D11BlendState() method.
    virtual ID3D11BlendState* DILIGENT_CALL_TYPE GetD3D11BlendState() override final { return m_pd3d11BlendState; }

    /// Implementation of IPipelineStateD3D11::GetD3D11RasterizerState() method.
    virtual ID3D11RasterizerState* DILIGENT_CALL_TYPE GetD3D11RasterizerState() override final { return m_pd3d11RasterizerState; }

    /// Implementation of IPipelineStateD3D11::GetD3D11DepthStencilState() method.
    virtual ID3D11DepthStencilState* DILIGENT_CALL_TYPE GetD3D11DepthStencilState() override final { return m_pd3d11DepthStencilState; }

    /// Implementation of IPipelineStateD3D11::GetD3D11InputLayout() method.
    virtual ID3D11InputLayout* DILIGENT_CALL_TYPE GetD3D11InputLayout() override final { return m_pd3d11InputLayout; }

    /// Implementation of IPipelineStateD3D11::GetD3D11VertexShader() method.
    virtual ID3D11VertexShader* DILIGENT_CALL_TYPE GetD3D11VertexShader() override final { return GetD3D11Shader<ID3D11VertexShader>(VSInd); }

    /// Implementation of IPipelineStateD3D11::GetD3D11PixelShader() method.
    virtual ID3D11PixelShader* DILIGENT_CALL_TYPE GetD3D11PixelShader() override final { return GetD3D11Shader<ID3D11PixelShader>(PSInd); }

    /// Implementation of IPipelineStateD3D11::GetD3D11GeometryShader() method.
    virtual ID3D11GeometryShader* DILIGENT_CALL_TYPE GetD3D11GeometryShader() override final { return GetD3D11Shader<ID3D11GeometryShader>(GSInd); }

    /// Implementation of IPipelineStateD3D11::GetD3D11DomainShader() method.
    virtual ID3D11DomainShader* DILIGENT_CALL_TYPE GetD3D11DomainShader() override final { return GetD3D11Shader<ID3D11DomainShader>(DSInd); }

    /// Implementation of IPipelineStateD3D11::GetD3D11HullShader() method.
    virtual ID3D11HullShader* DILIGENT_CALL_TYPE GetD3D11HullShader() override final { return GetD3D11Shader<ID3D11HullShader>(HSInd); }

    /// Implementation of IPipelineStateD3D11::GetD3D11ComputeShader() method.
    virtual ID3D11ComputeShader* DILIGENT_CALL_TYPE GetD3D11ComputeShader() override final { return GetD3D11Shader<ID3D11ComputeShader>(CSInd); }

    Uint32 GetNumShaders() const { return m_NumShaders; }

    const D3D11ShaderResourceCounters& GetBaseBindings(Uint32 Index) const
    {
        VERIFY_EXPR(Index < GetResourceSignatureCount());
        return m_BaseBindings[Index];
    }

    Uint8 GetNumPixelUAVs() const
    {
        return m_NumPixelUAVs;
    }

#ifdef DILIGENT_DEVELOPMENT
    using ShaderResourceCacheArrayType = std::array<ShaderResourceCacheD3D11*, MAX_RESOURCE_SIGNATURES>;
    using BaseBindingsArrayType        = std::array<D3D11ShaderResourceCounters, MAX_RESOURCE_SIGNATURES>;
    void DvpVerifySRBResources(const ShaderResourceCacheArrayType& ResourceCaches,
                               const BaseBindingsArrayType&        BaseBindings) const;
#endif

    using THandleRemappedBytecodeFn  = std::function<void(size_t ShaderIdx, ShaderD3D11Impl* pShader, ID3DBlob* pPatchedBytecode)>;
    using TValidateShaderResourcesFn = std::function<void(ShaderD3D11Impl* pShader)>;
    using TValidateShaderBindingsFn  = std::function<void(const ShaderD3D11Impl* pShader, const ResourceBinding::TMap& BindingsMap)>;
    using TShaderStages              = std::vector<ShaderD3D11Impl*>;

    static void RemapOrVerifyShaderResources(const TShaderStages&                                     Shaders,
                                             const RefCntAutoPtr<PipelineResourceSignatureD3D11Impl>* pSignatures,
                                             Uint32                                                   SignatureCount,
                                             D3D11ShaderResourceCounters*                             pBaseBindings, // [SignatureCount]
                                             const THandleRemappedBytecodeFn&                         HandleRemappedBytecodeFn,
                                             const TValidateShaderResourcesFn&                        ValidateShaderResourcesFn = {},
                                             const TValidateShaderBindingsFn&                         ValidateShaderBindingsFn  = {}) noexcept(false);

    static PipelineResourceSignatureDescWrapper GetDefaultResourceSignatureDesc(
        const TShaderStages&              ShaderStages,
        const char*                       PSOName,
        const PipelineResourceLayoutDesc& ResourceLayout,
        Uint32                            SRBAllocationGranularity) noexcept(false);

private:
    template <typename PSOCreateInfoType>
    void InitInternalObjects(const PSOCreateInfoType& CreateInfo,
                             CComPtr<ID3DBlob>&       pVSByteCode);

    void InitResourceLayouts(const PipelineStateCreateInfo&       CreateInfo,
                             const std::vector<ShaderD3D11Impl*>& Shaders,
                             CComPtr<ID3DBlob>&                   pVSByteCode);

    void Destruct();

    void ValidateShaderResources(const ShaderD3D11Impl* pShader);

    template <typename D3D11ShaderType>
    D3D11ShaderType* GetD3D11Shader(Uint32 ShaderInd)
    {
        auto idx = m_ShaderIndices[ShaderInd];
        return idx >= 0 ? static_cast<D3D11ShaderType*>(m_ppd3d11Shaders[idx].p) : nullptr;
    }

private:
    // ShaderTypeIndex (e.g. VSInd, PSInd, etc.) -> index in m_ppd3d11Shaders array
    std::array<Int8, D3D11ResourceBindPoints::NumShaderTypes> m_ShaderIndices = {-1, -1, -1, -1, -1, -1};

    // The number of shader stages in this pipeline
    Uint8 m_NumShaders = 0;

    // The total number of pixel shader UAVs used by this pipeline, including render targets
    Uint8 m_NumPixelUAVs = 0;

    CComPtr<ID3D11BlendState>        m_pd3d11BlendState;
    CComPtr<ID3D11RasterizerState>   m_pd3d11RasterizerState;
    CComPtr<ID3D11DepthStencilState> m_pd3d11DepthStencilState;
    CComPtr<ID3D11InputLayout>       m_pd3d11InputLayout;

    using D3D11ShaderAutoPtrType             = CComPtr<ID3D11DeviceChild>;
    D3D11ShaderAutoPtrType* m_ppd3d11Shaders = nullptr; // Shader array indexed by m_ShaderIndices[]

    D3D11ShaderResourceCounters* m_BaseBindings = nullptr; // [GetResourceSignatureCount()]

#ifdef DILIGENT_DEVELOPMENT
    // Shader resources for all shaders in all shader stages in the pipeline.
    std::vector<std::shared_ptr<const ShaderResourcesD3D11>> m_ShaderResources;

    // Shader resource attributions for every resource in m_ShaderResources, in the same order.
    std::vector<ResourceAttribution> m_ResourceAttibutions;
#endif
};

} // namespace Diligent
