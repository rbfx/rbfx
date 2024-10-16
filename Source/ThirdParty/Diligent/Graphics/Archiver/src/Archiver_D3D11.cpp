/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "ArchiverImpl.hpp"
#include "Archiver_Inc.hpp"

#include "../../GraphicsEngineD3D11/include/pch.h"
#include "RenderDeviceD3D11Impl.hpp"
#include "PipelineResourceSignatureD3D11Impl.hpp"
#include "PipelineStateD3D11Impl.hpp"
#include "ShaderD3D11Impl.hpp"
#include "DeviceObjectArchiveD3D11.hpp"
#include "SerializedPipelineStateImpl.hpp"

namespace Diligent
{

template <>
struct SerializedResourceSignatureImpl::SignatureTraits<PipelineResourceSignatureD3D11Impl>
{
    static constexpr DeviceType Type = DeviceType::Direct3D11;
    using InternalDataType           = PipelineResourceSignatureInternalDataD3D11;

    template <SerializerMode Mode>
    using PRSSerializerType = PRSSerializerD3D11<Mode>;
};

namespace
{

struct CompiledShaderD3D11 final : SerializedShaderImpl::CompiledShader
{
    ShaderD3D11Impl ShaderD3D11;

    CompiledShaderD3D11(IReferenceCounters*                pRefCounters,
                        const ShaderCreateInfo&            ShaderCI,
                        const ShaderD3D11Impl::CreateInfo& D3D11ShaderCI,
                        IRenderDevice*                     pRenderDeviceD3D11) :
        ShaderD3D11{pRefCounters, ClassPtrCast<RenderDeviceD3D11Impl>(pRenderDeviceD3D11), ShaderCI, D3D11ShaderCI, true}
    {}

    virtual SerializedData Serialize(ShaderCreateInfo ShaderCI) const override final
    {
        const IDataBlob* pBytecode = ShaderD3D11.GetD3DBytecode();

        ShaderCI.Source       = nullptr;
        ShaderCI.FilePath     = nullptr;
        ShaderCI.Macros       = {};
        ShaderCI.ByteCode     = pBytecode->GetConstDataPtr();
        ShaderCI.ByteCodeSize = pBytecode->GetSize();
        return SerializedShaderImpl::SerializeCreateInfo(ShaderCI);
    }

    virtual IShader* GetDeviceShader() override final
    {
        return &ShaderD3D11;
    }

    virtual bool IsCompiling() const override final
    {
        return ShaderD3D11.IsCompiling();
    }

    virtual RefCntAutoPtr<IAsyncTask> GetCompileTask() const override final
    {
        return ShaderD3D11.GetCompileTask();
    }
};

struct ShaderStageInfoD3D11
{
    ShaderStageInfoD3D11() {}

    ShaderStageInfoD3D11(const SerializedShaderImpl* _pShader) :
        Type{_pShader->GetDesc().ShaderType},
        pShader{GetShaderD3D11(_pShader)},
        pSerialized{_pShader}
    {}

    // Needed only for ray tracing
    void Append(const SerializedShaderImpl*) {}

    constexpr Uint32 Count() const { return 1; }

