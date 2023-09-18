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

#include "../Precompiled.h"

#include "../RenderPipeline/OutlinePass.h"

#include "../RenderPipeline/BatchRenderer.h"
#include "../RenderPipeline/RenderBufferManager.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

namespace
{

Pass* GetFirstPass(Technique* technique, const ea::vector<unsigned>& passIndices)
{
    for (unsigned index : passIndices)
    {
        if (Pass* pass = technique->GetPass(index))
            return pass;
    }
    return nullptr;
}

const TextureFormat outlineTextureFormat = TextureFormat::TEX_FORMAT_RGBA8_UNORM;

} // namespace

OutlineScenePass::OutlineScenePass(RenderPipelineInterface* renderPipeline,
    DrawableProcessor* drawableProcessor, BatchStateCacheCallback* callback,
    const StringVector& outlinedPasses, DrawableProcessorPassFlags flags)
    : ScenePass(renderPipeline, drawableProcessor, callback,
        DrawableProcessorPassFlag::BatchCallback | DrawableProcessorPassFlag::PipelineStateCallback | flags,
        "base") // Pass here doesn't matter
{
    ea::transform(outlinedPasses.begin(), outlinedPasses.end(), std::back_inserter(outlinedPasses_),
        [](const ea::string& pass) { return Technique::GetPassIndex(pass); });

    const PipelineStateOutputDesc outputDesc{
        TextureFormat::TEX_FORMAT_UNKNOWN, 1, {static_cast<TextureFormat>(outlineTextureFormat)}};
    SetDeferredOutputDesc(outputDesc);
}

void OutlineScenePass::SetOutlineGroups(Scene* scene, bool drawDebugOutlines)
{
    scene->GetComponents<OutlineGroup>(outlineGroups_);

    if (!drawDebugOutlines)
        ea::erase_if(outlineGroups_, [](const OutlineGroup* group) { return group->IsDebug(); });

    const bool hasDrawables = ea::any_of(outlineGroups_.begin(), outlineGroups_.end(),
        [](const OutlineGroup* group) { return group->HasDrawables(); });
    SetEnabled(hasDrawables);
}

DrawableProcessorPass::AddBatchResult OutlineScenePass::AddCustomBatch(
    unsigned threadIndex, Drawable* drawable, unsigned sourceBatchIndex, Technique* technique)
{
    if (outlineGroups_.empty())
        return {};

    // TODO: Check if all groups are empty
    bool batchAdded = false;
    for (OutlineGroup* outlineGroup : outlineGroups_)
    {
        if (!outlineGroup->ContainsDrawable(drawable))
            continue;

        if (Pass* referencePass = GetFirstPass(technique, outlinedPasses_))
        {
            geometryBatches_.PushBack(threadIndex, GeometryBatch::Deferred(drawable, sourceBatchIndex, referencePass, outlineGroup));

            batchAdded = true;
        }
    }

    return {batchAdded, false};
}

bool OutlineScenePass::CreatePipelineState(GraphicsPipelineStateDesc& desc, PipelineStateBuilder* builder,
    const BatchStateCreateKey& key, const BatchStateCreateContext& ctx)
{
    ShaderProgramCompositor* compositor = builder->GetShaderProgramCompositor();

    desc.debugName_ = Format("Outline Pass for material '{}'", key.material_->GetName());

    desc.depthWriteEnabled_ = false;
    desc.depthCompareFunction_ = CMP_ALWAYS;

    desc.colorWriteEnabled_ = true;
    desc.blendMode_ = BLEND_REPLACE;
    desc.alphaToCoverageEnabled_ = false;

    desc.fillMode_ = FILL_SOLID;
    desc.cullMode_ = CULL_NONE;

    desc.scissorTestEnabled_ = true;

    shaderProgramDesc_.Clear();
    compositor->ProcessUserBatch(shaderProgramDesc_, GetFlags(),
        key.drawable_, key.geometry_, key.geometryType_, key.material_, key.pass_,
        nullptr, false, BatchCompositorSubpass::Ignored);

    shaderProgramDesc_.shaderName_[PS] = "v2/M_OutlinePixel";
    shaderProgramDesc_.shaderDefines_[PS] = "";

    const Texture* diffMap = key.material_->GetTexture(ShaderResources::Albedo);
    const bool needAlphaMask = key.pass_->IsAlphaMask() || (key.pass_->GetBlendMode() != BLEND_REPLACE && diffMap);
    if (needAlphaMask)
    {
        shaderProgramDesc_.AddShaderDefines(PS, "ALPHAMASK");
        desc.samplers_.Add(ShaderResources::Albedo, diffMap->GetSamplerStateDesc());
    }

    const bool isStereo = GetFlags().Test(DrawableProcessorPassFlag::StereoInstancing);
    builder->SetupInputLayoutAndPrimitiveType(desc, shaderProgramDesc_, key.geometry_, isStereo);
    builder->SetupShaders(desc, shaderProgramDesc_);

    return true;
}

