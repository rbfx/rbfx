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

#include <unordered_map>
#include <array>
#include <mutex>

#include "Archiver.h"
#include "PipelineResourceSignature.h"
#include "PipelineState.h"
#include "DataBlob.h"
#include "FileStream.h"

#include "DeviceObjectArchive.hpp"
#include "RefCntAutoPtr.hpp"
#include "ObjectBase.hpp"

#include "HashUtils.hpp"

#include "SerializationDeviceImpl.hpp"
#include "SerializedShaderImpl.hpp"
#include "SerializedRenderPassImpl.hpp"
#include "SerializedResourceSignatureImpl.hpp"
#include "SerializedPipelineStateImpl.hpp"

namespace Diligent
{

class ArchiverImpl final : public ObjectBase<IArchiver>
{
public:
    using TBase = ObjectBase<IArchiver>;

    ArchiverImpl(IReferenceCounters*      pRefCounters,
                 SerializationDeviceImpl* pDevice);
    ~ArchiverImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Archiver, TBase)

    /// Implementation of IArchiver::SerializeToBlob().
    virtual Bool DILIGENT_CALL_TYPE SerializeToBlob(Uint32 ContentVersion, IDataBlob** ppBlob) override final;

    /// Implementation of IArchiver::SerializeToStream().
    virtual Bool DILIGENT_CALL_TYPE SerializeToStream(Uint32 ContentVersion, IFileStream* pStream) override final;

    /// Implementation of IArchiver::AddShader().
    virtual Bool DILIGENT_CALL_TYPE AddShader(IShader* pShader) override final;

    /// Implementation of IArchiver::AddPipelineState().
    virtual Bool DILIGENT_CALL_TYPE AddPipelineState(IPipelineState* pPSO) override final;

    /// Implementation of IArchiver::AddPipelineResourceSignature().
    virtual Bool DILIGENT_CALL_TYPE AddPipelineResourceSignature(IPipelineResourceSignature* pSignature) override final;

    /// Implementation of IArchiver::Reset().
    virtual void DILIGENT_CALL_TYPE Reset() override final;

    /// Implementation of IArchiver::GetShader().
    virtual IShader* DILIGENT_CALL_TYPE GetShader(const char* Name) override final;

    /// Implementation of IArchiver::GetPipelineState().
    virtual IPipelineState* DILIGENT_CALL_TYPE GetPipelineState(PIPELINE_TYPE PSOType,
                                                                const char*   PSOName) override final;

    /// Implementation of IArchiver::GetPipelineResourceSignature().
    virtual IPipelineResourceSignature* DILIGENT_CALL_TYPE GetPipelineResourceSignature(const char* PRSName) override final;

private:
    bool AddRenderPass(IRenderPass* pRP);

private:
    using DeviceType   = DeviceObjectArchive::DeviceType;
    using ResourceType = DeviceObjectArchive::ResourceType;

    RefCntAutoPtr<SerializationDeviceImpl> m_pSerializationDevice;

    template <typename Type>
    using NamedObjectHashMap = std::unordered_map<HashMapStringKey, RefCntAutoPtr<Type>>;

    std::mutex                                          m_SignaturesMtx;
    NamedObjectHashMap<SerializedResourceSignatureImpl> m_Signatures;

    std::mutex                                   m_RenderPassesMtx;
    NamedObjectHashMap<SerializedRenderPassImpl> m_RenderPasses;

    std::mutex                               m_ShadersMtx;
    NamedObjectHashMap<SerializedShaderImpl> m_Shaders;

    using NamedResourceKey = DeviceObjectArchive::NamedResourceKey;
    using PSOHashMapType   = std::unordered_map<NamedResourceKey, RefCntAutoPtr<SerializedPipelineStateImpl>, NamedResourceKey::Hasher>;

    std::mutex     m_PipelinesMtx;
    PSOHashMapType m_Pipelines;
};

} // namespace Diligent
