// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/IO/FileIdentifier.h"

#include <EASTL/string.h>

namespace Urho3D
{

URHO3D_API void LogShaderSource(const FileIdentifier& fileName, ea::string_view defines, ea::string_view source);

} // namespace Urho3D
