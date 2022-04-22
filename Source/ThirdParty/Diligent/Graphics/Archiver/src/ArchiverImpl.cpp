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

#include "ArchiverImpl.hpp"
#include "Archiver_Inc.hpp"

#include <bitset>

#include "PipelineStateBase.hpp"
#include "PSOSerializer.hpp"
#include "DataBlobImpl.hpp"
#include "MemoryFileStream.hpp"
#include "SerializedPipelineStateImpl.hpp"

namespace Diligent
{

static ArchiverImpl::ChunkType PipelineTypeToChunkType(PIPELINE_TYPE PipelineType)
{
    using ChunkType = ArchiverImpl::ChunkType;
    static_assert(PIPELINE_TYPE_COUNT == 5, "Please handle the new pipeline type below");
    switch (PipelineType)
    {
        case PIPELINE_TYPE_GRAPHICS:
        case PIPELINE_TYPE_MESH:
            return ChunkType::GraphicsPipelineStates;

        case PIPELINE_TYPE_COMPUTE:
            return ChunkType::ComputePipelineStates;

        case PIPELINE_TYPE_RAY_TRACING:
            return ChunkType::RayTracingPipelineStates;

        case PIPELINE_TYPE_TILE:
            return ChunkType::TilePipelineStates;

        default:
            UNEXPECTED("Unexpected pipeline type");
            return ChunkType::Undefined;
    }
}

ArchiverImpl::ArchiverImpl(IReferenceCounters* pRefCounters, SerializationDeviceImpl* pDevice) :
    TBase{pRefCounters},
    m_pSerializationDevice{pDevice}
{}

ArchiverImpl::~ArchiverImpl()
{
}

const SerializedData& ArchiverImpl::GetDeviceData(const SerializedResourceSignatureImpl& PRS, DeviceType Type)
{
    const auto* pMem = PRS.GetDeviceData(Type);
    if (pMem != nullptr)
        return *pMem;

    static const SerializedData NullData;
    return NullData;
}

const SerializedData& ArchiverImpl::GetDeviceData(const PendingData& Pending, const SerializedPipelineStateImpl& PSO, DeviceType Type)
{
    auto it = Pending.PSOShaderIndices.find(&PSO);
    if (it == Pending.PSOShaderIndices.end())
    {
        UNEXPECTED("Shader indices are not found for PSO '", PSO.GetDesc().Name, "'. This is likely a bug.");
        static const SerializedData NullData;
        return NullData;
    }

    return it->second[static_cast<size_t>(Type)];
}


template <typename MapType>
Uint32* ArchiverImpl::InitNamedResourceArrayHeader(ChunkType      Type,
                                                   const MapType& Map,
                                                   PendingData&   Pending)
{
    VERIFY_EXPR(!Map.empty());

    const auto ChunkInd = static_cast<size_t>(Type);

    auto& DataOffsetArray = Pending.DataOffsetArrayPerChunk[ChunkInd];
    auto& ChunkData       = Pending.ChunkData[ChunkInd];
    auto& Count           = Pending.ResourceCountPerChunk[ChunkInd];

    Count = StaticCast<Uint32>(Map.size());

    ChunkData = TDataElement{GetRawAllocator()};
    ChunkData.AddSpace<NamedResourceArrayHeader>();
    ChunkData.AddSpace<Uint32>(Count); // NameLength
    ChunkData.AddSpace<Uint32>(Count); // ***DataSize
    ChunkData.AddSpace<Uint32>(Count); // ***DataOffset

    for (const auto& NameAndData : Map)
        ChunkData.AddSpaceForString(NameAndData.first.GetStr());

    ChunkData.Reserve();

    auto& Header = *ChunkData.Construct<NamedResourceArrayHeader>(Count);
    VERIFY_EXPR(Header.Count == Count);

    auto* NameLengthArray = ChunkData.ConstructArray<Uint32>(Count);
    auto* DataSizeArray   = ChunkData.ConstructArray<Uint32>(Count);
    DataOffsetArray       = ChunkData.ConstructArray<Uint32>(Count); // will be initialized later

    Uint32 i = 0;
    for (const auto& NameAndData : Map)
    {
        const auto* Name    = NameAndData.first.GetStr();
        const auto  NameLen = strlen(Name);

        auto* pStr = ChunkData.CopyString(Name, NameLen);
        (void)pStr;

        NameLengthArray[i] = StaticCast<Uint32>(NameLen + 1);
        DataSizeArray[i]   = StaticCast<Uint32>(NameAndData.second->GetCommonData().Size());
        ++i;
    }

    return DataSizeArray;
}

Bool ArchiverImpl::SerializeToBlob(IDataBlob** ppBlob)
{
    DEV_CHECK_ERR(ppBlob != nullptr, "ppBlob must not be null");
    if (ppBlob == nullptr)
        return false;

    *ppBlob = nullptr;

    auto pDataBlob  = DataBlobImpl::Create(0);
    auto pMemStream = MemoryFileStream::Create(pDataBlob);

    if (!SerializeToStream(pMemStream))
        return false;

    pDataBlob->QueryInterface(IID_DataBlob, reinterpret_cast<IObject**>(ppBlob));
    return true;
}

void ArchiverImpl::ReserveSpace(PendingData& Pending) const
{
    auto& CommonData    = Pending.CommonData;
    auto& PerDeviceData = Pending.PerDeviceData;

    CommonData = TDataElement{GetRawAllocator()};
    for (auto& DeviceData : PerDeviceData)
        DeviceData = TDataElement{GetRawAllocator()};

    // Reserve space for shaders
    for (Uint32 type = 0; type < DeviceDataCount; ++type)
    {
        const auto& Shaders = Pending.Shaders[type];
        if (Shaders.Bytecodes.empty())
            continue;

        auto& DeviceData = PerDeviceData[type];
        DeviceData.AddSpace<FileOffsetAndSize>(Shaders.Bytecodes.size());
        for (const auto& Bytecode : Shaders.Bytecodes)
        {
            DeviceData.AddSpace(Bytecode.get().Size());
        }
    }

    // Reserve space for pipeline resource signatures
    for (const auto& PRS : m_Signatures)
    {
        CommonData.AddSpace<PRSDataHeader>();
        CommonData.AddSpace(PRS.second->GetCommonData().Size());

        for (Uint32 type = 0; type < DeviceDataCount; ++type)
        {
            const auto& Data = GetDeviceData(*PRS.second, static_cast<DeviceType>(type));
            PerDeviceData[type].AddSpace(Data.Size());
        }
    }

    // Reserve space for render passes
    for (const auto& RP : m_RenderPasses)
    {
        CommonData.AddSpace<RPDataHeader>();
        CommonData.AddSpace(RP.second->GetCommonData().Size());
    }

    // Reserve space for pipelines
    for (const auto& Pipelines : m_Pipelines)
    {
        for (const auto& PSO : Pipelines)
        {
            CommonData.AddSpace<PSODataHeader>();
            CommonData.AddSpace(PSO.second->GetCommonData().Size());

            for (Uint32 type = 0; type < DeviceDataCount; ++type)
            {
                const auto& Data = GetDeviceData(Pending, *PSO.second, static_cast<DeviceType>(type));
                PerDeviceData[type].AddSpace(Data.Size());
            }
        }
    }

    static_assert(ChunkCount == 9, "Reserve space for new chunk type");

    Pending.CommonData.Reserve();
    for (auto& DeviceData : Pending.PerDeviceData)
        DeviceData.Reserve();
}

void ArchiverImpl::WriteDebugInfo(PendingData& Pending) const
{
    auto& Chunk = Pending.ChunkData[static_cast<size_t>(ChunkType::ArchiveDebugInfo)];

    auto SerializeDebugInfo = [](auto& Ser) //
    {
        Uint32 APIVersion = DILIGENT_API_VERSION;
        Ser(APIVersion);

        const char* GitHash = nullptr;
#ifdef DILIGENT_CORE_COMMIT_HASH
        GitHash = DILIGENT_CORE_COMMIT_HASH;
#endif
        Ser(GitHash);
    };

    Serializer<SerializerMode::Measure> MeasureSer;
    SerializeDebugInfo(MeasureSer);

    VERIFY_EXPR(Chunk.IsEmpty());
    const auto Size = MeasureSer.GetSize();
    if (Size == 0)
        return;

    Chunk = TDataElement{GetRawAllocator()};
    Chunk.AddSpace(Size);
    Chunk.Reserve();
    Serializer<SerializerMode::Write> Ser{SerializedData{Chunk.Allocate(Size), Size}};
    SerializeDebugInfo(Ser);
}

template <typename HeaderType>
HeaderType* WriteHeader(ArchiverImpl::ChunkType     Type,
                        const SerializedData&       SrcData,
                        ArchiverImpl::TDataElement& DstChunk,
                        Uint32&                     DstOffset,
                        Uint32&                     DstArraySize)
{
    auto* pHeader = DstChunk.Construct<HeaderType>(Type);
    VERIFY_EXPR(pHeader->Type == Type);
    DstOffset = StaticCast<Uint32>(reinterpret_cast<const Uint8*>(pHeader) - DstChunk.GetDataPtr<const Uint8>());
    // DeviceSpecificDataSize & DeviceSpecificDataOffset will be initialized later

    DstChunk.Copy(SrcData.Ptr(), SrcData.Size());
    DstArraySize += sizeof(*pHeader);

    return pHeader;
}

template <typename HeaderType>
void WritePerDeviceData(HeaderType&                 Header,
                        ArchiverImpl::DeviceType    Type,
                        const SerializedData&       SrcData,
                        ArchiverImpl::TDataElement& DstChunk)
{
    if (!SrcData)
        return;

    auto* const pDst   = DstChunk.Copy(SrcData.Ptr(), SrcData.Size());
    const auto  Offset = reinterpret_cast<const Uint8*>(pDst) - DstChunk.GetDataPtr<const Uint8>();
    Header.SetSize(Type, StaticCast<Uint32>(SrcData.Size()));
    Header.SetOffset(Type, StaticCast<Uint32>(Offset));
}

template <typename DataHeaderType, typename MapType, typename WritePerDeviceDataType>
void ArchiverImpl::WriteDeviceObjectData(ChunkType Type, PendingData& Pending, MapType& ObjectMap, WritePerDeviceDataType WriteDeviceData) const
{
    if (ObjectMap.empty())
        return;

    auto* DataSizeArray   = InitNamedResourceArrayHeader(Type, ObjectMap, Pending);
    auto* DataOffsetArray = Pending.DataOffsetArrayPerChunk[static_cast<size_t>(Type)];

    Uint32 j = 0;
    for (auto& Obj : ObjectMap)
    {
        auto* pHeader = WriteHeader<DataHeaderType>(Type, Obj.second->GetCommonData(),
                                                    Pending.CommonData,
                                                    DataOffsetArray[j], DataSizeArray[j]);

        for (Uint32 type = 0; type < DeviceDataCount; ++type)
        {
            WriteDeviceData(*pHeader, static_cast<DeviceType>(type), Obj.second);
        }
        ++j;
    }
}

void ArchiverImpl::WriteShaderData(PendingData& Pending) const
{
    {
        bool HasShaders = false;
        for (Uint32 type = 0; type < DeviceDataCount && !HasShaders; ++type)
            HasShaders = !Pending.Shaders[type].Bytecodes.empty();
        if (!HasShaders)
            return;
    }

    const auto ChunkInd        = static_cast<Uint32>(ChunkType::Shaders);
    auto&      Chunk           = Pending.ChunkData[ChunkInd];
    Uint32*    DataOffsetArray = nullptr; // Pending.DataOffsetArrayPerChunk[ChunkInd];
    Uint32*    DataSizeArray   = nullptr;
    {
        VERIFY_EXPR(Chunk.IsEmpty());
        Chunk = TDataElement{GetRawAllocator()};
        Chunk.AddSpace<ShadersDataHeader>();
        Chunk.Reserve();

        auto& Header    = *Chunk.Construct<ShadersDataHeader>(ChunkType::Shaders);
        DataSizeArray   = Header.DeviceSpecificDataSize.data();
        DataOffsetArray = Header.DeviceSpecificDataOffset.data();

        Pending.ResourceCountPerChunk[ChunkInd] = DeviceDataCount;
    }

    for (Uint32 dev = 0; dev < DeviceDataCount; ++dev)
    {
        const auto& Shaders = Pending.Shaders[dev];
        auto&       Dst     = Pending.PerDeviceData[dev];

        if (Shaders.Bytecodes.empty())
            continue;

        VERIFY(Dst.GetCurrentSize() == 0, "Shaders must be written first");

        // Write common data
        auto* pOffsetAndSize = Dst.ConstructArray<FileOffsetAndSize>(Shaders.Bytecodes.size());
        DataOffsetArray[dev] = StaticCast<Uint32>(reinterpret_cast<const Uint8*>(pOffsetAndSize) - Dst.GetDataPtr<const Uint8>());
        DataSizeArray[dev]   = StaticCast<Uint32>(sizeof(FileOffsetAndSize) * Shaders.Bytecodes.size());

        for (const auto& Bytecode : Shaders.Bytecodes)
        {
            const auto& Src  = Bytecode.get();
            auto* const pDst = Dst.Copy(Src.Ptr(), Src.Size());

            pOffsetAndSize->Offset = StaticCast<Uint32>(reinterpret_cast<const Uint8*>(pDst) - Dst.GetDataPtr<const Uint8>());
            pOffsetAndSize->Size   = StaticCast<Uint32>(Src.Size());
            ++pOffsetAndSize;
        }
    }
}

void ArchiverImpl::UpdateOffsetsInArchive(PendingData& Pending) const
{
    auto& ChunkData    = Pending.ChunkData;
    auto& HeaderData   = Pending.HeaderData;
    auto& OffsetInFile = Pending.OffsetInFile;

    Uint32 NumChunks = 0;
    for (auto& Chunk : ChunkData)
    {
        NumChunks += (Chunk.IsEmpty() ? 0 : 1);
    }

    HeaderData = TDataElement{GetRawAllocator()};
    HeaderData.AddSpace<ArchiveHeader>();
    HeaderData.AddSpace<ChunkHeader>(NumChunks);
    HeaderData.Reserve();
    auto&       FileHeader   = *HeaderData.Construct<ArchiveHeader>();
    auto* const ChunkHeaders = HeaderData.ConstructArray<ChunkHeader>(NumChunks);

    FileHeader.MagicNumber = DeviceObjectArchiveBase::HeaderMagicNumber;
    FileHeader.Version     = DeviceObjectArchiveBase::HeaderVersion;
    FileHeader.NumChunks   = NumChunks;

    // Update offsets to the NamedResourceArrayHeader
    OffsetInFile    = HeaderData.GetCurrentSize();
    size_t ChunkIdx = 0;
    for (Uint32 i = 0; i < ChunkData.size(); ++i)
    {
        if (ChunkData[i].IsEmpty())
            continue;

        auto& ChunkHdr  = ChunkHeaders[ChunkIdx++];
        ChunkHdr.Type   = static_cast<ChunkType>(i);
        ChunkHdr.Size   = StaticCast<Uint32>(ChunkData[i].GetCurrentSize());
        ChunkHdr.Offset = StaticCast<Uint32>(OffsetInFile);

        OffsetInFile += ChunkHdr.Size;
    }

    // Common data
    {
        for (Uint32 i = 0; i < NumChunks; ++i)
        {
            const auto& ChunkHdr = ChunkHeaders[i];
            VERIFY_EXPR(ChunkHdr.Size > 0);
            const auto ChunkInd = static_cast<Uint32>(ChunkHdr.Type);
            const auto Count    = Pending.ResourceCountPerChunk[ChunkInd];

            for (Uint32 j = 0; j < Count; ++j)
            {
                // Update offsets to the ***DataHeader
                if (Pending.DataOffsetArrayPerChunk[ChunkInd] != nullptr)
                {
                    Uint32& Offset = Pending.DataOffsetArrayPerChunk[ChunkInd][j];
                    Offset         = (Offset == InvalidOffset ? InvalidOffset : StaticCast<Uint32>(Offset + OffsetInFile));
                }
            }
        }

        if (!Pending.CommonData.IsEmpty())
            OffsetInFile += Pending.CommonData.GetCurrentSize();
    }

    // Device specific data
    for (Uint32 dev = 0; dev < DeviceDataCount; ++dev)
    {
        if (Pending.PerDeviceData[dev].IsEmpty())
        {
            FileHeader.BlockBaseOffsets[dev] = InvalidOffset;
        }
        else
        {
            FileHeader.BlockBaseOffsets[dev] = StaticCast<Uint32>(OffsetInFile);
            OffsetInFile += Pending.PerDeviceData[dev].GetCurrentSize();
        }
    }
}

void ArchiverImpl::WritePendingDataToStream(const PendingData& Pending, IFileStream* pStream) const
{
    const size_t InitialSize = pStream->GetSize();
    pStream->Write(Pending.HeaderData.GetDataPtr(), Pending.HeaderData.GetCurrentSize());

    for (auto& Chunk : Pending.ChunkData)
    {
        if (Chunk.IsEmpty())
            continue;

        pStream->Write(Chunk.GetDataPtr(), Chunk.GetCurrentSize());
    }

    if (!Pending.CommonData.IsEmpty())
        pStream->Write(Pending.CommonData.GetDataPtr(), Pending.CommonData.GetCurrentSize());

    for (auto& DevData : Pending.PerDeviceData)
    {
        if (DevData.IsEmpty())
            continue;

        pStream->Write(DevData.GetDataPtr(), DevData.GetCurrentSize());
    }

    VERIFY_EXPR(InitialSize + pStream->GetSize() == Pending.OffsetInFile);
}

Bool ArchiverImpl::SerializeToStream(IFileStream* pStream)
{
    DEV_CHECK_ERR(pStream != nullptr, "pStream must not be null");
    if (pStream == nullptr)
        return false;

    PendingData Pending;

    // Process shader byte codes and initialize PSO shader indices
    for (const auto& Pipelines : m_Pipelines)
    {
        for (const auto& PSO : Pipelines)
        {
            auto& PerDeviceShaderIndices = Pending.PSOShaderIndices[PSO.second];

            const auto& SrcPerDeviceShaders = PSO.second->GetData().Shaders;
            for (size_t device_type = 0; device_type < SrcPerDeviceShaders.size(); ++device_type)
            {
                const auto& SrcShaders = SrcPerDeviceShaders[device_type];
                if (SrcShaders.empty())
                    continue;

                auto& DstShaders = Pending.Shaders[device_type];

                std::vector<Uint32> ShaderIndices;
                ShaderIndices.reserve(SrcShaders.size());
                for (const auto& SrcShader : SrcShaders)
                {
                    VERIFY_EXPR(SrcShader.Data);

                    auto it_inserted = DstShaders.HashToIdx.emplace(SrcShader.Hash, StaticCast<Uint32>(DstShaders.Bytecodes.size()));
                    if (it_inserted.second)
                    {
                        DstShaders.Bytecodes.emplace_back(SrcShader.Data);
                    }
                    ShaderIndices.emplace_back(it_inserted.first->second);
                }

                ShaderIndexArray Indices{ShaderIndices.data(), static_cast<Uint32>(ShaderIndices.size())};

                Serializer<SerializerMode::Measure> MeasureSer;
                PSOSerializer<SerializerMode::Measure>::SerializeShaderIndices(MeasureSer, Indices, nullptr);
                auto& SerializedIndices{PerDeviceShaderIndices[device_type]};
                SerializedIndices = MeasureSer.AllocateData(GetRawAllocator());

                Serializer<SerializerMode::Write> Ser{SerializedIndices};
                PSOSerializer<SerializerMode::Write>::SerializeShaderIndices(Ser, Indices, nullptr);
                VERIFY_EXPR(Ser.IsEnded());
            }
        }
    }

    ReserveSpace(Pending);
    WriteDebugInfo(Pending);
    WriteShaderData(Pending);

    auto WritePRSPerDeviceData = [&Pending](PRSDataHeader& Header, DeviceType Type, const RefCntAutoPtr<SerializedResourceSignatureImpl>& pPRS) //
    {
        WritePerDeviceData(Header, Type, GetDeviceData(*pPRS, Type), Pending.PerDeviceData[static_cast<size_t>(Type)]);
    };
    WriteDeviceObjectData<PRSDataHeader>(ChunkType::ResourceSignature, Pending, m_Signatures, WritePRSPerDeviceData);

    WriteDeviceObjectData<RPDataHeader>(ChunkType::RenderPass, Pending, m_RenderPasses, [](RPDataHeader& Header, DeviceType Type, const RefCntAutoPtr<SerializedRenderPassImpl>&) {});

    auto WritePSOPerDeviceData = [&Pending](PSODataHeader& Header, DeviceType Type, const auto& pPSO) //
    {
        WritePerDeviceData(Header, Type, GetDeviceData(Pending, *pPSO, Type), Pending.PerDeviceData[static_cast<size_t>(Type)]);
    };

    for (Uint32 type = 0; type < m_Pipelines.size(); ++type)
    {
        const auto& Pipelines = m_Pipelines[type];
        const auto  chunkType = PipelineTypeToChunkType(static_cast<PIPELINE_TYPE>(type));
        WriteDeviceObjectData<PSODataHeader>(chunkType, Pending, Pipelines, WritePSOPerDeviceData);
    }

    static_assert(ChunkCount == 9, "Write data for new chunk type");

    UpdateOffsetsInArchive(Pending);
    WritePendingDataToStream(Pending, pStream);

    return true;
}

bool ArchiverImpl::AddPipelineResourceSignature(IPipelineResourceSignature* pPRS)
{
    DEV_CHECK_ERR(pPRS != nullptr, "Pipeline resource signature must not be null");
    if (pPRS == nullptr)
        return false;

    RefCntAutoPtr<SerializedResourceSignatureImpl> pSerializedPRS{pPRS, IID_SerializedResourceSignature};
    if (!pSerializedPRS)
    {
        UNEXPECTED("Resource signature '", pPRS->GetDesc().Name, "' was not created by a serialization device.");
        return false;
    }
    const auto* Name = pSerializedPRS->GetName();

    std::lock_guard<std::mutex> Lock{m_SignaturesMtx};

    auto it_inserted = m_Signatures.emplace(HashMapStringKey{Name, true}, pSerializedPRS);
    if (!it_inserted.second && it_inserted.first->second != pSerializedPRS)
    {
        if (*it_inserted.first->second != *pSerializedPRS)
        {
            LOG_ERROR_MESSAGE("Pipeline resource signature with name '", Name, "' is already present in the archive. All signatures must have unique names.");
            return false;
        }
    }
    return true;
}


bool ArchiverImpl::AddRenderPass(IRenderPass* pRP)
{
    DEV_CHECK_ERR(pRP != nullptr, "Render pass must not be null");
    if (pRP == nullptr)
        return false;

    RefCntAutoPtr<SerializedRenderPassImpl> pSerializedRP{pRP, IID_SerializedRenderPass};
    if (!pSerializedRP)
    {
        UNEXPECTED("Render pass'", pRP->GetDesc().Name, "' was not created by a serialization device.");
        return false;
    }

    const auto* Name = pSerializedRP->GetDesc().Name;

    std::lock_guard<std::mutex> Lock{m_RenderPassesMtx};

    auto it_inserted = m_RenderPasses.emplace(HashMapStringKey{Name, true}, pSerializedRP);
    if (!it_inserted.second && it_inserted.first->second != pSerializedRP)
    {
        if (*it_inserted.first->second != *pSerializedRP)
        {
            LOG_ERROR_MESSAGE("Render pass with name '", Name, "' is already present in the archive. All render passes must have unique names.");
            return false;
        }
    }
    return true;
}

Bool ArchiverImpl::AddPipelineState(IPipelineState* pPSO)
{
    DEV_CHECK_ERR(pPSO != nullptr, "Pipeline state must not be null");
    if (pPSO == nullptr)
        return false;

    RefCntAutoPtr<SerializedPipelineStateImpl> pSerializedPSO{pPSO, IID_SerializedPipelineState};
    if (!pSerializedPSO)
    {
        UNEXPECTED("Pipeline state '", pPSO->GetDesc().Name, "' was not created by a serialization device.");
        return false;
    }

    const auto& Desc = pSerializedPSO->GetDesc();
    const auto* Name = Desc.Name;
    // Mesh pipelines are serialized as graphics pipeliens
    const auto PipelineType = Desc.PipelineType == PIPELINE_TYPE_MESH ? PIPELINE_TYPE_GRAPHICS : Desc.PipelineType;

    {
        std::lock_guard<std::mutex> Lock{m_PipelinesMtx[PipelineType]};

        auto it_inserted = m_Pipelines[PipelineType].emplace(HashMapStringKey{Name, true}, pSerializedPSO);
        if (!it_inserted.second)
        {
            LOG_ERROR_MESSAGE("Pipeline state with name '", Name, "' is already present in the archive. All pipelines of the same type must have unique names.");
            return false;
        }
    }

    bool Res = true;
    if (auto* pRenderPass = pSerializedPSO->GetRenderPass())
    {
        if (!AddRenderPass(pRenderPass))
            Res = false;
    }

    const auto& Signatures = pSerializedPSO->GetSignatures();
    for (auto& pSign : Signatures)
    {
        if (!AddPipelineResourceSignature(pSign.RawPtr<IPipelineResourceSignature>()))
            Res = false;
    }

    return Res;
}

} // namespace Diligent
