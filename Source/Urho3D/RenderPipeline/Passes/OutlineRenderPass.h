// Copyright (c) 2022-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderPipeline/RenderBuffer.h"
#include "Urho3D/RenderPipeline/RenderPass.h"
#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"

namespace Urho3D
{

/// Post-processing pass that renders outline from color buffer.
class URHO3D_API OutlineRenderPass : public RenderPass
{
    URHO3D_OBJECT(OutlineRenderPass, RenderPass);

public:
    static constexpr StringHash ColorBufferId = "outline"_sh;

    explicit OutlineRenderPass(Context* context);

    static void RegisterObject(Context* context);

    /// Implement RenderPass.
    /// @{
    void CollectParameters(StringVariantMap& params) const override;
    void InitializeView(RenderPipelineView* view) override;
    void UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params) override;
    void Render(const SharedRenderPassState& sharedState) override;
    /// @}

private:
    void InvalidateCache();
    void RestoreCache(const SharedRenderPassState& sharedState);

    struct PipelineStateCache
    {
        StaticPipelineStateId gamma_{};
        StaticPipelineStateId linear_{};
    };
    ea::optional<PipelineStateCache> pipelineStates_;

    WeakPtr<RenderBuffer> colorBuffer_;
};

} // namespace Urho3D
