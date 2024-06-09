// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/Passes/FullScreenShaderPass.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/Technique.h"
#include "Urho3D/RenderPipeline/RenderBufferManager.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

FullScreenShaderPass::FullScreenShaderPass(Context* context)
    : RenderPass(context)
{
}

void FullScreenShaderPass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<FullScreenShaderPass>(Category_RenderPass);

    // clang-format off
    URHO3D_COPY_BASE_ATTRIBUTES(RenderPass);
    URHO3D_ATTRIBUTE_EX("Shader Name", ea::string, attributes_.shaderName_, InvalidateCache, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Shader Defines", ea::string, attributes_.shaderDefines_, InvalidateCache, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE_EX("Blend Mode", attributes_.blendMode_, InvalidateCache, blendModeNames, BLEND_REPLACE, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Read+Write Color Output", bool, attributes_.needReadWriteColorBuffer_, InvalidateCache, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bilinear Color Sampler", bool, attributes_.needBilinearColorSampler_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Disable On Default Parameters", bool, attributes_.disableOnDefaultParameters_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Parameters Prefix", ea::string, attributes_.parametersPrefix_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Parameters", StringVariantMap, attributes_.parameters_, Variant::emptyStringVariantMap, AM_DEFAULT);
    // clang-format on
}

void FullScreenShaderPass::CollectParameters(StringVariantMap& params) const
{
    for (const auto& [name, defaultValue] : attributes_.parameters_)
    {
        const ea::string externalName = attributes_.parametersPrefix_ + name;
        if (!params.contains(externalName))
            params[externalName] = defaultValue;
    }
}

void FullScreenShaderPass::InitializeView(RenderPipelineView* view)
{
    shaderParametersSources_.clear();
    shaderParametersDefaults_.clear();
    shaderParameters_.clear();
    shaderResources_.clear();

    for (const auto& [name, defaultValue] : attributes_.parameters_)
    {
        const ea::string externalName = attributes_.parametersPrefix_ + name;

        shaderParametersSources_.push_back(StringHash{externalName});
        shaderParametersDefaults_.push_back(defaultValue);
        shaderParameters_.push_back(ShaderParameterDesc{name, defaultValue});
    }
}

void FullScreenShaderPass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
    const unsigned numShaderParameters = shaderParameters_.size();
    bool shouldBeDisabled = attributes_.disableOnDefaultParameters_;
    for (unsigned i = 0; i < numShaderParameters; ++i)
    {
        const auto iter = params.find_by_hash(shaderParametersSources_[i].Value());
        if (iter != params.end())
        {
            shaderParameters_[i].value_ = iter->second;
            if (shouldBeDisabled && iter->second != shaderParametersDefaults_[i])
                shouldBeDisabled = false;
        }
    }

    traits_.needReadWriteColorBuffer_ = attributes_.needReadWriteColorBuffer_;
    traits_.needBilinearColorSampler_ = attributes_.needBilinearColorSampler_;
    isEnabledInternally_ = !shouldBeDisabled;
}

void FullScreenShaderPass::InvalidateCache()
{
    cache_ = ea::nullopt;
}

void FullScreenShaderPass::RestoreCache(const SharedRenderPassState& sharedState)
{
    if (cache_)
        return;

    cache_ = Cache{};
    cache_->debugComment_ = Format("Apply shader \"{}\"({})", attributes_.shaderName_, attributes_.shaderDefines_);

    ea::vector<NamedSamplerStateDesc> samplersAdjusted;

    if (attributes_.needReadWriteColorBuffer_)
        samplersAdjusted.emplace_back(ShaderResources::Albedo, SamplerStateDesc::Bilinear());

    cache_->pipelineStateId_ = sharedState.renderBufferManager_->CreateQuadPipelineState(
        attributes_.blendMode_, attributes_.shaderName_, attributes_.shaderDefines_, samplersAdjusted);
}

void FullScreenShaderPass::Render(const SharedRenderPassState& sharedState)
{
    RestoreCache(sharedState);

    if (attributes_.needReadWriteColorBuffer_)
        sharedState.renderBufferManager_->SwapColorBuffers(false);
    sharedState.renderBufferManager_->SetOutputRenderTargets();

    if (attributes_.needReadWriteColorBuffer_)
    {
        sharedState.renderBufferManager_->DrawFeedbackViewportQuad(
            cache_->debugComment_, cache_->pipelineStateId_, shaderResources_, shaderParameters_);
    }
    else
    {
        sharedState.renderBufferManager_->DrawViewportQuad(
            cache_->debugComment_, cache_->pipelineStateId_, shaderResources_, shaderParameters_);
    }
}

} // namespace Urho3D
