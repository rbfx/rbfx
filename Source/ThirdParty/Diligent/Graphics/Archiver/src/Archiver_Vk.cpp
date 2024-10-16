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

#include "VulkanUtilities/VulkanHeaders.h"
#include "RenderDeviceVkImpl.hpp"
#include "PipelineResourceSignatureVkImpl.hpp"
#include "PipelineStateVkImpl.hpp"
#include "ShaderVkImpl.hpp"
#include "DeviceObjectArchiveVk.hpp"
#include "SerializedPipelineStateImpl.hpp"

namespace Diligent
{
namespace
{

struct CompiledShaderVk : SerializedShaderImpl::CompiledShader
{
    ShaderVkImpl ShaderVk;

    CompiledShaderVk(IReferenceCounters*             pRefCounters,
                     const ShaderCreateInfo&         ShaderCI,
                     const ShaderVkImpl::CreateInfo& VkShaderCI,
                     IRenderDevice*                  pRenderDeviceVk) :
        ShaderVk{pRefCounters, ClassPtrCast<RenderDeviceVkImpl>(pRenderDeviceVk), ShaderCI, VkShaderCI, true}
    {}

    virtual SerializedData Serialize(ShaderCreateInfo ShaderCI) const override final
    {
        const auto& SPIRV = ShaderVk.GetSPIRV();

        ShaderCI.Source       = nullptr;
        ShaderCI.FilePath     = nullptr;
        ShaderCI.Macros       = {};
        ShaderCI.ByteCode     = SPIRV.data();
        ShaderCI.ByteCodeSize = SPIRV.size() * sizeof(SPIRV[0]);
        return SerializedShaderImpl::SerializeCreateInfo(ShaderCI);
    }

    virtual IShader* GetDeviceShader() override final
    {
        return &ShaderVk;
    }

    virtual bool IsCompiling() const override final
    {
        return ShaderVk.IsCompiling();
    }

    virtual RefCntAutoPtr<IAsyncTask> GetCompileTask() const override final
    {
        return ShaderVk.GetCompileTask();
    }
};

inline const ShaderVkImpl* GetShaderVk(const SerializedShaderImpl* pShader)
{
    const auto* pCompiledShaderVk = pShader->GetShader<const CompiledShaderVk>(DeviceObjectArchive::DeviceType::Vulkan);
    return pCompiledShaderVk != nullptr ? &pCompiledShaderVk->ShaderVk : nullptr;
}

struct ShaderStageInfoVk : PipelineStateVkImpl::ShaderStageInfo
{
    ShaderStageInfoVk() :
        ShaderStageInfo{} {}

    ShaderStageInfoVk(const SerializedShaderImpl* pShader) :
        ShaderStageInfo{GetShaderVk(pShader)},
        Serialized{pShader}
    {}

    void Append(const SerializedShaderImpl* pShader)
    {
        ShaderStageInfo::Append(GetShaderVk(pShader));
        Serialized.push_back(pShader);
    }

    std::vector<const SerializedShaderImpl*> Serialized;
};
} // namespace


template <>
struct SerializedResourceSignatureImpl::SignatureTraits<PipelineResourceSignatureVkImpl>
{
    static constexpr DeviceType Type = DeviceType::Vulkan;

