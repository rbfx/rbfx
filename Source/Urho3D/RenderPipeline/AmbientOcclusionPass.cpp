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
#include "Urho3D/Graphics/Camera.h"

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
        bool resetCachedTextures = false;
        settings_ = settings;
        if (resetCachedTextures)
            InitializeTextures();
    }
}

void AmbientOcclusionPass::InitializeTextures()
{
    const unsigned format = Graphics::GetRGBFormat();
    const Vector2 sizeMultiplier = Vector2::ONE / static_cast<float>(1 << 0);
    const RenderBufferParams params{format, 1, RenderBufferFlag::BilinearFiltering};
    textures_.ssao_ = renderBufferManager_->CreateColorBuffer(params, sizeMultiplier);
}


void AmbientOcclusionPass::InitializeStates()
{
    pipelineStates_ = CachedStates{};
    pipelineStates_->ssao_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "EVALUATE_OCCLUSTION");
    pipelineStates_->blur_ = renderBufferManager_->CreateQuadPipelineState(BLEND_REPLACE, "v2/P_SSAO", "BLUR");
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

void AmbientOcclusionPass::Execute()
{
    if (!pipelineStates_)
        InitializeStates();

    if (!pipelineStates_->IsValid())
        return;

    const Texture2D* viewportTexture = renderBufferManager_->GetSecondaryColorTexture();
    const IntVector2 inputSize = viewportTexture->GetSize();
    const Vector2 inputInvSize = Vector2::ONE / static_cast<Vector2>(inputSize);

    renderBufferManager_->SwapColorBuffers(false);

    const ShaderResourceDesc shaderResources[] = {
        {TU_DEPTHBUFFER, renderBufferManager_->GetDepthStencilTexture()}
    };
    const ShaderParameterDesc shaderParameters[] = {
        {"InputInvSize", inputInvSize},
    };
    renderBufferManager_->SetRenderTargets(nullptr, {textures_.ssao_});
    renderBufferManager_->DrawViewportQuad("Apply SSAO", pipelineStates_->ssao_, shaderResources, shaderParameters);
    renderBufferManager_->SetOutputRenderTargers();

    BlurTexture();
}

} // namespace Urho3D
