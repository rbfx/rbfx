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
/// Definition of the Diligent::RenderStateCacheImpl class

#include <unordered_map>
#include <mutex>

#include "RenderStateCache.h"
#include "SerializationDevice.h"
#include "Archiver.h"
#include "UniqueIdentifier.hpp"
#include "ObjectBase.hpp"
#include "XXH128Hasher.hpp"

namespace Diligent
{

/// Implementation of IRenderStateCache
class RenderStateCacheImpl final : public ObjectBase<IRenderStateCache>
{
public:
    using TBase = ObjectBase<IRenderStateCache>;

public:
    RenderStateCacheImpl(IReferenceCounters*               pRefCounters,
                         const RenderStateCacheCreateInfo& CreateInfo);

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_RenderStateCache, TBase);

    virtual bool DILIGENT_CALL_TYPE Load(const IDataBlob* pArchive,
                                         Uint32           ContentVersion,
                                         bool             MakeCopy) override final
    {
        return m_pDearchiver->LoadArchive(pArchive, ContentVersion, MakeCopy);
    }

    virtual bool DILIGENT_CALL_TYPE CreateShader(const ShaderCreateInfo& ShaderCI,
                                                 IShader**               ppShader) override final;

    virtual bool DILIGENT_CALL_TYPE CreateGraphicsPipelineState(
        const GraphicsPipelineStateCreateInfo& PSOCreateInfo,
        IPipelineState**                       ppPipelineState) override final
    {
        return CreatePipelineState(PSOCreateInfo, ppPipelineState);
    }

    virtual bool DILIGENT_CALL_TYPE CreateComputePipelineState(
        const ComputePipelineStateCreateInfo& PSOCreateInfo,
        IPipelineState**                      ppPipelineState) override final
    {
        return CreatePipelineState(PSOCreateInfo, ppPipelineState);
    }

    virtual bool DILIGENT_CALL_TYPE CreateRayTracingPipelineState(
        const RayTracingPipelineStateCreateInfo& PSOCreateInfo,
        IPipelineState**                         ppPipelineState) override final
    {
        return CreatePipelineState(PSOCreateInfo, ppPipelineState);
    }

    virtual bool DILIGENT_CALL_TYPE CreateTilePipelineState(
        const TilePipelineStateCreateInfo& PSOCreateInfo,
        IPipelineState**                   ppPipelineState) override final
    {
        return CreatePipelineState(PSOCreateInfo, ppPipelineState);
    }

    virtual Bool DILIGENT_CALL_TYPE WriteToBlob(Uint32 ContentVersion, IDataBlob** ppBlob) override final;

    virtual Bool DILIGENT_CALL_TYPE WriteToStream(Uint32 ContentVersion, IFileStream* pStream) override final;

    virtual void DILIGENT_CALL_TYPE Reset() override final;

    virtual Uint32 DILIGENT_CALL_TYPE Reload(ReloadGraphicsPipelineCallbackType ReloadGraphicsPipeline, void* pUserData) override final;

    virtual Uint32 DILIGENT_CALL_TYPE GetContentVersion() const override final
    {
        return m_pDearchiver ? m_pDearchiver->GetContentVersion() : ~0u;
    }

    bool CreateShaderInternal(const ShaderCreateInfo& ShaderCI,
                              IShader**               ppShader);

    template <typename CreateInfoType>
    bool CreatePipelineStateInternal(const CreateInfoType& PSOCreateInfo,
                                     IPipelineState**      ppPipelineState);

    RefCntAutoPtr<IShader> FindReloadableShader(IShader* pShader);

private:
    static std::string HashToStr(Uint64 Low, Uint64 High);

    static std::string MakeHashStr(const char* Name, const XXH128Hash& Hash);

    template <typename CreateInfoType>
    struct SerializedPsoCIWrapperBase;

    template <typename CreateInfoType>
    struct SerializedPsoCIWrapper;

    template <typename CreateInfoType>
    bool CreatePipelineState(const CreateInfoType& PSOCreateInfo,
                             IPipelineState**      ppPipelineState);

private:
    RefCntAutoPtr<IRenderDevice>                   m_pDevice;
    const RENDER_DEVICE_TYPE                       m_DeviceType;
    const size_t                                   m_DeviceHash; // Hash of the device-specific properties
    const RenderStateCacheCreateInfo               m_CI;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> m_pReloadSource;
    RefCntAutoPtr<ISerializationDevice>            m_pSerializationDevice;
    RefCntAutoPtr<IArchiver>                       m_pArchiver;
    RefCntAutoPtr<IDearchiver>                     m_pDearchiver;

    std::mutex                                             m_ShadersMtx;
    std::unordered_map<XXH128Hash, RefCntWeakPtr<IShader>> m_Shaders;

    std::mutex                                                   m_ReloadableShadersMtx;
    std::unordered_map<UniqueIdentifier, RefCntWeakPtr<IShader>> m_ReloadableShaders;

    std::mutex                                                    m_PipelinesMtx;
    std::unordered_map<XXH128Hash, RefCntWeakPtr<IPipelineState>> m_Pipelines;

    std::mutex                                                          m_ReloadablePipelinesMtx;
    std::unordered_map<UniqueIdentifier, RefCntWeakPtr<IPipelineState>> m_ReloadablePipelines;
};

} // namespace Diligent
