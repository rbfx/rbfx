// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderPipeline/RenderBuffer.h"
#include "Urho3D/RenderPipeline/RenderPass.h"
#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"

#include <EASTL/optional.h>
#include <EASTL/tuple.h>

namespace Urho3D
{

URHO3D_SHADER_CONST(Bloom, LuminanceWeights);
URHO3D_SHADER_CONST(Bloom, Threshold);
URHO3D_SHADER_CONST(Bloom, InputInvSize);
URHO3D_SHADER_CONST(Bloom, Intensity);

/// Post-processing pass that applies bloom to scene.
class URHO3D_API BloomPass : public RenderPass
{
    URHO3D_OBJECT(BloomPass, RenderPass);

public:
    explicit BloomPass(Context* context);
    static void RegisterObject(Context* context);

    /// Implement RenderPass.
    /// @{
    void CollectParameters(StringVariantMap& params) const override;
    void InitializeView(RenderPipelineView* view) override;
    void UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params) override;
    void Update(const SharedRenderPassState& sharedState) override;
    void Render(const SharedRenderPassState& sharedState) override;
    /// @}

protected:
    static constexpr unsigned MaxIterations = 16;
    struct Parameters
    {
        unsigned numIterations_{5};
        float minBrightness_{0.8f};
        float maxBrightness_{1.0f};
        float intensity_{1.0f};
        float iterationFactor_{1.0f};

        /// Utility operators
        /// @{
        auto Tie() const { return ea::tie(numIterations_, minBrightness_, maxBrightness_, intensity_, iterationFactor_); }
        bool operator==(const Parameters& rhs) const { return Tie() == rhs.Tie(); }
        bool operator!=(const Parameters& rhs) const { return Tie() != rhs.Tie(); }
        /// @}
    };

    void InvalidateTextureCache();
    void InvalidatePipelineStateCache();
    void RestoreTextureCache(const SharedRenderPassState& sharedState);
    void RestorePipelineStateCache(const SharedRenderPassState& sharedState);

    unsigned GatherBrightRegions(RenderBufferManager* renderBufferManager, RenderBuffer* destination);
    void BlurTexture(RenderBufferManager* renderBufferManager, RenderBuffer* final, RenderBuffer* temporary);
    void CopyTexture(RenderBufferManager* renderBufferManager, RenderBuffer* source, RenderBuffer* destination);
    void ApplyBloom(RenderBufferManager* renderBufferManager, RenderBuffer* bloom, float intensity);

    auto GetShaderParameters(const Vector2& inputInvSize) const
    {
        const float thresholdGap = ea::max(0.01f, parameters_.maxBrightness_ - parameters_.minBrightness_);
        const ShaderParameterDesc result[] = {
            {Bloom_LuminanceWeights, luminanceWeights_},
            {Bloom_Threshold, Vector2(parameters_.minBrightness_, 1.0f / thresholdGap)},
            {Bloom_InputInvSize, inputInvSize},
        };
        return ea::to_array(result);
    }

    struct PipelineStateCache
    {
        StaticPipelineStateId bright_{};
        StaticPipelineStateId blurV_{};
        StaticPipelineStateId blurH_{};
        StaticPipelineStateId bloom_{};
    };
    ea::optional<PipelineStateCache> pipelineStates_;

    struct TextureCache
    {
        SharedPtr<RenderBuffer> final_;
        SharedPtr<RenderBuffer> temporary_;
    };
    ea::vector<TextureCache> textures_;

    bool isHDR_{};
    Parameters parameters_;
    Vector3 luminanceWeights_;
    ea::vector<float> intensityMultipliers_;
};

} // namespace Urho3D
