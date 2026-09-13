// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderAPI/RenderAPIDefs.h"
#include "Urho3D/Shader/ShaderDefs.h"

#include <EASTL/optional.h>

namespace Urho3D
{

URHO3D_API ea::optional<ea::pair<unsigned, unsigned>> FindVersionTag(ea::string_view shaderCode);

/// Shader translated to the target language.
struct TargetShader
{
    TargetShaderLanguage language_;
    ea::string sourceCode_;
    ea::string compilerOutput_;

    const bool IsValid() const { return !sourceCode_.empty(); }
    operator bool() const { return IsValid(); }
};

/// Convert universal GLSL shader to SPIR-V.
URHO3D_API void ParseUniversalShader(SpirVShader& output, ShaderType shaderType, ea::string_view sourceCode,
    const ShaderDefineArray& shaderDefines, TargetShaderLanguage targetLanguage);

/// Convert SPIR-V shader to target shader language.
URHO3D_API void TranslateSpirVShader(
    TargetShader& output, const SpirVShader& shader, TargetShaderLanguage targetLanguage);

/// Extract vertex attributes from SPIR-V.
URHO3D_API VertexShaderAttributeVector GetVertexAttributesFromSpirV(const SpirVShader& shader);

}
