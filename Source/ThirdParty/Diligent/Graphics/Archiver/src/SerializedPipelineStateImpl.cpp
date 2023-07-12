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

#include <bitset>

#include "SerializedPipelineStateImpl.hpp"
#include "Constants.h"
#include "SerializationDeviceImpl.hpp"
#include "SerializedResourceSignatureImpl.hpp"
#include "SerializedShaderImpl.hpp"
#include "PSOSerializer.hpp"
#include "Align.hpp"
#include "FileSystem.hpp"

namespace Diligent
{

DeviceObjectArchive::DeviceType ArchiveDeviceDataFlagToArchiveDeviceType(ARCHIVE_DEVICE_DATA_FLAGS DataTypeFlag);

namespace
{

#define LOG_PSO_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of PSO is invalid: ", ##__VA_ARGS__)
#define VERIFY_PSO(Expr, ...)                     \
    do                                            \
    {                                             \
        if (!(Expr))                              \
        {                                         \
            LOG_PSO_ERROR_AND_THROW(__VA_ARGS__); \
        }                                         \
    } while (false)

void ValidatePipelineStateArchiveInfo(const PipelineStateCreateInfo&  PSOCreateInfo,
                                      const PipelineStateArchiveInfo& ArchiveInfo,
                                      const ARCHIVE_DEVICE_DATA_FLAGS ValidDeviceFlags) noexcept(false)
{
    VERIFY_PSO(ArchiveInfo.DeviceFlags != ARCHIVE_DEVICE_DATA_FLAG_NONE, "At least one bit must be set in DeviceFlags");
    VERIFY_PSO((ArchiveInfo.DeviceFlags & ValidDeviceFlags) == ArchiveInfo.DeviceFlags, "DeviceFlags contain unsupported device type");

    VERIFY_PSO(PSOCreateInfo.PSODesc.Name != nullptr, "Pipeline name in PSOCreateInfo.PSODesc.Name must not be null");
    VERIFY_PSO((PSOCreateInfo.ResourceSignaturesCount != 0) == (PSOCreateInfo.ppResourceSignatures != nullptr),
               "ppResourceSignatures must not be null if ResourceSignaturesCount is not zero");

    std::bitset<MAX_RESOURCE_SIGNATURES> PRSExists{0};
    for (Uint32 i = 0; i < PSOCreateInfo.ResourceSignaturesCount; ++i)
    {
        VERIFY_PSO(PSOCreateInfo.ppResourceSignatures[i] != nullptr, "ppResourceSignatures[", i, "] must not be null");

        const auto& Desc = PSOCreateInfo.ppResourceSignatures[i]->GetDesc();
        VERIFY_EXPR(Desc.BindingIndex < PRSExists.size());

        VERIFY_PSO(!PRSExists[Desc.BindingIndex], "PRS binding index must be unique");
        PRSExists[Desc.BindingIndex] = true;
    }
}

#undef LOG_PSO_ERROR_AND_THROW


template <SerializerMode Mode>
void SerializePSOCreateInfo(Serializer<Mode>&                                 Ser,
                            const GraphicsPipelineStateCreateInfo&            PSOCreateInfo,
                            std::array<const char*, MAX_RESOURCE_SIGNATURES>& PRSNames)
{
    const char* RPName = PSOCreateInfo.GraphicsPipeline.pRenderPass != nullptr ? PSOCreateInfo.GraphicsPipeline.pRenderPass->GetDesc().Name : "";
    PSOSerializer<Mode>::SerializeCreateInfo(Ser, PSOCreateInfo, PRSNames, nullptr, RPName);
}

template <SerializerMode Mode>
void SerializePSOCreateInfo(Serializer<Mode>&                                 Ser,
                            const ComputePipelineStateCreateInfo&             PSOCreateInfo,
                            std::array<const char*, MAX_RESOURCE_SIGNATURES>& PRSNames)
{
    PSOSerializer<Mode>::SerializeCreateInfo(Ser, PSOCreateInfo, PRSNames, nullptr);
}

template <SerializerMode Mode>
void SerializePSOCreateInfo(Serializer<Mode>&                                 Ser,
                            const TilePipelineStateCreateInfo&                PSOCreateInfo,
                            std::array<const char*, MAX_RESOURCE_SIGNATURES>& PRSNames)
{
    PSOSerializer<Mode>::SerializeCreateInfo(Ser, PSOCreateInfo, PRSNames, nullptr);
}

template <SerializerMode Mode>
void SerializePSOCreateInfo(Serializer<Mode>&                                 Ser,
                            const RayTracingPipelineStateCreateInfo&          PSOCreateInfo,
                            std::array<const char*, MAX_RESOURCE_SIGNATURES>& PRSNames)
{
    using RayTracingShaderMapType = SerializedPipelineStateImpl::RayTracingShaderMapType;
    RayTracingShaderMapType ShaderMapVk;
    RayTracingShaderMapType ShaderMapD3D12;
#if VULKAN_SUPPORTED
    SerializedPipelineStateImpl::ExtractShadersVk(PSOCreateInfo, ShaderMapVk);
    VERIFY_EXPR(!ShaderMapVk.empty());
#endif
#if D3D12_SUPPORTED
    SerializedPipelineStateImpl::ExtractShadersD3D12(PSOCreateInfo, ShaderMapD3D12);
    VERIFY_EXPR(!ShaderMapD3D12.empty());
#endif

    VERIFY(ShaderMapVk.empty() || ShaderMapD3D12.empty() || ShaderMapVk == ShaderMapD3D12,
           "Ray tracing shader map must be same for Vulkan and Direct3D12 backends");

    RayTracingShaderMapType ShaderMap;
    if (!ShaderMapVk.empty())
        std::swap(ShaderMap, ShaderMapVk);
    else if (!ShaderMapD3D12.empty())
        std::swap(ShaderMap, ShaderMapD3D12);
    else
        return;

    auto RemapShaders = [&ShaderMap](Uint32& outIndex, IShader* const& inShader) //
    {
        auto Iter = ShaderMap.find(inShader);
        if (Iter != ShaderMap.end())
            outIndex = Iter->second;
        else
            outIndex = ~0u;
    };
    PSOSerializer<Mode>::SerializeCreateInfo(Ser, PSOCreateInfo, PRSNames, nullptr, RemapShaders);
}

template <typename PSOCreateInfoType>
IRenderPass* RenderPassFromCI(const PSOCreateInfoType& CreateInfo)
{
    return nullptr;
}

template <>
IRenderPass* RenderPassFromCI(const GraphicsPipelineStateCreateInfo& CreateInfo)
{
    return CreateInfo.GraphicsPipeline.pRenderPass;
}

#if METAL_SUPPORTED
std::string GetPSODumpFolder(const std::string& Root, const PipelineStateDesc& PSODesc, ARCHIVE_DEVICE_DATA_FLAGS DeviceFlag)
{
    std::string DumpDir{Root};
    if (DumpDir.empty())
        return DumpDir;

    if (DumpDir.back() != FileSystem::SlashSymbol)
        DumpDir += FileSystem::SlashSymbol;

    // Note: the same directory structure is used by the render state packager
    DumpDir += GetArchiveDeviceDataFlagString(DeviceFlag);
    DumpDir += FileSystem::SlashSymbol;
    DumpDir += GetPipelineTypeString(PSODesc.PipelineType);
    DumpDir += FileSystem::SlashSymbol;
    DumpDir += PSODesc.Name;

    return DumpDir;
}
#endif

} // namespace

template <typename PSOCreateInfoType>
SerializedPipelineStateImpl::SerializedPipelineStateImpl(IReferenceCounters*             pRefCounters,
                                                         SerializationDeviceImpl*        pDevice,
                                                         const PSOCreateInfoType&        CreateInfo,
                                                         const PipelineStateArchiveInfo& ArchiveInfo) :
    TBase{pRefCounters},
    m_pSerializationDevice{pDevice},
    m_Name{CreateInfo.PSODesc.Name != nullptr ? CreateInfo.PSODesc.Name : ""},
    m_Desc //
    {
        [this](PipelineStateDesc Desc) //
        {
            Desc.Name = m_Name.c_str();
            // We don't need resource layout and we don't copy variables and immutable samplers
            Desc.ResourceLayout = {};
            return Desc;
        }(CreateInfo.PSODesc) //
    },
    m_pRenderPass{RenderPassFromCI(CreateInfo)}
{
    if (CreateInfo.PSODesc.Name == nullptr || CreateInfo.PSODesc.Name[0] == '\0')
        LOG_ERROR_AND_THROW("Serialized pipeline state name can't be null or empty");

    ValidatePipelineStateArchiveInfo(CreateInfo, ArchiveInfo, pDevice->GetSupportedDeviceFlags());
    ValidatePSOCreateInfo(pDevice, CreateInfo);

    auto DeviceBits = ArchiveInfo.DeviceFlags;
    if ((DeviceBits & ARCHIVE_DEVICE_DATA_FLAG_GL) != 0 && (DeviceBits & ARCHIVE_DEVICE_DATA_FLAG_GLES) != 0)
    {
        // OpenGL and GLES use the same device data. Clear one flag to avoid shader duplication.
        DeviceBits &= ~ARCHIVE_DEVICE_DATA_FLAG_GLES;
    }

    m_Data.Aux.NoShaderReflection = (ArchiveInfo.PSOFlags & PSO_ARCHIVE_FLAG_STRIP_REFLECTION) != 0;
    while (DeviceBits != 0)
    {
        const auto Flag = ExtractLSB(DeviceBits);

        static_assert(ARCHIVE_DEVICE_DATA_FLAG_LAST == ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS, "Please update the switch below to handle the new data type");
        switch (Flag)
        {
#if D3D11_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_D3D11:
                PatchShadersD3D11(CreateInfo);
                break;
#endif
#if D3D12_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_D3D12:
                PatchShadersD3D12(CreateInfo);
                break;
#endif
#if GL_SUPPORTED || GLES_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_GL:
            case ARCHIVE_DEVICE_DATA_FLAG_GLES:
                PatchShadersGL(CreateInfo);
                break;
#endif
#if VULKAN_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_VULKAN:
                PatchShadersVk(CreateInfo);
                break;
#endif
#if METAL_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS:
            case ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS:
                PatchShadersMtl(CreateInfo, ArchiveDeviceDataFlagToArchiveDeviceType(Flag),
                                GetPSODumpFolder(pDevice->GetMtlProperties().DumpFolder, GetDesc(), Flag));
                break;
#endif
            case ARCHIVE_DEVICE_DATA_FLAG_NONE:
                UNEXPECTED("ARCHIVE_DEVICE_DATA_FLAG_NONE (0) should never occur");
                break;

            default:
                LOG_ERROR_MESSAGE("Unexpected render device type");
                break;
        }
    }

    if (!m_Data.Common)
    {
        if (CreateInfo.ResourceSignaturesCount == 0)
        {
#if GL_SUPPORTED || GLES_SUPPORTED
            if (ArchiveInfo.DeviceFlags & (ARCHIVE_DEVICE_DATA_FLAG_GL | ARCHIVE_DEVICE_DATA_FLAG_GLES))
            {
                // We must add empty device signature for OpenGL after all other devices are processed,
                // otherwise this empty description will be used as common signature description.
                PrepareDefaultSignatureGL(CreateInfo);
            }
#endif
        }
        else
        {
            m_Data.DoNotPackSignatures = (ArchiveInfo.PSOFlags & PSO_ARCHIVE_FLAG_DO_NOT_PACK_SIGNATURES) != 0;
        }

        auto   SignaturesCount = CreateInfo.ResourceSignaturesCount;
        auto** ppSignatures    = CreateInfo.ppResourceSignatures;

        IPipelineResourceSignature* DefaultSignatures[1] = {};
        if (m_pDefaultSignature)
        {
            DefaultSignatures[0] = m_pDefaultSignature;
            ppSignatures         = DefaultSignatures;
            SignaturesCount      = 1;
        }

        TPRSNames PRSNames = {};
        m_Signatures.resize(SignaturesCount);
        for (Uint32 i = 0; i < SignaturesCount; ++i)
        {
            auto* pSignature = ppSignatures[i];
            VERIFY(pSignature != nullptr, "This error should've been caught by ValidatePipelineResourceSignatures");
            m_Signatures[i] = pSignature;
            PRSNames[i]     = pSignature->GetDesc().Name;
        }

        auto SerializePsoCI = [&](auto& Ser) //
        {
            SerializePSOCreateInfo(Ser, CreateInfo, PRSNames);
            constexpr auto SerMode = std::remove_reference<decltype(Ser)>::type::GetMode();
            PSOSerializer<SerMode>::SerializeAuxData(Ser, m_Data.Aux, nullptr);
        };

        {
            Serializer<SerializerMode::Measure> Ser;
            SerializePsoCI(Ser);
            m_Data.Common = Ser.AllocateData(GetRawAllocator());
        }

        {
            Serializer<SerializerMode::Write> Ser{m_Data.Common};
            SerializePsoCI(Ser);
            VERIFY_EXPR(Ser.IsEnded());
        }
    }
}

INSTANTIATE_SERIALIZED_PSO_CTOR(GraphicsPipelineStateCreateInfo);
INSTANTIATE_SERIALIZED_PSO_CTOR(ComputePipelineStateCreateInfo);
INSTANTIATE_SERIALIZED_PSO_CTOR(TilePipelineStateCreateInfo);
INSTANTIATE_SERIALIZED_PSO_CTOR(RayTracingPipelineStateCreateInfo);

SerializedPipelineStateImpl::~SerializedPipelineStateImpl()
{}

void SerializedPipelineStateImpl::SerializeShaderCreateInfo(DeviceType              Type,
                                                            const ShaderCreateInfo& CI)
{
    Data::ShaderInfo ShaderData;
    ShaderData.Data  = SerializedShaderImpl::SerializeCreateInfo(CI);
    ShaderData.Stage = CI.Desc.ShaderType;
    ShaderData.Hash  = ShaderData.Data.GetHash();
#ifdef DILIGENT_DEBUG
    for (const auto& Data : m_Data.Shaders[static_cast<size_t>(Type)])
        VERIFY(Data.Hash != ShaderData.Hash, "Shader with the same hash is already in the list.");
#endif
    m_Data.Shaders[static_cast<size_t>(Type)].emplace_back(std::move(ShaderData));
}

Uint32 DILIGENT_CALL_TYPE SerializedPipelineStateImpl::GetPatchedShaderCount(ARCHIVE_DEVICE_DATA_FLAGS DeviceType) const
{
    DEV_CHECK_ERR(IsPowerOfTwo(DeviceType), "Only single device data flag is expected");
    const auto  Type    = ArchiveDeviceDataFlagToArchiveDeviceType(DeviceType);
    const auto& Shaders = m_Data.Shaders[static_cast<size_t>(Type)];
    return StaticCast<Uint32>(Shaders.size());
}

ShaderCreateInfo DILIGENT_CALL_TYPE SerializedPipelineStateImpl::GetPatchedShaderCreateInfo(
    ARCHIVE_DEVICE_DATA_FLAGS DeviceType,
    Uint32                    ShaderIndex) const
{
    DEV_CHECK_ERR(IsPowerOfTwo(DeviceType), "Only single device data flag is expected");

    ShaderCreateInfo ShaderCI;

    const auto  Type    = ArchiveDeviceDataFlagToArchiveDeviceType(DeviceType);
    const auto& Shaders = m_Data.Shaders[static_cast<size_t>(Type)];
    if (ShaderIndex < Shaders.size())
    {
        {
            Serializer<SerializerMode::Read> Ser{Shaders[ShaderIndex].Data};
            ShaderSerializer<SerializerMode::Read>::SerializeCI(Ser, ShaderCI);
        }
        if (Type == DeviceType::Metal_MacOS || Type == DeviceType::Metal_iOS)
        {
            // See DeviceObjectArchiveMtlImpl::UnpackShader and ShaderMtlSerializer<>::SerializeSource
            Serializer<SerializerMode::Read> Ser{SerializedData{const_cast<void*>(ShaderCI.ByteCode), ShaderCI.ByteCodeSize}};
            Ser.SerializeBytes(ShaderCI.ByteCode, ShaderCI.ByteCodeSize);
        }
    }
    else
    {
        DEV_ERROR("Shader index (", ShaderIndex, ") is out of range. Call GetPatchedShaderCount() to get the shader count.");
    }

    return ShaderCI;
}

} // namespace Diligent
