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
 *  In no event and under no legal theory, whether in tort (including neWebGPUigence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly neWebGPUigent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include <webgpu/webgpu.h>

#include "ArchiverImpl.hpp"
#include "Archiver_Inc.hpp"

#include "RenderDeviceWebGPUImpl.hpp"
#include "PipelineResourceSignatureWebGPUImpl.hpp"
#include "PipelineStateWebGPUImpl.hpp"
#include "ShaderWebGPUImpl.hpp"
#include "DeviceObjectArchiveWebGPU.hpp"
#include "SerializedPipelineStateImpl.hpp"
#include "ShaderToolsCommon.hpp"

namespace Diligent
{

namespace
{

struct CompiledShaderWebGPU final : SerializedShaderImpl::CompiledShader
{
    ShaderWebGPUImpl ShaderWebGPU;

    CompiledShaderWebGPU(IReferenceCounters*                 pRefCounters,
                         const ShaderCreateInfo&             ShaderCI,
                         const ShaderWebGPUImpl::CreateInfo& WebGPUShaderCI,
                         IRenderDevice*                      pRenderDeviceWebGPU) :
        ShaderWebGPU{pRefCounters, ClassPtrCast<RenderDeviceWebGPUImpl>(pRenderDeviceWebGPU), ShaderCI, WebGPUShaderCI, true}
    {}

    virtual SerializedData Serialize(ShaderCreateInfo ShaderCI) const override final
    {
        const std::string& WGSL = ShaderWebGPU.GetWGSL();

        ShaderCI.Source         = WGSL.c_str();
        ShaderCI.SourceLength   = WGSL.length();
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_WGSL;
        ShaderCI.FilePath       = nullptr;
        ShaderCI.Macros         = {};
        ShaderCI.ByteCode       = nullptr;
        return SerializedShaderImpl::SerializeCreateInfo(ShaderCI);
    }

    virtual IShader* GetDeviceShader() override final
    {
        return &ShaderWebGPU;
    }

    virtual bool IsCompiling() const override final
    {
        return ShaderWebGPU.IsCompiling();
    }

    virtual RefCntAutoPtr<IAsyncTask> GetCompileTask() const override final
    {
        return ShaderWebGPU.GetCompileTask();
    }
};


struct ShaderStageInfoWebGPU
{
    ShaderStageInfoWebGPU() {}

    ShaderStageInfoWebGPU(const SerializedShaderImpl* _pShader) :
        Type{_pShader->GetDesc().ShaderType},
        pShader{GetShaderWebGPU(_pShader)},
        pSerialized{_pShader}
    {
    }

    // Needed only for ray tracing
    void Append(const SerializedShaderImpl*) {}

    constexpr Uint32 Count() const { return 1; }

    SHADER_TYPE                 Type        = SHADER_TYPE_UNKNOWN;
    ShaderWebGPUImpl*           pShader     = nullptr;
    const SerializedShaderImpl* pSerialized = nullptr;

private:
    static ShaderWebGPUImpl* GetShaderWebGPU(const SerializedShaderImpl* pShader)
    {
        auto* pCompiledShaderWebGPU = pShader->GetShader<CompiledShaderWebGPU>(DeviceObjectArchive::DeviceType::WebGPU);
        return pCompiledShaderWebGPU != nullptr ? &pCompiledShaderWebGPU->ShaderWebGPU : nullptr;
    }
};

#ifdef DILIGENT_DEBUG
inline SHADER_TYPE GetShaderStageType(const ShaderStageInfoWebGPU& Stage)
{
    return Stage.Type;
}
#endif

} // namespace

template <>
struct SerializedResourceSignatureImpl::SignatureTraits<PipelineResourceSignatureWebGPUImpl>
{
    static constexpr DeviceType Type = DeviceType::WebGPU;

    template <SerializerMode Mode>
    using PRSSerializerType = PRSSerializerWebGPU<Mode>;
};

template <typename CreateInfoType>
void SerializedPipelineStateImpl::PatchShadersWebGPU(const CreateInfoType& CreateInfo) noexcept(false)
{
    std::vector<ShaderStageInfoWebGPU> ShaderStages;
    SHADER_TYPE                        ActiveShaderStages    = SHADER_TYPE_UNKNOWN;
    constexpr bool                     WaitUntilShadersReady = true;
    PipelineStateUtils::ExtractShaders<SerializedShaderImpl>(CreateInfo, ShaderStages, WaitUntilShadersReady, ActiveShaderStages);

    PipelineStateWebGPUImpl::TShaderStages ShaderStagesWebGPU;
    ShaderStagesWebGPU.reserve(ShaderStages.size());
    for (ShaderStageInfoWebGPU& Src : ShaderStages)
    {
        ShaderStagesWebGPU.emplace_back(Src.pShader);
    }

    auto** ppSignatures    = CreateInfo.ppResourceSignatures;
    auto   SignaturesCount = CreateInfo.ResourceSignaturesCount;

    IPipelineResourceSignature* DefaultSignatures[1] = {};
    if (CreateInfo.ResourceSignaturesCount == 0)
    {
        CreateDefaultResourceSignature<PipelineStateWebGPUImpl, PipelineResourceSignatureWebGPUImpl>(DeviceType::WebGPU, CreateInfo.PSODesc, ActiveShaderStages, ShaderStagesWebGPU);

        DefaultSignatures[0] = m_pDefaultSignature;
        SignaturesCount      = 1;
        ppSignatures         = DefaultSignatures;
    }

    {
        // Sort signatures by binding index.
        // Note that SignaturesCount will be overwritten with the maximum binding index.
        SignatureArray<PipelineResourceSignatureWebGPUImpl> Signatures = {};
        SortResourceSignatures(ppSignatures, SignaturesCount, Signatures, SignaturesCount);

        // Same as PipelineLayoutWebGPU::Create()
        PipelineStateWebGPUImpl::TBindIndexToBindGroupIndex BindIndexToBGIndex = {};

        Uint32 BindGroupLayoutCount = 0;
        for (Uint32 i = 0; i < SignaturesCount; ++i)
        {
            const auto& pSignature = Signatures[i];
            if (pSignature == nullptr)
                continue;

            VERIFY_EXPR(pSignature->GetDesc().BindingIndex == i);
            BindIndexToBGIndex[i] = StaticCast<PipelineStateWebGPUImpl::TBindIndexToBindGroupIndex::value_type>(BindGroupLayoutCount);

            for (auto GroupId : {PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_STATIC_MUTABLE, PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_DYNAMIC})
            {
                if (pSignature->HasBindGroup(GroupId))
                    ++BindGroupLayoutCount;
            }
        }
        VERIFY_EXPR(BindGroupLayoutCount <= MAX_RESOURCE_SIGNATURES * 2);

        PipelineStateWebGPUImpl::RemapOrVerifyShaderResources(ShaderStagesWebGPU,
                                                              Signatures.data(),
                                                              SignaturesCount,
                                                              BindIndexToBGIndex,
                                                              false, // bVerifyOnly
                                                              CreateInfo.PSODesc.Name);
    }

    VERIFY_EXPR(m_Data.Shaders[static_cast<size_t>(DeviceType::WebGPU)].empty());
    for (size_t i = 0; i < ShaderStagesWebGPU.size(); ++i)
    {
        const std::string& WGSL     = ShaderStagesWebGPU[i].GetWGSL();
        ShaderCreateInfo   ShaderCI = ShaderStages[i].pSerialized->GetCreateInfo();

        ShaderCI.Source         = WGSL.c_str();
        ShaderCI.SourceLength   = WGSL.length();
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_WGSL;
        ShaderCI.EntryPoint     = ShaderStagesWebGPU[i].pShader->GetEntryPoint();
        ShaderCI.FilePath       = nullptr;
        ShaderCI.Macros         = {};
        ShaderCI.ByteCode       = nullptr;
        SerializeShaderCreateInfo(DeviceType::WebGPU, ShaderCI);
    }
}

INSTANTIATE_PATCH_SHADER_METHODS(PatchShadersWebGPU)
INSTANTIATE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureWebGPUImpl)

void SerializationDeviceImpl::GetPipelineResourceBindingsWebGPU(const PipelineResourceBindingAttribs& Info,
                                                                std::vector<PipelineResourceBinding>& ResourceBindings)
{
    const auto ShaderStages = (Info.ShaderStages == SHADER_TYPE_UNKNOWN ? static_cast<SHADER_TYPE>(~0u) : Info.ShaderStages);

    SignatureArray<PipelineResourceSignatureWebGPUImpl> Signatures      = {};
    Uint32                                              SignaturesCount = 0;
    SortResourceSignatures(Info.ppResourceSignatures, Info.ResourceSignaturesCount, Signatures, SignaturesCount);

    Uint32 BindGroupCount = 0;
    for (Uint32 sign = 0; sign < SignaturesCount; ++sign)
    {
        const PipelineResourceSignatureWebGPUImpl* pSignature = Signatures[sign];
        if (pSignature == nullptr)
            continue;

        for (Uint32 r = 0; r < pSignature->GetTotalResourceCount(); ++r)
        {
            const PipelineResourceDesc&          ResDesc = pSignature->GetResourceDesc(r);
            const PipelineResourceAttribsWebGPU& ResAttr = pSignature->GetResourceAttribs(r);
            if ((ResDesc.ShaderStages & ShaderStages) == 0)
                continue;

            ResourceBindings.push_back(ResDescToPipelineResBinding(ResDesc, ResDesc.ShaderStages, ResAttr.BindingIndex, BindGroupCount + ResAttr.BindGroup));
        }

        // Same as PipelineLayoutWebGPU::Create()
        for (auto GroupId : {PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_STATIC_MUTABLE, PipelineResourceSignatureWebGPUImpl::BIND_GROUP_ID_DYNAMIC})
        {
            if (pSignature->HasBindGroup(GroupId))
                ++BindGroupCount;
        }
    }
    VERIFY_EXPR(BindGroupCount <= MAX_RESOURCE_SIGNATURES * 2);
    VERIFY_EXPR(BindGroupCount >= Info.ResourceSignaturesCount);
}

void SerializedShaderImpl::CreateShaderWebGPU(IReferenceCounters*     pRefCounters,
                                              const ShaderCreateInfo& ShaderCI,
                                              IDataBlob**             ppCompilerOutput) noexcept(false)
{
    const ShaderWebGPUImpl::CreateInfo WebGPUShaderCI{
        m_pDevice->GetDeviceInfo(),
        m_pDevice->GetAdapterInfo(),
        // Do not overwrite compiler output from other APIs.
        // TODO: collect all outputs.
        ppCompilerOutput == nullptr || *ppCompilerOutput == nullptr ? ppCompilerOutput : nullptr,
        m_pDevice->GetShaderCompilationThreadPool(),
    };
    CreateShader<CompiledShaderWebGPU>(DeviceType::WebGPU, pRefCounters, ShaderCI, WebGPUShaderCI, m_pDevice->GetRenderDevice(RENDER_DEVICE_TYPE_WEBGPU));
}

} // namespace Diligent
