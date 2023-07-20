// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#include "Urho3D/Core/Macros.h"
#include "Urho3D/Core/ProcessUtils.h"
#include "Urho3D/Core/StringUtils.h"
#include "Urho3D/IO/ArchiveSerialization.h"

#include <Diligent/Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp>

namespace Urho3D
{

namespace
{

// TODO: Move to common place
struct OptionalSerializer
{
    template <class T> void operator()(Archive& archive, const char* name, T& value) const
    {
        if (!value)
        {
            URHO3D_ASSERT(archive.IsInput());
            value.emplace();
        }
        SerializeValue(archive, name, *value);
    }
};

void SerializeQueryTypes(Archive& archive, ea::optional<unsigned> (&sizes)[Diligent::QUERY_TYPE_NUM_TYPES])
{
    // clang-format off
    SerializeStrictlyOptionalValue(archive, "queryPoolSize_occlusion", sizes[Diligent::QUERY_TYPE_OCCLUSION], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "queryPoolSize_binaryOcclusion", sizes[Diligent::QUERY_TYPE_BINARY_OCCLUSION], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "queryPoolSize_timestamp", sizes[Diligent::QUERY_TYPE_TIMESTAMP], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "queryPoolSize_pipelineStatistics", sizes[Diligent::QUERY_TYPE_PIPELINE_STATISTICS], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "queryPoolSize_duration", sizes[Diligent::QUERY_TYPE_DURATION], ea::nullopt, OptionalSerializer{});
    // clang-format on
}

const ea::array<ea::string, MAX_VERTEX_ELEMENT_SEMANTICS> shaderInputsNames = {
    "iPos", // SEM_POSITION
    "iNormal", // SEM_NORMAL
    "iBinormal", // SEM_BINORMAL
    "iTangent", // SEM_TANGENT
    "iTexCoord", // SEM_TEXCOORD
    "iColor", // SEM_COLOR
    "iBlendWeights", // SEM_BLENDWEIGHTS
    "iBlendIndices", // SEM_BLENDINDICES
    "", // SEM_OBJECTINDEX
};

const auto textureFormatMapSRGB = []
{
    ea::unordered_map<TextureFormat, TextureFormat> toSrgb = {
        {TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFormat::TEX_FORMAT_RGBA8_UNORM_SRGB},
        {TextureFormat::TEX_FORMAT_BGRA8_UNORM, TextureFormat::TEX_FORMAT_BGRA8_UNORM_SRGB},
        {TextureFormat::TEX_FORMAT_BGRX8_UNORM, TextureFormat::TEX_FORMAT_BGRX8_UNORM_SRGB},
        {TextureFormat::TEX_FORMAT_BC1_UNORM, TextureFormat::TEX_FORMAT_BC1_UNORM_SRGB},
        {TextureFormat::TEX_FORMAT_BC2_UNORM, TextureFormat::TEX_FORMAT_BC2_UNORM_SRGB},
        {TextureFormat::TEX_FORMAT_BC3_UNORM, TextureFormat::TEX_FORMAT_BC3_UNORM_SRGB},
        {TextureFormat::TEX_FORMAT_BC7_UNORM, TextureFormat::TEX_FORMAT_BC7_UNORM_SRGB},
    };

    ea::unordered_map<TextureFormat, TextureFormat> fromSrgb;
    for (const auto& [linear, srgb] : toSrgb)
        fromSrgb[srgb] = linear;
    return ea::make_pair(toSrgb, fromSrgb);
}();

} // namespace

bool IsOpenGLESBackend(RenderBackend backend)
{
    const PlatformId platform = GetPlatform();
    const bool isMobilePlatform = platform == PlatformId::Android || platform == PlatformId::iOS
        || platform == PlatformId::tvOS || platform == PlatformId::Web || platform == PlatformId::RaspberryPi;
    return backend == RenderBackend::OpenGL && isMobilePlatform;
}

bool IsMetalBackend(RenderBackend backend)
{
    const PlatformId platform = GetPlatform();
    const bool isApplePlatform =
        platform == PlatformId::iOS || platform == PlatformId::tvOS || platform == PlatformId::MacOS;
    return backend == RenderBackend::Vulkan && isApplePlatform;
}

const ea::string& ToString(RenderBackend backend)
{
    static const EnumArray<ea::string, RenderBackend> backendNames{{
        "D3D11",
        "D3D12",
        "OpenGL",
        "Vulkan",
    }};
    return backendNames[backend];
}

const ea::string& ToString(ShaderType type)
{
    static const ea::array<ea::string, MAX_SHADER_TYPES> shaderTypeNames = {
        "Vertex",
        "Pixel",
        "Geometry",
        "Hull",
        "Domain",
        "Compute",
    };
    return shaderTypeNames[type];
}

ea::optional<VertexShaderAttribute> ParseVertexAttribute(ea::string_view name)
{
    for (unsigned index = 0; index < URHO3D_ARRAYSIZE(shaderInputsNames); ++index)
    {
        const ea::string& semanticName = shaderInputsNames[index];
        VertexElementSemantic semanticValue = static_cast<VertexElementSemantic>(index);

        const unsigned semanticPos = name.find(semanticName);
        if (semanticPos == ea::string::npos)
            continue;

        const unsigned semanticIndexPos = semanticPos + semanticName.length();
        const unsigned semanticIndex = ToUInt(&name[semanticIndexPos]);
        return VertexShaderAttribute{semanticValue, semanticIndex};
    }
    return ea::nullopt;
}

const ea::string& ToShaderInputName(VertexElementSemantic semantic)
{
    return shaderInputsNames[semantic];
}

Diligent::SHADER_TYPE ToInternalShaderType(ShaderType type)
{
    static const ea::array<Diligent::SHADER_TYPE, MAX_SHADER_TYPES> shaderTypes = {
        Diligent::SHADER_TYPE_VERTEX,
        Diligent::SHADER_TYPE_PIXEL,
        Diligent::SHADER_TYPE_GEOMETRY,
        Diligent::SHADER_TYPE_HULL,
        Diligent::SHADER_TYPE_DOMAIN,
        Diligent::SHADER_TYPE_COMPUTE,
    };
    return shaderTypes[type];
}

unsigned GetMipLevelCount(const IntVector3& size)
{
    unsigned maxLevels = 1;
    IntVector3 dim = size;
    while (dim.x_ > 1 || dim.y_ > 1 || dim.z_ > 1)
    {
        ++maxLevels;
        dim.x_ = dim.x_ > 1 ? (dim.x_ >> 1u) : 1;
        dim.y_ = dim.y_ > 1 ? (dim.y_ >> 1u) : 1;
        dim.z_ = dim.z_ > 1 ? (dim.z_ >> 1u) : 1;
    }
    return maxLevels;
}

IntVector3 GetMipLevelSize(const IntVector3& size, unsigned level)
{
    IntVector3 dim = size;
    dim.x_ >>= level;
    dim.y_ >>= level;
    dim.z_ >>= level;
    return VectorMax(dim, IntVector3::ONE);
}

bool IsTextureFormatSRGB(TextureFormat format)
{
    return textureFormatMapSRGB.second.contains(format);
}

TextureFormat SetTextureFormatSRGB(TextureFormat format, bool sRGB)
{
    const auto& map = sRGB ? textureFormatMapSRGB.first : textureFormatMapSRGB.second;
    const auto iter = map.find(format);
    return iter != map.end() ? iter->second : format;
}

bool IsDepthTextureFormat(TextureFormat format)
{
    const Diligent::COMPONENT_TYPE componentType = Diligent::GetTextureFormatAttribs(format).ComponentType;
    return componentType == Diligent::COMPONENT_TYPE_DEPTH || componentType == Diligent::COMPONENT_TYPE_DEPTH_STENCIL;
}

bool IsStencilTextureFormat(TextureFormat format)
{
    return Diligent::GetTextureFormatAttribs(format).ComponentType == Diligent::COMPONENT_TYPE_DEPTH_STENCIL;
}

RenderBackend SelectRenderBackend(ea::optional<RenderBackend> requestedBackend)
{
    static const EnumArray<RenderBackend, PlatformId> defaultBackend{{
        RenderBackend::D3D11, // Windows
        RenderBackend::D3D11, // UniversalWindowsPlatform
        RenderBackend::OpenGL, // Linux
        RenderBackend::OpenGL, // Android
        RenderBackend::OpenGL, // RaspberryPi
        RenderBackend::OpenGL, // MacOS
        RenderBackend::OpenGL, // iOS
        RenderBackend::OpenGL, // tvOS
        RenderBackend::OpenGL, // Web
    }};

    ea::vector<RenderBackend> supportedBackends;
#if D3D11_SUPPORTED
    supportedBackends.push_back(RenderBackend::D3D11);
#endif
#if D3D12_SUPPORTED
    supportedBackends.push_back(RenderBackend::D3D12);
#endif
#if GL_SUPPORTED || GLES_SUPPORTED
    supportedBackends.push_back(RenderBackend::OpenGL);
#endif
#if VULKAN_SUPPORTED
    supportedBackends.push_back(RenderBackend::Vulkan);
#endif

    URHO3D_ASSERT(!supportedBackends.empty(), "Unexpected engine configuration");\

    const PlatformId platform = GetPlatform();
    const RenderBackend backend = requestedBackend.value_or(defaultBackend[platform]);
    if (supportedBackends.contains(backend))
        return backend;

    return supportedBackends.front();
}

ShaderTranslationPolicy SelectShaderTranslationPolicy(
    RenderBackend backend, ea::optional<ShaderTranslationPolicy> requestedPolicy)
{
    static const EnumArray<ShaderTranslationPolicy, RenderBackend> defaultPolicy{{
        ShaderTranslationPolicy::Translate, // D3D11
        ShaderTranslationPolicy::Translate, // D3D12
        ShaderTranslationPolicy::Verbatim, // OpenGL
        ShaderTranslationPolicy::Optimize // Vulkan
    }};

    ea::vector<ShaderTranslationPolicy> supportedPolicies;
    if (backend == RenderBackend::OpenGL)
        supportedPolicies.push_back(ShaderTranslationPolicy::Verbatim);
#ifdef URHO3D_SHADER_TRANSLATOR
    supportedPolicies.push_back(ShaderTranslationPolicy::Translate);
#endif
#ifdef URHO3D_SHADER_OPTIMIZER
    supportedPolicies.push_back(ShaderTranslationPolicy::Optimize);
#endif

    URHO3D_ASSERT(!supportedPolicies.empty(), "Unexpected engine configuration");

    const ShaderTranslationPolicy policy = requestedPolicy.value_or(defaultPolicy[backend]);
    if (supportedPolicies.contains(policy))
        return policy;

    return supportedPolicies.front();
}

void SerializeValue(Archive& archive, const char* name, Diligent::VulkanDescriptorPoolSize& value)
{
    auto block = archive.OpenUnorderedBlock(name);
    SerializeValueAsType<unsigned>(archive, "maxDescriptorSets", value.MaxDescriptorSets);
    SerializeValueAsType<unsigned>(archive, "numSeparateSamplerDescriptors", value.NumSeparateSamplerDescriptors);
    SerializeValueAsType<unsigned>(archive, "numCombinedSamplerDescriptors", value.NumCombinedSamplerDescriptors);
    SerializeValueAsType<unsigned>(archive, "numSampledImageDescriptors", value.NumSampledImageDescriptors);
    SerializeValueAsType<unsigned>(archive, "numStorageImageDescriptors", value.NumStorageImageDescriptors);
    SerializeValueAsType<unsigned>(archive, "numUniformBufferDescriptors", value.NumUniformBufferDescriptors);
    SerializeValueAsType<unsigned>(archive, "numStorageBufferDescriptors", value.NumStorageBufferDescriptors);
    SerializeValueAsType<unsigned>(archive, "numUniformTexelBufferDescriptors", value.NumUniformTexelBufferDescriptors);
    SerializeValueAsType<unsigned>(archive, "numStorageTexelBufferDescriptors", value.NumStorageTexelBufferDescriptors);
    SerializeValueAsType<unsigned>(archive, "numInputAttachmentDescriptors", value.NumInputAttachmentDescriptors);
    SerializeValueAsType<unsigned>(archive, "numAccelStructDescriptors", value.NumAccelStructDescriptors);
}

void SerializeValue(Archive& archive, const char* name, RenderDeviceSettingsVulkan& value)
{
    auto block = archive.OpenUnorderedBlock(name);
    // clang-format off
    SerializeStrictlyOptionalValue(archive, "mainDescriptorPoolSize", value.mainDescriptorPoolSize_, ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "dynamicDescriptorPoolSize", value.dynamicDescriptorPoolSize_, ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "deviceLocalMemoryPageSize", value.deviceLocalMemoryPageSize_, ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "hostVisibleMemoryPageSize", value.hostVisibleMemoryPageSize_, ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "deviceLocalMemoryReserveSize", value.deviceLocalMemoryReserveSize_, ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "hostVisibleMemoryReserveSize", value.hostVisibleMemoryReserveSize_, ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "uploadHeapPageSize", value.uploadHeapPageSize_, ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "dynamicHeapSize", value.dynamicHeapSize_, ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "dynamicHeapPageSize", value.dynamicHeapPageSize_, ea::nullopt, OptionalSerializer{});
    SerializeQueryTypes(archive, value.queryPoolSizes_);
    // clang-format on
}

void SerializeValue(Archive& archive, const char* name, RenderDeviceSettingsD3D12& value)
{
    auto block = archive.OpenUnorderedBlock(name);
    // clang-format off
    SerializeStrictlyOptionalValue(archive, "mainDescriptorPoolSize_cvb_srv_uav", value.cpuDescriptorHeapAllocationSize_[0], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "mainDescriptorPoolSize_sampler", value.cpuDescriptorHeapAllocationSize_[1], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "mainDescriptorPoolSize_rtv", value.cpuDescriptorHeapAllocationSize_[2], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "mainDescriptorPoolSize_dsv", value.cpuDescriptorHeapAllocationSize_[3], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "gpuDescriptorHeapSize_cvb_srv_uav", value.gpuDescriptorHeapSize_[0], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "gpuDescriptorHeapSize_sampler", value.gpuDescriptorHeapSize_[1], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "gpuDescriptorHeapDynamicSize_cvb_srv_uav", value.gpuDescriptorHeapDynamicSize_[0], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "gpuDescriptorHeapDynamicSize_sampler", value.gpuDescriptorHeapDynamicSize_[1], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "dynamicDescriptorAllocationChunkSize_cvb_srv_uav", value.dynamicDescriptorAllocationChunkSize_[0], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "dynamicDescriptorAllocationChunkSize_sampler", value.dynamicDescriptorAllocationChunkSize_[1], ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "dynamicHeapPageSize", value.dynamicHeapPageSize_, ea::nullopt, OptionalSerializer{});
    SerializeStrictlyOptionalValue(archive, "numDynamicHeapPagesToReserve", value.numDynamicHeapPagesToReserve_, ea::nullopt, OptionalSerializer{});
    SerializeQueryTypes(archive, value.queryPoolSizes_);
    // clang-format on
}

} // namespace Urho3D
