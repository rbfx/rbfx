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

#include "pch.h"

#include "PipelineStateD3D11Impl.hpp"

#include <array>
#include <unordered_map>

#include "WinHPreface.h"
#include <d3dcompiler.h>
#include "WinHPostface.h"

#include "RenderDeviceD3D11Impl.hpp"
#include "ShaderResourceBindingD3D11Impl.hpp"
#include "RenderPassD3D11Impl.hpp"
#include "DataBlobImpl.hpp"

#include "EngineMemory.h"
#include "DXBCUtils.hpp"
#include "D3DShaderResourceValidation.hpp"

namespace Diligent
{

constexpr INTERFACE_ID PipelineStateD3D11Impl::IID_InternalImpl;

__forceinline SHADER_TYPE GetShaderStageType(const ShaderD3D11Impl* pShader)
{
    return pShader->GetDesc().ShaderType;
}

PipelineResourceSignatureDescWrapper PipelineStateD3D11Impl::GetDefaultResourceSignatureDesc(
    const TShaderStages&              Shaders,
    const char*                       PSOName,
    const PipelineResourceLayoutDesc& ResourceLayout,
    Uint32                            SRBAllocationGranularity) noexcept(false)
{
    PipelineResourceSignatureDescWrapper SignDesc{PSOName, ResourceLayout, SRBAllocationGranularity};

    std::unordered_map<ShaderResourceHashKey, const D3DShaderResourceAttribs&, ShaderResourceHashKey::Hasher> UniqueResources;
    for (auto* pShader : Shaders)
    {
        const auto& ShaderResources = *pShader->GetShaderResources();
        const auto  ShaderType      = ShaderResources.GetShaderType();
        VERIFY_EXPR(ShaderType == pShader->GetDesc().ShaderType);

        ShaderResources.ProcessResources(
            [&](const D3DShaderResourceAttribs& Attribs, Uint32) //
            {
                const char* const SamplerSuffix =
                    (ShaderResources.IsUsingCombinedTextureSamplers() && Attribs.GetInputType() == D3D_SIT_SAMPLER) ?
                    ShaderResources.GetCombinedSamplerSuffix() :
                    nullptr;

                const auto VarDesc = FindPipelineResourceLayoutVariable(ResourceLayout, Attribs.Name, ShaderType, SamplerSuffix);
                // Note that Attribs.Name != VarDesc.Name for combined samplers
                const auto it_assigned = UniqueResources.emplace(ShaderResourceHashKey{VarDesc.ShaderStages, Attribs.Name}, Attribs);
                if (it_assigned.second)
                {
                    if (Attribs.BindCount == 0)
                    {
                        LOG_ERROR_AND_THROW("Resource '", Attribs.Name, "' in shader '", pShader->GetDesc().Name, "' is a runtime-sized array. ",
                                            "Use explicit resource signature to specify the array size.");
                    }

                    const auto ResType  = Attribs.GetShaderResourceType();
                    const auto ResFlags = Attribs.GetPipelineResourceFlags() | ShaderVariableFlagsToPipelineResourceFlags(VarDesc.Flags);
                    SignDesc.AddResource(VarDesc.ShaderStages, Attribs.Name, Attribs.BindCount, ResType, VarDesc.Type, ResFlags);
                }
                else
                {
                    VerifyD3DResourceMerge(PSOName, it_assigned.first->second, Attribs);
                }
            } //
        );

        // Merge combined sampler suffixes
        if (ShaderResources.IsUsingCombinedTextureSamplers() && ShaderResources.GetNumSamplers() > 0)
        {
            SignDesc.SetCombinedSamplerSuffix(ShaderResources.GetCombinedSamplerSuffix());
        }
    }

    return SignDesc;
}

void PipelineStateD3D11Impl::RemapOrVerifyShaderResources(const TShaderStages&                                     Shaders,
                                                          const RefCntAutoPtr<PipelineResourceSignatureD3D11Impl>* pSignatures,
                                                          Uint32                                                   SignatureCount,
                                                          D3D11ShaderResourceCounters*                             pBaseBindings, // [SignatureCount]
                                                          const THandleRemappedBytecodeFn&                         HandleRemappedBytecodeFn,
                                                          const TValidateShaderResourcesFn&                        ValidateShaderResourcesFn,
                                                          const TValidateShaderBindingsFn&                         ValidateShaderBindingsFn) noexcept(false)
{
    // Verify that pipeline layout is compatible with shader resources and remap resource bindings.
    for (size_t s = 0; s < Shaders.size(); ++s)
    {
        auto* const      pShader    = Shaders[s];
        auto const       ShaderType = pShader->GetDesc().ShaderType;
        const IDataBlob* pBytecode  = Shaders[s]->GetD3DBytecode();

        ResourceBinding::TMap ResourceMap;
        for (Uint32 sign = 0; sign < SignatureCount; ++sign)
        {
            const PipelineResourceSignatureD3D11Impl* const pSignature = pSignatures[sign];
            if (pSignature == nullptr)
                continue;

            VERIFY_EXPR(pSignature->GetDesc().BindingIndex == sign);
            pSignature->UpdateShaderResourceBindingMap(ResourceMap, ShaderType, pBaseBindings[sign]);
        }

        if (ValidateShaderResourcesFn)
            ValidateShaderResourcesFn(pShader);

        if (HandleRemappedBytecodeFn)
        {
            RefCntAutoPtr<IDataBlob> pPatchedBytecode = DataBlobImpl::MakeCopy(pBytecode);
            if (!DXBCUtils::RemapResourceBindings(ResourceMap, pPatchedBytecode->GetDataPtr(), pPatchedBytecode->GetSize()))
                LOG_ERROR_AND_THROW("Failed to remap resource bindings in shader '", pShader->GetDesc().Name, "'.");

            HandleRemappedBytecodeFn(s, pShader, pPatchedBytecode);
        }

        if (ValidateShaderBindingsFn)
        {
            ValidateShaderBindingsFn(pShader, ResourceMap);
        }
    }
}


void PipelineStateD3D11Impl::InitResourceLayouts(const PipelineStateCreateInfo&       CreateInfo,
                                                 const std::vector<ShaderD3D11Impl*>& Shaders,
                                                 RefCntAutoPtr<IDataBlob>&            pVSByteCode)
{
    const auto InternalFlags = GetInternalCreateFlags(CreateInfo);
    if (m_UsingImplicitSignature && (InternalFlags & PSO_CREATE_INTERNAL_FLAG_IMPLICIT_SIGNATURE0) == 0)
    {
        const auto SignDesc = GetDefaultResourceSignatureDesc(Shaders, m_Desc.Name, m_Desc.ResourceLayout, m_Desc.SRBAllocationGranularity);
        InitDefaultSignature(SignDesc, GetActiveShaderStages(), false /*IsDeviceInternal*/);
        VERIFY_EXPR(m_Signatures[0]);
    }

    D3D11ShaderResourceCounters ResCounters = {};
    if (m_Desc.IsAnyGraphicsPipeline())
    {
        // In Direct3D11, UAVs use the same register space as render targets
        ResCounters[D3D11_RESOURCE_RANGE_UAV][PSInd] = m_pGraphicsPipelineData->Desc.NumRenderTargets;
    }

    for (Uint32 sign = 0; sign < m_SignatureCount; ++sign)
    {
        const PipelineResourceSignatureD3D11Impl* const pSignature = m_Signatures[sign];
        if (pSignature == nullptr)
            continue;

        VERIFY_EXPR(pSignature->GetDesc().BindingIndex == sign);
        m_BaseBindings[sign] = ResCounters;
        pSignature->ShiftBindings(ResCounters);
    }

    m_NumPixelUAVs = ResCounters[D3D11_RESOURCE_RANGE_UAV][PSInd];

#ifdef DILIGENT_DEVELOPMENT
    for (Uint32 s = 0; s < D3D11ResourceBindPoints::NumShaderTypes; ++s)
    {
        const auto ShaderType = GetShaderTypeFromIndex(s);
        DEV_CHECK_ERR(ResCounters[D3D11_RESOURCE_RANGE_CBV][s] <= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                      "Constant buffer count ", Uint32{ResCounters[D3D11_RESOURCE_RANGE_CBV][s]},
                      " in ", GetShaderTypeLiteralName(ShaderType), " stage exceeds D3D11 limit ",
                      D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);
        DEV_CHECK_ERR(ResCounters[D3D11_RESOURCE_RANGE_SRV][s] <= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
                      "SRV count ", Uint32{ResCounters[D3D11_RESOURCE_RANGE_SRV][s]},
                      " in ", GetShaderTypeLiteralName(ShaderType), " stage exceeds D3D11 limit ",
                      D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        DEV_CHECK_ERR(ResCounters[D3D11_RESOURCE_RANGE_SAMPLER][s] <= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
                      "Sampler count ", Uint32{ResCounters[D3D11_RESOURCE_RANGE_SAMPLER][s]},
                      " in ", GetShaderTypeLiteralName(ShaderType), " stage exceeds D3D11 limit ",
                      D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        DEV_CHECK_ERR(ResCounters[D3D11_RESOURCE_RANGE_UAV][s] <= D3D11_PS_CS_UAV_REGISTER_COUNT,
                      "UAV count ", Uint32{ResCounters[D3D11_RESOURCE_RANGE_UAV][s]},
                      " in ", GetShaderTypeLiteralName(ShaderType), " stage exceeds D3D11 limit ",
                      D3D11_PS_CS_UAV_REGISTER_COUNT);
    }
#endif

    const auto HandleRemappedBytecode = [this, &pVSByteCode](size_t ShaderIdx, ShaderD3D11Impl* pShader, IDataBlob* pPatchedBytecode) //
    {
        m_ppd3d11Shaders[ShaderIdx] = pShader->GetD3D11Shader(pPatchedBytecode);
        VERIFY_EXPR(m_ppd3d11Shaders[ShaderIdx]); // GetD3D11Shader() throws an exception in case of an error

        if (pShader->GetDesc().ShaderType == SHADER_TYPE_VERTEX)
            pVSByteCode = pPatchedBytecode;
    };
    const auto HandleRemappedBytecodeFn = (CreateInfo.Flags & PSO_CREATE_FLAG_DONT_REMAP_SHADER_RESOURCES) == 0 ?
        THandleRemappedBytecodeFn{HandleRemappedBytecode} :
        THandleRemappedBytecodeFn{nullptr};

    const auto ValidateBindings = [this](const ShaderD3D11Impl* pShader, const ResourceBinding::TMap& BindingsMap) //
    {
        ValidateShaderResourceBindings(m_Desc.Name, *pShader->GetShaderResources(), BindingsMap);
    };
    const auto ValidateBindingsFn = !HandleRemappedBytecodeFn && (InternalFlags & PSO_CREATE_INTERNAL_FLAG_NO_SHADER_REFLECTION) == 0 ?
        TValidateShaderBindingsFn{ValidateBindings} :
        TValidateShaderBindingsFn{nullptr};

    if (HandleRemappedBytecodeFn || ValidateBindingsFn)
    {
        RemapOrVerifyShaderResources(
            Shaders,
            m_Signatures,
            m_SignatureCount,
            m_BaseBindings,
            HandleRemappedBytecodeFn,
            [this](ShaderD3D11Impl* pShader) //
            {
                ValidateShaderResources(pShader);
            },
            ValidateBindingsFn);
    }

    if (!HandleRemappedBytecodeFn)
    {
        for (size_t s = 0; s < Shaders.size(); ++s)
        {
            auto* const pShader = Shaders[s];
            m_ppd3d11Shaders[s] = pShader->GetD3D11Shader();
            VERIFY_EXPR(m_ppd3d11Shaders[s]);

            if (pShader->GetDesc().ShaderType == SHADER_TYPE_VERTEX)
            {
                pVSByteCode = pShader->GetD3DBytecode();
            }
        }
    }
}

template <typename PSOCreateInfoType>
void PipelineStateD3D11Impl::InitInternalObjects(const PSOCreateInfoType&  CreateInfo,
                                                 RefCntAutoPtr<IDataBlob>& pVSByteCode)
{
    std::vector<ShaderD3D11Impl*> Shaders;
    ExtractShaders<ShaderD3D11Impl>(CreateInfo, Shaders, /*WaitUntilShadersReady = */ true);

    m_NumShaders = static_cast<Uint8>(Shaders.size());
    for (Uint32 s = 0; s < m_NumShaders; ++s)
    {
        const auto ShaderType    = Shaders[s]->GetDesc().ShaderType;
        const auto ShaderTypeIdx = GetShaderTypeIndex(ShaderType);
        VERIFY_EXPR(m_ShaderIndices[ShaderTypeIdx] < 0);
        m_ShaderIndices[ShaderTypeIdx] = static_cast<Int8>(s);
    }

    FixedLinearAllocator MemPool{GetRawAllocator()};

    ReserveSpaceForPipelineDesc(CreateInfo, MemPool);
    MemPool.AddSpace<D3D11ShaderAutoPtrType>(m_NumShaders);

    // m_SignatureCount is initialized by ReserveSpaceForPipelineDesc()
    MemPool.AddSpace<D3D11ShaderResourceCounters>(m_SignatureCount);

    MemPool.Reserve();

    InitializePipelineDesc(CreateInfo, MemPool);
    m_ppd3d11Shaders = MemPool.ConstructArray<D3D11ShaderAutoPtrType>(m_NumShaders);
    m_BaseBindings   = MemPool.ConstructArray<D3D11ShaderResourceCounters>(m_SignatureCount);

    InitResourceLayouts(CreateInfo, Shaders, pVSByteCode);
}

void PipelineStateD3D11Impl::InitializePipeline(const GraphicsPipelineStateCreateInfo& CreateInfo)
{
    RefCntAutoPtr<IDataBlob> pVSByteCode;
    InitInternalObjects(CreateInfo, pVSByteCode);

    if (GetD3D11VertexShader() == nullptr)
        LOG_ERROR_AND_THROW("Vertex shader is null");

    const GraphicsPipelineDesc& GraphicsPipeline = m_pGraphicsPipelineData->Desc;
    ID3D11Device* const         pDeviceD3D11     = m_pDevice->GetD3D11Device();

    D3D11_BLEND_DESC D3D11BSDesc = {};
    BlendStateDesc_To_D3D11_BLEND_DESC(GraphicsPipeline.BlendDesc, D3D11BSDesc);
    CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateBlendState(&D3D11BSDesc, &m_pd3d11BlendState),
                           "Failed to create D3D11 blend state object");

    D3D11_RASTERIZER_DESC D3D11RSDesc = {};
    RasterizerStateDesc_To_D3D11_RASTERIZER_DESC(GraphicsPipeline.RasterizerDesc, D3D11RSDesc);
    CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateRasterizerState(&D3D11RSDesc, &m_pd3d11RasterizerState),
                           "Failed to create D3D11 rasterizer state");

    D3D11_DEPTH_STENCIL_DESC D3D11DSSDesc = {};
    DepthStencilStateDesc_To_D3D11_DEPTH_STENCIL_DESC(GraphicsPipeline.DepthStencilDesc, D3D11DSSDesc);
    CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateDepthStencilState(&D3D11DSSDesc, &m_pd3d11DepthStencilState),
                           "Failed to create D3D11 depth stencil state");

