// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/GraphicsUtils.h"

#include "Urho3D/IO/Log.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

VertexBufferArray ToVertexBufferArray(const Geometry* geometry, VertexBuffer* instancingBuffer)
{
    VertexBufferArray vertexBuffers{};
    const unsigned numVertexBuffers = geometry->GetNumVertexBuffers() + (instancingBuffer ? 1 : 0);
    if (numVertexBuffers > MaxVertexStreams)
    {
        URHO3D_ASSERTLOG(false, "Too many vertex buffers: PipelineState cannot handle more than {}", MaxVertexStreams);
        return {};
    }

    ea::copy_n(geometry->GetVertexBuffers().begin(), numVertexBuffers, vertexBuffers.begin());
    if (instancingBuffer)
        vertexBuffers[numVertexBuffers - 1] = instancingBuffer;
    return vertexBuffers;
}

}

void InitializeInputLayout(InputLayoutDesc& desc, const VertexBufferArray& vertexBuffers)
{
    desc.size_ = 0;

    for (unsigned bufferIndex = 0; bufferIndex < vertexBuffers.size(); ++bufferIndex)
    {
        VertexBuffer* vertexBuffer = vertexBuffers[bufferIndex];
        if (!vertexBuffer)
            continue;

        const auto& elements = vertexBuffer->GetElements();
        const unsigned numElements = ea::min<unsigned>(elements.size(), MaxNumVertexElements - desc.size_);
        if (numElements > 0)
        {
            for (unsigned i = 0; i < numElements; ++i)
            {
                const VertexElement& vertexElement = elements[i];
                InputLayoutElementDesc& layoutElement = desc.elements_[desc.size_ + i];

                layoutElement.bufferIndex_ = bufferIndex;
                layoutElement.bufferStride_ = vertexBuffer->GetVertexSize();
                layoutElement.elementOffset_ = vertexElement.offset_;
                layoutElement.instanceStepRate_ = vertexElement.stepRate_;

                layoutElement.elementType_ = vertexElement.type_;
                layoutElement.elementSemantic_ = vertexElement.semantic_;
                layoutElement.elementSemanticIndex_ = vertexElement.index_;
            }
            desc.size_ += numElements;
        }
        if (elements.size() != numElements)
        {
            URHO3D_LOGWARNING("Too many vertex elements: PipelineState cannot handle more than {}", MaxNumVertexElements);
        }
    }
}

void InitializeInputLayoutAndPrimitiveType(
    GraphicsPipelineStateDesc& desc, const Geometry* geometry, VertexBuffer* instancingBuffer)
{
    const VertexBufferArray vertexBuffers = ToVertexBufferArray(geometry, instancingBuffer);
    InitializeInputLayout(desc.inputLayout_, vertexBuffers);
    desc.primitiveType_ = geometry->GetPrimitiveType();
}

} // namespace Urho3D
