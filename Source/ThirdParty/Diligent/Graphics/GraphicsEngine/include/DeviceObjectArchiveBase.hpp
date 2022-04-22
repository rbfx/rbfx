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

/// \file
/// Implementation of the Diligent::DeviceObjectArchiveBase class

// Archive file format
//
// | ArchiveHeader |
//
// | ChunkHeader | --> offset --> | NamedResourceArrayHeader |
//
// | NamedResourceArrayHeader | --> offset --> | ***DataHeader |
//
// | ***DataHeader | --> offset --> | device specific data |

#include <array>
#include <mutex>
#include <vector>

#include "Dearchiver.h"
#include "DeviceObjectArchive.h"
#include "PipelineState.h"

#include "ObjectBase.hpp"
#include "PipelineResourceSignatureBase.hpp"
#include "PipelineStateBase.hpp"

#include "HashUtils.hpp"
#include "RefCntAutoPtr.hpp"
#include "DynamicLinearAllocator.hpp"
#include "Serializer.hpp"

namespace Diligent
{

/// Class implementing base functionality of the device object archive object
class DeviceObjectArchiveBase : public ObjectBase<IDeviceObjectArchive>
{
public:
    using TObjectBase = ObjectBase<IDeviceObjectArchive>;

    enum class DeviceType : Uint32
    {
        OpenGL, // same as GLES
        Direct3D11,
        Direct3D12,
        Vulkan,
        Metal_MacOS,
        Metal_iOS,
        Count
    };

    using TPRSNames = std::array<const char*, MAX_RESOURCE_SIGNATURES>;

    struct ShaderIndexArray
    {
        const Uint32* pIndices = nullptr;
        Uint32        Count    = 0;
    };

    // Serialized pipeline state auxiliary data
    struct SerializedPSOAuxData
    {
        // Shaders have been serialized without the shader reflection information.
        bool NoShaderReflection = false;
    };

    /// \param pRefCounters - Reference counters object that controls the lifetime of this device object archive.
    /// \param pArchive     - Source data that this archive will be created from.
    /// \param DevType      - Device type.
    DeviceObjectArchiveBase(IReferenceCounters* pRefCounters,
                            IArchive*           pArchive,
                            DeviceType          DevType);

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DeviceObjectArchive, TObjectBase)

    virtual void DILIGENT_CALL_TYPE ClearResourceCache() override final;

    void UnpackGraphicsPSO(const PipelineStateUnpackInfo& UnpackInfo, IPipelineState** ppPSO);
    void UnpackComputePSO(const PipelineStateUnpackInfo& UnpackInfo, IPipelineState** ppPSO);
    void UnpackRayTracingPSO(const PipelineStateUnpackInfo& UnpackInfo, IPipelineState** ppPSO);
    void UnpackTilePSO(const PipelineStateUnpackInfo& UnpackInfo, IPipelineState** ppPSO);
    void UnpackRenderPass(const RenderPassUnpackInfo& UnpackInfo, IRenderPass** ppRP);

protected:
    static constexpr Uint32 HeaderMagicNumber = 0xDE00000A;
    static constexpr Uint32 HeaderVersion     = 2;
    static constexpr Uint32 DataPtrAlign      = sizeof(Uint64);

    friend class ArchiverImpl;
    friend class ArchiveRepacker;

