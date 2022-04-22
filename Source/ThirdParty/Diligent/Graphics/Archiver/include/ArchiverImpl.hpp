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
#include <vector>
#include <mutex>

#include "Archiver.h"
#include "ArchiverFactory.h"
#include "PipelineResourceSignature.h"
#include "PipelineState.h"
#include "DataBlob.h"
#include "FileStream.h"

#include "DeviceObjectArchiveBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "ObjectBase.hpp"

#include "HashUtils.hpp"
#include "BasicMath.hpp"
#include "PlatformMisc.hpp"
#include "FixedLinearAllocator.hpp"
#include "Serializer.hpp"

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

    ArchiverImpl(IReferenceCounters* pRefCounters, SerializationDeviceImpl* pDevice);
    ~ArchiverImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Archiver, TBase)

    /// Implementation of IArchiver::SerializeToBlob().
    virtual Bool DILIGENT_CALL_TYPE SerializeToBlob(IDataBlob** ppBlob) override final;

    /// Implementation of IArchiver::SerializeToStream().
    virtual Bool DILIGENT_CALL_TYPE SerializeToStream(IFileStream* pStream) override final;

    /// Implementation of IArchiver::AddPipelineState().
    virtual Bool DILIGENT_CALL_TYPE AddPipelineState(IPipelineState* pPSO) override final;

    /// Implementation of IArchiver::AddPipelineResourceSignature().
    virtual Bool DILIGENT_CALL_TYPE AddPipelineResourceSignature(IPipelineResourceSignature* pSignature) override final;

public:
    using DeviceType   = DeviceObjectArchiveBase::DeviceType;
    using ChunkType    = DeviceObjectArchiveBase::ChunkType;
    using TDataElement = FixedLinearAllocator;

private:
    using ArchiveHeader            = DeviceObjectArchiveBase::ArchiveHeader;
    using ChunkHeader              = DeviceObjectArchiveBase::ChunkHeader;
    using NamedResourceArrayHeader = DeviceObjectArchiveBase::NamedResourceArrayHeader;
    using FileOffsetAndSize        = DeviceObjectArchiveBase::FileOffsetAndSize;
    using PRSDataHeader            = DeviceObjectArchiveBase::PRSDataHeader;
    using PSODataHeader            = DeviceObjectArchiveBase::PSODataHeader;
    using RPDataHeader             = DeviceObjectArchiveBase::RPDataHeader;
    using ShadersDataHeader        = DeviceObjectArchiveBase::ShadersDataHeader;
    using TPRSNames                = DeviceObjectArchiveBase::TPRSNames;
    using ShaderIndexArray         = DeviceObjectArchiveBase::ShaderIndexArray;
    using SerializedPSOAuxData     = DeviceObjectArchiveBase::SerializedPSOAuxData;

    static constexpr auto InvalidOffset   = DeviceObjectArchiveBase::BaseDataHeader::InvalidOffset;
    static constexpr auto DeviceDataCount = static_cast<size_t>(DeviceType::Count);
    static constexpr auto ChunkCount      = static_cast<size_t>(ChunkType::Count);

    RefCntAutoPtr<SerializationDeviceImpl> m_pSerializationDevice;


    template <typename Type>
    using NamedObjectHashMap = std::unordered_map<HashMapStringKey, Type, HashMapStringKey::Hasher>;

    std::mutex                                                         m_SignaturesMtx;
    NamedObjectHashMap<RefCntAutoPtr<SerializedResourceSignatureImpl>> m_Signatures;

    std::mutex                                                  m_RenderPassesMtx;
    NamedObjectHashMap<RefCntAutoPtr<SerializedRenderPassImpl>> m_RenderPasses;

    using PSOHashMapType = NamedObjectHashMap<RefCntAutoPtr<SerializedPipelineStateImpl>>;

    std::array<std::mutex, PIPELINE_TYPE_COUNT>     m_PipelinesMtx;
    std::array<PSOHashMapType, PIPELINE_TYPE_COUNT> m_Pipelines;

    struct PerDeviceShaderData
    {
        std::unordered_map<size_t, Uint32>                        HashToIdx;
        std::vector<std::reference_wrapper<const SerializedData>> Bytecodes;
    };

    struct PendingData
    {
        TDataElement                              HeaderData;                   // ArchiveHeader, ChunkHeader[]
        std::array<TDataElement, ChunkCount>      ChunkData;                    // NamedResourceArrayHeader
        std::array<Uint32*, ChunkCount>           DataOffsetArrayPerChunk = {}; // pointer to NamedResourceArrayHeader::DataOffset - offsets to ***DataHeader
        std::array<Uint32, ChunkCount>            ResourceCountPerChunk   = {}; //
        TDataElement                              CommonData;                   // ***DataHeader
        std::array<TDataElement, DeviceDataCount> PerDeviceData;                // device specific data
        size_t                                    OffsetInFile = 0;

        std::array<PerDeviceShaderData, DeviceDataCount> Shaders;
        // Serialized global shader indices in the archive for each shader of each device type
        std::unordered_map<const SerializedPipelineStateImpl*, std::array<SerializedData, DeviceDataCount>> PSOShaderIndices;
    };

    static const SerializedData& GetDeviceData(const SerializedResourceSignatureImpl& PRS, DeviceType Type);
    static const SerializedData& GetDeviceData(const PendingData& Pending, const SerializedPipelineStateImpl& PSO, DeviceType Type);

    void ReserveSpace(PendingData& Pending) const;
    void WriteDebugInfo(PendingData& Pending) const;
    void WriteShaderData(PendingData& Pending) const;
    template <typename DataHeaderType, typename MapType, typename WritePerDeviceDataType>
    void WriteDeviceObjectData(ChunkType Type, PendingData& Pending, MapType& Map, WritePerDeviceDataType WriteDeviceData) const;

    void UpdateOffsetsInArchive(PendingData& Pending) const;
    void WritePendingDataToStream(const PendingData& Pending, IFileStream* pStream) const;

    template <typename MapType>
    static Uint32* InitNamedResourceArrayHeader(ChunkType      Type,
                                                const MapType& Map,
                                                PendingData&   Pending);

    bool AddRenderPass(IRenderPass* pRP);
};

} // namespace Diligent
