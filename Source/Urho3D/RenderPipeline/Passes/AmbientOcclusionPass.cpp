// Copyright (c) 2022-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/Passes/AmbientOcclusionPass.h"

#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderPipeline/RenderBufferManager.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Resource/ResourceCache.h"

namespace Urho3D
{

namespace
{

static const ea::string comment{"Screen-space ambient occlusion"};

static const ea::string downscaleName{"SSAO: Downscale"};
static const ea::string strengthName{"SSAO: Strength"};
static const ea::string exponentName{"SSAO: Exponent"};
static const ea::string radiusNearName{"SSAO: Near Radius"};
static const ea::string distanceNearName{"SSAO: Near Distance"};
static const ea::string radiusFarName{"SSAO: Far Radius"};
static const ea::string distanceFarName{"SSAO: Far Distance"};
static const ea::string fadeDistanceBeginName{"SSAO: Begin Fade Distance"};
static const ea::string fadeDistanceEndName{"SSAO: End Fade Distance"};
static const ea::string blurDepthThresholdName{"SSAO: Depth Threshold"};
static const ea::string blurNormalThresholdName{"SSAO: Normal Threshold"};

} // namespace

AmbientOcclusionPass::AmbientOcclusionPass(Context* context)
    : RenderPass(context)
{
    SetComment(comment);
}

void AmbientOcclusionPass::RegisterObject(Context* context)
{
    context->AddFactoryReflection<AmbientOcclusionPass>(Category_RenderPass);

    URHO3D_COPY_BASE_ATTRIBUTES(RenderPass);
    URHO3D_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Comment", comment);
}

void AmbientOcclusionPass::CollectParameters(StringVariantMap& params) const
{
    DeclareParameter(downscaleName, Parameters{}.downscale_, params);
    DeclareParameter(strengthName, Parameters{}.strength_, params);
    DeclareParameter(exponentName, Parameters{}.exponent_, params);
    DeclareParameter(radiusNearName, Parameters{}.radiusNear_, params);
    DeclareParameter(distanceNearName, Parameters{}.distanceNear_, params);
    DeclareParameter(radiusFarName, Parameters{}.radiusFar_, params);
    DeclareParameter(distanceFarName, Parameters{}.distanceFar_, params);
    DeclareParameter(fadeDistanceBeginName, Parameters{}.fadeDistanceBegin_, params);
    DeclareParameter(fadeDistanceEndName, Parameters{}.fadeDistanceEnd_, params);
    DeclareParameter(blurDepthThresholdName, Parameters{}.blurDepthThreshold_, params);
    DeclareParameter(blurNormalThresholdName, Parameters{}.blurNormalThreshold_, params);
}

void AmbientOcclusionPass::InitializeView(RenderPipelineView* view)
{
}

void AmbientOcclusionPass::UpdateParameters(const RenderPipelineSettings& settings, const StringVariantMap& params)
{
    if (!settings.renderBufferManager_.readableDepth_)
    {
        isEnabledInternally_ = false;
        return;
    }

    Parameters newParameters;

    if (const Variant& value = LoadParameter(downscaleName, params); value.GetType() == VAR_INT)
        newParameters.downscale_ = value.GetUInt();
    if (const Variant& value = LoadParameter(strengthName, params); value.GetType() == VAR_FLOAT)
        newParameters.strength_ = value.GetFloat();
    if (const Variant& value = LoadParameter(exponentName, params); value.GetType() == VAR_FLOAT)
        newParameters.exponent_ = value.GetFloat();
    if (const Variant& value = LoadParameter(radiusNearName, params); value.GetType() == VAR_FLOAT)
        newParameters.radiusNear_ = value.GetFloat();
    if (const Variant& value = LoadParameter(distanceNearName, params); value.GetType() == VAR_FLOAT)
        newParameters.distanceNear_ = value.GetFloat();
    if (const Variant& value = LoadParameter(radiusFarName, params); value.GetType() == VAR_FLOAT)
        newParameters.radiusFar_ = value.GetFloat();
    if (const Variant& value = LoadParameter(distanceFarName, params); value.GetType() == VAR_FLOAT)
        newParameters.distanceFar_ = value.GetFloat();
    if (const Variant& value = LoadParameter(fadeDistanceBeginName, params); value.GetType() == VAR_FLOAT)
        newParameters.fadeDistanceBegin_ = value.GetFloat();
    if (const Variant& value = LoadParameter(fadeDistanceEndName, params); value.GetType() == VAR_FLOAT)
        newParameters.fadeDistanceEnd_ = value.GetFloat();
    if (const Variant& value = LoadParameter(blurDepthThresholdName, params); value.GetType() == VAR_FLOAT)
        newParameters.blurDepthThreshold_ = value.GetFloat();
    if (const Variant& value = LoadParameter(blurNormalThresholdName, params); value.GetType() == VAR_FLOAT)
        newParameters.blurNormalThreshold_ = value.GetFloat();

    if (parameters_.downscale_ != newParameters.downscale_)
        InvalidateTextureCache();

    isEnabledInternally_ = true;
    parameters_ = newParameters;
}

void AmbientOcclusionPass::Update(const SharedRenderPassState& sharedState)
{
    RestoreTextureCache(sharedState);
}

void AmbientOcclusionPass::Render(const SharedRenderPassState& sharedState)
{
    ConnectToRenderBuffer(normalBuffer_, SharedRenderPassState::NormalBufferId, sharedState, false);

    RestorePipelineStateCache(sharedState);
    if (parameters_.strength_ <= 0.0f)
        return;

    if (sharedState.renderBufferManager_->GetDepthStencilTexture()->GetParams().multiSample_ != 1)
    {
        URHO3D_LOGERROR("AmbientOcclusionPass: MSAA is not supported");
        return;
    }

    if (!sharedState.renderCamera_)
    {
        URHO3D_LOGERROR("AmbientOcclusionPass: Render camera is not available");
        return;
    }

    // Convert texture coordinates into clip space
    Matrix4 clipToTextureSpace = Matrix4::IDENTITY;
    clipToTextureSpace.SetScale(Vector3(0.5f, 0.5f, 1.0f));
    clipToTextureSpace.SetTranslation(Vector3(0.5f, 0.5f, 0.0f));

    const Matrix4 viewToTextureSpace = clipToTextureSpace * sharedState.renderCamera_->GetGPUProjection(true);
    const Matrix4 textureToViewSpace = viewToTextureSpace.Inverse();

    // TODO: Support preview
    EvaluateAO(sharedState.renderBufferManager_, sharedState.renderCamera_, viewToTextureSpace, textureToViewSpace);
    BlurTexture(sharedState.renderBufferManager_, textureToViewSpace);
    Blit(sharedState.renderBufferManager_, pipelineStates_->combine_);
}

void AmbientOcclusionPass::InvalidateTextureCache()
{
    textures_ = ea::nullopt;
}

void AmbientOcclusionPass::InvalidatePipelineStateCache()
{
    pipelineStates_ = ea::nullopt;
}

void AmbientOcclusionPass::RestoreTextureCache(const SharedRenderPassState& sharedState)
{
    if (textures_)
        return;

    textures_ = TextureCache{};

    const auto cache = context_->GetSubsystem<ResourceCache>();
    textures_->noise_ = cache->GetResource<Texture2D>("Textures/SSAONoise.png");

    const Vector2 sizeMultiplier = Vector2::ONE / static_cast<float>(1 << parameters_.downscale_);
    const RenderBufferParams params{TextureFormat::TEX_FORMAT_RGBA8_UNORM, 1, RenderBufferFlag::BilinearFiltering};
    textures_->currentTarget_ = sharedState.renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    textures_->previousTarget_ = sharedState.renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
}

void AmbientOcclusionPass::RestorePipelineStateCache(const SharedRenderPassState& sharedState)
{
    if (pipelineStates_)
        return;

    pipelineStates_ = PipelineStateCache{};

    static const NamedSamplerStateDesc ssaoSamplers[] = {
        {ShaderResources::Albedo, SamplerStateDesc::Bilinear()},
        {ShaderResources::Normal, SamplerStateDesc::Bilinear()},
        {ShaderResources::DepthBuffer, SamplerStateDesc::Nearest()},
    };
    static const NamedSamplerStateDesc blurSamplers[] = {
        {ShaderResources::Albedo, SamplerStateDesc::Bilinear()},
        {ShaderResources::Normal, SamplerStateDesc::Bilinear()},
        {ShaderResources::DepthBuffer, SamplerStateDesc::Nearest()},
    };
    static const NamedSamplerStateDesc applySamplers[] = {
        {ShaderResources::Albedo, SamplerStateDesc::Bilinear()},
    };

    pipelineStates_ = PipelineStateCache{};

    RenderBufferManager* renderBufferManager = sharedState.renderBufferManager_;

    pipelineStates_->ssaoForward_ =
        renderBufferManager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION", ssaoSamplers);
    pipelineStates_->ssaoDeferred_ = renderBufferManager->CreateQuadPipelineState(
        BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION DEFERRED", ssaoSamplers);
    pipelineStates_->blurForward_ =
        renderBufferManager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "BLUR", blurSamplers);
    pipelineStates_->blurDeferred_ =
        renderBufferManager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "BLUR DEFERRED", blurSamplers);
    pipelineStates_->combine_ =
        renderBufferManager->CreateQuadPipelineState(BLEND_ALPHA, "v2/P_SSAO", "COMBINE", applySamplers);
    pipelineStates_->preview_ =
        renderBufferManager->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "PREVIEW", applySamplers);
}