    template <SerializerMode Mode>
    using PRSSerializerType = PRSSerializerVk<Mode>;
};

template <typename CreateInfoType>
void SerializedPipelineStateImpl::PatchShadersVk(const CreateInfoType& CreateInfo) noexcept(false)
{
    std::vector<ShaderStageInfoVk> ShaderStages;
    SHADER_TYPE                    ActiveShaderStages    = SHADER_TYPE_UNKNOWN;
    constexpr bool                 WaitUntilShadersReady = true;
    PipelineStateUtils::ExtractShaders<SerializedShaderImpl>(CreateInfo, ShaderStages, WaitUntilShadersReady, ActiveShaderStages);

    PipelineStateVkImpl::TShaderStages ShaderStagesVk{ShaderStages.size()};
    for (size_t i = 0; i < ShaderStagesVk.size(); ++i)
    {
        auto& Src   = ShaderStages[i];
        auto& Dst   = ShaderStagesVk[i];
        Dst.Type    = Src.Type;
        Dst.Shaders = std::move(Src.Shaders);
        Dst.SPIRVs  = std::move(Src.SPIRVs);
    }

    auto** ppSignatures    = CreateInfo.ppResourceSignatures;
    auto   SignaturesCount = CreateInfo.ResourceSignaturesCount;

    IPipelineResourceSignature* DefaultSignatures[1] = {};
    if (CreateInfo.ResourceSignaturesCount == 0)
    {
        CreateDefaultResourceSignature<PipelineStateVkImpl, PipelineResourceSignatureVkImpl>(DeviceType::Vulkan, CreateInfo.PSODesc, ActiveShaderStages, ShaderStagesVk);

        DefaultSignatures[0] = m_pDefaultSignature;
        SignaturesCount      = 1;
        ppSignatures         = DefaultSignatures;
    }

    {
        // Sort signatures by binding index.
        // Note that SignaturesCount will be overwritten with the maximum binding index.
        SignatureArray<PipelineResourceSignatureVkImpl> Signatures = {};
        SortResourceSignatures(ppSignatures, SignaturesCount, Signatures, SignaturesCount);

        // Same as PipelineLayoutVk::Create()
        PipelineStateVkImpl::TBindIndexToDescSetIndex BindIndexToDescSetIndex = {};
        Uint32                                        DescSetLayoutCount      = 0;
        for (Uint32 i = 0; i < SignaturesCount; ++i)
        {
            const auto& pSignature = Signatures[i];
            if (pSignature == nullptr)
                continue;

            VERIFY_EXPR(pSignature->GetDesc().BindingIndex == i);
            BindIndexToDescSetIndex[i] = StaticCast<PipelineStateVkImpl::TBindIndexToDescSetIndex::value_type>(DescSetLayoutCount);

            for (auto SetId : {PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_STATIC_MUTABLE, PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_DYNAMIC})
            {
                if (pSignature->GetDescriptorSetSize(SetId) != ~0u)
                    ++DescSetLayoutCount;
            }
        }
        VERIFY_EXPR(DescSetLayoutCount <= MAX_RESOURCE_SIGNATURES * 2);

        const auto bStripReflection = m_Data.Aux.NoShaderReflection;
        PipelineStateVkImpl::RemapOrVerifyShaderResources(ShaderStagesVk,
                                                          Signatures.data(),
                                                          SignaturesCount,
                                                          BindIndexToDescSetIndex,
                                                          false, // bVerifyOnly
                                                          bStripReflection,
                                                          CreateInfo.PSODesc.Name);
    }

    VERIFY_EXPR(m_Data.Shaders[static_cast<size_t>(DeviceType::Vulkan)].empty());
    for (size_t j = 0; j < ShaderStagesVk.size(); ++j)
    {
        const auto& Stage = ShaderStagesVk[j];
        for (size_t i = 0; i < Stage.Count(); ++i)
        {
            const auto& SPIRV     = Stage.SPIRVs[i];
            auto        ShaderCI  = ShaderStages[j].Serialized[i]->GetCreateInfo();
            ShaderCI.Source       = nullptr;
            ShaderCI.FilePath     = nullptr;
            ShaderCI.Macros       = {};
            ShaderCI.ByteCode     = SPIRV.data();
            ShaderCI.ByteCodeSize = SPIRV.size() * sizeof(SPIRV[0]);
            SerializeShaderCreateInfo(DeviceType::Vulkan, ShaderCI);
        }
    }
}

INSTANTIATE_PATCH_SHADER_METHODS(PatchShadersVk)
INSTANTIATE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureVkImpl)

