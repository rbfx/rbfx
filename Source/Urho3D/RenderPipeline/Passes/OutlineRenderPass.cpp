// Copyright (c) 2022-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/Passes/OutlineRenderPass.h"

#include "Urho3D/RenderPipeline/BatchRenderer.h"
#include "Urho3D/RenderPipeline/RenderBufferManager.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

namespace
{

static const ea::string comment{"Draw outline of the contents of the color buffer"};

} // namespace

OutlineRenderPass::OutlineRenderPass(Context* context)
    : RenderPass(context)
{
    SetComment(comment);
}

void OutlineRenderPass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<OutlineRenderPass>(Category_RenderPass);

    URHO3D_COPY_BASE_ATTRIBUTES(RenderPass);
    URHO3D_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Comment", comment);
}

void OutlineRenderPass::CollectParameters(StringVariantMap& params) const
{
}

void OutlineRenderPass::InitializeView(RenderPipelineView* view)
{
}

void OutlineRenderPass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
}

void OutlineRenderPass::Render(const SharedRenderPassState& sharedState)
{
    ConnectToRenderBuffer(colorBuffer_, ColorBufferId, sharedState);

    if (!colorBuffer_ || !colorBuffer_->IsEnabled())
        return;

    RestoreCache(sharedState);

    const bool inLinearSpace = sharedState.renderBufferManager_->IsLinearColorSpace();
    const StaticPipelineStateId pipelineState = inLinearSpace ? pipelineStates_->linear_ : pipelineStates_->gamma_;

    RawTexture* texture = colorBuffer_->GetTexture();
    const Vector2 inputInvSize = Vector2::ONE / texture->GetParams().size_.ToVector2();

    const ShaderParameterDesc result[] = {{"InputInvSize", inputInvSize}};
    const ShaderResourceDesc shaderResources[] = {{ShaderResources::Albedo, texture}};

    sharedState.renderBufferManager_->SetOutputRenderTargets();
    sharedState.renderBufferManager_->DrawViewportQuad("Apply outline", pipelineState, shaderResources, result);
}

void OutlineRenderPass::InvalidateCache()
{
    pipelineStates_ = ea::nullopt;
}

void OutlineRenderPass::RestoreCache(const SharedRenderPassState& sharedState)
{
    if (pipelineStates_)
        return;

    pipelineStates_ = PipelineStateCache{};

    static const NamedSamplerStateDesc samplers[] = {{ShaderResources::Albedo, SamplerStateDesc::Bilinear()}};

    pipelineStates_->linear_ = sharedState.renderBufferManager_->CreateQuadPipelineState(
        BLEND_ALPHA, "v2/P_Outline", "URHO3D_GAMMA_CORRECTION", samplers);
    pipelineStates_->gamma_ =
        sharedState.renderBufferManager_->CreateQuadPipelineState(BLEND_ALPHA, "v2/P_Outline", "", samplers);
}

} // namespace Urho3D
