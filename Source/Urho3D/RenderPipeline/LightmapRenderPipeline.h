// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Object.h"
#include "../RenderPipeline/RenderBuffer.h"
#include "../RenderPipeline/RenderPipelineDefs.h"

namespace Urho3D
{

/// RenderPipeline used to render geometry buffer for lightmap baking.
class URHO3D_API LightmapRenderPipelineView
    : public Object
    , public RenderPipelineInterface
{
    URHO3D_OBJECT(LightmapRenderPipelineView, Object);

public:
    explicit LightmapRenderPipelineView(Context* context);
    ~LightmapRenderPipelineView() override;

    /// Render geometry buffer. May be called only once.
    void RenderGeometryBuffer(Viewport* viewport, int textureSize);

    /// Return geometry buffer textures
    /// @{
    RawTexture* GetPositionBuffer() const { return positionBuffer_->GetTexture(); }
    RawTexture* GetSmoothPositionBuffer() const { return smoothPositionBuffer_->GetTexture(); }
    RawTexture* GetFaceNormalBuffer() const { return faceNormalBuffer_->GetTexture(); }
    RawTexture* GetSmoothNormalBuffer() const { return smoothNormalBuffer_->GetTexture(); }
    RawTexture* GetAlbedoBuffer() const { return albedoBuffer_->GetTexture(); }
    RawTexture* GetEmissionBuffer() const { return emissionBuffer_->GetTexture(); }
    /// @}

    /// Implement RenderPipelineInterface
    /// @{
    Context* GetContext() const override { return BaseClassName::GetContext(); }
    RenderPipelineDebugger* GetDebugger() override { return nullptr; }
    bool IsLinearColorSpace() const override { return true; }
    /// @}

private:
    SharedPtr<RenderBuffer> depthBuffer_;
    SharedPtr<RenderBuffer> positionBuffer_;
    SharedPtr<RenderBuffer> smoothPositionBuffer_;
    SharedPtr<RenderBuffer> faceNormalBuffer_;
    SharedPtr<RenderBuffer> smoothNormalBuffer_;
    SharedPtr<RenderBuffer> albedoBuffer_;
    SharedPtr<RenderBuffer> emissionBuffer_;
};

}