    // Create input layout
    const auto& InputLayout = GraphicsPipeline.InputLayout;
    if (InputLayout.NumElements > 0)
    {
        std::vector<D3D11_INPUT_ELEMENT_DESC, STDAllocatorRawMem<D3D11_INPUT_ELEMENT_DESC>> d311InputElements(STD_ALLOCATOR_RAW_MEM(D3D11_INPUT_ELEMENT_DESC, GetRawAllocator(), "Allocator for vector<D3D11_INPUT_ELEMENT_DESC>"));
        LayoutElements_To_D3D11_INPUT_ELEMENT_DESCs(InputLayout, d311InputElements);

        CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateInputLayout(d311InputElements.data(), static_cast<UINT>(d311InputElements.size()), pVSByteCode->GetConstDataPtr(), pVSByteCode->GetSize(), &m_pd3d11InputLayout),
                               "Failed to create the Direct3D11 input layout");
    }
}

void PipelineStateD3D11Impl::InitializePipeline(const ComputePipelineStateCreateInfo& CreateInfo)
{
    RefCntAutoPtr<IDataBlob> pVSByteCode;
    InitInternalObjects(CreateInfo, pVSByteCode);
    VERIFY(!pVSByteCode, "There must be no VS in a compute pipeline.");
}

// Used by TPipelineStateBase::Construct()
inline std::vector<const ShaderD3D11Impl*> GetStageShaders(const ShaderD3D11Impl* Stage)
{
    return {Stage};
}