    SHADER_TYPE                 Type        = SHADER_TYPE_UNKNOWN;
    ShaderD3D11Impl*            pShader     = nullptr;
    const SerializedShaderImpl* pSerialized = nullptr;

private:
    static ShaderD3D11Impl* GetShaderD3D11(const SerializedShaderImpl* pShader)
    {
        auto* pCompiledShaderD3D11 = pShader->GetShader<CompiledShaderD3D11>(DeviceObjectArchive::DeviceType::Direct3D11);
        return pCompiledShaderD3D11 != nullptr ? &pCompiledShaderD3D11->ShaderD3D11 : nullptr;
    }
};

inline SHADER_TYPE GetShaderStageType(const ShaderStageInfoD3D11& Stage) { return Stage.Type; }

template <typename CreateInfoType>
void InitD3D11ShaderResourceCounters(const CreateInfoType& CreateInfo, D3D11ShaderResourceCounters& ResCounters)
{}

void InitD3D11ShaderResourceCounters(const GraphicsPipelineStateCreateInfo& CreateInfo, D3D11ShaderResourceCounters& ResCounters)
{
    VERIFY_EXPR(CreateInfo.PSODesc.IsAnyGraphicsPipeline());

    // In Direct3D11, UAVs use the same register space as render targets
    ResCounters[D3D11_RESOURCE_RANGE_UAV][PSInd] = CreateInfo.GraphicsPipeline.NumRenderTargets;
}

} // namespace


template <typename CreateInfoType>
void SerializedPipelineStateImpl::PatchShadersD3D11(const CreateInfoType& CreateInfo) noexcept(false)
{
    std::vector<ShaderStageInfoD3D11> ShaderStages;
    SHADER_TYPE                       ActiveShaderStages    = SHADER_TYPE_UNKNOWN;
    constexpr bool                    WaitUntilShadersReady = true;
    PipelineStateUtils::ExtractShaders<SerializedShaderImpl>(CreateInfo, ShaderStages, WaitUntilShadersReady, ActiveShaderStages);

    std::vector<ShaderD3D11Impl*> ShadersD3D11{ShaderStages.size()};
    for (size_t i = 0; i < ShadersD3D11.size(); ++i)
        ShadersD3D11[i] = ShaderStages[i].pShader;

    auto** ppSignatures    = CreateInfo.ppResourceSignatures;
    auto   SignaturesCount = CreateInfo.ResourceSignaturesCount;

    IPipelineResourceSignature* DefaultSignatures[1] = {};
    if (CreateInfo.ResourceSignaturesCount == 0)
    {
        CreateDefaultResourceSignature<PipelineStateD3D11Impl, PipelineResourceSignatureD3D11Impl>(DeviceType::Direct3D11, CreateInfo.PSODesc, ActiveShaderStages, ShadersD3D11);

        DefaultSignatures[0] = m_pDefaultSignature;
        SignaturesCount      = 1;
        ppSignatures         = DefaultSignatures;
    }

    std::vector<RefCntAutoPtr<IDataBlob>> ShaderBytecode{ShaderStages.size()};
    {
        // Sort signatures by binding index.
        // Note that SignaturesCount will be overwritten with the maximum binding index.
        SignatureArray<PipelineResourceSignatureD3D11Impl> Signatures = {};
        SortResourceSignatures(ppSignatures, SignaturesCount, Signatures, SignaturesCount);

        D3D11ShaderResourceCounters ResCounters = {};
        InitD3D11ShaderResourceCounters(CreateInfo, ResCounters);
        std::array<D3D11ShaderResourceCounters, MAX_RESOURCE_SIGNATURES> BaseBindings = {};
        for (Uint32 i = 0; i < SignaturesCount; ++i)
        {
            const PipelineResourceSignatureD3D11Impl* const pSignature = Signatures[i];
            if (pSignature == nullptr)
                continue;

            BaseBindings[i] = ResCounters;
            pSignature->ShiftBindings(ResCounters);
        }

        PipelineStateD3D11Impl::RemapOrVerifyShaderResources(
            ShadersD3D11,
            Signatures.data(),
            SignaturesCount,
            BaseBindings.data(),
            [&ShaderBytecode](size_t ShaderIdx, ShaderD3D11Impl* pShader, IDataBlob* pPatchedBytecode) //
            {
                ShaderBytecode[ShaderIdx] = pPatchedBytecode;
            });
    }

    VERIFY_EXPR(m_Data.Shaders[static_cast<size_t>(DeviceType::Direct3D11)].empty());
    for (size_t i = 0; i < ShadersD3D11.size(); ++i)
    {
        const IDataBlob* pBytecode = ShaderBytecode[i];
        ShaderCreateInfo ShaderCI  = ShaderStages[i].pSerialized->GetCreateInfo();

        ShaderCI.Source       = nullptr;
        ShaderCI.FilePath     = nullptr;
        ShaderCI.Macros       = {};
        ShaderCI.ByteCode     = pBytecode->GetConstDataPtr();
        ShaderCI.ByteCodeSize = pBytecode->GetSize();
        SerializeShaderCreateInfo(DeviceType::Direct3D11, ShaderCI);
    }
    VERIFY_EXPR(m_Data.Shaders[static_cast<size_t>(DeviceType::Direct3D11)].size() == ShadersD3D11.size());
}

INSTANTIATE_PATCH_SHADER_METHODS(PatchShadersD3D11)
INSTANTIATE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureD3D11Impl)