void OutlineScenePass::OnBatchesReady()
{
    for (PipelineBatch& batch : deferredBatches_)
    {
        auto outlineGroup = static_cast<OutlineGroup*>(batch.userData_);
        batch.material_ = outlineGroup->GetOutlineMaterial(batch.material_);
    }

    BatchCompositor::FillSortKeys(sortedBatches_, deferredBatches_);
    ea::sort(sortedBatches_.begin(), sortedBatches_.end());

    batchGroup_ = {sortedBatches_};
    batchGroup_.flags_ = BatchRenderFlag::EnableInstancingForStaticGeometry;
    if (linearColorSpace_)
        batchGroup_.flags_ |= BatchRenderFlag::LinearColorSpace;
}

void OutlineScenePass::PrepareInstancingBuffer(BatchRenderer* batchRenderer)
{
    batchRenderer->PrepareInstancingBuffer(batchGroup_);
}

OutlinePass::OutlinePass(RenderPipelineInterface* renderPipeline, RenderBufferManager* renderBufferManager)
    : BaseClassName(renderPipeline, renderBufferManager)
{
    renderPipeline->OnRenderBegin.Subscribe(this, &OutlinePass::OnRenderBegin);
}

void OutlinePass::OnRenderBegin(const CommonFrameInfo& frameInfo)
{
    if (!enabled_)
        return;

    if (!outlineBuffer_)
    {
        const RenderBufferParams params{outlineTextureFormat, 1, RenderBufferFlag::BilinearFiltering};
        const Vector2 sizeMultiplier = Vector2::ONE; // / 2.0f;
        outlineBuffer_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    }

    static const NamedSamplerStateDesc samplers[] = {{ShaderResources::Albedo, SamplerStateDesc::Bilinear()}};
    if (pipelineStateLinear_ == StaticPipelineStateId::Invalid)
    {
        pipelineStateLinear_ = renderBufferManager_->CreateQuadPipelineState(
            BLEND_ALPHA, "v2/P_Outline", "URHO3D_GAMMA_CORRECTION", samplers);
    }
    if (pipelineStateGamma_ == StaticPipelineStateId::Invalid)
    {
        pipelineStateGamma_ = renderBufferManager_->CreateQuadPipelineState(BLEND_ALPHA, "v2/P_Outline", "", samplers);
    }
}

void OutlinePass::Execute(Camera* camera)
{
    if (!enabled_)
        return;

    const bool inLinearSpace = renderBufferManager_->IsLinearColorSpace();
    const StaticPipelineStateId pipelineState = inLinearSpace ? pipelineStateLinear_ : pipelineStateGamma_;

    auto texture = outlineBuffer_->GetTexture();
    const Vector2 inputInvSize = Vector2::ONE / texture->GetParams().size_.ToVector2();

    const ShaderParameterDesc result[] = {{"InputInvSize", inputInvSize}};
    const ShaderResourceDesc shaderResources[] = {{ShaderResources::Albedo, texture}};

    renderBufferManager_->SetOutputRenderTargets();
    renderBufferManager_->DrawViewportQuad("Apply outline", pipelineState, shaderResources, result);
}

}
