// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/Passes/AutoExposurePass.h"

#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderPipeline/RenderBufferManager.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

static const ea::string comment{"Adjust exposure of the camera within specified range"};

static const ea::string autoExposureName{"Exposure: Automatic"};
static const ea::string minExposureName{"Exposure: Min"};
static const ea::string maxExposureName{"Exposure: Max"};
static const ea::string adaptRateName{"Exposure: Adapt Rate"};

} // namespace

AutoExposurePass::AutoExposurePass(Context* context)
    : RenderPass(context)
{
    traits_.needReadWriteColorBuffer_ = true;
    SetComment(comment);
}

void AutoExposurePass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<AutoExposurePass>(Category_RenderPass);

    URHO3D_COPY_BASE_ATTRIBUTES(RenderPass);
    URHO3D_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Comment", comment);
}

void AutoExposurePass::CollectParameters(StringVariantMap& params) const
{
    DeclareParameter(autoExposureName, Parameters{}.autoExposure_, params);
    DeclareParameter(minExposureName, Parameters{}.minExposure_, params);
    DeclareParameter(maxExposureName, Parameters{}.maxExposure_, params);
    DeclareParameter(adaptRateName, Parameters{}.adaptRate_, params);
}

void AutoExposurePass::InitializeView(RenderPipelineView* view)
{
    auto renderDevice = GetSubsystem<RenderDevice>();
    renderDevice->OnDeviceRestored.Subscribe(this, [&] { isAdaptedLuminanceInitialized_ = false; });
}

void AutoExposurePass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
    if (settings.renderBufferManager_.colorSpace_ != RenderPipelineColorSpace::LinearHDR)
    {
        isEnabledInternally_ = false;
        return;
    }

    Parameters newParameters;

    if (const Variant& value = LoadParameter(autoExposureName, params); value.GetType() == VAR_BOOL)
        newParameters.autoExposure_ = value.GetBool();
    if (const Variant& value = LoadParameter(minExposureName, params); value.GetType() == VAR_FLOAT)
        newParameters.minExposure_ = value.GetFloat();
    if (const Variant& value = LoadParameter(maxExposureName, params); value.GetType() == VAR_FLOAT)
        newParameters.maxExposure_ = value.GetFloat();
    if (const Variant& value = LoadParameter(adaptRateName, params); value.GetType() == VAR_FLOAT)
        newParameters.adaptRate_ = value.GetFloat();

    if (parameters_.autoExposure_ != newParameters.autoExposure_)
    {
        InvalidateTextureCache();
        InvalidatePipelineStateCache();
    }

    isEnabledInternally_ = true;
    parameters_ = newParameters;
}

void AutoExposurePass::Update(const SharedRenderPassState& sharedState)
{
    RestoreTextureCache(sharedState);
}

void AutoExposurePass::Render(const SharedRenderPassState& sharedState)
{
    RestorePipelineStateCache(sharedState);

    RenderBufferManager* manager = sharedState.renderBufferManager_;
    manager->SwapColorBuffers(false);

    if (parameters_.autoExposure_)
    {
        EvaluateDownsampledColorBuffer(manager);
        EvaluateLuminance(manager);
        EvaluateAdaptedLuminance(manager);
    }

    const ShaderResourceDesc shaderResources[] = {
        {ShaderResources::Normal, parameters_.autoExposure_ ? textures_->adaptedLum_->GetTexture() : nullptr}};
    const ShaderParameterDesc shaderParameters[] = {
        {"MinMaxExposure", Vector2(parameters_.minExposure_, parameters_.maxExposure_)},
        {"AutoExposureMiddleGrey", 0.6f},
    };
    manager->SetOutputRenderTargets();
    manager->DrawFeedbackViewportQuad(
        "Apply exposure", pipelineStates_->autoExposure_, shaderResources, shaderParameters);
}

void AutoExposurePass::InvalidateTextureCache()
{
    textures_ = ea::nullopt;
}

void AutoExposurePass::InvalidatePipelineStateCache()
{
    pipelineStates_ = ea::nullopt;
}

void AutoExposurePass::RestoreTextureCache(const SharedRenderPassState& sharedState)
{
    if (textures_)
        return;

    textures_ = CachedTextures{};

    if (!parameters_.autoExposure_)
        return;

    const RenderBufferFlags flagFixedBilinear =
        RenderBufferFlag::BilinearFiltering | RenderBufferFlag::FixedTextureSize;
    const RenderBufferFlags flagFixedNearest = RenderBufferFlag::FixedTextureSize;
    const RenderBufferFlags flagFixedNearestPersistent =
        RenderBufferFlag::FixedTextureSize | RenderBufferFlag::Persistent;
    const auto rgbaFormat = TextureFormat::TEX_FORMAT_RGBA16_FLOAT;
    const auto rgFormat = TextureFormat::TEX_FORMAT_RG16_FLOAT;

    RenderBufferManager* manager = sharedState.renderBufferManager_;
    textures_->color128_ = manager->CreateColorBuffer({rgbaFormat, 1, flagFixedBilinear}, {128, 128});
    textures_->lum64_ = manager->CreateColorBuffer({rgFormat, 1, flagFixedBilinear}, {64, 64});
    textures_->lum16_ = manager->CreateColorBuffer({rgFormat, 1, flagFixedBilinear}, {16, 16});
    textures_->lum4_ = manager->CreateColorBuffer({rgFormat, 1, flagFixedBilinear}, {4, 4});
    textures_->lum1_ = manager->CreateColorBuffer({rgFormat, 1, flagFixedNearest}, {1, 1});
    textures_->adaptedLum_ = manager->CreateColorBuffer({rgFormat, 1, flagFixedNearestPersistent}, {1, 1});
    textures_->prevAdaptedLum_ = manager->CreateColorBuffer({rgFormat, 1, flagFixedNearest}, {1, 1});
}

