// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/Graphics/VertexBuffer.h"
#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"

namespace Urho3D
{

/// Instancing buffer compositor.
class URHO3D_API InstancingBuffer : public Object
{
    URHO3D_OBJECT(InstancingBuffer, Object);

public:
    /// Stride of one element in bytes.
    static const unsigned ElementStride = 4 * sizeof(float);

    explicit InstancingBuffer(Context* context);
    void SetSettings(const InstancingBufferSettings& settings);

    /// Begin buffer composition.
    void Begin();
    /// End buffer composition and commit added instances to GPU.
    void End();

    /// Return index of next added instance.
    unsigned GetNextInstanceIndex() const { return vertexBuffer_->GetVertexCount(); }

    /// Add instance to buffer. Use SetElements to fill it after.
    unsigned AddInstance()
    {
        const auto indexAndData = vertexBuffer_->AddVertices(1);
        currentInstanceData_ = indexAndData.second;
        return indexAndData.first;
    }

    /// Set one or more 4-float elements in current instance.
    void SetElements(const void* data, unsigned index, unsigned count)
    {
        memcpy(currentInstanceData_ + index * ElementStride, data, count * ElementStride);
    }

    /// Getters
    /// @{
    const InstancingBufferSettings& GetSettings() const { return settings_; }
    VertexBuffer* GetVertexBuffer() const { return vertexBuffer_ ? vertexBuffer_->GetVertexBuffer() : nullptr; }
    bool IsEnabled() const { return settings_.enableInstancing_; }
    /// @}

private:
    void Initialize();

    InstancingBufferSettings settings_;
    SharedPtr<DynamicVertexBuffer> vertexBuffer_;

    unsigned char* currentInstanceData_{};
};

}
