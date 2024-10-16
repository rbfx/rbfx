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

#include "RenderStateCache.h"
#include "RenderStateCache.hpp"
#include "RenderStateCacheImpl.hpp"

#include "ReloadableShader.hpp"
#include "ReloadablePipelineState.hpp"
#include "AsyncPipelineState.hpp"

#include <array>
#include <mutex>
#include <vector>

#include "Archiver.h"
#include "Dearchiver.h"
#include "ArchiverFactory.h"
#include "ArchiverFactoryLoader.h"

#include "PipelineStateBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "SerializationDevice.h"
#include "SerializedShader.h"
#include "CallbackWrapper.hpp"
#include "GraphicsAccessories.hpp"
#include "GraphicsUtilities.h"
#include "ShaderSourceFactoryUtils.hpp"

namespace Diligent
{

Bool RenderStateCacheImpl::WriteToBlob(Uint32 ContentVersion, IDataBlob** ppBlob)
{
    if (ContentVersion == ~0u)
    {
        ContentVersion = GetContentVersion();
        if (ContentVersion == ~0u)
            ContentVersion = 0;
    }

    // Load new render states from archiver to dearchiver

    RefCntAutoPtr<IDataBlob> pNewData;
    m_pArchiver->SerializeToBlob(ContentVersion, &pNewData);
    if (!pNewData)
    {
        LOG_ERROR_MESSAGE("Failed to serialize render state data");
        return false;
    }

    if (!m_pDearchiver->LoadArchive(pNewData, ContentVersion))
    {
        LOG_ERROR_MESSAGE("Failed to add new render state data to existing archive");
        return false;
    }

    m_pArchiver->Reset();

    return m_pDearchiver->Store(ppBlob);
}

Bool RenderStateCacheImpl::WriteToStream(Uint32 ContentVersion, IFileStream* pStream)
{
    DEV_CHECK_ERR(pStream != nullptr, "pStream must not be null");
    if (pStream == nullptr)
        return false;

    RefCntAutoPtr<IDataBlob> pDataBlob;
    if (!WriteToBlob(ContentVersion, &pDataBlob))
        return false;

    return pStream->Write(pDataBlob->GetConstDataPtr(), pDataBlob->GetSize());
}

void RenderStateCacheImpl::Reset()
{
    m_pDearchiver->Reset();
    m_pArchiver->Reset();
    m_Shaders.clear();
    m_ReloadableShaders.clear();
    m_Pipelines.clear();
    m_ReloadablePipelines.clear();
}

RefCntAutoPtr<IShader> RenderStateCacheImpl::FindReloadableShader(IShader* pShader)
{
    std::lock_guard<std::mutex> Guard{m_ReloadableShadersMtx};

    auto it = m_ReloadableShaders.find(pShader->GetUniqueID());
    if (it == m_ReloadableShaders.end())
        return {};

    auto pReloadableShader = it->second.Lock();
    if (!pReloadableShader)
        m_ReloadableShaders.erase(it);

    return pReloadableShader;
}

std::string RenderStateCacheImpl::HashToStr(Uint64 Low, Uint64 High)
{
    static constexpr std::array<char, 16> Symbols = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    std::string Str;
    for (auto Part : {High, Low})
    {
        for (Uint64 i = 0; i < 16; ++i)
            Str += Symbols[(Part >> (Uint64{60} - i * 4)) & 0xFu];
    }

    return Str;
}

std::string RenderStateCacheImpl::MakeHashStr(const char* Name, const XXH128Hash& Hash)
{
    std::string HashStr = HashToStr(Hash.LowPart, Hash.HighPart);
    if (Name != nullptr)
        HashStr = std::string{Name} + " [" + HashStr + ']';
    return HashStr;
}


static size_t ComputeDeviceAttribsHash(IRenderDevice* pDevice)
{
    if (pDevice == nullptr)
        return 0;

    const RenderDeviceInfo& DeviceInfo = pDevice->GetDeviceInfo();
    return ComputeHash(DeviceInfo.Type, DeviceInfo.NDC.MinZ, DeviceInfo.Features.SeparablePrograms);
}

RenderStateCacheImpl::RenderStateCacheImpl(IReferenceCounters*               pRefCounters,
                                           const RenderStateCacheCreateInfo& CreateInfo) :
    TBase{pRefCounters},
    // clang-format off
    m_pDevice      {CreateInfo.pDevice},
    m_DeviceType   {CreateInfo.pDevice != nullptr ? CreateInfo.pDevice->GetDeviceInfo().Type : RENDER_DEVICE_TYPE_UNDEFINED},
    m_DeviceHash   {ComputeDeviceAttribsHash(CreateInfo.pDevice)},
    m_CI           {CreateInfo},
    m_pReloadSource{CreateInfo.pReloadSource}
// clang-format on
{
    if (CreateInfo.pDevice == nullptr)
        LOG_ERROR_AND_THROW("CreateInfo.pDevice must not be null");

    IArchiverFactory* pArchiverFactory = nullptr;
#if EXPLICITLY_LOAD_ARCHIVER_FACTORY_DLL
    auto GetArchiverFactory = LoadArchiverFactory();
    if (GetArchiverFactory != nullptr)
    {
        pArchiverFactory = GetArchiverFactory();
    }
#else
    pArchiverFactory       = GetArchiverFactory();
#endif
    VERIFY_EXPR(pArchiverFactory != nullptr);

    SerializationDeviceCreateInfo SerializationDeviceCI;
    SerializationDeviceCI.DeviceInfo                        = m_pDevice->GetDeviceInfo();
    SerializationDeviceCI.AdapterInfo                       = m_pDevice->GetAdapterInfo();
    SerializationDeviceCI.pAsyncShaderCompilationThreadPool = m_pDevice->GetShaderCompilationThreadPool();

    switch (m_DeviceType)
    {
        case RENDER_DEVICE_TYPE_D3D11:
            SerializationDeviceCI.D3D11.FeatureLevel = SerializationDeviceCI.DeviceInfo.APIVersion;
            break;

        case RENDER_DEVICE_TYPE_D3D12:
            SerializationDeviceCI.D3D12.ShaderVersion = SerializationDeviceCI.DeviceInfo.MaxShaderVersion.HLSL;
            break;

        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
            SerializationDeviceCI.GL.ZeroToOneClipZ  = SerializationDeviceCI.DeviceInfo.NDC.MinZ == 0;
            SerializationDeviceCI.GL.OptimizeShaders = m_CI.OptimizeGLShaders;
            break;

        case RENDER_DEVICE_TYPE_VULKAN:
            SerializationDeviceCI.Vulkan.ApiVersion = SerializationDeviceCI.DeviceInfo.APIVersion;
            break;

        case RENDER_DEVICE_TYPE_METAL:
            break;

        case RENDER_DEVICE_TYPE_WEBGPU:
            break;

        default:
            UNEXPECTED("Unknown device type");
    }

    pArchiverFactory->CreateSerializationDevice(SerializationDeviceCI, &m_pSerializationDevice);
    if (!m_pSerializationDevice)
        LOG_ERROR_AND_THROW("Failed to create serialization device");

    m_pSerializationDevice->AddRenderDevice(m_pDevice);

    pArchiverFactory->CreateArchiver(m_pSerializationDevice, &m_pArchiver);
    if (!m_pArchiver)
        LOG_ERROR_AND_THROW("Failed to create archiver");

    DearchiverCreateInfo DearchiverCI;
    m_pDevice->GetEngineFactory()->CreateDearchiver(DearchiverCI, &m_pDearchiver);
    if (!m_pDearchiver)
        LOG_ERROR_AND_THROW("Failed to create dearchiver");
}

#define RENDER_STATE_CACHE_LOG(Level, ...)                         \
    do                                                             \
    {                                                              \
        if (m_CI.LogLevel >= Level)                                \
        {                                                          \
            LOG_INFO_MESSAGE("Render state cache: ", __VA_ARGS__); \
        }                                                          \
    } while (false)

bool RenderStateCacheImpl::CreateShader(const ShaderCreateInfo& ShaderCI,
                                        IShader**               ppShader)
{
    if (ppShader == nullptr)
    {
        DEV_ERROR("ppShader must not be null");
        return false;
    }
    DEV_CHECK_ERR(*ppShader == nullptr, "Overwriting reference to existing shader may cause memory leaks");

    *ppShader = nullptr;

    RefCntAutoPtr<IShader> pShader;

    const auto FoundInCache = CreateShaderInternal(ShaderCI, &pShader);
    if (!pShader)
        return false;

    if (m_CI.EnableHotReload)
    {
        // Wrap shader in a reloadable shader object
        {
            std::lock_guard<std::mutex> Guard{m_ReloadableShadersMtx};

            auto it = m_ReloadableShaders.find(pShader->GetUniqueID());
            if (it != m_ReloadableShaders.end())
            {
                if (auto pReloadableShader = it->second.Lock())
                    *ppShader = pReloadableShader.Detach();
                else
                    m_ReloadableShaders.erase(it);
            }
        }

        if (*ppShader == nullptr)
        {
            auto _ShaderCI = ShaderCI;

            RefCntAutoPtr<IShaderSourceInputStreamFactory> pCompoundReloadSource;
            if (m_pReloadSource)
            {
                if (ShaderCI.pShaderSourceStreamFactory)
                {
                    // Create compound shader source factory that will first try to load shader from the reload source
                    // and if it fails, will fall back to the original source factory.
                    pCompoundReloadSource =
                        CreateCompoundShaderSourceFactory({m_pReloadSource, ShaderCI.pShaderSourceStreamFactory});
                    _ShaderCI.pShaderSourceStreamFactory = pCompoundReloadSource;
                }
                else
                {
                    _ShaderCI.pShaderSourceStreamFactory = m_pReloadSource;
                }
            }
            ReloadableShader::Create(this, pShader, _ShaderCI, ppShader);

            std::lock_guard<std::mutex> Guard{m_ReloadableShadersMtx};
            m_ReloadableShaders.emplace(pShader->GetUniqueID(), RefCntWeakPtr<IShader>{*ppShader});
        }
    }
    else
    {
        *ppShader = pShader.Detach();
    }

    return FoundInCache;
}

bool RenderStateCacheImpl::CreateShaderInternal(const ShaderCreateInfo& ShaderCI,
                                                IShader**               ppShader)
{
    VERIFY_EXPR(ppShader != nullptr && *ppShader == nullptr);

    XXH128State Hasher;
#ifdef DILIGENT_DEBUG
    constexpr bool IsDebug = true;
#else
    constexpr bool IsDebug = false;
#endif
    Hasher.Update(ShaderCI, m_DeviceHash, IsDebug);
    const auto Hash = Hasher.Digest();

    // First, try to check if the shader has already been requested
    {
        std::lock_guard<std::mutex> Guard{m_ShadersMtx};

        auto it = m_Shaders.find(Hash);
        if (it != m_Shaders.end())
        {
            if (auto pShader = it->second.Lock())
            {
                *ppShader = pShader.Detach();
                RENDER_STATE_CACHE_LOG(RENDER_STATE_CACHE_LOG_LEVEL_VERBOSE, "Reusing existing shader '", (ShaderCI.Desc.Name ? ShaderCI.Desc.Name : ""), "'.");
                return true;
            }
            else
            {
                m_Shaders.erase(it);
            }
        }
    }

    class AddShaderHelper
    {
    public:
        AddShaderHelper(RenderStateCacheImpl& Cache, const XXH128Hash& Hash, IShader** ppShader) :
            m_Cache{Cache},
            m_Hash{Hash},
            m_ppShader{ppShader}
        {
        }

        ~AddShaderHelper()
        {
            if (*m_ppShader != nullptr)
            {
                std::lock_guard<std::mutex> Guard{m_Cache.m_ShadersMtx};
                m_Cache.m_Shaders.emplace(m_Hash, *m_ppShader);
            }
        }

    private:
        RenderStateCacheImpl& m_Cache;
        const XXH128Hash&     m_Hash;
        IShader** const       m_ppShader;
    };
    AddShaderHelper AutoAddShader{*this, Hash, ppShader};

    const auto HashStr = MakeHashStr(ShaderCI.Desc.Name, Hash);

    // Try to find the shader in the loaded archive
    {
        auto Callback = MakeCallback(
            [&ShaderCI](ShaderDesc& Desc) {
                Desc.Name = ShaderCI.Desc.Name;
            });

        ShaderUnpackInfo UnpackInfo;
        UnpackInfo.Name             = HashStr.c_str();
        UnpackInfo.pDevice          = m_pDevice;
        UnpackInfo.ModifyShaderDesc = Callback;
        UnpackInfo.pUserData        = Callback;
        RefCntAutoPtr<IShader> pShader;
        m_pDearchiver->UnpackShader(UnpackInfo, &pShader);
        if (pShader)
        {
            if (pShader->GetDesc() == ShaderCI.Desc)
            {
                RENDER_STATE_CACHE_LOG(RENDER_STATE_CACHE_LOG_LEVEL_VERBOSE, "Found shader '", HashStr, "' in the archive.");
                *ppShader = pShader.Detach();
                return true;
            }
            else
            {
                LOG_ERROR_MESSAGE("Description of shader '", (ShaderCI.Desc.Name != nullptr ? ShaderCI.Desc.Name : "<unnamed>"),
                                  "' does not match the description of the shader unpacked from the cache. This may be the result of a "
                                  "hash conflict, though the probability of this should be virtually zero.");
            }
        }
    }

    // Next, try to find the shader in the archiver
    RefCntAutoPtr<IShader> pArchivedShader{m_pArchiver->GetShader(HashStr.c_str())};
    const auto             FoundInArchive = (pArchivedShader != nullptr);
    if (!pArchivedShader)
    {
        auto ArchiveShaderCI      = ShaderCI;
        ArchiveShaderCI.Desc.Name = HashStr.c_str();
        ShaderArchiveInfo ArchiveInfo;
        ArchiveInfo.DeviceFlags = RenderDeviceTypeToArchiveDataFlag(m_DeviceType);
        m_pSerializationDevice->CreateShader(ArchiveShaderCI, ArchiveInfo, &pArchivedShader);
        if (pArchivedShader)
        {
            if (m_pArchiver->AddShader(pArchivedShader))
                RENDER_STATE_CACHE_LOG(RENDER_STATE_CACHE_LOG_LEVEL_NORMAL, "Added shader '", HashStr, "'.");
            else
                LOG_ERROR_MESSAGE("Failed to archive shader '", HashStr, "'.");
        }
    }

    if (pArchivedShader)
    {
        RefCntAutoPtr<ISerializedShader> pSerializedShader{pArchivedShader, IID_SerializedShader};
        VERIFY(pSerializedShader, "Shader object is not a serialized shader");
        if (pSerializedShader)
        {
            if (RefCntAutoPtr<IShader> pShader{pSerializedShader->GetDeviceShader(m_DeviceType)})
            {
                if (pShader->GetDesc() == ShaderCI.Desc)
                {
                    *ppShader = pShader.Detach();
                    return FoundInArchive;
                }
                else
                {
                    LOG_ERROR_MESSAGE("Description of shader '", (ShaderCI.Desc.Name != nullptr ? ShaderCI.Desc.Name : "<unnamed>"),
                                      "' does not match the description of the shader recently added to the cache. This may be the result of a "
                                      "hash conflict, though the probability of this should be virtually zero.");
                }
            }
            else
            {
                UNEXPECTED("Device shader must not be null");
            }
        }
    }

    if (*ppShader == nullptr)
    {
        m_pDevice->CreateShader(ShaderCI, ppShader);
    }

    return false;
}

template <typename CreateInfoType>
struct RenderStateCacheImpl::SerializedPsoCIWrapperBase
{
    SerializedPsoCIWrapperBase(ISerializationDevice* pSerializationDevice, RENDER_DEVICE_TYPE DeviceType, const CreateInfoType& _CI) :
        CI{_CI},
        ppSignatures{_CI.ppResourceSignatures, _CI.ppResourceSignatures + _CI.ResourceSignaturesCount}
    {
        CI.ppResourceSignatures = !ppSignatures.empty() ? ppSignatures.data() : nullptr;

        // Replace signatures with serialized signatures
        for (size_t i = 0; i < ppSignatures.size(); ++i)
        {
            auto& pSign = ppSignatures[i];
            if (pSign == nullptr)
                continue;

            auto SignDesc = pSign->GetDesc();
            // Add hash to the signature name
            XXH128State Hasher;
            Hasher.Update(SignDesc, DeviceType);
            const auto Hash    = Hasher.Digest();
            const auto HashStr = MakeHashStr(SignDesc.Name, Hash);
            SignDesc.Name      = HashStr.c_str();

            ResourceSignatureArchiveInfo ArchiveInfo;
            ArchiveInfo.DeviceFlags = RenderDeviceTypeToArchiveDataFlag(DeviceType);
            RefCntAutoPtr<IPipelineResourceSignature> pSerializedSign;
            pSerializationDevice->CreatePipelineResourceSignature(SignDesc, ArchiveInfo, &pSerializedSign);
            if (!pSerializedSign)
            {
                LOG_ERROR_AND_THROW("Failed to serialize pipeline resource signature '", SignDesc.Name, "'.");
            }
            pSign = pSerializedSign;
            SerializedObjects.emplace_back(std::move(pSerializedSign));
        }

        ConvertShadersToSerialized<>(pSerializationDevice, DeviceType);
    }

