// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Object.h"
#include "../Graphics/GraphicsDefs.h"
#include "../RenderPipeline/RenderPipelineDefs.h"
#include "../RenderPipeline/PipelineBatchSortKey.h"

#include <EASTL/span.h>

namespace Urho3D
{

class Camera;
class DrawableProcessor;
class DrawCommandQueue;
class InstancingBuffer;
class ShadowSplitProcessor;

/// Common parameters of batch rendering
struct BatchRenderingContext
{
    DrawCommandQueue& drawQueue_;
    const Camera& camera_;
    const ShadowSplitProcessor* outputShadowSplit_{};
    unsigned instanceMultiplier_ { 1u };

    ea::span<const ShaderResourceDesc> globalResources_;
    ea::span<const ShaderParameterDesc> frameParameters_;
    ea::span<const ShaderParameterDesc> cameraParameters_;

    BatchRenderingContext(DrawCommandQueue& drawQueue, const Camera& camera);
    BatchRenderingContext(DrawCommandQueue& drawQueue, const ShadowSplitProcessor& outputShadowSplit);
};

/// Utility class to convert pipeline batches into sequence of draw commands.
class URHO3D_API BatchRenderer : public Object
{
    URHO3D_OBJECT(BatchRenderer, Object);

public:
    BatchRenderer(RenderPipelineInterface* renderPipeline, const DrawableProcessor* drawableProcessor,
        InstancingBuffer* instancingBuffer);
    void SetSettings(const BatchRendererSettings& settings);

    /// Render batches
    /// @{
    void RenderBatches(const BatchRenderingContext& ctx, PipelineBatchGroup<PipelineBatchByState> batchGroup);
    void RenderBatches(const BatchRenderingContext& ctx, PipelineBatchGroup<PipelineBatchBackToFront> batchGroup);
    void RenderLightVolumeBatches(const BatchRenderingContext& ctx,
        ea::span<const PipelineBatchByState> batches);
    /// @}

    /// Store instancing data for batches.
    /// @{
    void PrepareInstancingBuffer(PipelineBatchGroup<PipelineBatchByState>& batches);
    void PrepareInstancingBuffer(PipelineBatchGroup<PipelineBatchBackToFront>& batches);
    /// @}

private:
    template <class T>
    void PrepareInstancingBufferImpl(PipelineBatchGroup<T>& batches);
    BatchRenderFlags AdjustRenderFlags(BatchRenderFlags flags) const;

    /// External dependencies
    /// @{
    Renderer* renderer_{};
    RenderPipelineDebugger* debugger_{};
    const DrawableProcessor* drawableProcessor_{};
    InstancingBuffer* instancingBuffer_{};
    /// @}

    BatchRendererSettings settings_;
};

}
