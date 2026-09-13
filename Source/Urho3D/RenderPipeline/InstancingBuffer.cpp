// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../IO/Log.h"
#include "../RenderPipeline/InstancingBuffer.h"

#include "../DebugNew.h"

namespace Urho3D
{

InstancingBuffer::InstancingBuffer(Context* context)
    : Object(context)
{
}

void InstancingBuffer::SetSettings(const InstancingBufferSettings& settings)
{
    if (settings_ != settings)
    {
        settings_ = settings;
        Initialize();
    }
}

void InstancingBuffer::Begin()
{
    if (vertexBuffer_)
        vertexBuffer_->Discard();
}

void InstancingBuffer::End()
{
    if (vertexBuffer_)
        vertexBuffer_->Commit();
}

void InstancingBuffer::Initialize()
{
    vertexBuffer_ = nullptr;

    if (settings_.enableInstancing_)
    {
        ea::vector<VertexElement> vertexElements;
        for (unsigned i = 0; i < settings_.numInstancingTexCoords_; ++i)
        {
            const unsigned index = settings_.firstInstancingTexCoord_ + i;
            vertexElements.push_back(VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, index, settings_.stepRate_));
        }

        vertexBuffer_ = MakeShared<DynamicVertexBuffer>(context_);
        vertexBuffer_->SetDebugName("InstancingBuffer");
        vertexBuffer_->Initialize(128, vertexElements);
    }
}

}
