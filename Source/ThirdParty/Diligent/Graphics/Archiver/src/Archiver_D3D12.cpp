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

#include "../../GraphicsEngineD3D12/include/pch.h"
#include "RenderDeviceD3D12Impl.hpp"
#include "PipelineResourceSignatureD3D12Impl.hpp"
#include "PipelineStateD3D12Impl.hpp"
#include "ShaderD3D12Impl.hpp"
#include "DeviceObjectArchiveD3D12.hpp"
#include "SerializedPipelineStateImpl.hpp"

namespace Diligent
{
namespace
{

struct CompiledShaderD3D12 final : SerializedShaderImpl::CompiledShader
{
    ShaderD3D12Impl ShaderD3D12;

    CompiledShaderD3D12(IReferenceCounters*                pRefCounters,
                        const ShaderCreateInfo&            ShaderCI,
                        const ShaderD3D12Impl::CreateInfo& D3D12ShaderCI,
                        IRenderDevice*                     pRenderDeviceD3D12) :
        ShaderD3D12{pRefCounters, ClassPtrCast<RenderDeviceD3D12Impl>(pRenderDeviceD3D12), ShaderCI, D3D12ShaderCI, true}
    {}

    virtual SerializedData Serialize(ShaderCreateInfo ShaderCI) const override final
    {
        const IDataBlob* pBytecode = ShaderD3D12.GetD3DBytecode();

        ShaderCI.Source       = nullptr;
        ShaderCI.FilePath     = nullptr;
        ShaderCI.Macros       = {};
        ShaderCI.ByteCode     = pBytecode->GetConstDataPtr();
        ShaderCI.ByteCodeSize = pBytecode->GetSize();
        return SerializedShaderImpl::SerializeCreateInfo(ShaderCI);
    }

    virtual IShader* GetDeviceShader() override final
    {
        return &ShaderD3D12;
    }

    virtual bool IsCompiling() const override final
    {
        return ShaderD3D12.IsCompiling();
    }

    virtual RefCntAutoPtr<IAsyncTask> GetCompileTask() const override final
    {
        return ShaderD3D12.GetCompileTask();
    }
};

inline const ShaderD3D12Impl* GetShaderD3D12(const SerializedShaderImpl* pShader)
{
    const auto* pCompiledShaderD3D12 = pShader->GetShader<const CompiledShaderD3D12>(DeviceObjectArchive::DeviceType::Direct3D12);
    return pCompiledShaderD3D12 != nullptr ? &pCompiledShaderD3D12->ShaderD3D12 : nullptr;
}

struct ShaderStageInfoD3D12 : PipelineStateD3D12Impl::ShaderStageInfo
{
    ShaderStageInfoD3D12() :
        ShaderStageInfo{} {}

    ShaderStageInfoD3D12(const SerializedShaderImpl* pShader) :
        ShaderStageInfo{GetShaderD3D12(pShader)},
        Serialized{pShader}
    {}

    void Append(const SerializedShaderImpl* pShader)
    {
        ShaderStageInfo::Append(GetShaderD3D12(pShader));
        Serialized.push_back(pShader);
    }

    std::vector<const SerializedShaderImpl*> Serialized;
};
} // namespace

template <>
struct SerializedResourceSignatureImpl::SignatureTraits<PipelineResourceSignatureD3D12Impl>
{
    static constexpr DeviceType Type = DeviceType::Direct3D12;

    template <SerializerMode Mode>
    using PRSSerializerType = PRSSerializerD3D12<Mode>;
};

template <typename CreateInfoType>
void SerializedPipelineStateImpl::PatchShadersD3D12(const CreateInfoType& CreateInfo) noexcept(false)
{
    std::vector<ShaderStageInfoD3D12> ShaderStages;
    SHADER_TYPE                       ActiveShaderStages    = SHADER_TYPE_UNKNOWN;
    constexpr bool                    WaitUntilShadersReady = true;
    PipelineStateUtils::ExtractShaders<SerializedShaderImpl>(CreateInfo, ShaderStages, WaitUntilShadersReady, ActiveShaderStages);

    PipelineStateD3D12Impl::TShaderStages ShaderStagesD3D12{ShaderStages.size()};
    for (size_t i = 0; i < ShaderStagesD3D12.size(); ++i)
    {
        auto& Src     = ShaderStages[i];
        auto& Dst     = ShaderStagesD3D12[i];
        Dst.Type      = Src.Type;
        Dst.Shaders   = std::move(Src.Shaders);
        Dst.ByteCodes = std::move(Src.ByteCodes);
    }

    auto** ppSignatures    = CreateInfo.ppResourceSignatures;
    auto   SignaturesCount = CreateInfo.ResourceSignaturesCount;

    IPipelineResourceSignature* DefaultSignatures[1] = {};
    if (CreateInfo.ResourceSignaturesCount == 0)
    {
        CreateDefaultResourceSignature<PipelineStateD3D12Impl, PipelineResourceSignatureD3D12Impl>(DeviceType::Direct3D12, CreateInfo.PSODesc, ActiveShaderStages, ShaderStagesD3D12, nullptr);

        DefaultSignatures[0] = m_pDefaultSignature;
        SignaturesCount      = 1;
        ppSignatures         = DefaultSignatures;
    }

    {
        // Sort signatures by binding index.
        // Note that SignaturesCount will be overwritten with the maximum binding index.
        SignatureArray<PipelineResourceSignatureD3D12Impl> Signatures = {};
        SortResourceSignatures(ppSignatures, SignaturesCount, Signatures, SignaturesCount);

        RootSignatureD3D12 RootSig{nullptr, nullptr, Signatures.data(), SignaturesCount, 0};
        PipelineStateD3D12Impl::RemapOrVerifyShaderResources(ShaderStagesD3D12,
                                                             Signatures.data(),
                                                             SignaturesCount,
                                                             RootSig,
                                                             m_pSerializationDevice->GetD3D12Properties().pDxCompiler);
    }

    VERIFY_EXPR(m_Data.Shaders[static_cast<size_t>(DeviceType::Direct3D12)].empty());
    for (size_t j = 0; j < ShaderStagesD3D12.size(); ++j)
    {
        const auto& Stage = ShaderStagesD3D12[j];
        for (size_t i = 0; i < Stage.Count(); ++i)
        {
            const IDataBlob* pBytecode = Stage.ByteCodes[i];
            ShaderCreateInfo ShaderCI  = ShaderStages[j].Serialized[i]->GetCreateInfo();

            ShaderCI.Source       = nullptr;
            ShaderCI.FilePath     = nullptr;
            ShaderCI.Macros       = {};
            ShaderCI.ByteCode     = pBytecode->GetConstDataPtr();
            ShaderCI.ByteCodeSize = pBytecode->GetSize();
            SerializeShaderCreateInfo(DeviceType::Direct3D12, ShaderCI);
        }
    }
}

INSTANTIATE_PATCH_SHADER_METHODS(PatchShadersD3D12)
INSTANTIATE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureD3D12Impl)


