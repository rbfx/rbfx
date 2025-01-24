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

/// Post-processing pass that adjusts HDR scene exposure.
class URHO3D_API AutoExposurePass : public RenderPass
{
    URHO3D_OBJECT(AutoExposurePass, RenderPass);

public:
    explicit AutoExposurePass(Context* context);
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
    struct Parameters
    {
        bool autoExposure_{};
        float minExposure_{1.0f};
        float maxExposure_{3.0f};
        float adaptRate_{0.6f};

        /// Utility operators
        /// @{
        auto Tie() const { return ea::tie(autoExposure_, minExposure_, maxExposure_, adaptRate_); }
        bool operator==(const Parameters& rhs) const { return Tie() == rhs.Tie(); }
        bool operator!=(const Parameters& rhs) const { return Tie() != rhs.Tie(); }
        /// @}
    };

    void InvalidateTextureCache();
    void InvalidatePipelineStateCache();
    void RestoreTextureCache(const SharedRenderPassState& sharedState);
    void RestorePipelineStateCache(const SharedRenderPassState& sharedState);

    void EvaluateDownsampledColorBuffer(RenderBufferManager* renderBufferManager);
    void EvaluateLuminance(RenderBufferManager* renderBufferManager);
    void EvaluateAdaptedLuminance(RenderBufferManager* renderBufferManager);

    bool isAdaptedLuminanceInitialized_{};
    Parameters parameters_;

    struct CachedTextures
    {
        SharedPtr<RenderBuffer> color128_;
        SharedPtr<RenderBuffer> lum64_;
        SharedPtr<RenderBuffer> lum16_;
        SharedPtr<RenderBuffer> lum4_;
        SharedPtr<RenderBuffer> lum1_;
        SharedPtr<RenderBuffer> adaptedLum_;
        SharedPtr<RenderBuffer> prevAdaptedLum_;
    };
    ea::optional<CachedTextures> textures_;

    struct CachedStates
    {
        StaticPipelineStateId lum64_{};
        StaticPipelineStateId lum16_{};
        StaticPipelineStateId lum4_{};
        StaticPipelineStateId lum1_{};
        StaticPipelineStateId adaptedLum_{};
        StaticPipelineStateId autoExposure_{};
    };
    ea::optional<CachedStates> pipelineStates_;
};

} // namespace Urho3D
