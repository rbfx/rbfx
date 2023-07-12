//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
