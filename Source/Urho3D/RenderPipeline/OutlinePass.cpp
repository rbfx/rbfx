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

#include "OutlinePass.h"

#include "BatchRenderer.h"
#include "RenderBufferManager.h"

namespace Urho3D
{
OutlineDrawableProcessorPass::OutlineDrawableProcessorPass(RenderPipelineInterface* renderPipeline,
    DrawableProcessor* drawableProcessor, BatchStateCacheCallback* callback, DrawableProcessorPassFlags flags,
    const ea::string& pass)
    : ScenePass(renderPipeline, drawableProcessor, callback, flags, pass)
{
    pass_ = MakeShared<Pass>("Outline");
    pass_->SetVertexShader("M_Default");
    pass_->SetPixelShader("M_Default");
    pass_->SetCullMode(CULL_NONE);
}

DrawableProcessorPass::AddBatchResult OutlineDrawableProcessorPass::AddBatch(
    unsigned threadIndex, Drawable* drawable, unsigned sourceBatchIndex, Technique* technique)
{
    if (!selectionGroup_ || selectionGroup_->Empty())
    {
        return {false, false};
    }

    if (!selectionGroup_->GetDrawables().contains(WeakPtr<Drawable>(drawable)))
    {
        return {false, false};
    }

    Pass* outlinePass = GetPass(technique);
    geometryBatches_.PushBack(threadIndex, {drawable, sourceBatchIndex, outlinePass, nullptr, nullptr, nullptr});
    return {true, false};
}

void OutlineDrawableProcessorPass::OnBatchesReady()
{
    BatchCompositor::FillSortKeys(sortedBaseBatches_, deferredBatches_);

    ea::sort(sortedBaseBatches_.begin(), sortedBaseBatches_.end());

    batchGroup_ = {sortedBaseBatches_};

    if (!GetFlags().Test(DrawableProcessorPassFlag::DisableInstancing))
    {
        batchGroup_.flags_ |= BatchRenderFlag::EnableInstancingForStaticGeometry;
    }
}

void OutlineDrawableProcessorPass::SetSelection(SelectionGroup* selectionGroup) { selectionGroup_ = selectionGroup; }

Pass* OutlineDrawableProcessorPass::GetPass(Technique* technique)
{
    return pass_;
}

void OutlineDrawableProcessorPass::PrepareInstacingBuffer(BatchRenderer* batchRenderer)
{
    batchRenderer->PrepareInstancingBuffer(batchGroup_);
}

OutlinePass::OutlinePass(RenderPipelineInterface* renderPipeline, RenderBufferManager* renderBufferManager)
    : BaseClassName(renderPipeline, renderBufferManager)
    , renderPipeline_(renderPipeline)
{
}

void OutlinePass::Execute()
{
    if (!pipelineStates_)
    {
        InitializeTextures();
        InitializeStates();
    }

    if (!pipelineStates_->IsValid())
        return;

    renderBufferManager_->SetOutputRenderTargers();
    auto texture = GetColorOutput()->GetTexture();
    if (texture)
    {
        const Vector2 inputInvSize = Vector2::ONE / static_cast<Vector2>(texture->GetSize());

        const ShaderParameterDesc result[] = {
            {"SelectionColor", color_},
            {"InputInvSize", inputInvSize},
        };

        const ShaderResourceDesc shaderResources[] = {{TU_DIFFUSE, texture}};
        renderBufferManager_->DrawViewportQuad("Apply bloom", pipelineStates_->outline_, shaderResources, result);
    }
}

void OutlinePass::InitializeTextures()
{
    const unsigned format = Graphics::GetRGBFormat();
    const RenderBufferParams params{format, 1, RenderBufferFlag::BilinearFiltering};
    const Vector2 sizeMultiplier = Vector2::ONE; // / 2.0f;
    texture_.temporary_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
}

void OutlinePass::InitializeStates()
{
    pipelineStates_ = CachedStates{};
    pipelineStates_->outline_ = renderBufferManager_->CreateQuadPipelineState(BLEND_ALPHA, "v2/P_Outline", "");
}

} // namespace Urho3D
