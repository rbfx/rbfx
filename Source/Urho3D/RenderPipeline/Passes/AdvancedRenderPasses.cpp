// Copyright (c) 2026 rbfx-blueprint contributors.
// Licensed under the MIT license.

#include "Urho3D/Precompiled.h"
#include "Urho3D/RenderPipeline/Passes/AdvancedRenderPasses.h"
#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

float LoadFloat(const StringVariantMap& params, const ea::string& name, float fallback)
{
    const auto iter = params.find(name);
    return iter != params.end() ? iter->second.GetFloat() : fallback;
}

unsigned LoadUInt(const StringVariantMap& params, const ea::string& name, unsigned fallback)
{
    const auto iter = params.find(name);
    return iter != params.end() ? iter->second.GetUInt() : fallback;
}

} // namespace

TemporalUpscalingPass::TemporalUpscalingPass(Context* context)
    : FullScreenShaderPass(context)
{
    RenderPass::attributes_.passName_ = "Postprocess: Temporal Upscaling";
    RenderPass::attributes_.isEnabledByDefault_ = false;
    RenderPass::attributes_.comment_ = "Temporal reconstruction and sharpening; enable only when a history-capable viewport is available.";

    StringVariantMap parameters;
    parameters["Feedback"] = feedback_;
    parameters["Sharpness"] = sharpness_;
    parameters["JitterScale"] = jitterScale_;
    ConfigureShader("v2/P_TemporalUpscaling", EMPTY_STRING, BLEND_REPLACE, true, true, false,
        "Temporal Upscaling: ", parameters);
}

void TemporalUpscalingPass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<TemporalUpscalingPass>(Category_RenderPass);
    URHO3D_COPY_BASE_ATTRIBUTES(FullScreenShaderPass);
}

void TemporalUpscalingPass::CollectParameters(StringVariantMap& params) const
{
    FullScreenShaderPass::CollectParameters(params);
}

void TemporalUpscalingPass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
    FullScreenShaderPass::UpdateParameters(settings, params);
    feedback_ = Clamp(LoadFloat(params, "Temporal Upscaling: Feedback", feedback_), 0.0f, 0.99f);
    sharpness_ = Clamp(LoadFloat(params, "Temporal Upscaling: Sharpness", sharpness_), 0.0f, 1.0f);
    jitterScale_ = Clamp(LoadFloat(params, "Temporal Upscaling: JitterScale", jitterScale_), 0.0f, 4.0f);
}

void TemporalUpscalingPass::Render(const SharedRenderPassState& sharedState)
{
    if (historyReset_)
    {
        ++historyGeneration_;
        historyReset_ = false;
    }
    FullScreenShaderPass::Render(sharedState);
}

VolumetricFogPass::VolumetricFogPass(Context* context)
    : FullScreenShaderPass(context)
{
    RenderPass::attributes_.passName_ = "Postprocess: Volumetric Fog";
    RenderPass::attributes_.isEnabledByDefault_ = false;
    RenderPass::attributes_.comment_ = "Opt-in fog integration pass with bounded density and height falloff.";

    StringVariantMap parameters;
    parameters["Density"] = density_;
    parameters["HeightFalloff"] = heightFalloff_;
    parameters["Anisotropy"] = anisotropy_;
    ConfigureShader("v2/P_VolumetricFog", EMPTY_STRING, BLEND_ALPHA, true, true, false,
        "Volumetric Fog: ", parameters);
}

void VolumetricFogPass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<VolumetricFogPass>(Category_RenderPass);
    URHO3D_COPY_BASE_ATTRIBUTES(FullScreenShaderPass);
}

void VolumetricFogPass::CollectParameters(StringVariantMap& params) const
{
    FullScreenShaderPass::CollectParameters(params);
}

