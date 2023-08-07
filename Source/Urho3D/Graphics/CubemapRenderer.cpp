//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Core/Context.h"
#include "../Graphics/Camera.h"
#include "../Graphics/CubemapRenderer.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextureCube.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../RenderAPI/DrawCommandQueue.h"
#include "../RenderAPI/RenderContext.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderAPI/RenderScope.h"
#include "../Scene/Node.h"

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

namespace
{

Quaternion faceRotations[MAX_CUBEMAP_FACES] = {
    Quaternion(0, 90, 0),
    Quaternion(0, -90, 0),
    Quaternion(-90, 0, 0),
    Quaternion(90, 0, 0),
    Quaternion(0, 0, 0),
    Quaternion(0, 180, 0),
};

ea::span<const unsigned> GetRayCounts(unsigned numLevels)
{
    static const unsigned rayCounts[]{1, 8, 16};
    static const unsigned rayCounts128[]{1, 8, 16, 16, 16, 16, 32, 32};
    static const unsigned rayCounts256[]{1, 8, 16, 16, 16, 16, 16, 32, 32};
    if (numLevels == 7)
        return rayCounts128;
    else if (numLevels == 8)
        return rayCounts256;
    else
        return rayCounts;
}

}

CubemapRenderer::CubemapRenderer(Scene* scene)
    : Object(scene->GetContext())
    , scene_(scene)
    , viewportTexture_(MakeShared<TextureCube>(context_))
    , filteredTexture_(MakeShared<TextureCube>(context_))
{
    InitializeRenderPipeline();
    InitializeCameras();
}

CubemapRenderer::~CubemapRenderer()
{
}

void CubemapRenderer::InitializeRenderPipeline()
{
    renderPipeline_ = MakeShared<RenderPipeline>(context_);

    RenderPipelineSettings settings = renderPipeline_->GetSettings();
    settings.drawDebugGeometry_ = false;
    renderPipeline_->SetSettings(settings);
}

void CubemapRenderer::InitializeCameras()
{
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        auto cameraNode = MakeShared<Node>(context_);
        cameraNode->SetWorldRotation(faceRotations[face]);
        auto camera = cameraNode->CreateComponent<Camera>();
        camera->SetFov(90.0f);
        camera->SetAspectRatio(1.0f);
        camera->SetDrawDebugGeometry(false);
        renderCameras_[face] = cameraNode;

        viewports_[face] = MakeShared<Viewport>(context_, scene_, camera, IntRect::ZERO, renderPipeline_);
    }
}

void CubemapRenderer::ConnectViewportsToTexture(TextureCube* texture)
{
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        RenderSurface* surface = texture->GetRenderSurface(static_cast<CubeMapFace>(face));
        surface->SetViewport(0, viewports_[face]);
    }

    UnsubscribeFromEvent(E_ENDVIEWRENDER);
    SubscribeToEvent(texture, E_ENDVIEWRENDER, &CubemapRenderer::ProcessFaceRendered);
}

void CubemapRenderer::DisconnectViewportsFromTexture(TextureCube* texture) const
{
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        RenderSurface* surface = texture->GetRenderSurface(static_cast<CubeMapFace>(face));
        surface->SetNumViewports(0);
    }
}

void CubemapRenderer::DefineTexture(TextureCube* texture, const CubemapRenderingSettings& settings, TextureFlags flags)
{
    texture->SetSize(
        settings.textureSize_, TextureFormat::TEX_FORMAT_RGBA8_UNORM, flags | TextureFlag::BindRenderTarget);
    texture->SetFilterMode(FILTER_TRILINEAR);
}

CubemapUpdateResult CubemapRenderer::Update(const CubemapUpdateParameters& params)
{
    if (!currentParams_.IsConsistentWith(params))
    {
        currentParams_ = params;

        updateStage_ = CubemapUpdateStage::Idle;
        numFacesToUpdate_ = 0;
        numFacesToRender_ = 0;
    }

    URHO3D_ASSERT(updateStage_ != CubemapUpdateStage::Ready);

    if (params.overrideFinalTexture_ && !IsTextureMatching(params.overrideFinalTexture_, params.settings_))
    {
        URHO3D_ASSERTLOG(0, "Invalid texture is used as override for CubemapRenderer::Update");
        return {MAX_CUBEMAP_FACES, true};
    }

    if (updateStage_ == CubemapUpdateStage::Idle)
        PrepareForUpdate(params);

    if (updateStage_ == CubemapUpdateStage::Idle && !params.slicedUpdate_)
        return UpdateFull();

    return UpdateSliced();
}

