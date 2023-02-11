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
#include "../Graphics/Renderer.h"
#include "../Graphics/Texture2D.h"
#include "../Resource/ResourceCache.h"
#include "RenderBufferManager.h"

namespace Urho3D
{

#ifdef DESKTOP_GRAPHICS

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
    const unsigned format = Graphics::GetRGBAFormat();
    const Vector2 sizeMultiplier = Vector2::ONE / static_cast<float>(1 << settings_.downscale_);
    const RenderBufferParams params{format, 1, RenderBufferFlag::BilinearFiltering};
    textures_.currentTarget_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    textures_.previousTarget_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    textures_.noise_ = context_->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/SSAONoise.png");
}


void AmbientOcclusionPass::InitializeStates()
{
    pipelineStates_ = CachedStates{};
    pipelineStates_->ssaoForward_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION");
    pipelineStates_->ssaoDeferred_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION DEFERRED");
    pipelineStates_->blurForward_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "BLUR");
    pipelineStates_->blurDeferred_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "BLUR DEFERRED");
    pipelineStates_->combine_ = renderBufferManager_->CreateQuadPipelineState(BLEND_ALPHA, "v2/P_SSAO", "COMBINE");
    pipelineStates_->preview_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "PREVIEW");
}

void AmbientOcclusionPass::EvaluateAO(Camera* camera, const Matrix4& viewToTextureSpace, const Matrix4& textureToViewSpace)
{
    static const Matrix4 flipMatrix{
        1.0f,  0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f,  0.0f, 1.0f, 0.0f,
        0.0f,  0.0f, 0.0f, 1.0f
    };

    const Texture2D* viewportTexture = renderBufferManager_->GetSecondaryColorTexture();
    const IntVector2 inputSize = viewportTexture->GetSize();
    const Vector2 inputInvSize = Vector2::ONE / inputSize.ToVector2();

#ifdef URHO3D_OPENGL
    const bool invertY = camera->GetFlipVertical();
#else
    const bool invertY = !camera->GetFlipVertical();
#endif
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
        {TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture()},
        {TU_DIFFUSE, textures_.noise_},
        {TU_NORMAL, (normalBuffer_) ? normalBuffer_->GetTexture2D() : nullptr}
    };

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale();

    renderBufferManager_->SetRenderTargets(nullptr, {textures_.currentTarget_});
    if (normalBuffer_)
    {
        drawParams.pipelineState_ = pipelineStates_->ssaoDeferred_;
    }
    else
    {
        drawParams.pipelineState_ = pipelineStates_->ssaoForward_;
    }
    renderBufferManager_->DrawQuad("Apply SSAO", drawParams);
    //renderBufferManager_->SetOutputRenderTargets();

    ea::swap(textures_.currentTarget_, textures_.previousTarget_);
}

void AmbientOcclusionPass::BlurTexture(const Matrix4& textureToViewSpace)
{
    const IntVector2 textureSize = textures_.currentTarget_->GetTexture2D()->GetSize();
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
        drawParams.pipelineState_ = pipelineStates_->blurDeferred_;
    else
        drawParams.pipelineState_ = pipelineStates_->blurForward_;

    {
        renderBufferManager_->SetRenderTargets(nullptr, {textures_.currentTarget_});
        const ShaderResourceDesc shaderResources[] = {
            {TU_DIFFUSE, textures_.previousTarget_->GetTexture2D()},
            {TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture()},
            {TU_NORMAL, normalBuffer_ ? normalBuffer_->GetTexture2D() : nullptr}};
        drawParams.resources_ = shaderResources;
        shaderParameters[0].value_ = Vector2(blurStep.x_, 0.0f);
        renderBufferManager_->DrawQuad("SSAO Blur Horizontally", drawParams);
        ea::swap(textures_.currentTarget_, textures_.previousTarget_);
    }

    {
        renderBufferManager_->SetRenderTargets(nullptr, {textures_.currentTarget_});
        const ShaderResourceDesc shaderResources[] = {
            {TU_DIFFUSE, textures_.previousTarget_->GetTexture2D()},
            {TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture()},
            {TU_NORMAL, normalBuffer_ ? normalBuffer_->GetTexture2D() : nullptr}};
        drawParams.resources_ = shaderResources;
        shaderParameters[0].value_ = Vector2(0.0f, blurStep.y_);
        renderBufferManager_->DrawQuad("SSAO Blur Vertically", drawParams);
        ea::swap(textures_.currentTarget_, textures_.previousTarget_);
    }
}

void AmbientOcclusionPass::Blit(PipelineState* state)
{
    renderBufferManager_->SetOutputRenderTargets();

    const ShaderResourceDesc shaderResources[] = {
        {TU_DIFFUSE, textures_.previousTarget_->GetTexture2D()}};

    renderBufferManager_->DrawViewportQuad("SSAO Combine", state, shaderResources, {});
}

void AmbientOcclusionPass::SetNormalBuffer(RenderBuffer* normalBuffer)
{
    normalBuffer_ = normalBuffer;
}

void AmbientOcclusionPass::Execute(Camera* camera)
{
    if (!pipelineStates_)
        InitializeStates();

    if (!pipelineStates_->IsValid())
        return;

    if (settings_.strength_ <= 0.0f)
        return;

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

#endif

} // namespace Urho3D


