// Copyright (c) 2026 rbfx-blueprint contributors.
// Licensed under the MIT license.

#pragma once

#include "Urho3D/RenderPipeline/Passes/FullScreenShaderPass.h"

namespace Urho3D
{

/// Temporal reconstruction pass with configurable feedback, sharpening and jitter scale.
class URHO3D_API TemporalUpscalingPass : public FullScreenShaderPass
{
    URHO3D_OBJECT(TemporalUpscalingPass, FullScreenShaderPass);

public:
    explicit TemporalUpscalingPass(Context* context);
    static void RegisterObject(Context* context);

    void CollectParameters(StringVariantMap& params) const override;
    void UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params) override;
    void Render(const SharedRenderPassState& sharedState) override;

    float GetFeedback() const { return feedback_; }
    float GetSharpness() const { return sharpness_; }
    float GetJitterScale() const { return jitterScale_; }
    unsigned GetHistoryGeneration() const { return historyGeneration_; }
    void ResetHistory() { historyReset_ = true; }

private:
    float feedback_{0.9f};
    float sharpness_{0.15f};
    float jitterScale_{1.0f};
    bool historyReset_{true};
    unsigned historyGeneration_{};
};

/// Fullscreen volumetric-fog integration pass. The shader is deliberately opt-in.
class URHO3D_API VolumetricFogPass : public FullScreenShaderPass
{
    URHO3D_OBJECT(VolumetricFogPass, FullScreenShaderPass);

public:
    explicit VolumetricFogPass(Context* context);
    static void RegisterObject(Context* context);

    void CollectParameters(StringVariantMap& params) const override;
    void UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params) override;

    float GetDensity() const { return density_; }
    float GetHeightFalloff() const { return heightFalloff_; }
    float GetAnisotropy() const { return anisotropy_; }

private:
    float density_{0.02f};
    float heightFalloff_{0.1f};
    float anisotropy_{0.0f};
};

/// Screen-space reflection integration pass with bounded ray steps and thickness control.
class URHO3D_API ScreenSpaceReflectionsPass : public FullScreenShaderPass
{
    URHO3D_OBJECT(ScreenSpaceReflectionsPass, FullScreenShaderPass);

public:
    explicit ScreenSpaceReflectionsPass(Context* context);
    static void RegisterObject(Context* context);

    void CollectParameters(StringVariantMap& params) const override;
    void UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params) override;

    unsigned GetMaxSteps() const { return maxSteps_; }
    float GetMaxDistance() const { return maxDistance_; }
    float GetThickness() const { return thickness_; }

private:
    unsigned maxSteps_{32};
    float maxDistance_{100.0f};
    float thickness_{0.1f};
};

/// Cascaded shadow configuration pass. Shadow atlas rendering remains owned by ShadowMapAllocator.
class URHO3D_API CascadedShadowMapsPass : public RenderPass
{
    URHO3D_OBJECT(CascadedShadowMapsPass, RenderPass);

public:
    explicit CascadedShadowMapsPass(Context* context);
    static void RegisterObject(Context* context);

    void CollectParameters(StringVariantMap& params) const override;
    void UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params) override;

    unsigned GetCascadeCount() const { return cascadeCount_; }
    float GetSplitLambda() const { return splitLambda_; }
    float GetMaxDistance() const { return maxDistance_; }
    float GetShadowBias() const { return shadowBias_; }

private:
    unsigned cascadeCount_{4};
    float splitLambda_{0.7f};
    float maxDistance_{200.0f};
    float shadowBias_{0.001f};
};

} // namespace Urho3D
