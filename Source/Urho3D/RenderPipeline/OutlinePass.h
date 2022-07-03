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

#include "PipelineBatchSortKey.h"
#include "ScenePass.h"
#include "SelectionGroup.h"
#include "../RenderPipeline/DrawableProcessor.h"
#include "../RenderPipeline/PostProcessPass.h"
#include "../RenderPipeline/RenderBuffer.h"
#include "../RenderPipeline/RenderPipelineDefs.h"

namespace Urho3D
{

class RenderBufferManager;
class RenderPipelineInterface;

class URHO3D_API OutlineDrawableProcessorPass : public ScenePass
{
    URHO3D_OBJECT(OutlineDrawableProcessorPass, ScenePass)

public:
    OutlineDrawableProcessorPass(RenderPipelineInterface* renderPipeline, DrawableProcessor* drawableProcessor,
        BatchStateCacheCallback* callback, DrawableProcessorPassFlags flags, const ea::string& pass);

    AddBatchResult AddBatch(
        unsigned threadIndex, Drawable* drawable, unsigned sourceBatchIndex, Technique* technique) override;

    void PrepareInstacingBuffer(BatchRenderer* batchRenderer) override;

    PipelineBatchGroup<PipelineBatchByState>& GetBatches() { return batchGroup_; }

    void OnBatchesReady() override;

    void SetSelection(SelectionGroup* selectionGroup);

private:
    Pass* GetPass(Technique* technique);

    PipelineBatchGroup<PipelineBatchByState> batchGroup_;

    ea::vector<PipelineBatchByState> sortedBaseBatches_;

    SharedPtr<Pass> pass_;

    SelectionGroup* selectionGroup_;
};

/// Post-processing pass that renders outline around selected objects.
class URHO3D_API OutlinePass : public PostProcessPass
{
    URHO3D_OBJECT(OutlinePass, PostProcessPass);

public:
    OutlinePass(RenderPipelineInterface* renderPipeline, RenderBufferManager* renderBufferManager);

    PostProcessPassFlags GetExecutionFlags() const override { return PostProcessPassFlag::NeedColorOutputReadAndWrite; }
    void Execute() override;

    RenderBuffer* GetColorOutput() { return texture_.temporary_; }

    void SetSelectionColor(const Color& color) { color_ = color; }
    const Color& GetSelectionColor() const { return color_; }

protected:
    void InitializeTextures();
    void InitializeStates();

    struct CachedTextures
    {
        SharedPtr<RenderBuffer> temporary_;
    };
    CachedTextures texture_;

    struct CachedStates
    {
        SharedPtr<PipelineState> outline_;

        bool IsValid() { return outline_->IsValid(); }
    };
    ea::optional<CachedStates> pipelineStates_;
    RenderPipelineInterface* renderPipeline_;
    Color color_{Color::WHITE};
};

} // namespace Urho3D
