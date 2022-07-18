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

#include "AmbientOcclusionPass.h"

#include "RenderBufferManager.h"
#include "../Graphics/Texture2D.h"
#include "../Resource/ResourceCache.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Renderer.h"

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
        bool resetCachedTextures = false;
        settings_ = settings;
        if (resetCachedTextures)
            InitializeTextures();
    }
}

void AmbientOcclusionPass::InitializeTextures()
{
    const unsigned format = Graphics::GetRGBAFormat();
    const Vector2 sizeMultiplier = Vector2::ONE / static_cast<float>(1 << 0);
    const RenderBufferParams params{format, 1, RenderBufferFlag::BilinearFiltering};
    textures_.currentTarget_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    textures_.previousTarget_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    textures_.noise_ =
        renderBufferManager_->GetContext()->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/SSAONoise.png");
}


void AmbientOcclusionPass::InitializeStates()
{
    pipelineStates_ = CachedStates{};
    pipelineStates_->ssao_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION");
    pipelineStates_->ssao_deferred_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION DEFERRED");
    pipelineStates_->blur_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "BLUR");
    pipelineStates_->blur_deferred_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "BLUR DEFERRED");
    pipelineStates_->combine_ = renderBufferManager_->CreateQuadPipelineState(BLEND_ALPHA, "v2/P_SSAO", "COMBINE");
    pipelineStates_->preview_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "PREVIEW");
}

void AmbientOcclusionPass::EvaluateAO(ea::span<ShaderParameterDesc> shaderParameters)
{
    const ShaderResourceDesc shaderResources[] = {
        {TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture()},
        {TU_DIFFUSE, textures_.noise_},
        {TU_NORMAL, (normalBuffer_) ? normalBuffer_->GetTexture2D() : nullptr}};

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale();

    renderBufferManager_->SetRenderTargets(nullptr, {textures_.currentTarget_});
    if (normalBuffer_)
    {
        drawParams.pipelineState_ = pipelineStates_->ssao_deferred_;
    }
    else
    {
        drawParams.pipelineState_ = pipelineStates_->ssao_;
    }
    renderBufferManager_->DrawQuad("Apply SSAO", drawParams);
    //renderBufferManager_->SetOutputRenderTargers();

    ea::swap(textures_.currentTarget_, textures_.previousTarget_);
}

void AmbientOcclusionPass::BlurTexture(ea::span<ShaderParameterDesc> shaderParameters)
{
    DrawQuadParams drawParams;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale();
    if (normalBuffer_)
    {
        drawParams.pipelineState_ = pipelineStates_->blur_deferred_;
    }
    else
    {
        drawParams.pipelineState_ = pipelineStates_->blur_;
    }

    {
        renderBufferManager_->SetRenderTargets(nullptr, {textures_.currentTarget_});
        const ShaderResourceDesc shaderResources[] = {
            {TU_DIFFUSE, textures_.previousTarget_->GetTexture2D()},
            {TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture()},
            {TU_NORMAL, (normalBuffer_) ? normalBuffer_->GetTexture2D() : nullptr}};
        drawParams.resources_ = shaderResources;
        shaderParameters[1].value_ = Vector2(shaderParameters[0].value_.GetVector2().x_, 0.0f);
        renderBufferManager_->DrawQuad("BlurV", drawParams);
        ea::swap(textures_.currentTarget_, textures_.previousTarget_);
    }

    {
        renderBufferManager_->SetRenderTargets(nullptr, {textures_.currentTarget_});
        const ShaderResourceDesc shaderResources[] = {
            {TU_DIFFUSE, textures_.previousTarget_->GetTexture2D()},
            {TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture()},
            {TU_NORMAL, (normalBuffer_) ? normalBuffer_->GetTexture2D() : nullptr}};
        drawParams.resources_ = shaderResources;
        shaderParameters[1].value_ = Vector2(0.0f, shaderParameters[0].value_.GetVector2().y_);
        renderBufferManager_->DrawQuad("BlurH", drawParams);
        ea::swap(textures_.currentTarget_, textures_.previousTarget_);
    }
}

void AmbientOcclusionPass::Blit(ea::span<ShaderParameterDesc> shaderParameters, PipelineState* state)
{
    renderBufferManager_->SwapColorBuffers(false);
    renderBufferManager_->SetOutputRenderTargers();

    const ShaderResourceDesc shaderResources[] = {
        {TU_DIFFUSE, textures_.previousTarget_->GetTexture2D()}};

    //renderBufferManager_->SetRenderTargets(nullptr, {renderBufferManager_->GetColorOutput()});
    renderBufferManager_->DrawViewportQuad("Preview SSAO", state, shaderResources, shaderParameters);
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

    const Texture2D* viewportTexture = renderBufferManager_->GetSecondaryColorTexture();
    const IntVector2 inputSize = viewportTexture->GetSize();
    const Vector2 inputInvSize = Vector2::ONE / static_cast<Vector2>(inputSize);

    // Convert texture coordinates into clip space
    Matrix4 texCoord = Matrix4::IDENTITY;
    texCoord.SetScale(Vector3(0.5f, 0.5f, 1.0f));
    texCoord.SetTranslation(Vector3(0.5f, 0.5f, 0.0f));
    const Matrix4 proj = texCoord*camera->GetGPUProjection();
    const Matrix4 invProj = proj.Inverse();

    renderBufferManager_->SwapColorBuffers(false);

    ShaderParameterDesc shaderParameters[] = {
        {"InputInvSize", inputInvSize},
        {"BlurStep", inputInvSize},
        {"SSAOStrength", settings_.strength_},
        {"SSAOExponent", settings_.exponent_},
        {"SSAORadius", settings_.radius_},
        {"SSAODepthThreshold", settings_.blurDepthThreshold_},
        {"SSAONormalThreshold", settings_.blurNormalThreshold_},
        {"Proj", proj},
        {"InvProj", invProj},
        {"CameraView", camera->GetView().ToMatrix4()},
    };

    EvaluateAO(shaderParameters);

    BlurTexture(shaderParameters);

    if (false)
    {
        Blit(shaderParameters, pipelineStates_->preview_);
    }
    else
    {
        Blit(shaderParameters, pipelineStates_->combine_);
    }
}

#endif

} // namespace Urho3D