    SerializedPsoCIWrapperBase(const SerializedPsoCIWrapperBase&) = delete;
    SerializedPsoCIWrapperBase(SerializedPsoCIWrapperBase&&)      = delete;
    SerializedPsoCIWrapperBase& operator=(const SerializedPsoCIWrapperBase&) = delete;
    SerializedPsoCIWrapperBase& operator=(SerializedPsoCIWrapperBase&&) = delete;

    void SetName(const char* Name)
    {
        VERIFY_EXPR(Name != nullptr);
        CI.PSODesc.Name = Name;
    }

    operator const CreateInfoType&()
    {
        return CI;
    }


private:
    template <typename CIType = CreateInfoType>
    typename std::enable_if<
        std::is_same<CIType, GraphicsPipelineStateCreateInfo>::value ||
        std::is_same<CIType, ComputePipelineStateCreateInfo>::value ||
        std::is_same<CIType, TilePipelineStateCreateInfo>::value>::type
    ConvertShadersToSerialized(ISerializationDevice* pSerializationDevice, RENDER_DEVICE_TYPE DeviceType)
    {
        ProcessPipelineStateCreateInfoShaders(CI,
                                              [&](IShader*& pShader) {
                                                  SerializeShader(pSerializationDevice, DeviceType, pShader);
                                              });
    }