PipelineStateD3D11Impl::PipelineStateD3D11Impl(IReferenceCounters*                    pRefCounters,
                                               RenderDeviceD3D11Impl*                 pRenderDeviceD3D11,
                                               const GraphicsPipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pRenderDeviceD3D11, CreateInfo}
{
    Construct<ShaderD3D11Impl>(CreateInfo);
}

PipelineStateD3D11Impl::PipelineStateD3D11Impl(IReferenceCounters*                   pRefCounters,
                                               RenderDeviceD3D11Impl*                pRenderDeviceD3D11,
                                               const ComputePipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pRenderDeviceD3D11, CreateInfo}
{
    Construct<ShaderD3D11Impl>(CreateInfo);
}

PipelineStateD3D11Impl::~PipelineStateD3D11Impl()
{
    // Make sure that asynchrous task is complete as it references the pipeline object.
    // This needs to be done in the final class before the destruction begins.
    GetStatus(/*WaitForCompletion =*/true);

    Destruct();
}

void PipelineStateD3D11Impl::Destruct()
{
    m_pd3d11BlendState.Release();
    m_pd3d11RasterizerState.Release();
    m_pd3d11DepthStencilState.Release();
    m_pd3d11InputLayout.Release();
    for (Uint32 s = 0; s < m_NumShaders; ++s)
        m_ppd3d11Shaders[s].~D3D11ShaderAutoPtrType();
    m_ppd3d11Shaders = nullptr;
    m_ShaderIndices.fill(-1);

    TPipelineStateBase::Destruct();
}