void AutoExposurePass::RestorePipelineStateCache(const SharedRenderPassState& sharedState)
{
    if (pipelineStates_)
        return;

    static const NamedSamplerStateDesc lumSamplers[] = {{ShaderResources::Albedo, SamplerStateDesc::Bilinear()}};
    static const NamedSamplerStateDesc adaptedLumSamplers[] = {
        {ShaderResources::Albedo, SamplerStateDesc::Bilinear()},
        {ShaderResources::Normal, SamplerStateDesc::Bilinear()},
    };
    static const NamedSamplerStateDesc applySamplers[] = {
        {ShaderResources::Albedo, SamplerStateDesc::Bilinear()},
        {ShaderResources::Normal, SamplerStateDesc::Bilinear()},
    };

    pipelineStates_ = CachedStates{};

    RenderBufferManager* manager = sharedState.renderBufferManager_;
    pipelineStates_->lum64_ =
        manager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", "LUMINANCE64", lumSamplers);
    pipelineStates_->lum16_ =
        manager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", "LUMINANCE16", lumSamplers);
    pipelineStates_->lum4_ =
        manager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", "LUMINANCE4", lumSamplers);
    pipelineStates_->lum1_ =
        manager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", "LUMINANCE1", lumSamplers);
    pipelineStates_->adaptedLum_ =
        manager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", "ADAPTLUMINANCE", adaptedLumSamplers);

    ea::string exposureDefines = "EXPOSURE ";
    if (parameters_.autoExposure_)
        exposureDefines += "AUTOEXPOSURE ";

    pipelineStates_->autoExposure_ =
        manager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_AutoExposure", exposureDefines, applySamplers);
}

void AutoExposurePass::EvaluateDownsampledColorBuffer(RenderBufferManager* renderBufferManager)
{
    RawTexture* viewportTexture = renderBufferManager->GetSecondaryColorTexture();

    renderBufferManager->SetRenderTargets(nullptr, {textures_->color128_});
    renderBufferManager->DrawTexture("Downsample color buffer", viewportTexture);
}

void AutoExposurePass::EvaluateLuminance(RenderBufferManager* renderBufferManager)
{
    ShaderResourceDesc shaderResources[1];
    shaderResources[0].name_ = ShaderResources::Albedo;
    ShaderParameterDesc shaderParameters[1];
    shaderParameters[0].name_ = "InputInvSize";

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager->GetDefaultClipToUVSpaceOffsetAndScale();

    shaderResources[0].texture_ = textures_->color128_->GetTexture();
    shaderParameters[0].value_ = Vector2::ONE / 128.0f;
    drawParams.pipelineStateId_ = pipelineStates_->lum64_;
    renderBufferManager->SetRenderTargets(nullptr, {textures_->lum64_});
    renderBufferManager->DrawQuad("Downsample luminosity buffer", drawParams);

    shaderResources[0].texture_ = textures_->lum64_->GetTexture();
    shaderParameters[0].value_ = Vector2::ONE / 64.0f;
    drawParams.pipelineStateId_ = pipelineStates_->lum16_;
    renderBufferManager->SetRenderTargets(nullptr, {textures_->lum16_});
    renderBufferManager->DrawQuad("Downsample luminosity buffer", drawParams);

    shaderResources[0].texture_ = textures_->lum16_->GetTexture();
    shaderParameters[0].value_ = Vector2::ONE / 16.0f;
    drawParams.pipelineStateId_ = pipelineStates_->lum4_;
    renderBufferManager->SetRenderTargets(nullptr, {textures_->lum4_});
    renderBufferManager->DrawQuad("Downsample luminosity buffer", drawParams);

    shaderResources[0].texture_ = textures_->lum4_->GetTexture();
    shaderParameters[0].value_ = Vector2::ONE / 4.0f;
    drawParams.pipelineStateId_ = pipelineStates_->lum1_;
    renderBufferManager->SetRenderTargets(nullptr, {textures_->lum1_});
    renderBufferManager->DrawQuad("Downsample luminosity buffer", drawParams);
}

void AutoExposurePass::EvaluateAdaptedLuminance(RenderBufferManager* renderBufferManager)
{
    renderBufferManager->SetRenderTargets(nullptr, {textures_->prevAdaptedLum_});
    RenderBuffer* sourceBuffer = isAdaptedLuminanceInitialized_ ? textures_->adaptedLum_ : textures_->lum1_;
    renderBufferManager->DrawTexture("Store previous luminance", sourceBuffer->GetTexture());

    const ShaderResourceDesc shaderResources[] = {{ShaderResources::Albedo, textures_->prevAdaptedLum_->GetTexture()},
        {ShaderResources::Normal, textures_->lum1_->GetTexture()}};
    const ShaderParameterDesc shaderParameters[] = {
        {"AdaptRate", parameters_.adaptRate_},
    };

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.pipelineStateId_ = pipelineStates_->adaptedLum_;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager->GetDefaultClipToUVSpaceOffsetAndScale();
    renderBufferManager->SetRenderTargets(nullptr, {textures_->adaptedLum_});
    renderBufferManager->DrawQuad("Adapt luminosity", drawParams);

    isAdaptedLuminanceInitialized_ = true;
}

} // namespace Urho3D
