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
#include "../Scene/Node.h"

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

}

CubemapRenderer::CubemapRenderer(Scene* scene)
    : Object(scene->GetContext())
    , scene_(scene)
{
    InitializeRenderPipeline();
    InitializeCameras();
    Define(CubemapRenderingParameters{});
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
        renderCameras_[face] = cameraNode;

        viewports_[face] = MakeShared<Viewport>(context_, scene_, camera, IntRect::ZERO, renderPipeline_);
    }
}

void CubemapRenderer::InitializeTexture(const CubemapRenderingParameters& params)
{
    if (!scene_)
        return;

    if (renderTexture_ && renderTexture_->GetWidth() == params.textureSize_)
        return;

    renderTexture_ = MakeShared<TextureCube>(context_);
    DefineTexture(renderTexture_, params);
    ConnectViewportsToTexture(renderTexture_);
}

void CubemapRenderer::ConnectViewportsToTexture(TextureCube* texture)
{
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        RenderSurface* surface = texture->GetRenderSurface(static_cast<CubeMapFace>(face));
        surface->SetViewport(0, viewports_[face]);
    }

    UnsubscribeFromEvent(E_ENDVIEWRENDER);
    SubscribeToEvent(texture, E_ENDVIEWRENDER, [&](StringHash, VariantMap&)
    {
        ProcessFaceRendered();
    });
}

void CubemapRenderer::DisconnectViewportsFromTexture(TextureCube* texture) const
{
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        RenderSurface* surface = texture->GetRenderSurface(static_cast<CubeMapFace>(face));
        surface->SetNumViewports(0);
    }
}

void CubemapRenderer::DefineTexture(TextureCube* texture, const CubemapRenderingParameters& params)
{
    texture->SetSize(params.textureSize_, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET);
    texture->SetFilterMode(FILTER_BILINEAR);
}

void CubemapRenderer::Define(const CubemapRenderingParameters& params)
{
    InitializeTexture(params);
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        auto camera = renderCameras_[face]->GetComponent<Camera>();
        camera->SetNearClip(params.nearClip_);
        camera->SetFarClip(params.farClip_);
        camera->SetViewMask(params.viewMask_);
    }
}

void CubemapRenderer::OverrideOutputTexture(TextureCube* texture)
{
    if (overrideTexture_ == texture)
        return;

    if (overrideTexture_)
    {
        DisconnectViewportsFromTexture(overrideTexture_);
        ConnectViewportsToTexture(renderTexture_);
    }

    overrideTexture_ = texture;

    if (overrideTexture_)
    {
        DisconnectViewportsFromTexture(renderTexture_);
        ConnectViewportsToTexture(overrideTexture_);
    }
}

CubemapUpdateResult CubemapRenderer::Update(const Vector3& position, bool slicedUpdate)
{
    for (Node* cameraNode : renderCameras_)
        cameraNode->SetWorldPosition(position);

    if (updateStage_ == CubemapUpdateStage::Idle && !slicedUpdate)
        return UpdateFull();

    return UpdateSliced();
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
    // TODO(reflection): Filter cubemap here
    TextureCube* destinationTexture = overrideTexture_ ? overrideTexture_ : renderTexture_;
    OnCubemapRendered(this, destinationTexture);
}

void CubemapRenderer::QueueFaceUpdate(CubeMapFace face)
{
    auto* renderer = GetSubsystem<Renderer>();
    if (!renderer)
        return;

    TextureCube* destinationTexture = overrideTexture_ ? overrideTexture_ : renderTexture_;

    RenderSurface* surface = destinationTexture->GetRenderSurface(static_cast<CubeMapFace>(face));
    renderer->QueueRenderSurface(surface);
}

}
