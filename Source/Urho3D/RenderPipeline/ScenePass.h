// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/RenderPipeline/PipelineBatchSortKey.h"
#include "Urho3D/RenderPipeline/BatchCompositor.h"
#include "Urho3D/RenderPipeline/DrawableProcessor.h"

#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

class RenderPipelineInterface;
class BatchRenderer;

/// Base type for scene pass.
class ScenePass : public BatchCompositorPass
{
    URHO3D_OBJECT(ScenePass, BatchCompositorPass);

public:
    /// Construct pass with forward lighting.
    ScenePass(RenderPipelineInterface* renderPipeline, DrawableProcessor* drawableProcessor,
        BatchStateCacheCallback* callback, DrawableProcessorPassFlags flags, const ea::string& deferredPass,
        const ea::string& unlitBasePass, const ea::string& litBasePass, const ea::string& lightPass);

    /// Construct pass without forward lighting.
    ScenePass(RenderPipelineInterface* renderPipeline, DrawableProcessor* drawableProcessor,
        BatchStateCacheCallback* callback, DrawableProcessorPassFlags flags, const ea::string& pass);

    /// Prepare instancing buffer for scene pass.
    virtual void PrepareInstancingBuffer(BatchRenderer* batchRenderer) = 0;
};

/// Scene pass with batches sorted by render order and pipeline state.
class URHO3D_API UnorderedScenePass : public ScenePass
{
    URHO3D_OBJECT(UnorderedScenePass, ScenePass);

public:
    using ScenePass::ScenePass;

    void PrepareInstancingBuffer(BatchRenderer* batchRenderer) override;

    const PipelineBatchGroup<PipelineBatchByState>& GetDeferredBatches() { return deferredBatchGroup_; }
    const PipelineBatchGroup<PipelineBatchByState>& GetBaseBatches() { return baseBatchGroup_; }
    const PipelineBatchGroup<PipelineBatchByState>& GetLightBatches() { return lightBatchGroup_; }

protected:
    void OnBatchesReady() override;

    ea::vector<PipelineBatchByState> sortedDeferredBatches_;
    ea::vector<PipelineBatchByState> sortedBaseBatches_;
    ea::vector<PipelineBatchByState> sortedLightBatches_;

    PipelineBatchGroup<PipelineBatchByState> deferredBatchGroup_;
    PipelineBatchGroup<PipelineBatchByState> baseBatchGroup_;
    PipelineBatchGroup<PipelineBatchByState> lightBatchGroup_;
};

/// Scene pass with batches sorted by render order and distance back to front.
class URHO3D_API BackToFrontScenePass : public ScenePass
{
    URHO3D_OBJECT(BackToFrontScenePass, ScenePass);

public:
    using ScenePass::ScenePass;

    void PrepareInstancingBuffer(BatchRenderer* batchRenderer) override;

    const PipelineBatchGroup<PipelineBatchBackToFront>& GetBatches() { return batchGroup_; }

    bool HasRefractionBatches() const { return hasRefractionBatches_; }

protected:
    void OnBatchesReady() override;

    ea::vector<PipelineBatchBackToFront> sortedBatches_;
    bool hasRefractionBatches_{};

    PipelineBatchGroup<PipelineBatchBackToFront> batchGroup_;
};

}
