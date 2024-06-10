// Copyright (c) 2022-2024 the rbfx project.
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

class RenderBufferManager;
class RenderPipelineInterface;

/// Post-processing pass that adjusts HDR scene exposure.
class URHO3D_API AmbientOcclusionPass : public RenderPass
{
    URHO3D_OBJECT(AmbientOcclusionPass, RenderPass)

public:
    explicit AmbientOcclusionPass(Context* context);
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
        unsigned downscale_{0};
        float strength_{0.7f};
        float exponent_{1.5f};

        float radiusNear_{0.05f};
        float distanceNear_{1.0f};
        float radiusFar_{1.0f};
        float distanceFar_{100.0f};

        float fadeDistanceBegin_{100.0f};
        float fadeDistanceEnd_{200.0f};

        float blurDepthThreshold_{0.1f};
        float blurNormalThreshold_{0.2f};

        /// Utility operators
        /// @{
        auto Tie() const
        {
            return ea::tie(downscale_, strength_, exponent_, radiusNear_, distanceNear_, radiusFar_, distanceFar_,
                fadeDistanceBegin_, fadeDistanceEnd_, blurDepthThreshold_, blurNormalThreshold_);
        }
        bool operator==(const Parameters& rhs) const { return Tie() == rhs.Tie(); }
        bool operator!=(const Parameters& rhs) const { return Tie() != rhs.Tie(); }
        /// @}
    };

    void InvalidateTextureCache();
    void InvalidatePipelineStateCache();
    void RestoreTextureCache(const SharedRenderPassState& sharedState);
    void RestorePipelineStateCache(const SharedRenderPassState& sharedState);

    void EvaluateAO(RenderBufferManager* renderBufferManager, Camera* camera, const Matrix4& viewToTextureSpace,
        const Matrix4& textureToViewSpace);
    void BlurTexture(RenderBufferManager* renderBufferManager, const Matrix4& textureToViewSpace);
    void Blit(RenderBufferManager* renderBufferManager, StaticPipelineStateId pipelineStateId);

    Parameters parameters_;

    WeakPtr<RenderBuffer> normalBuffer_;

    struct TextureCache
    {
        SharedPtr<Texture2D> noise_;
        SharedPtr<RenderBuffer> currentTarget_;
        SharedPtr<RenderBuffer> previousTarget_;
    };
    ea::optional<TextureCache> textures_;

    struct PipelineStateCache
    {
        StaticPipelineStateId ssaoForward_{};
        StaticPipelineStateId ssaoDeferred_{};
        StaticPipelineStateId blurForward_{};
        StaticPipelineStateId blurDeferred_{};
        StaticPipelineStateId combine_{};
        StaticPipelineStateId preview_{};
    };
    ea::optional<PipelineStateCache> pipelineStates_;
};

} // namespace Urho3D

