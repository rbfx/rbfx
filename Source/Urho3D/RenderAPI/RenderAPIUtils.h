// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypes.h>

#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>

namespace Urho3D
{

URHO3D_API const ea::string& ToString(RenderBackend backend);

URHO3D_API ea::optional<VertexShaderAttribute> ParseVertexAttribute(ea::string_view name);
URHO3D_API const ea::string& ToShaderInputName(VertexElementSemantic semantic);
URHO3D_API Diligent::SHADER_TYPE ToInternalShaderType(ShaderType type);

} // namespace Urho3D
