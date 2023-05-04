// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

// TODO(diligent): Remove this include
#include "Urho3D/Graphics/GraphicsDefs.h"

#include <Diligent/Graphics/GraphicsEngine/interface/InputLayout.h>

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

struct VertexShaderAttribute
{
    VertexElementSemantic semantic_{};
    unsigned semanticIndex_{};
    unsigned inputIndex_{};
};

/// Description of vertex shader attributes.
using VertexShaderAttributeVector = ea::fixed_vector<VertexShaderAttribute, Diligent::MAX_LAYOUT_ELEMENTS>;

} // namespace Urho3D
