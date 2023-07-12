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

#include "DeviceObjectArchive.hpp"

#include <algorithm>
#include <sstream>

#include "Shader.h"
#include "EngineMemory.h"
#include "DataBlobImpl.hpp"
#include "PSOSerializer.hpp"

namespace Diligent
{

DeviceObjectArchive::DeviceType RenderDeviceTypeToArchiveDeviceType(RENDER_DEVICE_TYPE Type)
{
    static_assert(RENDER_DEVICE_TYPE_COUNT == 7, "Did you add a new render device type? Please handle it here.");
    switch (Type)
    {
        // clang-format off
        case RENDER_DEVICE_TYPE_D3D11:  return DeviceObjectArchive::DeviceType::Direct3D11;
        case RENDER_DEVICE_TYPE_D3D12:  return DeviceObjectArchive::DeviceType::Direct3D12;
        case RENDER_DEVICE_TYPE_GL:     return DeviceObjectArchive::DeviceType::OpenGL;
        case RENDER_DEVICE_TYPE_GLES:   return DeviceObjectArchive::DeviceType::OpenGL;
        case RENDER_DEVICE_TYPE_VULKAN: return DeviceObjectArchive::DeviceType::Vulkan;
#if PLATFORM_MACOS
        case RENDER_DEVICE_TYPE_METAL:  return DeviceObjectArchive::DeviceType::Metal_MacOS;
#elif PLATFORM_IOS || PLATFORM_TVOS
        case RENDER_DEVICE_TYPE_METAL:  return DeviceObjectArchive::DeviceType::Metal_iOS;
#endif
        // clang-format on
        default:
            UNEXPECTED("Unexpected device type");
            return DeviceObjectArchive::DeviceType::Count;
    }
}

DeviceObjectArchive::ArchiveHeader::ArchiveHeader() noexcept
{
#ifdef DILIGENT_CORE_COMMIT_HASH
    GitHash = DILIGENT_CORE_COMMIT_HASH;
#endif
}

namespace
{

template <SerializerMode Mode>
struct ArchiveSerializer
{
    Serializer<Mode>& Ser;

    template <typename T>
    using ConstQual = typename Serializer<Mode>::template ConstQual<T>;

    using ArchiveHeader = DeviceObjectArchive::ArchiveHeader;
    using ResourceData  = DeviceObjectArchive::ResourceData;
    using ShadersVector = std::vector<SerializedData>;

    bool SerializeHeader(ConstQual<ArchiveHeader>& Header) const
    {
        ASSERT_SIZEOF64(Header, 24, "Please handle new members here");
        // NB: this must match header deserialization in DeviceObjectArchive::Deserialize
        return Ser(Header.MagicNumber, Header.Version, Header.APIVersion, Header.ContentVersion, Header.GitHash);
    }

    bool SerializeResourceData(ConstQual<ResourceData>& ResData) const
    {
        if (!Ser.Serialize(ResData.Common))
            return true;

        for (auto& DevData : ResData.DeviceSpecific)
        {
            if (!Ser.Serialize(DevData))
                return false;
        }

        return true;
    }

