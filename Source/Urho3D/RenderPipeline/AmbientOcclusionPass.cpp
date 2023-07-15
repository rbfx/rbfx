//
// Copyright (c) 2022-2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "AmbientOcclusionPass.h"

#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Texture2D.h"
#include "../Resource/ResourceCache.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderPipeline/RenderBufferManager.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"

namespace Urho3D
{

AmbientOcclusionPass::AmbientOcclusionPass(
    RenderPipelineInterface* renderPipeline, RenderBufferManager* renderBufferManager)
    : PostProcessPass(renderPipeline, renderBufferManager)
{
    InitializeTextures();
}

void AmbientOcclusionPass::SetSettings(const AmbientOcclusionPassSettings& settings)
{
    if (settings_ != settings)
    {
        const bool resetCachedTextures = settings_.downscale_ != settings.downscale_;
        settings_ = settings;
        if (resetCachedTextures)
            InitializeTextures();
    }
}

void AmbientOcclusionPass::InitializeTextures()
{
    const Vector2 sizeMultiplier = Vector2::ONE / static_cast<float>(1 << settings_.downscale_);
    const RenderBufferParams params{TextureFormat::TEX_FORMAT_RGBA8_UNORM, 1, RenderBufferFlag::BilinearFiltering};
    textures_.currentTarget_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    textures_.previousTarget_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    textures_.noise_ = context_->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/SSAONoise.png");
}


void AmbientOcclusionPass::InitializeStates()
{
    static const NamedSamplerStateDesc ssaoSamplers[] = {
        {ShaderResources::DiffMap, SamplerStateDesc::Bilinear()},
        {ShaderResources::NormalMap, SamplerStateDesc::Bilinear()},
        {ShaderResources::DepthBuffer, SamplerStateDesc::Bilinear()},
    };
    static const NamedSamplerStateDesc blurSamplers[] = {
        {ShaderResources::DiffMap, SamplerStateDesc::Bilinear()},
        {ShaderResources::NormalMap, SamplerStateDesc::Bilinear()},
        {ShaderResources::DepthBuffer, SamplerStateDesc::Bilinear()},
    };
    static const NamedSamplerStateDesc applySamplers[] = {
        {ShaderResources::DiffMap, SamplerStateDesc::Bilinear()},
    };

    pipelineStates_ = CachedStates{};
    pipelineStates_->ssaoForward_ =
        renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION", ssaoSamplers);
    pipelineStates_->ssaoDeferred_ = renderBufferManager_->CreateQuadPipelineState(
        BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION DEFERRED", ssaoSamplers);
    pipelineStates_->blurForward_ =
        renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "BLUR", blurSamplers);
    pipelineStates_->blurDeferred_ =
        renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "BLUR DEFERRED", blurSamplers);
    pipelineStates_->combine_ =
        renderBufferManager_->CreateQuadPipelineState(BLEND_ALPHA, "v2/P_SSAO", "COMBINE", applySamplers);
    pipelineStates_->preview_ =
        renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "PREVIEW", applySamplers);
}

void AmbientOcclusionPass::EvaluateAO(Camera* camera, const Matrix4& viewToTextureSpace, const Matrix4& textureToViewSpace)
{
    static const Matrix4 flipMatrix{
        1.0f,  0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f,  0.0f, 1.0f, 0.0f,
        0.0f,  0.0f, 0.0f, 1.0f
    };

    auto renderDevice = GetSubsystem<RenderDevice>();
    const bool isOpenGL = renderDevice->GetBackend() == RenderBackend::OpenGL;

    const Vector2 inputInvSize = renderBufferManager_->GetInvOutputSize();

    const bool invertY = isOpenGL == camera->GetFlipVertical();
    const Matrix4 worldToViewSpace = camera->GetView().ToMatrix4();
    const Matrix4 worldToViewSpaceCorrected = invertY ? flipMatrix * worldToViewSpace : worldToViewSpace;

    Vector4 radiusInfo;
    radiusInfo.x_ = (settings_.radiusFar_ - settings_.radiusNear_) / ea::max(1.0f, settings_.distanceFar_ - settings_.distanceNear_);
    radiusInfo.y_ = settings_.radiusNear_ - radiusInfo.x_ * settings_.distanceNear_;
    radiusInfo.z_ = settings_.radiusNear_;
    radiusInfo.w_ = settings_.radiusFar_;

    const ShaderParameterDesc shaderParameters[] = {
        {"InputInvSize", inputInvSize},
        {"BlurStep", inputInvSize},
        {"Strength", settings_.strength_},
        {"Exponent", settings_.exponent_},
        {"RadiusInfo", radiusInfo},
        {"FadeDistance", Vector2{settings_.fadeDistanceBegin_, settings_.fadeDistanceEnd_}},
        {"ViewToTexture", viewToTextureSpace},
        {"TextureToView", textureToViewSpace},
        {"WorldToView", worldToViewSpaceCorrected},
    };

    const ShaderResourceDesc shaderResources[] = {
        {"DepthBuffer", renderBufferManager_->GetDepthStencilTexture()},
        {"DiffMap", textures_.noise_},
        {"NormalMap", normalBuffer_ ? normalBuffer_->GetTexture() : nullptr}
    };

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale();

    renderBufferManager_->SetRenderTargets(nullptr, {textures_.currentTarget_});
    if (normalBuffer_)
    {
        drawParams.pipelineStateId_ = pipelineStates_->ssaoDeferred_;
    }
    else
    {
        drawParams.pipelineStateId_ = pipelineStates_->ssaoForward_;
    }
    renderBufferManager_->DrawQuad("Apply SSAO", drawParams);
    //renderBufferManager_->SetOutputRenderTargets();

    ea::swap(textures_.currentTarget_, textures_.previousTarget_);
}