    // Archive header contains the block offsets.
    // Any block can be added or removed without patching all offsets in the archive,
    // only the base offsets need to be patched.
    enum class BlockOffsetType : Uint32
    {
        // Device-specific data
        OpenGL,
        Direct3D11,
        Direct3D12,
        Vulkan,
        Metal_MacOS,
        Metal_iOS,
        Count
    };
    using TBlockBaseOffsets = std::array<Uint32, static_cast<size_t>(BlockOffsetType::Count)>;

#define CHECK_HEADER_SIZE(Header, Size)                                                                                                           \
    static_assert(sizeof(Header) % 8 == 0, "sizeof(" #Header ") must be a multiple of 8. Use padding to align it.");                              \
    static_assert(sizeof(Header) == Size, "sizeof(" #Header ") must be " #Size ". Reading binary archive will result in invalid memory access."); \
    static_assert(sizeof(Header) % alignof(Header) == 0, "sizeof(" #Header ") is not a multiple of its alignment.");

    struct ArchiveHeader
    {
        Uint32            MagicNumber      = 0;
        Uint32            Version          = 0;
        TBlockBaseOffsets BlockBaseOffsets = {};
        Uint32            NumChunks        = 0;
        Uint32            _Padding         = ~0u;

        //ChunkHeader     Chunks  [NumChunks]
    };
    CHECK_HEADER_SIZE(ArchiveHeader, 40)

    enum class ChunkType : Uint32
    {
        Undefined = 0,
        ArchiveDebugInfo,
        ResourceSignature,
        GraphicsPipelineStates,
        ComputePipelineStates,
        RayTracingPipelineStates,
        TilePipelineStates,
        RenderPass,
        Shaders,
        Count
    };

    struct ChunkHeader
    {
        ChunkHeader() noexcept {}
        ChunkHeader(ChunkType _Type) noexcept :
            Type{_Type}
        {}

        ChunkType Type     = ChunkType::Undefined;
        Uint32    Size     = 0;
        Uint32    Offset   = 0; // offset to NamedResourceArrayHeader
        Uint32    _Padding = ~0u;

        bool operator==(const ChunkHeader& Rhs) const { return Type == Rhs.Type && Size == Rhs.Size && Offset == Rhs.Offset; }
    };
    CHECK_HEADER_SIZE(ChunkHeader, 16)

    struct NamedResourceArrayHeader
    {
        Uint32 Count    = 0;
        Uint32 _Padding = ~0u;

        //Uint32 NameLength    [Count]
        //Uint32 ***DataSize   [Count]
        //Uint32 ***DataOffset [Count] // for PRSDataHeader / PSODataHeader
        //char   NameData      []
    };
    CHECK_HEADER_SIZE(NamedResourceArrayHeader, 8)

    struct BaseDataHeader
    {
        using Uint32Array = std::array<Uint32, static_cast<size_t>(DeviceType::Count)>;

        static constexpr Uint32 InvalidOffset = ~0u;

        BaseDataHeader(ChunkType _Type) noexcept :
            Type{_Type}
        {
            DeviceSpecificDataOffset.fill(Uint32{InvalidOffset});
        }

        const ChunkType Type      = ChunkType::Undefined;
        const Uint32    _Padding0 = ~0u;

        Uint32Array DeviceSpecificDataSize{};
        Uint32Array DeviceSpecificDataOffset{};

        Uint32 GetSize(DeviceType DevType) const { return DeviceSpecificDataSize[static_cast<size_t>(DevType)]; }
        Uint32 GetOffset(DeviceType DevType) const { return DeviceSpecificDataOffset[static_cast<size_t>(DevType)]; }
        Uint32 GetEndOffset(DeviceType DevType) const { return GetOffset(DevType) + GetSize(DevType); }

        void SetSize(DeviceType DevType, Uint32 Size) { DeviceSpecificDataSize[static_cast<size_t>(DevType)] = Size; }
        void SetOffset(DeviceType DevType, Uint32 Offset) { DeviceSpecificDataOffset[static_cast<size_t>(DevType)] = Offset; }
    };
    CHECK_HEADER_SIZE(BaseDataHeader, 56)

    struct PRSDataHeader : BaseDataHeader
    {
        PRSDataHeader(ChunkType _Type) noexcept :
            BaseDataHeader{_Type}
        {
            VERIFY_EXPR(Type == ChunkType::ResourceSignature);
        }
        //PipelineResourceSignatureDesc
        //PipelineResourceSignatureInternalData
    };
    CHECK_HEADER_SIZE(PRSDataHeader, 56)


    struct PSODataHeader : BaseDataHeader
    {
        PSODataHeader(ChunkType _Type) noexcept :
            BaseDataHeader{_Type}
        {
            VERIFY_EXPR((Type == ChunkType::GraphicsPipelineStates ||
                         Type == ChunkType::ComputePipelineStates ||
                         Type == ChunkType::RayTracingPipelineStates ||
                         Type == ChunkType::TilePipelineStates));
        }

        //GraphicsPipelineStateCreateInfo | ComputePipelineStateCreateInfo | TilePipelineStateCreateInfo | RayTracingPipelineStateCreateInfo
    };
    CHECK_HEADER_SIZE(PSODataHeader, 56)


    struct ShadersDataHeader : BaseDataHeader
    {
        ShadersDataHeader(ChunkType _Type = ChunkType::Shaders) noexcept :
            BaseDataHeader{_Type}
        {
            VERIFY_EXPR(Type == ChunkType::Shaders);
        }
    };
    CHECK_HEADER_SIZE(ShadersDataHeader, 56)


    struct RPDataHeader
    {
        RPDataHeader(ChunkType _Type) noexcept :
            Type{_Type}
        {
            VERIFY_EXPR(Type == ChunkType::RenderPass);
        }

        const ChunkType Type      = ChunkType::RenderPass;
        const Uint32    _Padding1 = ~0u;
    };
    CHECK_HEADER_SIZE(RPDataHeader, 8)

#undef CHECK_HEADER_SIZE

private:
    struct FileOffsetAndSize
    {
        Uint32 Offset = 0;
        Uint32 Size   = 0;

        constexpr bool operator==(const FileOffsetAndSize& Rhs) const { return Offset == Rhs.Offset && Size == Rhs.Size; }

        static constexpr FileOffsetAndSize Invalid() { return {~0u, ~0u}; }
    };

    template <typename ResPtrType> // RefCntAutoPtr or RefCntWeakPtr
    struct FileOffsetSizeAndRes : FileOffsetAndSize
    {
        ResPtrType pRes;

        explicit FileOffsetSizeAndRes(const FileOffsetAndSize& OffsetAndSize) noexcept :
            FileOffsetAndSize{OffsetAndSize}
        {}
    };


    template <typename ResType>
    class OffsetSizeAndResourceMap
    {
    public:
        OffsetSizeAndResourceMap() noexcept {};

        OffsetSizeAndResourceMap(const OffsetSizeAndResourceMap&) = delete;
        OffsetSizeAndResourceMap(OffsetSizeAndResourceMap&&)      = delete;
        OffsetSizeAndResourceMap& operator=(const OffsetSizeAndResourceMap&) = delete;
        OffsetSizeAndResourceMap& operator=(OffsetSizeAndResourceMap&&) = delete;

        void Insert(const char* Name, Uint32 Offset, Uint32 Size);

        FileOffsetAndSize GetOffsetAndSize(const char* Name, const char*& StoredNamePtr);

        bool GetResource(const char* Name, ResType** ppResource);
        void SetResource(const char* Name, ResType* pResource);

        void ReleaseResources();

    private:
        std::mutex m_Mtx;
        // Keep weak resource references in the cache
        std::unordered_map<HashMapStringKey, FileOffsetSizeAndRes<RefCntWeakPtr<ResType>>, HashMapStringKey::Hasher> m_Map;
    };

    OffsetSizeAndResourceMap<IPipelineResourceSignature> m_PRSMap;
    OffsetSizeAndResourceMap<IPipelineState>             m_GraphicsPSOMap;
    OffsetSizeAndResourceMap<IPipelineState>             m_ComputePSOMap;
    OffsetSizeAndResourceMap<IPipelineState>             m_TilePSOMap;
    OffsetSizeAndResourceMap<IPipelineState>             m_RayTracingPSOMap;
    OffsetSizeAndResourceMap<IRenderPass>                m_RenderPassMap;

    // Use strong references for shaders
    using TShaderOffsetAndCache = std::vector<FileOffsetSizeAndRes<RefCntAutoPtr<IShader>>>;

    std::mutex            m_ShadersGuard;
    TShaderOffsetAndCache m_Shaders;

    struct
    {
        String GitHash;
        Uint32 APIVersion = 0;
    } m_DebugInfo;

    RefCntAutoPtr<IArchive> m_pArchive; // archive is thread-safe
    const DeviceType        m_DevType;
    TBlockBaseOffsets       m_BaseOffsets = {};

    template <typename ResourceHandlerType>
    static void ReadNamedResources(IArchive*           pArchive,
                                   const ChunkHeader&  Chunk,
                                   ResourceHandlerType Handler) noexcept(false);

    template <typename ResType>
    void ReadNamedResources(const ChunkHeader& Chunk, OffsetSizeAndResourceMap<ResType>& ResourceMap) noexcept(false);
    void ReadShaders(const ChunkHeader& Chunk) noexcept(false);
    void ReadArchiveDebugInfo(const ChunkHeader& Chunk) noexcept(false);

    BlockOffsetType GetBlockOffsetType() const;

    static const char* ChunkTypeToResName(ChunkType Type);

protected:
    struct PRSData
    {
        DynamicLinearAllocator        Allocator;
        const PRSDataHeader*          pHeader = nullptr;
        PipelineResourceSignatureDesc Desc{};

        static constexpr ChunkType ExpectedChunkType = ChunkType::ResourceSignature;

        explicit PRSData(IMemoryAllocator& Allocator, Uint32 BlockSize = 1 << 10) :
            Allocator{Allocator, BlockSize}
        {}

        bool Deserialize(const char* Name, Serializer<SerializerMode::Read>& Ser);
    };

    template <typename CreateInfoType>
    struct PSOData
    {
        DynamicLinearAllocator Allocator;
        const PSODataHeader*   pHeader = nullptr;
        CreateInfoType         CreateInfo{};
        PSOCreateInternalInfo  InternalCI;
        SerializedPSOAuxData   AuxData;
        TPRSNames              PRSNames{};
        const char*            RenderPassName = nullptr;

        // Strong references to pipeline resource signatures, render pass, etc.
        std::vector<RefCntAutoPtr<IDeviceObject>> Objects;
        std::vector<RefCntAutoPtr<IShader>>       Shaders;

        static const ChunkType ExpectedChunkType;

        explicit PSOData(IMemoryAllocator& Allocator, Uint32 BlockSize = 2 << 10) :
            Allocator{Allocator, BlockSize}
        {}

        bool Deserialize(const char* Name, Serializer<SerializerMode::Read>& Ser);
        void AssignShaders();
        void CreatePipeline(IRenderDevice* pDevice, IPipelineState** ppPSO);

    private:
        void DeserializeInternal(Serializer<SerializerMode::Read>& Ser);
    };

private:
    struct RPData
    {
        DynamicLinearAllocator Allocator;
        const RPDataHeader*    pHeader = nullptr;
        RenderPassDesc         Desc{};

        static constexpr ChunkType ExpectedChunkType = ChunkType::RenderPass;

        explicit RPData(IMemoryAllocator& Allocator, Uint32 BlockSize = 1 << 10) :
            Allocator{Allocator, BlockSize}
        {}

        bool Deserialize(const char* Name, Serializer<SerializerMode::Read>& Ser);
    };

    template <typename ResType, typename ReourceDataType>
    bool LoadResourceData(OffsetSizeAndResourceMap<ResType>& ResourceMap,
                          const char*                        ResourceName,
                          ReourceDataType&                   ResData);

    template <typename HeaderType>
    SerializedData GetDeviceSpecificData(const HeaderType&       Header,
                                         DynamicLinearAllocator& Allocator,
                                         const char*             ResTypeName,
                                         BlockOffsetType         BlockType);

    template <typename CreateInfoType>
    bool UnpackPSOSignatures(PSOData<CreateInfoType>& PSO, IRenderDevice* pDevice);

    template <typename CreateInfoType>
    bool UnpackPSORenderPass(PSOData<CreateInfoType>& PSO, IRenderDevice* pDevice) { return true; }

    template <typename CreateInfoType>
    bool UnpackPSOShaders(PSOData<CreateInfoType>& PSO, IRenderDevice* pDevice);

    template <typename PSOCreateInfoType>
    bool ModifyPipelineStateCreateInfo(PSOCreateInfoType&             CreateInfo,
                                       const PipelineStateUnpackInfo& DeArchiveInfo);

    template <typename CreateInfoType>
    void UnpackPipelineStateImpl(const PipelineStateUnpackInfo&            UnpackInfo,
                                 IPipelineState**                          ppPSO,
                                 OffsetSizeAndResourceMap<IPipelineState>& PSOMap);

protected:
    template <typename RenderDeviceImplType, typename PRSSerializerType>
    RefCntAutoPtr<IPipelineResourceSignature> UnpackResourceSignatureImpl(
        const ResourceSignatureUnpackInfo& DeArchiveInfo,
        bool                               IsImplicit);

    virtual RefCntAutoPtr<IPipelineResourceSignature> UnpackResourceSignature(const ResourceSignatureUnpackInfo& DeArchiveInfo, bool IsImplicit) = 0;

    virtual RefCntAutoPtr<IShader> UnpackShader(const ShaderCreateInfo& ShaderCI,
                                                IRenderDevice*          pDevice);
};

template <typename RenderDeviceImplType, typename PRSSerializerType>
RefCntAutoPtr<IPipelineResourceSignature> DeviceObjectArchiveBase::UnpackResourceSignatureImpl(
    const ResourceSignatureUnpackInfo& DeArchiveInfo,
    bool                               IsImplicit)
{
    RefCntAutoPtr<IPipelineResourceSignature> pSignature;
    // Do not reuse implicit signatures
    if (!IsImplicit && m_PRSMap.GetResource(DeArchiveInfo.Name, pSignature.RawDblPtr()))
        return pSignature;

    PRSData PRS{GetRawAllocator()};
    if (!LoadResourceData(m_PRSMap, DeArchiveInfo.Name, PRS))
        return {};

    PRS.Desc.SRBAllocationGranularity = DeArchiveInfo.SRBAllocationGranularity;

    const auto Data = GetDeviceSpecificData(*PRS.pHeader, PRS.Allocator, "Resource signature", GetBlockOffsetType());
    if (!Data)
        return {};

    Serializer<SerializerMode::Read> Ser{Data};

    bool SpecialDesc = false;
    Ser(SpecialDesc);
    if (SpecialDesc)
    {
        // The signature uses special description that differs from the common
        const auto* Name = PRS.Desc.Name;
        PRS.Desc         = {};
        PRS.Deserialize(Name, Ser);
    }

    typename PRSSerializerType::InternalDataType InternalData;
    PRSSerializerType::SerializeInternalData(Ser, InternalData, &PRS.Allocator);
    VERIFY_EXPR(Ser.IsEnded());

    auto* pRenderDevice = ClassPtrCast<RenderDeviceImplType>(DeArchiveInfo.pDevice);
    pRenderDevice->CreatePipelineResourceSignature(PRS.Desc, InternalData, &pSignature);

    if (!IsImplicit)
        m_PRSMap.SetResource(DeArchiveInfo.Name, pSignature.RawPtr());

    return pSignature;
}


template <typename ResourceHandlerType>
void DeviceObjectArchiveBase::ReadNamedResources(IArchive*           pArchive,
                                                 const ChunkHeader&  Chunk,
                                                 ResourceHandlerType Handler) noexcept(false)
{
    VERIFY_EXPR(Chunk.Type == ChunkType::ResourceSignature ||
                Chunk.Type == ChunkType::GraphicsPipelineStates ||
                Chunk.Type == ChunkType::ComputePipelineStates ||
                Chunk.Type == ChunkType::RayTracingPipelineStates ||
                Chunk.Type == ChunkType::TilePipelineStates ||
                Chunk.Type == ChunkType::RenderPass);

    std::vector<Uint8> Data(Chunk.Size);
    if (!pArchive->Read(Chunk.Offset, Data.size(), Data.data()))
    {
        LOG_ERROR_AND_THROW("Failed to read resource list from archive");
    }

    FixedLinearAllocator InPlaceAlloc{Data.data(), Data.size()};

    const auto& Header          = *InPlaceAlloc.Allocate<NamedResourceArrayHeader>();
    const auto* NameLengthArray = InPlaceAlloc.Allocate<Uint32>(Header.Count);
    const auto* DataSizeArray   = InPlaceAlloc.Allocate<Uint32>(Header.Count);
    const auto* DataOffsetArray = InPlaceAlloc.Allocate<Uint32>(Header.Count);

    // Read names
    for (Uint32 i = 0; i < Header.Count; ++i)
    {
        if (InPlaceAlloc.GetCurrentSize() + NameLengthArray[i] > Data.size())
        {
            LOG_ERROR_AND_THROW("Failed to read archive data");
        }
        if (DataOffsetArray[i] + DataSizeArray[i] > pArchive->GetSize())
        {
            LOG_ERROR_AND_THROW("Failed to read archive data");
        }
        const auto* Name = InPlaceAlloc.Allocate<char>(NameLengthArray[i]);
        VERIFY_EXPR(strlen(Name) + 1 == NameLengthArray[i]);

        Handler(Name, DataOffsetArray[i], DataSizeArray[i]);
    }
}

} // namespace Diligent
