// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/Passes/ToneMappingPass.h"

#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/RenderPipeline/RenderBufferManager.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

static const ea::string comment{"Convert color from linear HDR space to gamma LDR space"};
static const ea::string modeName{"Tone Mapping: Mode"};
static const ea::string modeMetadataName{"Tone Mapping: Mode@"};

static const StringVector modeMetadata{
    "None",
    "Reinhard",
    "ReinhardWhite",
    "Uncharted2",
};

} // namespace

ToneMappingPass::ToneMappingPass(Context* context)
    : RenderPass(context)
{
    SetComment(comment);
}

void ToneMappingPass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ToneMappingPass>(Category_RenderPass);

    URHO3D_COPY_BASE_ATTRIBUTES(RenderPass);
    URHO3D_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Comment", comment);
}

void ToneMappingPass::CollectParameters(StringVariantMap& params) const
{
    if (!params.contains(modeName))
        params[modeName] = static_cast<int>(Mode::None);
    if (!params.contains(modeMetadataName))
        params[modeMetadataName] = modeMetadata;
}

void ToneMappingPass::InitializeView(RenderPipelineView* view)
{
}

void ToneMappingPass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
    if (settings.renderBufferManager_.colorSpace_ != RenderPipelineColorSpace::LinearHDR)
    {
        isEnabledInternally_ = false;
        InvalidateCache();
        return;
    }

    isEnabledInternally_ = true;

    const auto iter = params.find(modeName);
    if (iter != params.end())
    {
        const auto numModes = static_cast<int>(Mode::Count);
        const int value = iter->second.GetInt();
        if (value >= 0 && value < numModes)
        {
            const auto newMode = static_cast<Mode>(value);
            if (mode_ != newMode)
            {
                mode_ = newMode;
                InvalidateCache();
            }
        }
    }
}

void ToneMappingPass::InvalidateCache()
{
    cache_ = ea::nullopt;
}

void ToneMappingPass::RestoreCache(const SharedRenderPassState& sharedState)
{
    if (cache_)
        return;

    cache_ = Cache{};

    ea::string defines;
    switch (mode_)
    {
    case Mode::Reinhard: defines += "REINHARD "; break;
    case Mode::ReinhardWhite: defines += "REINHARDWHITE "; break;
    case Mode::Uncharted2: defines += "UNCHARTED2 "; break;
    default: break;
    }

    static const NamedSamplerStateDesc samplers[] = {{ShaderResources::Albedo, SamplerStateDesc::Bilinear()}};
    cache_->pipelineStateId_ =
        sharedState.renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_ToneMapping", defines, samplers);
}

void ToneMappingPass::Execute(const SharedRenderPassState& sharedState)
{
    RestoreCache(sharedState);

    sharedState.renderBufferManager_->SwapColorBuffers(false);

    sharedState.renderBufferManager_->SetOutputRenderTargets();
    sharedState.renderBufferManager_->DrawFeedbackViewportQuad("Apply tone mapping", cache_->pipelineStateId_, {}, {});
}

} // namespace Urho3D
