// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderPipeline/RenderPass.h"
#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"

#include <EASTL/optional.h>

namespace Urho3D
{

/// Post-processing pass that converts HDR linear color input to LDR gamma color.
class URHO3D_API ToneMappingPass : public RenderPass
{
    URHO3D_OBJECT(ToneMappingPass, RenderPass);

public:
    enum class Mode
    {
        None,
        Reinhard,
        ReinhardWhite,
        Uncharted2,

        Count
    };

    explicit ToneMappingPass(Context* context);

    static void RegisterObject(Context* context);

    /// Implement RenderPass.
    /// @{
    void CollectParameters(StringVariantMap& params) const override;
    void InitializeView(RenderPipelineView* view) override;
    void UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params) override;
    void Render(const SharedRenderPassState& sharedState) override;
    /// @}

protected:
    void InvalidateCache();
    void RestoreCache(const SharedRenderPassState& sharedState);

    struct PipelineStateCache
    {
        StaticPipelineStateId default_{};
    };
    ea::optional<PipelineStateCache> pipelineStates_;

    Mode mode_{};
};

} // namespace Urho3D