    template <typename CIType = CreateInfoType>
    typename std::enable_if<std::is_same<CIType, RayTracingPipelineStateCreateInfo>::value>::type
    ConvertShadersToSerialized(ISerializationDevice* pSerializationDevice, RENDER_DEVICE_TYPE DeviceType)
    {
        // Handled in SerializedPsoCIWrapper<RayTracingPipelineStateCreateInfo>
    }

protected:
    // Replaces shader with a serialized shader
    void SerializeShader(ISerializationDevice* pSerializationDevice, RENDER_DEVICE_TYPE DeviceType, IShader*& pShader)
    {
        if (pShader == nullptr)
            return;

        RefCntAutoPtr<IObject> pObject;
        pShader->GetReferenceCounters()->QueryObject(&pObject);
        RefCntAutoPtr<IShader> pSerializedShader{pObject, IID_SerializedShader};
        if (!pSerializedShader)
        {
            ShaderCreateInfo ShaderCI;
            ShaderCI.Desc = pShader->GetDesc();

            Uint64 Size = 0;
            pShader->GetBytecode(&ShaderCI.ByteCode, Size);
            ShaderCI.ByteCodeSize = static_cast<size_t>(Size);
            if (DeviceType == RENDER_DEVICE_TYPE_GL || DeviceType == RENDER_DEVICE_TYPE_GLES)
            {
                ShaderCI.Source         = static_cast<const char*>(ShaderCI.ByteCode);
                ShaderCI.ByteCode       = nullptr;
                ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM;
            }
            else if (DeviceType == RENDER_DEVICE_TYPE_METAL)
            {
                ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_MSL_VERBATIM;
            }
            else if (DeviceType == RENDER_DEVICE_TYPE_WEBGPU)
            {
                ShaderCI.Source                         = static_cast<const char*>(ShaderCI.ByteCode);
                ShaderCI.ByteCode                       = nullptr;
                ShaderCI.SourceLanguage                 = SHADER_SOURCE_LANGUAGE_WGSL;
                ShaderCI.WebGPUEmulatedArrayIndexSuffix = GetWebGPUEmulatedArrayIndexSuffix(pShader);
            }
            ShaderArchiveInfo ArchiveInfo;
            ArchiveInfo.DeviceFlags = RenderDeviceTypeToArchiveDataFlag(DeviceType);
            pSerializationDevice->CreateShader(ShaderCI, ArchiveInfo, &pSerializedShader);
            if (!pSerializedShader)
            {
                LOG_ERROR_AND_THROW("Failed to serialize shader '", ShaderCI.Desc.Name, "'.");
            }
        }

        pShader = pSerializedShader;
        SerializedObjects.emplace_back(std::move(pSerializedShader));
    }

protected:
    CreateInfoType CI;