void CubemapRenderer::PrepareForUpdate(const CubemapUpdateParameters& params)
{
    // Assign current textures
    if (params.filterResult_)
    {
        currentViewportTexture_ = viewportTexture_;
        currentFilteredTexture_ = params.overrideFinalTexture_ ? params.overrideFinalTexture_ : filteredTexture_;
    }
    else
    {
        currentViewportTexture_ = params.overrideFinalTexture_ ? params.overrideFinalTexture_ : viewportTexture_;
        currentFilteredTexture_ = nullptr;
    }

    // Initialize contents
    if (currentViewportTexture_ == viewportTexture_ && !IsTextureMatching(viewportTexture_, params.settings_))
    {
        DefineTexture(viewportTexture_, params.settings_, TextureFlag::None);
        ConnectViewportsToTexture(viewportTexture_);
        viewportsConnectedToSelf_ = true;
    }
    if (currentFilteredTexture_ == filteredTexture_ && !IsTextureMatching(filteredTexture_, params.settings_))
    {
        DefineTexture(filteredTexture_, params.settings_, TextureFlag::BindUnorderedAccess);
    }

    if (currentViewportTexture_ != viewportTexture_)
    {
        if (viewportsConnectedToSelf_)
        {
            DisconnectViewportsFromTexture(viewportTexture_);
            viewportsConnectedToSelf_ = false;
        }
        ConnectViewportsToTexture(currentViewportTexture_);
    }
    else if (!viewportsConnectedToSelf_)
    {
        ConnectViewportsToTexture(viewportTexture_);
        viewportsConnectedToSelf_ = true;
    }

    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        auto camera = renderCameras_[face]->GetComponent<Camera>();
        camera->SetNearClip(params.settings_.nearClip_);
        camera->SetFarClip(params.settings_.farClip_);
        camera->SetViewMask(params.settings_.viewMask_);
    }

    for (Node* cameraNode : renderCameras_)
        cameraNode->SetWorldPosition(params.position_);
}

bool CubemapRenderer::IsTextureMatching(TextureCube* textureCube, const CubemapRenderingSettings& settings) const
{
    return textureCube && textureCube->GetWidth() == settings.textureSize_;
}

CubemapUpdateResult CubemapRenderer::UpdateFull()
{
    updateStage_ = CubemapUpdateStage::Ready;
    numFacesToUpdate_ = 0;
    numFacesToRender_ = MAX_CUBEMAP_FACES;

    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
        QueueFaceUpdate(static_cast<CubeMapFace>(face));

    return {MAX_CUBEMAP_FACES, true};
}

CubemapUpdateResult CubemapRenderer::UpdateSliced()
{
    if (updateStage_ == CubemapUpdateStage::Idle)
    {
        updateStage_ = CubemapUpdateStage::RenderFaces;
        numFacesToUpdate_ = MAX_CUBEMAP_FACES;
    }

    if (updateStage_ == CubemapUpdateStage::RenderFaces)
    {
        --numFacesToUpdate_;

        const auto face = static_cast<CubeMapFace>(numFacesToUpdate_);
        QueueFaceUpdate(face);

        if (numFacesToUpdate_ == 0)
        {
            updateStage_ = CubemapUpdateStage::Ready;
            numFacesToRender_ = 1;
        }
    }

    const bool isComplete = updateStage_ == CubemapUpdateStage::Ready;
    return {1, isComplete};
}

void CubemapRenderer::ProcessFaceRendered()
{
    URHO3D_ASSERTLOG(updateStage_ != CubemapUpdateStage::Ready || numFacesToRender_ > 0);

    if (updateStage_ == CubemapUpdateStage::Ready && numFacesToRender_ > 0)
    {
        --numFacesToRender_;
        if (numFacesToRender_ == 0)
        {
            updateStage_ = CubemapUpdateStage::Idle;
            ProcessCubemapRendered();
        }
    }
}