IMPLEMENT_QUERY_INTERFACE2(PipelineStateD3D11Impl, IID_PipelineStateD3D11, IID_InternalImpl, TPipelineStateBase)


bool PipelineStateD3D11Impl::IsCompatibleWith(const IPipelineState* pPSO) const
{
    if (!TPipelineStateBase::IsCompatibleWith(pPSO))
        return false;

    RefCntAutoPtr<PipelineStateD3D11Impl> pPSOImpl{const_cast<IPipelineState*>(pPSO), PipelineStateImplType::IID_InternalImpl};
    VERIFY(pPSOImpl, "Unknown PSO implementation type");

    const auto& rhs = *pPSOImpl;
    if (m_ActiveShaderStages != rhs.m_ActiveShaderStages)
        return false;

    return true;
}

void PipelineStateD3D11Impl::ValidateShaderResources(const ShaderD3D11Impl* pShader)
{
    const auto& pShaderResources = pShader->GetShaderResources();
    const auto  ShaderType       = pShader->GetDesc().ShaderType;

#ifdef DILIGENT_DEVELOPMENT
    m_ShaderResources.emplace_back(pShaderResources);
#endif

    // Check compatibility between shader resources and resource signature.
    pShaderResources->ProcessResources(
        [&](const D3DShaderResourceAttribs& Attribs, Uint32) //
        {
#ifdef DILIGENT_DEVELOPMENT
            m_ResourceAttibutions.emplace_back();
#endif

            const auto IsSampler = Attribs.GetInputType() == D3D_SIT_SAMPLER;
            if (IsSampler && pShaderResources->IsUsingCombinedTextureSamplers())
                return;

#ifdef DILIGENT_DEVELOPMENT
            auto& ResAttribution = m_ResourceAttibutions.back();
#else
            ResourceAttribution ResAttribution;
#endif
            ResAttribution = GetResourceAttribution(Attribs.Name, ShaderType);
            if (!ResAttribution)
            {
                LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name, "' contains resource '", Attribs.Name,
                                    "' that is not present in any pipeline resource signature used to create pipeline state '",
                                    m_Desc.Name, "'.");
            }

            const auto ResType  = Attribs.GetShaderResourceType();
            const auto ResFlags = Attribs.GetPipelineResourceFlags();

            const auto* const pSignature = ResAttribution.pSignature;
            VERIFY_EXPR(pSignature != nullptr);

            if (ResAttribution.ResourceIndex != ResourceAttribution::InvalidResourceIndex)
            {
                auto ResDesc = pSignature->GetResourceDesc(ResAttribution.ResourceIndex);
                if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT)
                    ResDesc.ResourceType = SHADER_RESOURCE_TYPE_TEXTURE_SRV;

                VERIFY(Attribs.BindCount != 0, "Runtime-sized arrays are not supported in Direct3D11.");
                VERIFY((ResDesc.Flags & PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY) == 0,
                       "Runtime-sized array flag is not supported in Direct3D11, this error must be handled by ValidatePipelineResourceSignatureDesc()");

                ValidatePipelineResourceCompatibility(ResDesc, ResType, ResFlags, Attribs.BindCount,
                                                      pShader->GetDesc().Name, pSignature->GetDesc().Name);
            }
            else if (ResAttribution.ImmutableSamplerIndex != ResourceAttribution::InvalidResourceIndex)
            {
                if (ResType != SHADER_RESOURCE_TYPE_SAMPLER)
                {
                    LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name, "' contains resource with name '", Attribs.Name,
                                        "' and type '", GetShaderResourceTypeLiteralName(ResType),
                                        "' that is not compatible with immutable sampler defined in pipeline resource signature '",
                                        pSignature->GetDesc().Name, "'.");
                }
            }
            else
            {
                UNEXPECTED("Either immutable sampler or resource index should be valid");
            }
        } //
    );
}

