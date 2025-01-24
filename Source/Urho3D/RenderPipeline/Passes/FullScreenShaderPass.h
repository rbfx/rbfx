// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderAPI/RenderAPIDefs.h"
#include "Urho3D/RenderPipeline/RenderPass.h"
#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"

namespace Urho3D
{

/// Render pass that applied shader to the entire output area.
class URHO3D_API FullScreenShaderPass : public RenderPass
{
    URHO3D_OBJECT(FullScreenShaderPass, RenderPass);

public:
    explicit FullScreenShaderPass(Context* context);

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

    struct Attributes
    {
        ea::string shaderName_;
        ea::string shaderDefines_;
        BlendMode blendMode_{BLEND_REPLACE};
        bool needReadWriteColorBuffer_{};
        bool needBilinearColorSampler_{};
        bool disableOnDefaultParameters_{};
        ea::string parametersPrefix_;
        StringVariantMap parameters_;
    } attributes_;

    struct Cache
    {
        StaticPipelineStateId pipelineStateId_{};
        ea::string debugComment_;
    };
    ea::optional<Cache> cache_;

    ea::vector<StringHash> shaderParametersSources_;
    ea::vector<Variant> shaderParametersDefaults_;
    ea::vector<ShaderParameterDesc> shaderParameters_;
    ea::vector<ShaderResourceDesc> shaderResources_;
};

} // namespace Urho3D