void CubemapRenderer::ProcessCubemapRendered()
{
    if (currentViewportTexture_ != viewportTexture_)
        DisconnectViewportsFromTexture(currentViewportTexture_);

    if (currentFilteredTexture_)
        FilterCubemap(currentViewportTexture_, currentFilteredTexture_);

    TextureCube* finalTexture = currentFilteredTexture_ ? currentFilteredTexture_ : currentViewportTexture_;
    OnCubemapRendered(this, finalTexture);

    currentViewportTexture_ = nullptr;
    currentFilteredTexture_ = nullptr;
}

void CubemapRenderer::QueueFaceUpdate(CubeMapFace face)
{
    auto* renderer = GetSubsystem<Renderer>();
    if (!renderer)
        return;

    RenderSurface* surface = currentViewportTexture_->GetRenderSurface(static_cast<CubeMapFace>(face));
    renderer->QueueRenderSurface(surface);
}

void CubemapRenderer::FilterCubemap(TextureCube* sourceTexture, TextureCube* destTexture)
{
    auto renderDevice = GetSubsystem<RenderDevice>();
    if (!renderDevice->GetCaps().computeShaders_)
    {
        URHO3D_LOGERROR("CubemapRenderer::FilterCubemap cannot be executed without compute shader support");
        return;
    }

    const unsigned numLevels = destTexture->GetLevels();
    EnsurePipelineStates(numLevels);
    if (!cachedPipelineStates_)
        return;

    RenderContext* renderContext = renderDevice->GetRenderContext();
    DrawCommandQueue* drawQueue = renderDevice->GetDefaultQueue();

    const RenderScope renderScope(renderContext, "CubemapRenderer::FilterCubemap");

    for (unsigned i = 0; i < numLevels; ++i)
        destTexture->CreateUAV(RawTextureUAVKey{}.FromLevel(i));

    drawQueue->Reset();

    for (unsigned i = 0; i < numLevels; ++i)
    {
        drawQueue->SetPipelineState(cachedPipelineStates_->pipelineStates_[i]);

        drawQueue->AddShaderResource("SourceTexture", sourceTexture);
        drawQueue->CommitShaderResources();

        drawQueue->AddUnorderedAccessView("OutputTexture", destTexture, RawTextureUAVKey{}.FromLevel(i));
        drawQueue->CommitUnorderedAccessViews();

        drawQueue->Dispatch({destTexture->GetLevelWidth(i), destTexture->GetLevelHeight(i), 6});
    }

    renderContext->ResetRenderTargets();
    renderContext->Execute(drawQueue);
}

void CubemapRenderer::EnsurePipelineStates(unsigned numLevels)
{
    if (cachedPipelineStates_ && cachedPipelineStates_->numLevels_ == numLevels)
        return;

    auto graphics = GetSubsystem<Graphics>();
    auto pipelineStateCache = GetSubsystem<PipelineStateCache>();

    CachedPipelineStates cache;
    cache.numLevels_ = numLevels;

    const auto rayCounts = GetRayCounts(numLevels);
    const float roughStep = 1.0f / static_cast<float>(numLevels - 1);
    for (unsigned i = 0; i < numLevels; ++i)
    {
        const auto levelWidth = static_cast<float>(1 << (numLevels - 1 - i));
        const unsigned rayCount = rayCounts[ea::max<unsigned>(i, rayCounts.size() - 1)];

        const ea::string shaderParams = Format("RAY_COUNT={}u FILTER_RES={}u FILTER_INV_RES={:.7f} ROUGHNESS={:.7f}",
            rayCount, levelWidth, 1.0f / levelWidth, roughStep * i);

        ComputePipelineStateDesc desc;
        desc.debugName_ = Format("C_FilterCubemap: {} of {}", i, numLevels);
        desc.computeShader_ = graphics->GetShader(CS, "v2/C_FilterCubemap", shaderParams);
        desc.samplers_.Add("SourceTexture", SamplerStateDesc::Bilinear());

        auto pipelineState = pipelineStateCache->GetComputePipelineState(desc);
        if (!pipelineState->IsValid())
        {
            URHO3D_LOGERROR("CubemapRenderer::FilterCubemap failed to create pipeline state");
            return;
        }

        cache.pipelineStates_.push_back(pipelineState);
    }

    cachedPipelineStates_ = cache;
}

}
