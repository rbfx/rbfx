// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

namespace Urho3D
{

class DrawCommandQueue;
class Geometry;
class IndexBuffer;
class VertexBuffer;

using VertexBufferArray = ea::array<VertexBuffer*, MaxVertexStreams>;

URHO3D_API void InitializeInputLayout(InputLayoutDesc& desc, const VertexBufferArray& vertexBuffers);
URHO3D_API void InitializeInputLayoutAndPrimitiveType(
    GraphicsPipelineStateDesc& desc, const Geometry* geometry, VertexBuffer* instancingBuffer = nullptr);

URHO3D_API void SetBuffersFromGeometry(
    DrawCommandQueue& drawQueue, const Geometry* geometry, VertexBuffer* instancingBuffer = nullptr);

} // namespace Urho3D
