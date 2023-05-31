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

const ea::string shaderInputsNames[] = {
    "iPos", // SEM_POSITION
    "iNormal", // SEM_NORMAL
    "iBinormal", // SEM_BINORMAL
    "iTangent", // SEM_TANGENT
    "iTexCoord", // SEM_TEXCOORD
    "iColor", // SEM_COLOR
    "iBlendWeights", // SEM_BLENDWEIGHTS
    "iBlendIndices", // SEM_BLENDINDICES
};

}

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
    static const StringVector backendNames = {
        "D3D11",
        "D3D12",
        "OpenGL",
        "Vulkan",
    };
    return backendNames[static_cast<unsigned>(backend)];
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
    if (semantic < URHO3D_ARRAYSIZE(shaderInputsNames))
        return shaderInputsNames[semantic];
    else
        return EMPTY_STRING;
}

Diligent::SHADER_TYPE ToInternalShaderType(ShaderType type)
{
    static Diligent::SHADER_TYPE shaderTypes[] = {
        Diligent::SHADER_TYPE_VERTEX,
        Diligent::SHADER_TYPE_PIXEL,
        Diligent::SHADER_TYPE_GEOMETRY,
        Diligent::SHADER_TYPE_HULL,
        Diligent::SHADER_TYPE_DOMAIN,
        Diligent::SHADER_TYPE_COMPUTE,
    };
    return type < MAX_SHADER_TYPES ? shaderTypes[type] : Diligent::SHADER_TYPE_UNKNOWN;
}

} // namespace Urho3D