void AmbientOcclusionPass::EvaluateAO(RenderBufferManager* renderBufferManager, Camera* camera,
    const Matrix4& viewToTextureSpace, const Matrix4& textureToViewSpace)
{
    static const Matrix4 flipMatrix{
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    auto renderDevice = GetSubsystem<RenderDevice>();
    const bool isOpenGL = renderDevice->GetBackend() == RenderBackend::OpenGL;

    const Vector2 inputInvSize = renderBufferManager->GetInvOutputSize();

    const bool invertY = isOpenGL == camera->GetFlipVertical();
    const Matrix4 worldToViewSpace = camera->GetView().ToMatrix4();
    const Matrix4 worldToViewSpaceCorrected = invertY ? flipMatrix * worldToViewSpace : worldToViewSpace;

    Vector4 radiusInfo;
    radiusInfo.x_ = (parameters_.radiusFar_ - parameters_.radiusNear_)
        / ea::max(1.0f, parameters_.distanceFar_ - parameters_.distanceNear_);
    radiusInfo.y_ = parameters_.radiusNear_ - radiusInfo.x_ * parameters_.distanceNear_;
    radiusInfo.z_ = parameters_.radiusNear_;
    radiusInfo.w_ = parameters_.radiusFar_;

    const ShaderParameterDesc shaderParameters[] = {
        {"InputInvSize"_sh, inputInvSize},
        {"BlurStep"_sh, inputInvSize},
        {"Strength"_sh, parameters_.strength_},
        {"Exponent"_sh, parameters_.exponent_},
        {"RadiusInfo"_sh, radiusInfo},
        {"FadeDistance"_sh, Vector2{parameters_.fadeDistanceBegin_, parameters_.fadeDistanceEnd_}},
        {"ViewToTexture"_sh, viewToTextureSpace},
        {"TextureToView"_sh, textureToViewSpace},
        {"WorldToView"_sh, worldToViewSpaceCorrected},
    };

    const ShaderResourceDesc shaderResources[] = {
        {ShaderResources::DepthBuffer, renderBufferManager->GetDepthStencilTexture()},
        {ShaderResources::Albedo, textures_->noise_},
        {ShaderResources::Normal, normalBuffer_ ? normalBuffer_->GetTexture() : nullptr}};

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager->GetDefaultClipToUVSpaceOffsetAndScale();

    renderBufferManager->SetRenderTargets(nullptr, {textures_->currentTarget_});
    if (normalBuffer_)
    {
        drawParams.pipelineStateId_ = pipelineStates_->ssaoDeferred_;
    }
    else
    {
        drawParams.pipelineStateId_ = pipelineStates_->ssaoForward_;
    }
    renderBufferManager->DrawQuad("Apply SSAO", drawParams);
    // renderBufferManager->SetOutputRenderTargets();

    ea::swap(textures_->currentTarget_, textures_->previousTarget_);
}

void AmbientOcclusionPass::BlurTexture(RenderBufferManager* renderBufferManager, const Matrix4& textureToViewSpace)
{
    const IntVector2 textureSize = textures_->currentTarget_->GetTexture()->GetParams().size_.ToIntVector2();
    const Vector2 blurStep = Vector2::ONE / textureSize.ToVector2();

    ShaderParameterDesc shaderParameters[] = {
        {"BlurStep"_sh, blurStep},
        {"BlurZThreshold"_sh, parameters_.blurDepthThreshold_},
        {"BlurNormalInvThreshold"_sh, 1.0f - parameters_.blurNormalThreshold_},
        {"TextureToView"_sh, textureToViewSpace},
    };

    DrawQuadParams drawParams;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager->GetDefaultClipToUVSpaceOffsetAndScale();
    if (normalBuffer_)
        drawParams.pipelineStateId_ = pipelineStates_->blurDeferred_;
    else
        drawParams.pipelineStateId_ = pipelineStates_->blurForward_;

    {
        renderBufferManager->SetRenderTargets(nullptr, {textures_->currentTarget_});
        const ShaderResourceDesc shaderResources[] = {
            {ShaderResources::Albedo, textures_->previousTarget_->GetTexture()},
            {ShaderResources::DepthBuffer, renderBufferManager->GetDepthStencilTexture()},
            {ShaderResources::Normal, normalBuffer_ ? normalBuffer_->GetTexture() : nullptr}};
        drawParams.resources_ = shaderResources;
        shaderParameters[0].value_ = Vector2(blurStep.x_, 0.0f);
        renderBufferManager->DrawQuad("SSAO Blur Horizontally", drawParams);
        ea::swap(textures_->currentTarget_, textures_->previousTarget_);
    }

    {
        renderBufferManager->SetRenderTargets(nullptr, {textures_->currentTarget_});
        const ShaderResourceDesc shaderResources[] = {
            {ShaderResources::Albedo, textures_->previousTarget_->GetTexture()},
            {ShaderResources::DepthBuffer, renderBufferManager->GetDepthStencilTexture()},
            {ShaderResources::Normal, normalBuffer_ ? normalBuffer_->GetTexture() : nullptr}};
        drawParams.resources_ = shaderResources;
        shaderParameters[0].value_ = Vector2(0.0f, blurStep.y_);
        renderBufferManager->DrawQuad("SSAO Blur Vertically", drawParams);
        ea::swap(textures_->currentTarget_, textures_->previousTarget_);
    }
}

void AmbientOcclusionPass::Blit(RenderBufferManager* renderBufferManager, StaticPipelineStateId pipelineStateId)
{
    renderBufferManager->SetOutputRenderTargets();

    const ShaderResourceDesc shaderResources[] = {{ShaderResources::Albedo, textures_->previousTarget_->GetTexture()}};

    renderBufferManager->DrawViewportQuad("SSAO Combine", pipelineStateId, shaderResources, {});
}

} // namespace Urho3D
