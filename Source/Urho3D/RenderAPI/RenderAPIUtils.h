// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>

namespace Urho3D
{

URHO3D_API ea::optional<VertexShaderAttribute> ParseVertexAttribute(ea::string_view name);
URHO3D_API const ea::string& ToShaderInputName(VertexElementSemantic semantic);

} // namespace Urho3D
