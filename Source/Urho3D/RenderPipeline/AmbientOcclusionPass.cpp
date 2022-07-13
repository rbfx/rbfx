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
    textures_.ssao_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
    textures_.noise_ =
        renderBufferManager_->GetContext()->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/SSAONoise.png");
}


void AmbientOcclusionPass::InitializeStates()
{
    pipelineStates_ = CachedStates{};
    pipelineStates_->ssao_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION");
    pipelineStates_->ssao_deferred_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSION DEFERRED");
    pipelineStates_->blur_ = renderBufferManager_->CreateQuadPipelineState(BLEND_ALPHA, "v2/P_SSAO", "BLUR");
    pipelineStates_->preview_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "PREVIEW");
}

void AmbientOcclusionPass::BlurTexture()
{
    const Texture2D* viewportTexture = renderBufferManager_->GetSecondaryColorTexture();
    const IntVector2 inputSize = viewportTexture->GetSize();
    const Vector2 inputInvSize = Vector2::ONE / static_cast<Vector2>(inputSize);

    renderBufferManager_->SwapColorBuffers(false);
    renderBufferManager_->SetOutputRenderTargers();

    const ShaderResourceDesc shaderResources[] = {{TU_DIFFUSE, textures_.ssao_->GetTexture2D()}};
    const ShaderParameterDesc shaderParameters[] = {
        {"InputInvSize", inputInvSize},
    };

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale();

    drawParams.pipelineState_ = pipelineStates_->blur_;
    renderBufferManager_->SetRenderTargets(nullptr, {renderBufferManager_->GetColorOutput()});
    renderBufferManager_->DrawQuad("Blur", drawParams);
}

void AmbientOcclusionPass::Preview()
{
    const Texture2D* viewportTexture = renderBufferManager_->GetSecondaryColorTexture();
    const IntVector2 inputSize = viewportTexture->GetSize();
    const Vector2 inputInvSize = Vector2::ONE / static_cast<Vector2>(inputSize);

    renderBufferManager_->SetOutputRenderTargers();

    const ShaderResourceDesc shaderResources[] = {{TU_DIFFUSE, textures_.ssao_->GetTexture2D()}};
    const ShaderParameterDesc shaderParameters[] = {
        {"InputInvSize", inputInvSize},
    };

    DrawQuadParams drawParams;
    drawParams.resources_ = shaderResources;
    drawParams.parameters_ = shaderParameters;
    drawParams.clipToUVOffsetAndScale_ = renderBufferManager_->GetDefaultClipToUVSpaceOffsetAndScale();

    drawParams.pipelineState_ = pipelineStates_->preview_;
    renderBufferManager_->SetRenderTargets(nullptr, {renderBufferManager_->GetColorOutput()});
    renderBufferManager_->DrawQuad("Preview SSAO", drawParams);
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

    const ShaderResourceDesc shaderResources[] = {
        {TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture()},
        {TU_DIFFUSE, textures_.noise_},
        {TU_NORMAL, (normalBuffer_) ? normalBuffer_->GetTexture2D() : nullptr}};
    const ShaderParameterDesc shaderParameters[] = {
        {"InputInvSize", inputInvSize},
        {"SSAOStrength", settings_.strength_},
        {"SSAORadius", settings_.radius_},
        {"Proj", proj},
        {"InvProj", invProj},
        {"CameraView", camera->GetView().ToMatrix4()},
    };
    renderBufferManager_->SetRenderTargets(nullptr, {textures_.ssao_});
    if (normalBuffer_)
    {
        renderBufferManager_->DrawViewportQuad("Apply SSAO", pipelineStates_->ssao_deferred_, shaderResources, shaderParameters);
    }
    else
    {
        renderBufferManager_->DrawViewportQuad("Apply SSAO", pipelineStates_->ssao_, shaderResources, shaderParameters);
    }
    renderBufferManager_->SetOutputRenderTargers();

    //Preview();

    BlurTexture();
}

#endif

} // namespace Urho3D