void AmbientOcclusionPass::BlurTexture(const Matrix4& textureToViewSpace)
{
    const IntVector2 textureSize = textures_.currentTarget_->GetTexture()->GetParams().size_.ToIntVector2();
    const Vector2 blurStep = Vector2::ONE / textureSize.ToVector2();

    ShaderParameterDesc shaderParameters[] = {
        {"BlurStep", blurStep},
        {"BlurZThreshold", settings_.blurDepthThreshold_},
        {"BlurNormalInvThreshold", 1.0f - settings_.blurNormalThreshold_},
        {"TextureToView", textureToViewSpace},
    };

    DrawQuadParams drawParams;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale();
    if (normalBuffer_)
        drawParams.pipelineStateId_ = pipelineStates_->blurDeferred_;
    else
        drawParams.pipelineStateId_ = pipelineStates_->blurForward_;

    {
        renderBufferManager_->SetRenderTargets(nullptr, {textures_.currentTarget_});
        const ShaderResourceDesc shaderResources[] = {
            {"DiffMap", textures_.previousTarget_->GetTexture()},
            {"DepthBuffer", renderBufferManager_->GetDepthStencilTexture()},
            {"NormalMap", normalBuffer_ ? normalBuffer_->GetTexture() : nullptr}};
        drawParams.resources_ = shaderResources;
        shaderParameters[0].value_ = Vector2(blurStep.x_, 0.0f);
        renderBufferManager_->DrawQuad("SSAO Blur Horizontally", drawParams);
        ea::swap(textures_.currentTarget_, textures_.previousTarget_);
    }

    {
        renderBufferManager_->SetRenderTargets(nullptr, {textures_.currentTarget_});
        const ShaderResourceDesc shaderResources[] = {
            {"DiffMap", textures_.previousTarget_->GetTexture()},
            {"DepthBuffer", renderBufferManager_->GetDepthStencilTexture()},
            {"NormalMap", normalBuffer_ ? normalBuffer_->GetTexture() : nullptr}};
        drawParams.resources_ = shaderResources;
        shaderParameters[0].value_ = Vector2(0.0f, blurStep.y_);
        renderBufferManager_->DrawQuad("SSAO Blur Vertically", drawParams);
        ea::swap(textures_.currentTarget_, textures_.previousTarget_);
    }
}

void AmbientOcclusionPass::Blit(StaticPipelineStateId pipelineStateId)
{
    renderBufferManager_->SetOutputRenderTargets();

    const ShaderResourceDesc shaderResources[] = {
        {"DiffMap", textures_.previousTarget_->GetTexture()}};

    renderBufferManager_->DrawViewportQuad("SSAO Combine", pipelineStateId, shaderResources, {});
}

void AmbientOcclusionPass::SetNormalBuffer(RenderBuffer* normalBuffer)
{
    normalBuffer_ = normalBuffer;
}

void AmbientOcclusionPass::Execute(Camera* camera)
{
    if (!pipelineStates_)
        InitializeStates();

    if (settings_.strength_ <= 0.0f)
        return;

    if (renderBufferManager_->GetDepthStencilTexture()->GetParams().multiSample_ != 1)
    {
        static bool logged = false;
        if (!logged)
        {
            URHO3D_LOGWARNING("AmbientOcclusionPass: MSAA is not supported");
            logged = true;
        }
        return;
    }

    // Convert texture coordinates into clip space
    Matrix4 clipToTextureSpace = Matrix4::IDENTITY;
    clipToTextureSpace.SetScale(Vector3(0.5f, 0.5f, 1.0f));
    clipToTextureSpace.SetTranslation(Vector3(0.5f, 0.5f, 0.0f));

    const Matrix4 viewToTextureSpace = clipToTextureSpace * camera->GetGPUProjection(true);
    const Matrix4 textureToViewSpace = viewToTextureSpace.Inverse();

    EvaluateAO(camera, viewToTextureSpace, textureToViewSpace);
    BlurTexture(textureToViewSpace);

    switch (settings_.ambientOcclusionMode_)
    {
    case AmbientOcclusionMode::Preview: Blit(pipelineStates_->preview_); break;
    default: Blit(pipelineStates_->combine_); break;
    }
}

} // namespace Urho3D
