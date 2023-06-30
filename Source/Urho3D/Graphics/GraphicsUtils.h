// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Graphics/Geometry.h"
#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/Graphics/IndexBuffer.h"
#include "Urho3D/Graphics/Texture.h"
#include "Urho3D/Graphics/VertexBuffer.h"
#include "Urho3D/RenderAPI/DrawCommandQueue.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

namespace Urho3D
{

class DrawCommandQueue;

using VertexBufferArray = ea::array<VertexBuffer*, MaxVertexStreams>;

URHO3D_API void InitializeInputLayout(InputLayoutDesc& desc, const VertexBufferArray& vertexBuffers);
URHO3D_API void InitializeInputLayoutAndPrimitiveType(
    GraphicsPipelineStateDesc& desc, const Geometry* geometry, VertexBuffer* instancingBuffer = nullptr);

inline void SetBuffersFromGeometry(
    DrawCommandQueue& drawQueue, const Geometry* geometry, VertexBuffer* instancingBuffer = nullptr)
{
    const auto& vertexBuffers = geometry->GetVertexBuffers();
    IndexBuffer* indexBuffer = geometry->GetIndexBuffer();

    if (vertexBuffers.size() + (instancingBuffer ? 1 : 0) > MaxVertexStreams)
    {
        URHO3D_ASSERT(false, "Too many vertex buffers");
        return;
    }

    RawVertexBufferArray mergedVertexBuffers{};
    for (unsigned i = 0; i < vertexBuffers.size(); ++i)
        mergedVertexBuffers[i] = vertexBuffers[i];
    if (instancingBuffer)
        mergedVertexBuffers[vertexBuffers.size()] = instancingBuffer;

    drawQueue.SetVertexBuffers(mergedVertexBuffers);
    drawQueue.SetIndexBuffer(indexBuffer);
}

inline void AddShaderResource(DrawCommandQueue& drawQueue, StringHash name, Texture* texture)
{
    if (!texture)
    {
        URHO3D_ASSERTLOG(false, "Trying to add null texture to DrawCommandQueue");
        return;
    }

    drawQueue.AddShaderResource(name, texture, texture->GetBackupTexture());
}

inline void AddNullableShaderResource(
    DrawCommandQueue& drawQueue, StringHash name, TextureType type, Texture* texture)
{
    drawQueue.AddNullableShaderResource(name, type, texture, texture ? texture->GetBackupTexture() : nullptr);
}

} // namespace Urho3D
