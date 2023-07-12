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
/// Implementation of the Diligent::DeviceObjectArchive class

#include <array>
#include <vector>
#include <unordered_map>

#include "GraphicsTypes.h"
#include "FileStream.h"

#include "HashUtils.hpp"
#include "RefCntAutoPtr.hpp"
#include "DynamicLinearAllocator.hpp"
#include "Serializer.hpp"
#include "DebugUtilities.hpp"

// Device object archive structure:
//
// | Header |  Resource Data  |  Shader Data  |
//
//     |  Resource Data  | = | Res1 | Res2 | ... | ResN |
//
//         | ResI | = | Type | Name | Common Data |  OpenGL data | D3D11 data | ...  | Metal-iOS data |
//
//     |  Shader Data  | =  |  OpenGL shaders | D3D11 shaders | ...  | Metal-iOS shaders |
//
// The header contains general information such as:
// - Magic number
// - Archive version
// - API version

// Resource data contains an array of resources. Each resource contains:
// - Type (Signature, Graphics Pipeline, Render Pass, etc.)
// - Name
// - Common data (e.g. a resource description)
// - Device-specific data (e.g. shader indices)
//
// Shader data contains an array of shaders for each device type
//
//
// For pipelines, device-specific data is the array of shader indices in the
// archive's shader array, e.g.:
//
// | PsoX | = |   Type   |   Name   |   Common Data   |   OpenGL data   |    D3D11 data   | ...
//              Graphics   "My PSO"    <Description>        {0, 1}             {1, 2}
//                                                                  ____________|  |
//                                                                 |               |
//                                                                 V               V
// | GL Shader 0 | GL Shader 1 |  ... | D3D11 Shader 0 | D3D11 Shader 1 | D3D11 Shader 2 | ...

namespace Diligent
{

/// Device object archive object.
class DeviceObjectArchive
{
public:
    enum class DeviceType : Uint32
    {
        OpenGL, // Same as GLES
        Direct3D11,
        Direct3D12,
        Vulkan,
        Metal_MacOS,
        Metal_iOS,
        Count
    };

    using TPRSNames = std::array<const char*, MAX_RESOURCE_SIGNATURES>;

    // Shader indices that identify patched PSO shaders in the archive's shader array.
    // Every PSO stores a shader index array in each device-sepcific data.
    // Note that indices may be different for different devices due to patching specifics.
    struct ShaderIndexArray
    {
        const Uint32* pIndices = nullptr;
        Uint32        Count    = 0;
    };

    // Serialized pipeline state auxiliary data.
    struct SerializedPSOAuxData
    {
        // Shaders have been serialized without the shader reflection information.
        bool NoShaderReflection = false;
    };

    // Archived resource type.
    enum class ResourceType : Uint32
    {
        Undefined = 0,
        StandaloneShader,
        ResourceSignature,
        GraphicsPipeline,
        ComputePipeline,
        RayTracingPipeline,
        TilePipeline,
        RenderPass,
        Count
    };

    static constexpr Uint32 HeaderMagicNumber = 0xDE00000A;
    static constexpr Uint32 ArchiveVersion    = 5;

    struct ArchiveHeader
    {
        ArchiveHeader() noexcept;

        Uint32      MagicNumber    = HeaderMagicNumber;
        Uint32      Version        = ArchiveVersion;
        Uint32      APIVersion     = DILIGENT_API_VERSION;
        Uint32      ContentVersion = 0;
        const char* GitHash        = nullptr;
    };

    struct ResourceData
    {
        // Device-agnostic data (e.g. description)
        SerializedData Common;

        // Device-specific data (e.g. device-specific resource signature data, PSO shader index array, etc.)
        std::array<SerializedData, static_cast<size_t>(DeviceType::Count)> DeviceSpecific;

        ResourceData MakeCopy(IMemoryAllocator& Allocator) const
        {
            ResourceData DataCopy;
            DataCopy.Common = Common.MakeCopy(Allocator);
            for (size_t i = 0; i < DeviceSpecific.size(); ++i)
                DataCopy.DeviceSpecific[i] = DeviceSpecific[i].MakeCopy(Allocator);
            return DataCopy;
        }

        bool operator==(const ResourceData& Other) const noexcept
        {
            if (Common != Other.Common)
                return false;

            for (size_t i = 0; i < DeviceSpecific.size(); ++i)
            {
                if (DeviceSpecific[i] != Other.DeviceSpecific[i])
                    return false;
            }

            return true;
        }