void VolumetricFogPass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
    FullScreenShaderPass::UpdateParameters(settings, params);
    density_ = Clamp(LoadFloat(params, "Volumetric Fog: Density", density_), 0.0f, 10.0f);
    heightFalloff_ = Clamp(LoadFloat(params, "Volumetric Fog: HeightFalloff", heightFalloff_), 0.0f, 10.0f);
    anisotropy_ = Clamp(LoadFloat(params, "Volumetric Fog: Anisotropy", anisotropy_), -0.99f, 0.99f);
}

ScreenSpaceReflectionsPass::ScreenSpaceReflectionsPass(Context* context)
    : FullScreenShaderPass(context)
{
    RenderPass::attributes_.passName_ = "Postprocess: Screen Space Reflections";
    RenderPass::attributes_.isEnabledByDefault_ = false;
    RenderPass::attributes_.comment_ = "Bounded screen-space reflection integration pass.";

    StringVariantMap parameters;
    parameters["MaxSteps"] = maxSteps_;
    parameters["MaxDistance"] = maxDistance_;
    parameters["Thickness"] = thickness_;
    ConfigureShader("v2/P_ScreenSpaceReflections", EMPTY_STRING, BLEND_ALPHA, true, true, false,
        "SSR: ", parameters);
}

void ScreenSpaceReflectionsPass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ScreenSpaceReflectionsPass>(Category_RenderPass);
    URHO3D_COPY_BASE_ATTRIBUTES(FullScreenShaderPass);
}

void ScreenSpaceReflectionsPass::CollectParameters(StringVariantMap& params) const
{
    FullScreenShaderPass::CollectParameters(params);
}

void ScreenSpaceReflectionsPass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
    FullScreenShaderPass::UpdateParameters(settings, params);
    maxSteps_ = Clamp(LoadUInt(params, "SSR: MaxSteps", maxSteps_), 4u, 256u);
    maxDistance_ = Clamp(LoadFloat(params, "SSR: MaxDistance", maxDistance_), 0.1f, 10000.0f);
    thickness_ = Clamp(LoadFloat(params, "SSR: Thickness", thickness_), 0.0001f, 10.0f);
}

CascadedShadowMapsPass::CascadedShadowMapsPass(Context* context)
    : RenderPass(context)
{
    RenderPass::attributes_.passName_ = "Lighting: Cascaded Shadow Maps";
    RenderPass::attributes_.isEnabledByDefault_ = true;
    RenderPass::attributes_.comment_ = "Stable cascade split policy consumed by ShadowMapAllocator and light processors.";
}

void CascadedShadowMapsPass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<CascadedShadowMapsPass>(Category_RenderPass);
    URHO3D_COPY_BASE_ATTRIBUTES(RenderPass);
    URHO3D_ATTRIBUTE("Cascade Count", unsigned, cascadeCount_, 4u, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Split Lambda", float, splitLambda_, 0.7f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Distance", float, maxDistance_, 200.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Shadow Bias", float, shadowBias_, 0.001f, AM_DEFAULT);
}

void CascadedShadowMapsPass::CollectParameters(StringVariantMap& params) const
{
    DeclareParameter("Cascaded Shadows: Cascade Count", cascadeCount_, params);
    DeclareParameter("Cascaded Shadows: Split Lambda", splitLambda_, params);
    DeclareParameter("Cascaded Shadows: Max Distance", maxDistance_, params);
    DeclareParameter("Cascaded Shadows: Bias", shadowBias_, params);
}

void CascadedShadowMapsPass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
    cascadeCount_ = Clamp(LoadUInt(params, "Cascaded Shadows: Cascade Count", cascadeCount_), 1u, 8u);
    splitLambda_ = Clamp(LoadFloat(params, "Cascaded Shadows: Split Lambda", splitLambda_), 0.0f, 1.0f);
    maxDistance_ = Clamp(LoadFloat(params, "Cascaded Shadows: Max Distance", maxDistance_), 1.0f, 100000.0f);
    shadowBias_ = Clamp(LoadFloat(params, "Cascaded Shadows: Bias", shadowBias_), 0.0f, 1.0f);
}

} // namespace Urho3D