    bool SerializeShaders(ConstQual<ShadersVector>& Shaders) const;
};

template <SerializerMode Mode>
bool ArchiveSerializer<Mode>::SerializeShaders(ConstQual<ShadersVector>& Shaders) const
{
    static_assert(Mode == SerializerMode::Measure || Mode == SerializerMode::Write, "Measure or Write mode is expected.");

    Uint32 NumShaders = static_cast<Uint32>(Shaders.size());
    if (!Ser(NumShaders))
        return false;

    for (const auto& Shader : Shaders)
    {
        if (!Ser.Serialize(Shader))
            return false;
    }

    return true;
}

template <>
bool ArchiveSerializer<SerializerMode::Read>::SerializeShaders(ShadersVector& Shaders) const
{
    Uint32 NumShaders = 0;
    if (!Ser(NumShaders))
        return false;

    Shaders.resize(NumShaders);
    for (auto& Shader : Shaders)
    {
        if (!Ser.Serialize(Shader))
            return false;
    }

    return true;
}

} // namespace

DeviceObjectArchive::DeviceObjectArchive(Uint32 ContentVersion) noexcept :
    m_ContentVersion{ContentVersion}
{
}

void DeviceObjectArchive::Deserialize(const CreateInfo& CI) noexcept(false)
{
    Serializer<SerializerMode::Read> Reader{
        SerializedData{
            const_cast<void*>(CI.pData->GetConstDataPtr()),
            CI.pData->GetSize(),
        },
    };
    ArchiveSerializer<SerializerMode::Read> ArchiveReader{Reader};

    // NB: this must match header serialization in DeviceObjectArchive::SerializeHeader
    ArchiveHeader Header;
    ASSERT_SIZEOF64(Header, 24, "Please handle new members here");
    if (!ArchiveReader.Ser(Header.MagicNumber))
        LOG_ERROR_AND_THROW("Failed to read device object archive header magic number.");

    if (Header.MagicNumber != HeaderMagicNumber)
        LOG_ERROR_AND_THROW("Invalid device object archive header.");

    if (!ArchiveReader.Ser(Header.Version))
        LOG_ERROR_AND_THROW("Failed to read device object archive version.");

    if (Header.Version != ArchiveVersion)
        LOG_ERROR_AND_THROW("Unsupported device object archive version: ", Header.Version, ". Expected version: ", Uint32{ArchiveVersion});

    if (!ArchiveReader.Ser(Header.APIVersion))
        LOG_ERROR_AND_THROW("Failed to read Diligent API version.");

    if (!ArchiveReader.Ser(Header.ContentVersion))
        LOG_ERROR_AND_THROW("Failed to read device object archive content version.");

    if (CI.ContentVersion != CreateInfo{}.ContentVersion && Header.ContentVersion != CI.ContentVersion)
        LOG_ERROR_AND_THROW("Invalid archive content version: ", Header.ContentVersion, ". Expected version: ", CI.ContentVersion);
    m_ContentVersion = Header.ContentVersion;

    if (!ArchiveReader.Ser(Header.GitHash))
        LOG_ERROR_AND_THROW("Failed to read Git Hash.");

    Uint32 NumResources = 0;
    if (!Reader(NumResources))
        LOG_ERROR_AND_THROW("Failed to read the number of named resources in the device object archive.");

    for (Uint32 res = 0; res < NumResources; ++res)
    {
        const char*  Name    = nullptr;
        ResourceType ResType = ResourceType::Undefined;
        if (!Reader(ResType, Name))
            LOG_ERROR_AND_THROW("Failed to read the type and name of resource ", res, "/", NumResources, '.');
        VERIFY_EXPR(Name != nullptr);

        // No need to make the name copy as we keep the source data blob alive.
        constexpr auto MakeNameCopy = false;
        auto&          ResData      = m_NamedResources[NamedResourceKey{ResType, Name, MakeNameCopy}];

        if (!ArchiveReader.SerializeResourceData(ResData))
            LOG_ERROR_AND_THROW("Failed to read data of resource '", Name, "'.");
    }

    for (auto& Shaders : m_DeviceShaders)
    {
        if (!ArchiveReader.SerializeShaders(Shaders))
            LOG_ERROR_AND_THROW("Failed to read shader data from the device object archive.");
    }
}

void DeviceObjectArchive::Serialize(IDataBlob** ppDataBlob) const
{
    if (ppDataBlob == nullptr)
    {
        DEV_ERROR("Pointer to the data blob object must not be null");
        return;
    }
    DEV_CHECK_ERR(*ppDataBlob == nullptr, "Data blob object must be null");

    auto SerializeThis = [this](auto& Ser) {
        constexpr auto SerMode    = std::remove_reference<decltype(Ser)>::type::GetMode();
        const auto     ArchiveSer = ArchiveSerializer<SerMode>{Ser};

        ArchiveHeader Header;
        Header.ContentVersion = m_ContentVersion;

        auto res = ArchiveSer.SerializeHeader(Header);
        VERIFY(res, "Failed to serialize header");

        Uint32 NumResources = StaticCast<Uint32>(m_NamedResources.size());
        res                 = Ser(NumResources);
        VERIFY(res, "Failed to serialize the number of resources");

        for (const auto& res_it : m_NamedResources)
        {
            const auto* Name    = res_it.first.GetName();
            const auto  ResType = res_it.first.GetType();

            res = Ser(ResType, Name);
            VERIFY(res, "Failed to serialize resource type and name");

            res = ArchiveSer.SerializeResourceData(res_it.second);
            VERIFY(res, "Failed to serialize resource data");
        }

        for (auto& Shaders : m_DeviceShaders)
        {
            res = ArchiveSer.SerializeShaders(Shaders);
            VERIFY(res, "Failed to serialize shaders");
        }
    };

    Serializer<SerializerMode::Measure> Measurer;
    SerializeThis(Measurer);

    auto pDataBlob = DataBlobImpl::Create(Measurer.GetSize());

    Serializer<SerializerMode::Write> Writer{SerializedData{pDataBlob->GetDataPtr(), pDataBlob->GetSize()}};
    SerializeThis(Writer);
    VERIFY_EXPR(Writer.IsEnded());

    *ppDataBlob = pDataBlob.Detach();
}


namespace
{

const char* ArchiveDeviceTypeToString(Uint32 dev)
{
    using DeviceType = DeviceObjectArchive::DeviceType;
    static_assert(static_cast<Uint32>(DeviceType::Count) == 6, "Please handle the new archive device type below");
    switch (static_cast<DeviceType>(dev))
    {
        // clang-format off
        case DeviceType::OpenGL:      return "OpenGL";
        case DeviceType::Direct3D11:  return "Direct3D11";
        case DeviceType::Direct3D12:  return "Direct3D12";
        case DeviceType::Vulkan:      return "Vulkan";
        case DeviceType::Metal_MacOS: return "Metal for MacOS";
        case DeviceType::Metal_iOS:   return "Metal for iOS";
        // clang-format on
        default:
            UNEXPECTED("Unexpected device type");
            return "unknown";
    }
}

const char* ResourceTypeToString(DeviceObjectArchive::ResourceType Type)
{
    using ResourceType = DeviceObjectArchive::ResourceType;
    static_assert(static_cast<size_t>(ResourceType::Count) == 8, "Please handle the new chunk type below");
    switch (Type)
    {
        // clang-format off
        case ResourceType::Undefined:          return "Undefined";
        case ResourceType::StandaloneShader:   return "Standalone Shaders";
        case ResourceType::ResourceSignature:  return "Resource Signatures";
        case ResourceType::GraphicsPipeline:   return "Graphics Pipelines";
        case ResourceType::ComputePipeline:    return "Compute Pipelines";
        case ResourceType::RayTracingPipeline: return "Ray-Tracing Pipelines";
        case ResourceType::TilePipeline:       return "Tile Pipelines";
        case ResourceType::RenderPass:         return "Render Passes";
        // clang-format on
        default:
            UNEXPECTED("Unexpected chunk type");
            return "";
    }
}

} // namespace


DeviceObjectArchive::DeviceObjectArchive(const CreateInfo& CI) noexcept(false) :
    m_pArchiveData{
        CI.MakeCopy ?
            DataBlobImpl::MakeCopy(CI.pData) :
            const_cast<IDataBlob*>(CI.pData) // Need to remove const for AddRef/Release
    }
{
    if (!m_pArchiveData)
        LOG_ERROR_AND_THROW("pData must not be null");

    Deserialize(CI);
}

const SerializedData& DeviceObjectArchive::GetDeviceSpecificData(ResourceType Type,
                                                                 const char*  Name,
                                                                 DeviceType   DevType) const noexcept
{
    auto it = m_NamedResources.find(NamedResourceKey{Type, Name});
    if (it == m_NamedResources.end())
    {
        LOG_ERROR_MESSAGE("Resource '", Name, "' is not present in the archive");
        static const SerializedData NullData;
        return NullData;
    }
    VERIFY_EXPR(SafeStrEqual(Name, it->first.GetName()));
    return it->second.DeviceSpecific[static_cast<size_t>(DevType)];
}

std::string DeviceObjectArchive::ToString() const
{
    std::stringstream Output;
    Output << "Archive contents:\n";

    constexpr char SeparatorLine[] = "------------------\n";
    constexpr char Ident1[]        = "  ";
    constexpr char Ident2[]        = "    ";

    // Print header
    {
        Output << "Header\n"
               << Ident1 << "Archive version: " << ArchiveVersion << '\n'
               << Ident1 << "Content version: " << m_ContentVersion << '\n';
    }

    constexpr char CommonDataName[] = "Common";

    auto GetNumFieldWidth = [](auto Number) //
    {
        size_t Width = 1;
        for (; Number >= 10; Number /= 10)
            ++Width;
        return Width;
    };

    // Print resources, e.g.:
    //
    //   ------------------
    //   Resource Signatures (1)
    //     Test PRS
    //       Common     1015 bytes
    //       OpenGL      729 bytes
    //       Direct3D11  384 bytes
    //       Direct3D12  504 bytes
    //       Vulkan      881 bytes
    {
        std::array<std::vector<std::reference_wrapper<const decltype(m_NamedResources)::value_type>>, static_cast<size_t>(ResourceType::Count)> ResourcesByType;
        for (const auto& it : m_NamedResources)
        {
            ResourcesByType[static_cast<size_t>(it.first.GetType())].emplace_back(it);
        }

        for (const auto& Resources : ResourcesByType)
        {
            if (Resources.empty())
                continue;

            const auto ResType = Resources.front().get().first.GetType();
            Output << SeparatorLine
                   << ResourceTypeToString(ResType) << " (" << Resources.size() << ")\n";
            // ------------------
            // Resource Signatures (1)

            for (const auto& res_ref : Resources)
            {
                const auto& it = res_ref.get();
                Output << Ident1 << it.first.GetName() << '\n';
                // ..Test PRS

                const auto& Res = it.second;

                auto   MaxSize       = Res.Common.Size();
                size_t MaxDevNameLen = strlen(CommonDataName);
                for (Uint32 i = 0; i < Res.DeviceSpecific.size(); ++i)
                {
                    const auto DevDataSize = Res.DeviceSpecific[i].Size();

                    MaxSize = std::max(MaxSize, DevDataSize);
                    if (DevDataSize != 0)
                        MaxDevNameLen = std::max(MaxDevNameLen, strlen(ArchiveDeviceTypeToString(i)));
                }
                const auto SizeFieldW = GetNumFieldWidth(MaxSize);

                Output << Ident2 << std::setw(static_cast<int>(MaxDevNameLen)) << std::left << CommonDataName << ' '
                       << std::setw(static_cast<int>(SizeFieldW)) << std::right << Res.Common.Size() << " bytes\n";
                // ....Common     1015 bytes

                for (Uint32 i = 0; i < Res.DeviceSpecific.size(); ++i)
                {
                    const auto DevDataSize = Res.DeviceSpecific[i].Size();
                    if (DevDataSize > 0)
                    {
                        Output << Ident2 << std::setw(static_cast<int>(MaxDevNameLen)) << std::left << ArchiveDeviceTypeToString(i) << ' '
                               << std::setw(static_cast<int>(SizeFieldW)) << std::right << DevDataSize << " bytes\n";
                        // ....OpenGL      729 bytes
                    }
                }
            }
        }
    }

    // Print shaders, e.g.
    //
    //   ------------------
    //   Shaders
    //     OpenGL(2)
    //       [0] 'Test VS' 4020 bytes
    //       [1] 'Test PS' 4020 bytes
    //     Vulkan(2)
    //       [0] 'Test VS' 8364 bytes
    //       [1] 'Test PS' 7380 bytes
    {
        bool HasShaders = false;
        for (const auto& Shaders : m_DeviceShaders)
        {
            if (!Shaders.empty())
                HasShaders = true;
        }

        if (HasShaders)
        {
            Output << SeparatorLine
                   << "Compiled Shaders\n";
            // ------------------
            // Compiled Shaders

            for (Uint32 dev = 0; dev < m_DeviceShaders.size(); ++dev)
            {
                const auto& Shaders = m_DeviceShaders[dev];
                if (Shaders.empty())
                    continue;
                Output << Ident1 << ArchiveDeviceTypeToString(dev) << '(' << Shaders.size() << ")\n";
                // ..OpenGL(2)

                std::vector<std::string> ShaderNames;
                ShaderNames.reserve(Shaders.size());

                size_t MaxSize    = 0;
                size_t MaxNameLen = 0;
                for (const auto& ShaderData : Shaders)
                {
                    MaxSize = std::max(MaxSize, ShaderData.Size());

                    ShaderCreateInfo                 ShaderCI;
                    Serializer<SerializerMode::Read> ShaderSer{ShaderData};
                    if (ShaderSerializer<SerializerMode::Read>::SerializeCI(ShaderSer, ShaderCI))
                        ShaderNames.emplace_back(std::string{'\''} + ShaderCI.Desc.Name + '\'');
                    else
                        ShaderNames.emplace_back("<Deserialization error>");
                    MaxNameLen = std::max(MaxNameLen, ShaderNames.back().size());
                }

                const auto IdxFieldW  = GetNumFieldWidth(Shaders.size());
                const auto SizeFieldW = GetNumFieldWidth(MaxSize);
                for (Uint32 idx = 0; idx < Shaders.size(); ++idx)
                {
                    Output << Ident2 << '[' << std::setw(static_cast<int>(IdxFieldW)) << std::right << idx << "] "
                           << std::setw(static_cast<int>(MaxNameLen)) << std::left << ShaderNames[idx] << ' '
                           << std::setw(static_cast<int>(SizeFieldW)) << std::right << Shaders[idx].Size() << " bytes\n";
                    // ....[0] 'Test VS' 4020 bytes
                }
            }
        }
    }

    return Output.str();
}


void DeviceObjectArchive::RemoveDeviceData(DeviceType Dev) noexcept(false)
{
    for (auto& res_it : m_NamedResources)
        res_it.second.DeviceSpecific[static_cast<size_t>(Dev)] = {};

    m_DeviceShaders[static_cast<size_t>(Dev)].clear();
}

void DeviceObjectArchive::AppendDeviceData(const DeviceObjectArchive& Src, DeviceType Dev) noexcept(false)
{
    auto& Allocator = GetRawAllocator();
    for (auto& dst_res_it : m_NamedResources)
    {
        auto& DstData = dst_res_it.second.DeviceSpecific[static_cast<size_t>(Dev)];
        // Clear dst device data to make sure we don't have invalid shader indices
        DstData = {};

        auto src_res_it = Src.m_NamedResources.find(dst_res_it.first);
        if (src_res_it == Src.m_NamedResources.end())
            continue;

        const auto& SrcData{src_res_it->second.DeviceSpecific[static_cast<size_t>(Dev)]};
        // Always copy src data even if it is empty
        DstData = SrcData.MakeCopy(Allocator);
    }

    // Copy all shaders to make sure PSO shader indices are correct
    const auto& SrcShaders = Src.m_DeviceShaders[static_cast<size_t>(Dev)];
    auto&       DstShaders = m_DeviceShaders[static_cast<size_t>(Dev)];
    DstShaders.clear();
    for (const auto& SrcShader : SrcShaders)
        DstShaders.emplace_back(SrcShader.MakeCopy(Allocator));
}

void DeviceObjectArchive::Merge(const DeviceObjectArchive& Src) noexcept(false)
{
    if (m_ContentVersion != Src.m_ContentVersion)
        LOG_WARNING_MESSAGE("Merging archives with different content versions (", m_ContentVersion, " and ", Src.m_ContentVersion, ").");

    static_assert(static_cast<size_t>(ResourceType::Count) == 8, "Did you add a new resource type? You may need to handle it here.");

    auto&                  Allocator = GetRawAllocator();
    DynamicLinearAllocator DynAllocator{Allocator, 512};

    // Copy shaders
    std::array<Uint32, static_cast<size_t>(DeviceType::Count)> ShaderBaseIndices{};
    for (size_t i = 0; i < m_DeviceShaders.size(); ++i)
    {
        const auto& SrcShaders = Src.m_DeviceShaders[i];
        auto&       DstShaders = m_DeviceShaders[i];
        ShaderBaseIndices[i]   = static_cast<Uint32>(DstShaders.size());
        if (SrcShaders.empty())
            continue;
        DstShaders.reserve(DstShaders.size() + SrcShaders.size());
        for (const auto& SrcShader : SrcShaders)
            DstShaders.emplace_back(SrcShader.MakeCopy(Allocator));
    }

    // Copy named resources
    for (auto& src_res_it : Src.m_NamedResources)
    {
        const auto  ResType = src_res_it.first.GetType();
        const auto* ResName = src_res_it.first.GetName();

        auto it_inserted = m_NamedResources.emplace(NamedResourceKey{ResType, ResName, /*CopyName = */ true}, src_res_it.second.MakeCopy(Allocator));
        if (!it_inserted.second)
        {
            // Silently skip duplicate resources
            if (it_inserted.first->second != src_res_it.second)
                LOG_WARNING_MESSAGE("Failed to copy resource '", ResName, "': resource with the same name already exists.");

            continue;
        }

        const auto IsStandaloneShader = (ResType == ResourceType::StandaloneShader);
        const auto IsPipeline =
            (ResType == ResourceType::GraphicsPipeline ||
             ResType == ResourceType::ComputePipeline ||
             ResType == ResourceType::RayTracingPipeline ||
             ResType == ResourceType::TilePipeline);

        // Update shader indices
        if (IsStandaloneShader || IsPipeline)
        {
            for (size_t i = 0; i < static_cast<size_t>(DeviceType::Count); ++i)
            {
                const auto BaseIdx = ShaderBaseIndices[i];

                auto& DeviceData = it_inserted.first->second.DeviceSpecific[i];
                if (!DeviceData)
                    continue;

                if (IsStandaloneShader)
                {
                    // For shaders, device-specific data is the serialized shader bytecode index
                    Uint32 ShaderIndex = 0;
                    {
                        Serializer<SerializerMode::Read> Ser{DeviceData};
                        if (!Ser(ShaderIndex))
                            LOG_ERROR_AND_THROW("Failed to deserialize standalone shader index. Archive file may be corrupted or invalid.");
                        VERIFY(Ser.IsEnded(), "No other data besides the shader index is expected");
                    }

                    ShaderIndex += BaseIdx;

                    {
                        Serializer<SerializerMode::Write> Ser{DeviceData};
                        Ser(ShaderIndex);
                        VERIFY_EXPR(Ser.IsEnded());
                    }
                }
                else if (IsPipeline)
                {
                    // For pipelines, device-specific data is the shader index array
                    ShaderIndexArray ShaderIndices;
                    {
                        Serializer<SerializerMode::Read> Ser{DeviceData};
                        if (!PSOSerializer<SerializerMode::Read>::SerializeShaderIndices(Ser, ShaderIndices, &DynAllocator))
                            LOG_ERROR_AND_THROW("Failed to deserialize PSO shader indices. Archive file may be corrupted or invalid.");
                        VERIFY(Ser.IsEnded(), "No other data besides shader indices is expected");
                    }

                    std::vector<Uint32> NewIndices{ShaderIndices.pIndices, ShaderIndices.pIndices + ShaderIndices.Count};
                    for (auto& Idx : NewIndices)
                        Idx += BaseIdx;

                    {
                        Serializer<SerializerMode::Write> Ser{DeviceData};
                        PSOSerializer<SerializerMode::Write>::SerializeShaderIndices(Ser, ShaderIndexArray{NewIndices.data(), ShaderIndices.Count}, nullptr);
                        VERIFY_EXPR(Ser.IsEnded());
                    }
                }
                else
                {
                    UNEXPECTED("Unexpected resource type");
                }
            }
        }
    }
}

void DeviceObjectArchive::Serialize(IFileStream* pStream) const
{
    DEV_CHECK_ERR(pStream != nullptr, "File stream must not be null");
    RefCntAutoPtr<IDataBlob> pDataBlob;
    Serialize(&pDataBlob);
    VERIFY_EXPR(pDataBlob);
    pStream->Write(pDataBlob->GetConstDataPtr(), pDataBlob->GetSize());
}

} // namespace Diligent
