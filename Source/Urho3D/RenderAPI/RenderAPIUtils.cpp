// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#include "Urho3D/Core/Macros.h"
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

} // namespace Urho3D