        bool operator!=(const ResourceData& Other) const noexcept
        {
            return !(*this == Other);
        }
    };

    struct NamedResourceKey
    {
        NamedResourceKey(ResourceType _Type,
                         const char*  _Name,
                         bool         CopyName = false) noexcept :
            Type{_Type},
            Name{_Name, CopyName}
        {}

        struct Hasher
        {
            size_t operator()(const NamedResourceKey& Key) const noexcept
            {
                return ComputeHash(static_cast<size_t>(Key.Type), Key.Name.GetHash());
            }
        };

        constexpr bool operator==(const NamedResourceKey& Key) const
        {
            return Type == Key.Type && Name == Key.Name;
        }

        const char* GetName() const noexcept
        {
            return Name.GetStr();
        }

        ResourceType GetType() const noexcept
        {
            return Type;
        }

    private:
        const ResourceType Type;
        HashMapStringKey   Name;
    };

    const IDataBlob* GetData() const
    {
        return m_pArchiveData;
    }

    Uint32 GetContentVersion() const
    {
        return m_ContentVersion;
    }

public:
    struct CreateInfo
    {
        const IDataBlob* pData          = nullptr;
        Uint32           ContentVersion = ~0u;
        bool             MakeCopy       = false;
    };
    /// Initializes a new device object archive from pData.
    explicit DeviceObjectArchive(const CreateInfo& CI) noexcept(false);

    /// Initializes an empty archive.
    explicit DeviceObjectArchive(Uint32 ContentVersion = 0) noexcept;

    void RemoveDeviceData(DeviceType Dev) noexcept(false);
    void AppendDeviceData(const DeviceObjectArchive& Src, DeviceType Dev) noexcept(false);
    void Merge(const DeviceObjectArchive& Src) noexcept(false);

    void Deserialize(const CreateInfo& CI) noexcept(false);
    void Serialize(IFileStream* pStream) const;
    void Serialize(IDataBlob** ppDataBlob) const;

    std::string ToString() const;

    template <typename ReourceDataType>
    bool LoadResourceCommonData(ResourceType     Type,
                                const char*      Name,
                                ReourceDataType& ResData) const
    {
        auto it = m_NamedResources.find(NamedResourceKey{Type, Name});
        if (it == m_NamedResources.end())
        {
            LOG_ERROR_MESSAGE("Resource '", Name, "' is not present in the archive");
            return false;
        }
        VERIFY_EXPR(SafeStrEqual(Name, it->first.GetName()));
        // Use string copy from the map
        Name = it->first.GetName();

        Serializer<SerializerMode::Read> Ser{it->second.Common};

        auto Res = ResData.Deserialize(Name, Ser);
        VERIFY_EXPR(Ser.IsEnded());
        return Res;
    }

    const SerializedData& GetDeviceSpecificData(ResourceType Type,
                                                const char*  Name,
                                                DeviceType   DevType) const noexcept;

    ResourceData& GetResourceData(ResourceType Type, const char* Name) noexcept
    {
        constexpr auto MakeCopy = true;
        return m_NamedResources[NamedResourceKey{Type, Name, MakeCopy}];
    }

    auto& GetDeviceShaders(DeviceType Type) noexcept
    {
        return m_DeviceShaders[static_cast<size_t>(Type)];
    }

    const auto& GetSerializedShader(DeviceType Type, size_t Idx) const noexcept
    {
        const auto& DeviceShaders = m_DeviceShaders[static_cast<size_t>(Type)];
        if (Idx < DeviceShaders.size())
            return DeviceShaders[Idx];

        static const SerializedData NullData;
        return NullData;
    }

    const auto& GetNamedResources() const
    {
        return m_NamedResources;
    }

private:
    // Named resources
    std::unordered_map<NamedResourceKey, ResourceData, NamedResourceKey::Hasher> m_NamedResources;

    // Shaders
    std::array<std::vector<SerializedData>, static_cast<size_t>(DeviceType::Count)> m_DeviceShaders;

    // Strong reference to the original data blob.
    // Resources will not make copies and reference this data.
    RefCntAutoPtr<IDataBlob> m_pArchiveData;

    Uint32 m_ContentVersion = 0;
};

DeviceObjectArchive::DeviceType RenderDeviceTypeToArchiveDeviceType(RENDER_DEVICE_TYPE Type);

} // namespace Diligent
