// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/StringUtils.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Technique.h"
#include "../RenderPipeline/BatchRenderer.h"
#include "../RenderPipeline/ScenePass.h"

#include <EASTL/sort.h>

#include "../DebugNew.h"

namespace Urho3D
{

ScenePass::ScenePass(RenderPipelineInterface* renderPipeline, DrawableProcessor* drawableProcessor,
    BatchStateCacheCallback* callback, DrawableProcessorPassFlags flags, const ea::string& deferredPass,
    const ea::string& unlitBasePass, const ea::string& litBasePass, const ea::string& lightPass)
    : BatchCompositorPass(renderPipeline, drawableProcessor, callback, flags, Technique::GetPassIndex(deferredPass),
        Technique::GetPassIndex(unlitBasePass), Technique::GetPassIndex(litBasePass), Technique::GetPassIndex(lightPass))
{
}

ScenePass::ScenePass(RenderPipelineInterface* renderPipeline, DrawableProcessor* drawableProcessor,
    BatchStateCacheCallback* callback, DrawableProcessorPassFlags flags, const ea::string& pass)
    : BatchCompositorPass(renderPipeline, drawableProcessor, callback, flags, M_MAX_UNSIGNED,
        Technique::GetPassIndex(pass), M_MAX_UNSIGNED, M_MAX_UNSIGNED)
{
}

void UnorderedScenePass::OnBatchesReady()
{
    BatchCompositor::FillSortKeys(sortedDeferredBatches_, deferredBatches_);
    BatchCompositor::FillSortKeys(sortedBaseBatches_, baseBatches_);
    BatchCompositor::FillSortKeys(sortedLightBatches_, lightBatches_, negativeLightBatches_);

    ea::sort(sortedDeferredBatches_.begin(), sortedDeferredBatches_.end());
    ea::sort(sortedBaseBatches_.begin(), sortedBaseBatches_.end());

    const unsigned numNegativeLightBatches = negativeLightBatches_.Size();
    ea::sort(sortedLightBatches_.begin(), sortedLightBatches_.end() - numNegativeLightBatches);
    ea::sort(sortedLightBatches_.end() - numNegativeLightBatches, sortedLightBatches_.end());

    deferredBatchGroup_ = { sortedDeferredBatches_ };
    baseBatchGroup_ = { sortedBaseBatches_ };
    lightBatchGroup_ = { sortedLightBatches_ };

    if (!GetFlags().Test(DrawableProcessorPassFlag::DisableInstancing))
    {
        deferredBatchGroup_.flags_ |= BatchRenderFlag::EnableInstancingForStaticGeometry;
        baseBatchGroup_.flags_ |= BatchRenderFlag::EnableInstancingForStaticGeometry;
        lightBatchGroup_.flags_ |= BatchRenderFlag::EnableInstancingForStaticGeometry;
    }

    if (HasLightPass())
    {
        baseBatchGroup_.flags_ |= BatchRenderFlag::EnablePixelLights;
        lightBatchGroup_.flags_ |= BatchRenderFlag::EnablePixelLights;
    }

    if (GetFlags().Test(DrawableProcessorPassFlag::HasAmbientLighting))
    {
        deferredBatchGroup_.flags_ |= BatchRenderFlag::EnableAmbientLighting;
        baseBatchGroup_.flags_ |= BatchRenderFlag::EnableAmbientAndVertexLighting;
    }

    if (GetFlags().Test(DrawableProcessorPassFlag::DepthOnlyPass))
    {
        deferredBatchGroup_.flags_ |= BatchRenderFlag::DisableColorOutput;
        baseBatchGroup_.flags_ |= BatchRenderFlag::DisableColorOutput;
    }

    if (GetFlags().Test(DrawableProcessorPassFlag::DeferredLightMaskToStencil))
    {
        deferredBatchGroup_.flags_ |= BatchRenderFlag::LightMaskToStencil;
    }

    if (linearColorSpace_)
    {
        deferredBatchGroup_.flags_ |= BatchRenderFlag::LinearColorSpace;
        baseBatchGroup_.flags_ |= BatchRenderFlag::LinearColorSpace;
        lightBatchGroup_.flags_ |= BatchRenderFlag::LinearColorSpace;
    }
}

void UnorderedScenePass::PrepareInstancingBuffer(BatchRenderer* batchRenderer)
{
    batchRenderer->PrepareInstancingBuffer(deferredBatchGroup_);
    batchRenderer->PrepareInstancingBuffer(baseBatchGroup_);
    batchRenderer->PrepareInstancingBuffer(lightBatchGroup_);
}

void BackToFrontScenePass::OnBatchesReady()
{
    BatchCompositor::FillSortKeys(sortedBatches_, baseBatches_, lightBatches_, negativeLightBatches_);

    // When rendering back-to-front, it's still important to render batches for each object in order:
    // - Base batch;
    // - Additive light batches;
    // - Subtractive light batches.
    // Multiply distance by some factor close to 1 if distance is greater than 0, ignore otherwise.

    const unsigned subtractiveLightBatchesBegin = sortedBatches_.size() - negativeLightBatches_.Size();
    const unsigned subtractiveLightBatchesEnd = sortedBatches_.size();
    const unsigned additiveLightBatchesBegin = subtractiveLightBatchesBegin - lightBatches_.Size();
    const unsigned additiveLightBatchesEnd = subtractiveLightBatchesBegin;

    static const float additiveDistanceFactor = 1 - M_EPSILON;
    static const float subtractiveDistanceFactor = 1 - 2 * M_EPSILON;

    // Validate distances before sorting, NaN may corrupt ea::sort
    for (PipelineBatchBackToFront& sortedBatch : sortedBatches_)
    {
        if (std::isfinite(sortedBatch.distance_))
            continue;

        const PipelineBatch& batch = *sortedBatch.pipelineBatch_;
        URHO3D_LOGERROR("Drawable batch [{}].{} has NaN distance",
            batch.drawable_->GetFullNameDebug(), batch.sourceBatchIndex_);
        sortedBatch.distance_ = M_LARGE_VALUE;
    }

    for (unsigned i = additiveLightBatchesBegin; i < additiveLightBatchesEnd; ++i)
        sortedBatches_[i].distance_ *= additiveDistanceFactor;

    for (unsigned i = subtractiveLightBatchesBegin; i < subtractiveLightBatchesEnd; ++i)
        sortedBatches_[i].distance_ *= subtractiveDistanceFactor;

    ea::sort(sortedBatches_.begin(), sortedBatches_.end());

    if (GetFlags().Test(DrawableProcessorPassFlag::RefractionPass))
    {
        hasRefractionBatches_ = false;
        for (const PipelineBatchBackToFront& sortedBatch : sortedBatches_)
        {
            // Assume refraction if blending is disabled
            if (sortedBatch.pipelineBatch_->pipelineState_->GetDesc().AsGraphics()->blendMode_ == BLEND_REPLACE)
            {
                hasRefractionBatches_ = true;
                break;
            }
        }
    }

    batchGroup_ = { sortedBatches_ };
    if (GetFlags().Test(DrawableProcessorPassFlag::HasAmbientLighting))
        batchGroup_.flags_ |= BatchRenderFlag::EnableAmbientAndVertexLighting;
    if (HasLightPass())
        batchGroup_.flags_ |= BatchRenderFlag::EnablePixelLights;
    if (!GetFlags().Test(DrawableProcessorPassFlag::DisableInstancing))
        batchGroup_.flags_ |= BatchRenderFlag::EnableInstancingForStaticGeometry;
    if (linearColorSpace_)
        batchGroup_.flags_ |= BatchRenderFlag::LinearColorSpace;
}

void BackToFrontScenePass::PrepareInstancingBuffer(BatchRenderer* batchRenderer)
{
    batchRenderer->PrepareInstancingBuffer(batchGroup_);
}

}
