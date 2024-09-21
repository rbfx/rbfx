// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/Passes/BloomPass.h"

#include "Urho3D/RenderPipeline/RenderBufferManager.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"

#include <EASTL/numeric.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

static const ea::string comment{"Create bloom around bright parts of the image"};

static const ea::string numIterationsName{"Bloom: Num Iterations"};
static const ea::string minBrightnessName{"Bloom: Min Brightness"};
static const ea::string maxBrightnessName{"Bloom: Max Brightness"};
static const ea::string intensityName{"Bloom: Base Intensity"};
static const ea::string iterationFactorName{"Bloom: Iteration Intensity Factor"};

} // namespace

BloomPass::BloomPass(Context* context)
    : RenderPass(context)
{
    traits_.needBilinearColorSampler_ = true;
    traits_.needReadWriteColorBuffer_ = true;
    SetComment(comment);
}

void BloomPass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<BloomPass>(Category_RenderPass);

    URHO3D_COPY_BASE_ATTRIBUTES(RenderPass);
    URHO3D_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Comment", comment);
}

void BloomPass::CollectParameters(StringVariantMap& params) const
{
    DeclareParameter(numIterationsName, Parameters{}.numIterations_, params);
    DeclareParameter(minBrightnessName, Parameters{}.minBrightness_, params);
    DeclareParameter(maxBrightnessName, Parameters{}.maxBrightness_, params);
    DeclareParameter(intensityName, Parameters{}.intensity_, params);
    DeclareParameter(iterationFactorName, Parameters{}.iterationFactor_, params);
}

void BloomPass::InitializeView(RenderPipelineView* view)
{
}

void BloomPass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
    Parameters newParameters;

    if (const Variant& value = LoadParameter(numIterationsName, params); value.GetType() == VAR_INT)
        newParameters.numIterations_ = Clamp(value.GetUInt(), 1u, MaxIterations);
    if (const Variant& value = LoadParameter(minBrightnessName, params); value.GetType() == VAR_FLOAT)
        newParameters.minBrightness_ = value.GetFloat();
    if (const Variant& value = LoadParameter(maxBrightnessName, params); value.GetType() == VAR_FLOAT)
        newParameters.maxBrightness_ = value.GetFloat();
    if (const Variant& value = LoadParameter(intensityName, params); value.GetType() == VAR_FLOAT)
        newParameters.intensity_ = value.GetFloat();
    if (const Variant& value = LoadParameter(iterationFactorName, params); value.GetType() == VAR_FLOAT)
        newParameters.iterationFactor_ = value.GetFloat();

    if (parameters_.numIterations_ != newParameters.numIterations_)
        InvalidateTextureCache();

    parameters_ = newParameters;
}

void BloomPass::Update(const SharedRenderPassState& sharedState)
{
    const bool isHDR = sharedState.renderBufferManager_->IsHDR();
    if (isHDR_ != isHDR)
    {
        isHDR_ = isHDR;
        InvalidateTextureCache();
    }

    RestoreTextureCache(sharedState);
}

void BloomPass::Render(const SharedRenderPassState& sharedState)
{
    RestorePipelineStateCache(sharedState);

    luminanceWeights_ = sharedState.renderBufferManager_->IsLinearColorSpace()
        ? Color::LUMINOSITY_LINEAR.ToVector3() : Color::LUMINOSITY_GAMMA.ToVector3();

    sharedState.renderBufferManager_->SwapColorBuffers(false);

    const unsigned numIterations = GatherBrightRegions(sharedState.renderBufferManager_, textures_[0].final_);
    for (unsigned i = 0; i < numIterations; ++i)
    {
        if (i > 0)
            CopyTexture(sharedState.renderBufferManager_, textures_[i - 1].final_, textures_[i].final_);
        BlurTexture(sharedState.renderBufferManager_, textures_[i].final_, textures_[i].temporary_);
    }

    intensityMultipliers_.resize(numIterations);
    for (unsigned i = 0; i < numIterations; ++i)
        intensityMultipliers_[i] = Pow(parameters_.iterationFactor_, static_cast<float>(i));
    const float totalIntensity = ea::accumulate(intensityMultipliers_.begin(), intensityMultipliers_.end(), 0.0f);
    for (unsigned i = 0; i < numIterations; ++i)
        intensityMultipliers_[i] *= parameters_.intensity_ / totalIntensity;

    sharedState.renderBufferManager_->SwapColorBuffers(false);
    sharedState.renderBufferManager_->SetOutputRenderTargets();
    for (unsigned i = 0; i < numIterations; ++i)
        ApplyBloom(sharedState.renderBufferManager_, textures_[i].final_, intensityMultipliers_[i]);
}

void BloomPass::InvalidateTextureCache()
{
    textures_.clear();
}

