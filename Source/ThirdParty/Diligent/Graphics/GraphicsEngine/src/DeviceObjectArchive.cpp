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
        return Ser(Header.MagicNumber, Header.Version, Header.APIVersion, Header.GitHash);
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

DeviceObjectArchive::DeviceObjectArchive() noexcept
{
}

void DeviceObjectArchive::Deserialize(const void* pData, size_t Size) noexcept(false)
{
    Serializer<SerializerMode::Read>        Reader{SerializedData{const_cast<void*>(pData), Size}};
    ArchiveSerializer<SerializerMode::Read> ArchiveReader{Reader};

    ArchiveHeader Header;
    if (!ArchiveReader.SerializeHeader(Header))
        LOG_ERROR_AND_THROW("Failed to read device object archive header.");

    if (Header.MagicNumber != HeaderMagicNumber)
        LOG_ERROR_AND_THROW("Invalid device object archive header.");

    if (Header.Version != ArchiveVersion)
        LOG_ERROR_AND_THROW("Unsupported device object archive version: ", Header.Version, ". Expected version: ", Uint32{ArchiveVersion});

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
    DEV_CHECK_ERR(ppDataBlob != nullptr, "Pointer to the data blob object must not be null");
    DEV_CHECK_ERR(*ppDataBlob == nullptr, "Data blob object must be null");

    auto SerializeThis = [this](auto& Ser) {
        constexpr auto SerMode    = std::remove_reference<decltype(Ser)>::type::GetMode();
        const auto     ArchiveSer = ArchiveSerializer<SerMode>{Ser};

        auto res = ArchiveSer.SerializeHeader(ArchiveHeader{});
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
    static_assert(static_cast<size_t>(ResourceType::Count) == 7, "Please handle the new chunk type below");
    switch (Type)
    {
        // clang-format off
        case ResourceType::Undefined:          return "Undefined";
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


DeviceObjectArchive::DeviceObjectArchive(const IDataBlob* pData, bool MakeCopy) noexcept(false) :
    m_pArchiveData{
        MakeCopy ?
            DataBlobImpl::MakeCopy(pData) :
            const_cast<IDataBlob*>(pData) // Need to remove const for AddRef/Release
    }
{
    if (!m_pArchiveData)
        LOG_ERROR_AND_THROW("pData must not be null");

    Deserialize(pData->GetConstDataPtr(), StaticCast<size_t>(pData->GetSize()));
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
               << Ident1 << "version: " << ArchiveVersion << '\n';
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
                   << "Shaders\n";
            // ------------------
            // Shaders

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
        dst_res_it.second.DeviceSpecific[static_cast<size_t>(Dev)] = SrcData.MakeCopy(Allocator);
    }

    // Copy all shaders to make sure PSO shader indices are correct
    const auto& SrcShaders = Src.m_DeviceShaders[static_cast<size_t>(Dev)];
    auto&       DstShaders = m_DeviceShaders[static_cast<size_t>(Dev)];
    DstShaders.clear();
    for (const auto& SrcShader : SrcShaders)
        DstShaders.emplace_back(SrcShader.MakeCopy(Allocator));
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