#ifdef DILIGENT_DEVELOPMENT
void PipelineStateD3D11Impl::DvpVerifySRBResources(const ShaderResourceCacheArrayType& ResourceCaches,
                                                   const BaseBindingsArrayType&        BaseBindings) const
{
    // Verify base bindings
    const auto SignCount = GetResourceSignatureCount();
    for (Uint32 sign = 0; sign < SignCount; ++sign)
    {
        const auto* pSignature = GetResourceSignature(sign);
        if (pSignature == nullptr || pSignature->GetTotalResourceCount() == 0)
            continue; // Skip null and empty signatures

        DEV_CHECK_ERR(GetBaseBindings(sign) == BaseBindings[sign],
                      "Bound resources use incorrect base binding indices. This may indicate a bug in resource signature compatibility comparison.");
    }

    auto attrib_it = m_ResourceAttibutions.begin();
    for (const auto& pResources : m_ShaderResources)
    {
        pResources->ProcessResources(
            [&](const D3DShaderResourceAttribs& Attribs, Uint32) //
            {
                if (*attrib_it && !attrib_it->IsImmutableSampler())
                {
                    const auto* pResourceCache = ResourceCaches[attrib_it->SignatureIndex];
                    DEV_CHECK_ERR(pResourceCache != nullptr, "No shader resource cache is set at index ", attrib_it->SignatureIndex);
                    attrib_it->pSignature->DvpValidateCommittedResource(Attribs, attrib_it->ResourceIndex, *pResourceCache,
                                                                        pResources->GetShaderName(), m_Desc.Name);
                }
                ++attrib_it;
            } //
        );
    }
    VERIFY_EXPR(attrib_it == m_ResourceAttibutions.end());
}

#endif // DILIGENT_DEVELOPMENT

} // namespace Diligent