void BloomPass::InvalidatePipelineStateCache()
{
    pipelineStates_ = ea::nullopt;
}

void BloomPass::RestoreTextureCache(const SharedRenderPassState& sharedState)
{
    if (!textures_.empty())
        return;

    textures_.resize(parameters_.numIterations_);

    for (unsigned i = 0; i < parameters_.numIterations_; ++i)
    {
        const TextureFormat format =
            isHDR_ ? TextureFormat::TEX_FORMAT_RGBA16_FLOAT : TextureFormat::TEX_FORMAT_RGBA8_UNORM;
        const RenderBufferParams params{format, 1, RenderBufferFlag::BilinearFiltering};
        // Start scale is 1
        // It's okay to do it unchecked because render buffers are never smaller than 1x1
        const Vector2 sizeMultiplier = Vector2::ONE / static_cast<float>(1 << i);
        textures_[i].final_ = sharedState.renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
        textures_[i].temporary_ = sharedState.renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    }
}

void BloomPass::RestorePipelineStateCache(const SharedRenderPassState& sharedState)
{
    if (pipelineStates_)
        return;

    static const NamedSamplerStateDesc samplers[] = {{ShaderResources::Albedo, SamplerStateDesc::Bilinear()}};

    pipelineStates_ = PipelineStateCache{};
    pipelineStates_->bright_ =
        sharedState.renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_Bloom", "BRIGHT", samplers);
    pipelineStates_->blurH_ =
        sharedState.renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_Bloom", "BLURH", samplers);
    pipelineStates_->blurV_ =
        sharedState.renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_Bloom", "BLURV", samplers);
    pipelineStates_->bloom_ =
        sharedState.renderBufferManager_->CreateQuadPipelineState(BLEND_ADD, "v2/P_Bloom", "COMBINE", samplers);
}

unsigned BloomPass::GatherBrightRegions(RenderBufferManager* renderBufferManager, RenderBuffer* destination)
{
    RawTexture* viewportTexture = renderBufferManager->GetSecondaryColorTexture();
    const IntVector2 inputSize = viewportTexture->GetParams().size_.ToIntVector2();
    const Vector2 inputInvSize = Vector2::ONE / inputSize.ToVector2();

    const ShaderResourceDesc shaderResources[] = {{ShaderResources::Albedo, viewportTexture}};
    const auto shaderParameters = GetShaderParameters(inputInvSize);

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager->GetDefaultClipToUVSpaceOffsetAndScale();
    drawParams.pipelineStateId_ = pipelineStates_->bright_;

    renderBufferManager->SetRenderTargets(nullptr, {destination});
    renderBufferManager->DrawQuad("Gather bright regions", drawParams);

    return Clamp(LogBaseTwo(ea::min(inputSize.x_, inputSize.y_)), 1u, parameters_.numIterations_);
}

void BloomPass::BlurTexture(RenderBufferManager* renderBufferManager, RenderBuffer* final, RenderBuffer* temporary)
{
    ShaderResourceDesc shaderResources[1];
    shaderResources[0].name_ = ShaderResources::Albedo;

    const Vector2 inputInvSize = Vector2::ONE / final->GetTexture()->GetParams().size_.ToVector2();
    const auto shaderParameters = GetShaderParameters(inputInvSize);

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager->GetDefaultClipToUVSpaceOffsetAndScale();

    shaderResources[0].texture_ = final->GetTexture();
    drawParams.pipelineStateId_ = pipelineStates_->blurH_;
    renderBufferManager->SetRenderTargets(nullptr, {temporary});
    renderBufferManager->DrawQuad("Blur vertically", drawParams);

    shaderResources[0].texture_ = temporary->GetTexture();
    drawParams.pipelineStateId_ = pipelineStates_->blurV_;
    renderBufferManager->SetRenderTargets(nullptr, {final});
    renderBufferManager->DrawQuad("Blur horizontally", drawParams);
}

void BloomPass::ApplyBloom(RenderBufferManager* renderBufferManager, RenderBuffer* bloom, float intensity)
{
    const ShaderResourceDesc shaderResources[] = {{ShaderResources::Albedo, bloom->GetTexture()}};
    const ShaderParameterDesc shaderParameters[] = {
        {Bloom_LuminanceWeights, luminanceWeights_}, {Bloom_Intensity, intensity}};
    renderBufferManager->DrawViewportQuad("Apply bloom", pipelineStates_->bloom_, shaderResources, shaderParameters);
}

void BloomPass::CopyTexture(RenderBufferManager* renderBufferManager, RenderBuffer* source, RenderBuffer* destination)
{
    renderBufferManager->SetRenderTargets(nullptr, {destination});
    renderBufferManager->DrawTexture("Downscale bloom", source->GetTexture());
}

} // namespace Urho3D