void SerializedShaderImpl::CreateShaderD3D12(IReferenceCounters*     pRefCounters,
                                             const ShaderCreateInfo& ShaderCI,
                                             IDataBlob**             ppCompilerOutput) noexcept(false)
{
    const auto& D3D12Props         = m_pDevice->GetD3D12Properties();
    const auto& DeviceInfo         = m_pDevice->GetDeviceInfo();
    const auto& AdapterInfo        = m_pDevice->GetAdapterInfo();
    auto*       pRenderDeviceD3D12 = m_pDevice->GetRenderDevice(RENDER_DEVICE_TYPE_D3D12);

    const ShaderD3D12Impl::CreateInfo D3D12ShaderCI{
        {
            DeviceInfo,
            AdapterInfo,
            D3D12Props.pDxCompiler,
            // Do not overwrite compiler output from other APIs.
            // TODO: collect all outputs.
            ppCompilerOutput == nullptr || *ppCompilerOutput == nullptr ? ppCompilerOutput : nullptr,
            m_pDevice->GetShaderCompilationThreadPool(),
        },
        D3D12Props.ShaderVersion,
    };
    CreateShader<CompiledShaderD3D12>(DeviceType::Direct3D12, pRefCounters, ShaderCI, D3D12ShaderCI, pRenderDeviceD3D12);
}

void SerializationDeviceImpl::GetPipelineResourceBindingsD3D12(const PipelineResourceBindingAttribs& Info,
                                                               std::vector<PipelineResourceBinding>& ResourceBindings)
{
    const auto ShaderStages = (Info.ShaderStages == SHADER_TYPE_UNKNOWN ? static_cast<SHADER_TYPE>(~0u) : Info.ShaderStages);

    SignatureArray<PipelineResourceSignatureD3D12Impl> Signatures      = {};
    Uint32                                             SignaturesCount = 0;
    SortResourceSignatures(Info.ppResourceSignatures, Info.ResourceSignaturesCount, Signatures, SignaturesCount);

    RootSignatureD3D12 RootSig{nullptr, nullptr, Signatures.data(), SignaturesCount, 0};
    for (Uint32 sign = 0; sign < SignaturesCount; ++sign)
    {
        const auto& pSignature = Signatures[sign];
        if (pSignature == nullptr)
            continue;

        const auto BaseRegisterSpace = RootSig.GetBaseRegisterSpace(sign);

        for (Uint32 r = 0; r < pSignature->GetTotalResourceCount(); ++r)
        {
            const auto& ResDesc = pSignature->GetResourceDesc(r);
            const auto& ResAttr = pSignature->GetResourceAttribs(r);
            if ((ResDesc.ShaderStages & ShaderStages) == 0)
                continue;

            ResourceBindings.push_back(ResDescToPipelineResBinding(ResDesc, ResDesc.ShaderStages, ResAttr.Register, BaseRegisterSpace + ResAttr.Space));
        }
    }
}

void SerializedPipelineStateImpl::ExtractShadersD3D12(const RayTracingPipelineStateCreateInfo& CreateInfo, RayTracingShaderMapType& ShaderMap)
{
    std::vector<ShaderStageInfoD3D12> ShaderStages;
    SHADER_TYPE                       ActiveShaderStages    = SHADER_TYPE_UNKNOWN;
    constexpr bool                    WaitUntilShadersReady = true;
    PipelineStateUtils::ExtractShaders<SerializedShaderImpl>(CreateInfo, ShaderStages, WaitUntilShadersReady, ActiveShaderStages);

    GetRayTracingShaderMap(ShaderStages, ShaderMap);
}

} // namespace Diligent