void SerializedShaderImpl::CreateShaderD3D11(IReferenceCounters*     pRefCounters,
                                             const ShaderCreateInfo& ShaderCI,
                                             IDataBlob**             ppCompilerOutput) noexcept(false)
{
    const ShaderD3D11Impl::CreateInfo D3D11ShaderCI{
        {
            m_pDevice->GetDeviceInfo(),
            m_pDevice->GetAdapterInfo(),
            nullptr, // pDXCompiler
            // Do not overwrite compiler output from other APIs.
            // TODO: collect all outputs.
            ppCompilerOutput == nullptr || *ppCompilerOutput == nullptr ? ppCompilerOutput : nullptr,
            m_pDevice->GetShaderCompilationThreadPool(),
        },
        static_cast<D3D_FEATURE_LEVEL>(m_pDevice->GetD3D11Properties().FeatureLevel),
    };
    CreateShader<CompiledShaderD3D11>(DeviceType::Direct3D11, pRefCounters, ShaderCI, D3D11ShaderCI, m_pDevice->GetRenderDevice(RENDER_DEVICE_TYPE_D3D11));
}

void SerializationDeviceImpl::GetPipelineResourceBindingsD3D11(const PipelineResourceBindingAttribs& Info,
                                                               std::vector<PipelineResourceBinding>& ResourceBindings)
{
    const auto            ShaderStages        = (Info.ShaderStages == SHADER_TYPE_UNKNOWN ? static_cast<SHADER_TYPE>(~0u) : Info.ShaderStages);
    constexpr SHADER_TYPE SupportedStagesMask = (SHADER_TYPE_ALL_GRAPHICS | SHADER_TYPE_COMPUTE);

    SignatureArray<PipelineResourceSignatureD3D11Impl> Signatures      = {};
    Uint32                                             SignaturesCount = 0;
    SortResourceSignatures(Info.ppResourceSignatures, Info.ResourceSignaturesCount, Signatures, SignaturesCount);

    D3D11ShaderResourceCounters BaseBindings = {};
    // In Direct3D11, UAVs use the same register space as render targets
    BaseBindings[D3D11_RESOURCE_RANGE_UAV][PSInd] = Info.NumRenderTargets;

    for (Uint32 sign = 0; sign < SignaturesCount; ++sign)
    {
        const PipelineResourceSignatureD3D11Impl* const pSignature = Signatures[sign];
        if (pSignature == nullptr)
            continue;

        for (Uint32 r = 0; r < pSignature->GetTotalResourceCount(); ++r)
        {
            const auto& ResDesc = pSignature->GetResourceDesc(r);
            const auto& ResAttr = pSignature->GetResourceAttribs(r);
            const auto  Range   = PipelineResourceSignatureD3D11Impl::ShaderResourceTypeToRange(ResDesc.ResourceType);

            for (auto Stages = (ShaderStages & SupportedStagesMask); Stages != 0;)
            {
                const auto ShaderStage = ExtractLSB(Stages);
                const auto ShaderInd   = GetShaderTypeIndex(ShaderStage);
                if ((ResDesc.ShaderStages & ShaderStage) == 0)
                    continue;

                VERIFY_EXPR(ResAttr.BindPoints.IsStageActive(ShaderInd));
                const auto Register = Uint32{BaseBindings[Range][ShaderInd]} + Uint32{ResAttr.BindPoints[ShaderInd]};
                ResourceBindings.push_back(ResDescToPipelineResBinding(ResDesc, ShaderStage, Register, 0 /*space*/));
            }
        }

        for (Uint32 samp = 0; samp < pSignature->GetImmutableSamplerCount(); ++samp)
        {
            const auto& ImtblSam = pSignature->GetImmutableSamplerDesc(samp);
            const auto& SampAttr = pSignature->GetImmutableSamplerAttribs(samp);
            const auto  Range    = D3D11_RESOURCE_RANGE_SAMPLER;

            for (auto Stages = (ShaderStages & SupportedStagesMask); Stages != 0;)
            {
                const auto ShaderStage = ExtractLSB(Stages);
                const auto ShaderInd   = GetShaderTypeIndex(ShaderStage);

                if ((ImtblSam.ShaderStages & ShaderStage) == 0)
                    continue;

                VERIFY_EXPR(SampAttr.BindPoints.IsStageActive(ShaderInd));
                const auto Binding = Uint32{BaseBindings[Range][ShaderInd]} + Uint32{SampAttr.BindPoints[ShaderInd]};

                PipelineResourceBinding Dst{};
                Dst.Name         = ImtblSam.SamplerOrTextureName;
                Dst.ResourceType = SHADER_RESOURCE_TYPE_SAMPLER;
                Dst.Register     = Binding;
                Dst.Space        = 0;
                Dst.ArraySize    = SampAttr.ArraySize;
                Dst.ShaderStages = ShaderStage;
                ResourceBindings.push_back(Dst);
            }
        }
        pSignature->ShiftBindings(BaseBindings);
    }
}

} // namespace Diligent
