// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Graphics/OutlineGroup.h"
#include "../RenderPipeline/DrawableProcessor.h"
#include "../RenderPipeline/PipelineBatchSortKey.h"
#include "../RenderPipeline/PipelineStateBuilder.h"
#include "../RenderPipeline/RenderBuffer.h"
#include "../RenderPipeline/RenderPipelineDefs.h"
#include "../RenderPipeline/ScenePass.h"

namespace Urho3D
{

class RenderBufferManager;
class RenderPipelineInterface;

class URHO3D_API OutlineScenePass : public ScenePass
{
    URHO3D_OBJECT(OutlineScenePass, ScenePass)

public:
    OutlineScenePass(RenderPipelineInterface* renderPipeline, DrawableProcessor* drawableProcessor,
        BatchStateCacheCallback* callback, const StringVector& outlinedPasses, DrawableProcessorPassFlags flags = DrawableProcessorPassFlag::None);

    /// Initialize outline groups from scene. Should be called every frame.
    void SetOutlineGroups(Scene* scene, bool drawDebugOutlines);

    /// Implement ScenePass
    /// @{
    AddBatchResult AddCustomBatch(
        unsigned threadIndex, Drawable* drawable, unsigned sourceBatchIndex, Technique* technique) override;
    bool CreatePipelineState(GraphicsPipelineStateDesc& desc, PipelineStateBuilder* builder,
        const BatchStateCreateKey& key, const BatchStateCreateContext& ctx) override;
    /// @}

    void PrepareInstancingBuffer(BatchRenderer* batchRenderer) override;

    const PipelineBatchGroup<PipelineBatchByState>& GetBatches() { return batchGroup_; }
    bool HasBatches() const { return !sortedBatches_.empty(); }

    void OnBatchesReady() override;

private:
    ea::vector<unsigned> outlinedPasses_;
    ea::vector<OutlineGroup*> outlineGroups_;

    /// Internal temporary containers:
    /// @{
    ShaderProgramDesc shaderProgramDesc_;
    ea::vector<PipelineBatchByState> sortedBatches_;
    PipelineBatchGroup<PipelineBatchByState> batchGroup_;
    /// @}
};

} // namespace Urho3D