    std::vector<IPipelineResourceSignature*> ppSignatures;
    std::vector<RefCntAutoPtr<IObject>>      SerializedObjects;
};

template <>
struct RenderStateCacheImpl::SerializedPsoCIWrapper<GraphicsPipelineStateCreateInfo> : SerializedPsoCIWrapperBase<GraphicsPipelineStateCreateInfo>
{
    SerializedPsoCIWrapper(ISerializationDevice* pSerializationDevice, RENDER_DEVICE_TYPE DeviceType, const GraphicsPipelineStateCreateInfo& _CI) :
        SerializedPsoCIWrapperBase<GraphicsPipelineStateCreateInfo>{pSerializationDevice, DeviceType, _CI}
    {
        CorrectGraphicsPipelineDesc(CI.GraphicsPipeline, pSerializationDevice->GetDeviceInfo().Features);

        // Replace render pass with serialized render pass
        if (CI.GraphicsPipeline.pRenderPass != nullptr)
        {
            auto RPDesc = CI.GraphicsPipeline.pRenderPass->GetDesc();
            // Add hash to the render pass name
            XXH128State Hasher;
            Hasher.Update(RPDesc, DeviceType);
            const auto Hash    = Hasher.Digest();
            const auto HashStr = MakeHashStr(RPDesc.Name, Hash);
            RPDesc.Name        = HashStr.c_str();

            RefCntAutoPtr<IRenderPass> pSerializedRP;
            pSerializationDevice->CreateRenderPass(RPDesc, &pSerializedRP);
            if (!pSerializedRP)
            {
                LOG_ERROR_AND_THROW("Failed to serialize render pass '", RPDesc.Name, "'.");
            }
            CI.GraphicsPipeline.pRenderPass = pSerializedRP;
            SerializedObjects.emplace_back(std::move(pSerializedRP));
        }
    }
};

template <>
struct RenderStateCacheImpl::SerializedPsoCIWrapper<ComputePipelineStateCreateInfo> : SerializedPsoCIWrapperBase<ComputePipelineStateCreateInfo>
{
    SerializedPsoCIWrapper(ISerializationDevice* pSerializationDevice, RENDER_DEVICE_TYPE DeviceType, const ComputePipelineStateCreateInfo& _CI) :
        SerializedPsoCIWrapperBase<ComputePipelineStateCreateInfo>{pSerializationDevice, DeviceType, _CI}
    {
    }
};

template <>
struct RenderStateCacheImpl::SerializedPsoCIWrapper<TilePipelineStateCreateInfo> : SerializedPsoCIWrapperBase<TilePipelineStateCreateInfo>
{
    SerializedPsoCIWrapper(ISerializationDevice* pSerializationDevice, RENDER_DEVICE_TYPE DeviceType, const TilePipelineStateCreateInfo& _CI) :
        SerializedPsoCIWrapperBase<TilePipelineStateCreateInfo>{pSerializationDevice, DeviceType, _CI}
    {
    }
};

template <>
struct RenderStateCacheImpl::SerializedPsoCIWrapper<RayTracingPipelineStateCreateInfo> : SerializedPsoCIWrapperBase<RayTracingPipelineStateCreateInfo>
{
    SerializedPsoCIWrapper(ISerializationDevice* pSerializationDevice, RENDER_DEVICE_TYPE DeviceType, const RayTracingPipelineStateCreateInfo& _CI) :
        SerializedPsoCIWrapperBase<RayTracingPipelineStateCreateInfo>{pSerializationDevice, DeviceType, _CI},
        // clang-format off
        pGeneralShaders      {_CI.pGeneralShaders,       _CI.pGeneralShaders       + _CI.GeneralShaderCount},
        pTriangleHitShaders  {_CI.pTriangleHitShaders,   _CI.pTriangleHitShaders   + _CI.TriangleHitShaderCount},
        pProceduralHitShaders{_CI.pProceduralHitShaders, _CI.pProceduralHitShaders + _CI.ProceduralHitShaderCount}
    // clang-format on
    {
        CI.pGeneralShaders       = pGeneralShaders.data();
        CI.pTriangleHitShaders   = pTriangleHitShaders.data();
        CI.pProceduralHitShaders = pProceduralHitShaders.data();

        for (RayTracingGeneralShaderGroup& GeneralShader : pGeneralShaders)
        {
            SerializeShader(pSerializationDevice, DeviceType, GeneralShader.pShader);
        }

        for (RayTracingTriangleHitShaderGroup& TriHitShader : pTriangleHitShaders)
        {
            SerializeShader(pSerializationDevice, DeviceType, TriHitShader.pAnyHitShader);
            SerializeShader(pSerializationDevice, DeviceType, TriHitShader.pClosestHitShader);
        }

        for (RayTracingProceduralHitShaderGroup& ProcHitShader : pProceduralHitShaders)
        {
            SerializeShader(pSerializationDevice, DeviceType, ProcHitShader.pAnyHitShader);
            SerializeShader(pSerializationDevice, DeviceType, ProcHitShader.pClosestHitShader);
            SerializeShader(pSerializationDevice, DeviceType, ProcHitShader.pIntersectionShader);
        }
    }

private:
    std::vector<RayTracingGeneralShaderGroup>       pGeneralShaders;
    std::vector<RayTracingTriangleHitShaderGroup>   pTriangleHitShaders;
    std::vector<RayTracingProceduralHitShaderGroup> pProceduralHitShaders;
};


void CreateRenderStateCache(const RenderStateCacheCreateInfo& CreateInfo,
                            IRenderStateCache**               ppCache)
{
    try
    {
        RefCntAutoPtr<IRenderStateCache> pCache{MakeNewRCObj<RenderStateCacheImpl>()(CreateInfo)};
        if (pCache)
            pCache->QueryInterface(IID_RenderStateCache, reinterpret_cast<IObject**>(ppCache));
    }
    catch (...)
    {
        LOG_ERROR("Failed to create the render state cache");
    }
}

template <typename CreateInfoType>
bool RenderStateCacheImpl::CreatePipelineState(const CreateInfoType& PSOCreateInfo,
                                               IPipelineState**      ppPipelineState)
{
    if (ppPipelineState == nullptr)
    {
        DEV_ERROR("ppPipelineState must not be null");
        return false;
    }
    DEV_CHECK_ERR(*ppPipelineState == nullptr, "Overwriting reference to existing pipeline state may cause memory leaks");

    *ppPipelineState = nullptr;

    RefCntAutoPtr<IPipelineState> pPSO;

    const auto FoundInCache = CreatePipelineStateInternal(PSOCreateInfo, &pPSO);
    if (!pPSO)
        return false;

    if (m_CI.EnableHotReload)
    {
        {
            std::lock_guard<std::mutex> Guard{m_ReloadablePipelinesMtx};

            auto it = m_ReloadablePipelines.find(pPSO->GetUniqueID());
            if (it != m_ReloadablePipelines.end())
            {
                if (auto pReloadablePSO = it->second.Lock())
                    *ppPipelineState = pReloadablePSO.Detach();
                else
                    m_ReloadablePipelines.erase(it);
            }
        }

        if (*ppPipelineState == nullptr)
        {
            ReloadablePipelineState::Create(this, pPSO, PSOCreateInfo, ppPipelineState);

            std::lock_guard<std::mutex> Guard{m_ReloadablePipelinesMtx};
            m_ReloadablePipelines.emplace(pPSO->GetUniqueID(), RefCntWeakPtr<IPipelineState>(*ppPipelineState));
        }
    }
    else
    {
        *ppPipelineState = pPSO.Detach();
    }

    return FoundInCache;
}

template <typename CreateInfoType>
bool RenderStateCacheImpl::CreatePipelineStateInternal(const CreateInfoType& PSOCreateInfo,
                                                       IPipelineState**      ppPipelineState)
{
    VERIFY_EXPR(ppPipelineState != nullptr && *ppPipelineState == nullptr);

    const SHADER_STATUS ShadersStatus = GetPipelineStateCreateInfoShadersStatus<CreateInfoType>(PSOCreateInfo);
    VERIFY(ShadersStatus != SHADER_STATUS_UNINITIALIZED, "Unexpected shader status");
    if (ShadersStatus == SHADER_STATUS_FAILED)
    {
        LOG_ERROR_MESSAGE("Failed to create pipeline state '", (PSOCreateInfo.PSODesc.Name ? PSOCreateInfo.PSODesc.Name : "<unnamed>"), "': one or more shaders failed to compile.");
        return false;
    }

    if (ShadersStatus == SHADER_STATUS_COMPILING)
    {
        // Note that async pipeline may be wrapped into ReloadablePipelineState.
        // This will work totally fine as reloadable pipeline will delegate all calls to the async pipeline
        // and may create another async pipeline.
        AsyncPipelineState::Create(this, PSOCreateInfo, ppPipelineState);
        return false;
    }

    XXH128State Hasher;
    Hasher.Update(PSOCreateInfo, m_DeviceHash);
    const auto Hash = Hasher.Digest();

    // First, try to check if the PSO has already been requested
    {
        std::lock_guard<std::mutex> Guard{m_PipelinesMtx};

        auto it = m_Pipelines.find(Hash);
        if (it != m_Pipelines.end())
        {
            if (auto pPSO = it->second.Lock())
            {
                *ppPipelineState = pPSO.Detach();
                RENDER_STATE_CACHE_LOG(RENDER_STATE_CACHE_LOG_LEVEL_VERBOSE, "Reusing existing pipeline '", (PSOCreateInfo.PSODesc.Name ? PSOCreateInfo.PSODesc.Name : ""), "'.");
                return true;
            }
            else
            {
                m_Pipelines.erase(it);
            }
        }
    }

    const auto HashStr = MakeHashStr(PSOCreateInfo.PSODesc.Name, Hash);

    bool FoundInCache = false;
    // Try to find PSO in the loaded archive
    {
        auto Callback = MakeCallback(
            [&PSOCreateInfo](PipelineStateCreateInfo& CI) {
                CI.PSODesc.Name = PSOCreateInfo.PSODesc.Name;
            });

        PipelineStateUnpackInfo UnpackInfo;
        UnpackInfo.PipelineType                  = PSOCreateInfo.PSODesc.PipelineType;
        UnpackInfo.Name                          = HashStr.c_str();
        UnpackInfo.pDevice                       = m_pDevice;
        UnpackInfo.ModifyPipelineStateCreateInfo = Callback;
        UnpackInfo.pUserData                     = Callback;
        RefCntAutoPtr<IPipelineState> pPSO;
        m_pDearchiver->UnpackPipelineState(UnpackInfo, &pPSO);
        if (pPSO)
        {
            const PIPELINE_STATE_STATUS Status = pPSO->GetStatus();
            if (Status == PIPELINE_STATE_STATUS_READY)
            {
                if (pPSO->GetDesc() == PSOCreateInfo.PSODesc)
                {
                    *ppPipelineState = pPSO.Detach();
                    FoundInCache     = true;
                }
                else
                {
                    LOG_ERROR_MESSAGE("Description of pipeline state '", (PSOCreateInfo.PSODesc.Name != nullptr ? PSOCreateInfo.PSODesc.Name : "<unnamed>"),
                                      "' does not match the description of the pipeline unpacked from the cache. This may be the result of a "
                                      "hash conflict, though the probability of this should be virtually zero.");
                }
            }
            else if (Status == PIPELINE_STATE_STATUS_COMPILING)
            {
                *ppPipelineState = pPSO.Detach();
                FoundInCache     = true;
            }
            else if (Status == PIPELINE_STATE_STATUS_FAILED)
            {
                LOG_ERROR_MESSAGE("Pipeline state '", (PSOCreateInfo.PSODesc.Name != nullptr ? PSOCreateInfo.PSODesc.Name : "<unnamed>"),
                                  "' is in failed state.");
            }
            else
            {
                UNEXPECTED("Unexpected pipeline state status ", GetPipelineStateStatusString(Status));
            }
        }
    }

    if (*ppPipelineState == nullptr)
    {
        m_pDevice->CreatePipelineState(PSOCreateInfo, ppPipelineState);
        if (*ppPipelineState == nullptr)
            return false;
    }

    {
        std::lock_guard<std::mutex> Guard{m_PipelinesMtx};
        m_Pipelines.emplace(Hash, *ppPipelineState);
    }

    if (FoundInCache)
    {
        RENDER_STATE_CACHE_LOG(RENDER_STATE_CACHE_LOG_LEVEL_VERBOSE, "Found pipeline '", HashStr, "' in the archive.");
        return true;
    }

    if (m_pArchiver->GetPipelineState(PSOCreateInfo.PSODesc.PipelineType, HashStr.c_str()) != nullptr)
        return true;

    try
    {
        // Make a copy of create info that contains serialized objects
        SerializedPsoCIWrapper<CreateInfoType> SerializedPsoCI{m_pSerializationDevice, m_DeviceType, PSOCreateInfo};
        SerializedPsoCI.SetName(HashStr.c_str());

        PipelineStateArchiveInfo ArchiveInfo;
        ArchiveInfo.DeviceFlags = RenderDeviceTypeToArchiveDataFlag(m_DeviceType);
        RefCntAutoPtr<IPipelineState> pSerializedPSO;
        m_pSerializationDevice->CreatePipelineState(SerializedPsoCI, ArchiveInfo, &pSerializedPSO);

        if (pSerializedPSO)
        {
            if (m_pArchiver->AddPipelineState(pSerializedPSO))
                RENDER_STATE_CACHE_LOG(RENDER_STATE_CACHE_LOG_LEVEL_NORMAL, "Added pipeline '", HashStr, "'.");
            else
                LOG_ERROR_MESSAGE("Failed to archive PSO '", HashStr, "'.");
        }
    }
    catch (...)
    {
    }

    return false;
}

Uint32 RenderStateCacheImpl::Reload(ReloadGraphicsPipelineCallbackType ReloadGraphicsPipeline, void* pUserData)
{
    if (!m_CI.EnableHotReload)
    {
        DEV_ERROR("This render state cache was not created with hot reload enabled. Set EnableHotReload to true.");
        return 0;
    }

    Uint32 NumStatesReloaded = 0;

    // Reload all shaders first
    {
        std::lock_guard<std::mutex> Guard{m_ReloadableShadersMtx};
        for (auto shader_it : m_ReloadableShaders)
        {
            if (auto pShader = shader_it.second.Lock())
            {
                RefCntAutoPtr<ReloadableShader> pReloadableShader{pShader, ReloadableShader::IID_InternalImpl};
                if (pReloadableShader)
                {
                    if (pReloadableShader->Reload())
                        ++NumStatesReloaded;
                }
                else
                {
                    UNEXPECTED("Shader object is not a ReloadableShader");
                }
            }
        }
    }

    // Reload pipelines.
    // Note that create info structs reference reloadable shaders, so that when pipelines
    // are re-created, they will automatically use reloaded shaders.
    {
        std::lock_guard<std::mutex> Guard{m_ReloadablePipelinesMtx};
        for (auto pso_it : m_ReloadablePipelines)
        {
            if (auto pPSO = pso_it.second.Lock())
            {
                RefCntAutoPtr<ReloadablePipelineState> pReloadablePSO{pPSO, ReloadablePipelineState::IID_InternalImpl};
                if (pPSO)
                {
                    if (pReloadablePSO->Reload(ReloadGraphicsPipeline, pUserData))
                        ++NumStatesReloaded;
                }
                else
                {
                    UNEXPECTED("Pipeline state object is not a ReloadablePipelineState");
                }
            }
        }
    }

    return NumStatesReloaded;
}

static constexpr char RenderStateCacheFileExtension[] = ".diligentcache";

std::string GetRenderStateCacheFilePath(const char* CacheLocation, const char* AppName, RENDER_DEVICE_TYPE DeviceType)
{
    if (CacheLocation == nullptr)
    {
        UNEXPECTED("Cache location is null");
        return "";
    }

    std::string StateCachePath = CacheLocation;
    if (StateCachePath == RenderStateCacheLocationAppData)
    {
        // Use the app data directory.
        StateCachePath = FileSystem::GetLocalAppDataDirectory(AppName);
    }
    else if (!StateCachePath.empty() && !FileSystem::PathExists(StateCachePath.c_str()))
    {
        // Use the user-provided directory
        FileSystem::CreateDirectory(StateCachePath.c_str());
    }

    if (!StateCachePath.empty() && !FileSystem::IsSlash(StateCachePath.back()))
        StateCachePath.push_back(FileSystem::SlashSymbol);

    if (AppName != nullptr)
    {
        StateCachePath += AppName;
        StateCachePath += '_';
    }
    StateCachePath += GetRenderDeviceTypeShortString(DeviceType);
    // Use different cache files for debug and release modes.
    // This is not required, but is convenient.
#ifdef DILIGENT_DEBUG
    StateCachePath += "_d";
#else
    StateCachePath += "_r";
#endif
    StateCachePath += RenderStateCacheFileExtension;

    return StateCachePath;
}

} // namespace Diligent

extern "C"
{
    void CreateRenderStateCache(const Diligent::RenderStateCacheCreateInfo& CreateInfo,
                                Diligent::IRenderStateCache**               ppCache)
    {
        Diligent::CreateRenderStateCache(CreateInfo, ppCache);
    }
}
