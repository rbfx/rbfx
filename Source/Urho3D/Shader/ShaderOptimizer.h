// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Shader/ShaderDefs.h"

#include <EASTL/string.h>

namespace Urho3D
{

URHO3D_API bool OptimizeSpirVShader(
    SpirVShader& shader, ea::string& optimizerOutput, TargetShaderLanguage targetLanguage);
}
