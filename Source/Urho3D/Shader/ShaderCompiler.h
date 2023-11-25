// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/ByteVector.h"
#include "Urho3D/Graphics/GraphicsDefs.h"

#include <EASTL/string.h>
#include <EASTL/string_view.h>

namespace Urho3D
{

bool CompileHLSLToBinary(ByteVector& bytecode, ea::string& compilerOutput, ea::string_view sourceCode, ShaderType type);

} // namespace Urho3D
