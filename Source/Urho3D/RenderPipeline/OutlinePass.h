//
// Copyright (c) 2022-2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Graphics/OutlineGroup.h"
#include "../RenderPipeline/DrawableProcessor.h"
#include "../RenderPipeline/PipelineBatchSortKey.h"
#include "../RenderPipeline/PostProcessPass.h"
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

/// Post-processing pass that renders outline around selected objects.
class URHO3D_API OutlinePass : public PostProcessPass
{
    URHO3D_OBJECT(OutlinePass, PostProcessPass);

public:
    OutlinePass(RenderPipelineInterface* renderPipeline, RenderBufferManager* renderBufferManager);

    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

    /// Implement PostProcessPass.
    /// @{
    PostProcessPassFlags GetExecutionFlags() const override { return PostProcessPassFlag::None; }
    void Execute(Camera* camera) override;
    /// @}

    RenderBuffer* GetColorOutput() { return outlineBuffer_; }

private:
    void OnRenderBegin(const CommonFrameInfo& frameInfo);

    bool enabled_{};

    StaticPipelineStateId pipelineStateGamma_{};
    StaticPipelineStateId pipelineStateLinear_{};
    SharedPtr<RenderBuffer> outlineBuffer_;
};

} // namespace Urho3D
