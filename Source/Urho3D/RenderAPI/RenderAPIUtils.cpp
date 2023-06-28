// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#include "Urho3D/Core/Macros.h"
#include "Urho3D/Core/ProcessUtils.h"
#include "Urho3D/Core/StringUtils.h"

namespace Urho3D
{

namespace
{

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

} // namespace Urho3D