void SerializedShaderImpl::CreateShaderVk(IReferenceCounters*     pRefCounters,
                                          const ShaderCreateInfo& ShaderCI,
                                          IDataBlob**             ppCompilerOutput)
{
    const auto& VkProps         = m_pDevice->GetVkProperties();
    const auto& DeviceInfo      = m_pDevice->GetDeviceInfo();
    const auto& AdapterInfo     = m_pDevice->GetAdapterInfo();
    auto*       pRenderDeviceVk = m_pDevice->GetRenderDevice(RENDER_DEVICE_TYPE_VULKAN);

    const ShaderVkImpl::CreateInfo VkShaderCI{
        VkProps.pDxCompiler,
        DeviceInfo,
        AdapterInfo,
        VkProps.VkVersion,
        VkProps.SupportsSpirv14,
        // Do not overwrite compiler output from other APIs.
        // TODO: collect all outputs.
        ppCompilerOutput == nullptr || *ppCompilerOutput == nullptr ? ppCompilerOutput : nullptr,
        m_pDevice->GetShaderCompilationThreadPool(),
    };
    CreateShader<CompiledShaderVk>(DeviceType::Vulkan, pRefCounters, ShaderCI, VkShaderCI, pRenderDeviceVk);
}

void SerializationDeviceImpl::GetPipelineResourceBindingsVk(const PipelineResourceBindingAttribs& Info,
                                                            std::vector<PipelineResourceBinding>& ResourceBindings)
{
    const auto ShaderStages = (Info.ShaderStages == SHADER_TYPE_UNKNOWN ? static_cast<SHADER_TYPE>(~0u) : Info.ShaderStages);

    SignatureArray<PipelineResourceSignatureVkImpl> Signatures      = {};
    Uint32                                          SignaturesCount = 0;
    SortResourceSignatures(Info.ppResourceSignatures, Info.ResourceSignaturesCount, Signatures, SignaturesCount);

    Uint32 DescSetLayoutCount = 0;
    for (Uint32 sign = 0; sign < SignaturesCount; ++sign)
    {
        const auto& pSignature = Signatures[sign];
        if (pSignature == nullptr)
            continue;

        for (Uint32 r = 0; r < pSignature->GetTotalResourceCount(); ++r)
        {
            const auto& ResDesc = pSignature->GetResourceDesc(r);
            const auto& ResAttr = pSignature->GetResourceAttribs(r);
            if ((ResDesc.ShaderStages & ShaderStages) == 0)
                continue;

            ResourceBindings.push_back(ResDescToPipelineResBinding(ResDesc, ResDesc.ShaderStages, ResAttr.BindingIndex, DescSetLayoutCount + ResAttr.DescrSet));
        }

        // Same as PipelineLayoutVk::Create()
        for (auto SetId : {PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_STATIC_MUTABLE, PipelineResourceSignatureVkImpl::DESCRIPTOR_SET_ID_DYNAMIC})
        {
            if (pSignature->GetDescriptorSetSize(SetId) != ~0u)
                ++DescSetLayoutCount;
        }
    }
    VERIFY_EXPR(DescSetLayoutCount <= MAX_RESOURCE_SIGNATURES * 2);
    VERIFY_EXPR(DescSetLayoutCount >= Info.ResourceSignaturesCount);
}

void SerializedPipelineStateImpl::ExtractShadersVk(const RayTracingPipelineStateCreateInfo& CreateInfo, RayTracingShaderMapType& ShaderMap)
{
    std::vector<ShaderStageInfoVk> ShaderStages;
    SHADER_TYPE                    ActiveShaderStages    = SHADER_TYPE_UNKNOWN;
    constexpr bool                 WaitUntilShadersReady = true;
    PipelineStateUtils::ExtractShaders<SerializedShaderImpl>(CreateInfo, ShaderStages, WaitUntilShadersReady, ActiveShaderStages);

    GetRayTracingShaderMap(ShaderStages, ShaderMap);
}

} // namespace Diligent
