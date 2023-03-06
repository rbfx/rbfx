/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include <memory>
#include <array>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "PSOSerializer.hpp"

namespace Diligent
{

namespace
{

template <typename SignatureType>
using SignatureArray = std::array<RefCntAutoPtr<SignatureType>, MAX_RESOURCE_SIGNATURES>;

template <typename SignatureType>
void SortResourceSignatures(IPipelineResourceSignature**                ppSrcSignatures,
                            Uint32                                      SrcSignaturesCount,
                            SignatureArray<SignatureType>&              SortedSignatures,
                            Uint32&                                     SortedSignaturesCount,
                            SerializedResourceSignatureImpl::DeviceType Type)
{
    for (Uint32 i = 0; i < SrcSignaturesCount; ++i)
    {
        const auto* pSerPRS = ClassPtrCast<SerializedResourceSignatureImpl>(ppSrcSignatures[i]);
        VERIFY_EXPR(pSerPRS != nullptr);

        const auto& Desc = pSerPRS->GetDesc();

        VERIFY(!SortedSignatures[Desc.BindingIndex], "Multiple signatures use the same binding index (", Desc.BindingIndex, ").");
        SortedSignatures[Desc.BindingIndex] = pSerPRS->template GetDeviceSignature<SignatureType>(Type);

        SortedSignaturesCount = std::max(SortedSignaturesCount, Uint32{Desc.BindingIndex} + 1u);
    }
}

template <typename SignatureType>
void SortResourceSignatures(IPipelineResourceSignature**   ppSrcSignatures,
                            Uint32                         SrcSignaturesCount,
                            SignatureArray<SignatureType>& SortedSignatures,
                            Uint32&                        SortedSignaturesCount)
{
    constexpr auto Type = SerializedResourceSignatureImpl::SignatureTraits<SignatureType>::Type;
    SortResourceSignatures<SignatureType>(ppSrcSignatures, SrcSignaturesCount,
                                          SortedSignatures, SortedSignaturesCount,
                                          Type);
}

} // namespace

template <typename PipelineStateImplType, typename SignatureImplType, typename ShaderStagesArrayType, typename... ExtraArgsType>
void SerializedPipelineStateImpl::CreateDefaultResourceSignature(DeviceType                   Type,
                                                                 const PipelineStateDesc&     PSODesc,
                                                                 SHADER_TYPE                  ActiveShaderStageFlags,
                                                                 const ShaderStagesArrayType& ShaderStages,
                                                                 const ExtraArgsType&... ExtraArgs)
{
    auto SignDesc = PipelineStateImplType::GetDefaultResourceSignatureDesc(ShaderStages, PSODesc.Name, PSODesc.ResourceLayout, PSODesc.SRBAllocationGranularity, ExtraArgs...);
    if (!m_pDefaultSignature)
    {
        // Create empty serialized signature
        m_pSerializationDevice->CreateSerializedResourceSignature(&m_pDefaultSignature, SignDesc.Get().Name);
        if (!m_pDefaultSignature)
            LOG_ERROR_AND_THROW("Failed to create default resource signature for PSO '", PSODesc.Name, "'.");
    }
    else
    {
        // Override the name to make sure it is consistent for all devices
        SignDesc.SetName(m_pDefaultSignature->GetName());
    }

    m_pDefaultSignature->CreateDeviceSignature<SignatureImplType>(Type, SignDesc, ActiveShaderStageFlags);
}

template <typename ShaderType, typename... ArgTypes>
void SerializedShaderImpl::CreateShader(DeviceType              Type,
                                        IReferenceCounters*     pRefCounters,
                                        const ShaderCreateInfo& ShaderCI,
                                        const ArgTypes&... Args)
{
    VERIFY(!m_Shaders[static_cast<size_t>(Type)], "Shader has already been initialized for this device type");
    m_Shaders[static_cast<size_t>(Type)] = std::make_unique<ShaderType>(pRefCounters, ShaderCI, Args...);
}


template <typename ImplType>
struct SerializedResourceSignatureImpl::TPRS final : PRSWapperBase
{
    ImplType PRS;

    TPRS(IReferenceCounters* pRefCounters, const PipelineResourceSignatureDesc& SignatureDesc, SHADER_TYPE ShaderStages) :
        PRS{pRefCounters, nullptr, SignatureDesc, ShaderStages, true /*Pretend device internal to allow null device*/}
    {}

    IPipelineResourceSignature* GetPRS() override { return &PRS; }
};


template <typename SignatureImplType>
void SerializedResourceSignatureImpl::CreateDeviceSignature(DeviceType                           Type,
                                                            const PipelineResourceSignatureDesc& Desc,
                                                            SHADER_TYPE                          ShaderStages)
{
    using Traits                = SignatureTraits<SignatureImplType>;
    using MeasureSerializerType = typename Traits::template PRSSerializerType<SerializerMode::Measure>;
    using WriteSerializerType   = typename Traits::template PRSSerializerType<SerializerMode::Write>;

    VERIFY_EXPR(Type == Traits::Type || (Type == DeviceType::Metal_iOS && Traits::Type == DeviceType::Metal_MacOS));
    VERIFY(!m_pDeviceSignatures[static_cast<size_t>(Type)], "Signature for this device type has already been initialized");

    auto  pDeviceSignature = std::make_unique<TPRS<SignatureImplType>>(GetReferenceCounters(), Desc, ShaderStages);
    auto& DeviceSignature  = *pDeviceSignature;
    // We must initialize at least one device signature before calling InitCommonData()
    m_pDeviceSignatures[static_cast<size_t>(Type)] = std::move(pDeviceSignature);

    const auto& SignDesc = DeviceSignature.GetPRS()->GetDesc();
    InitCommonData(SignDesc);

    const auto InternalData = DeviceSignature.PRS.GetInternalData();

    const auto& CommonDesc = GetDesc();

    // Check if the device signature description differs from common description
    bool SpecialDesc = !(CommonDesc == SignDesc);

    {
        Serializer<SerializerMode::Measure> MeasureSer;

        MeasureSer(SpecialDesc);
        if (SpecialDesc)
            MeasureSerializerType::SerializeDesc(MeasureSer, SignDesc, nullptr);

        MeasureSerializerType::SerializeInternalData(MeasureSer, InternalData, nullptr);

        DeviceSignature.Data = MeasureSer.AllocateData(GetRawAllocator());
    }

    {
        Serializer<SerializerMode::Write> Ser{DeviceSignature.Data};

        Ser(SpecialDesc);
        if (SpecialDesc)
            WriteSerializerType::SerializeDesc(Ser, SignDesc, nullptr);

        WriteSerializerType::SerializeInternalData(Ser, InternalData, nullptr);

        VERIFY_EXPR(Ser.IsEnded());
    }
}

} // namespace Diligent
