// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Mutex.h"
#include "../Graphics/Drawable.h"
#include "../RenderAPI/PipelineState.h"
#include "../RenderAPI/RenderAPIUtils.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderAPI/RenderPool.h"
#include "../Graphics/RenderSurface.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/Viewport.h"
#include "../RenderPipeline/RenderBuffer.h"
#include "../RenderPipeline/RenderPipelineDefs.h"
#include "../IO/Log.h"

#include <EASTL/unordered_set.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

IntVector2 CalculateRenderTargetSize(const IntRect& viewportRect,
    const Vector2& sizeMultiplier, const IntVector2& explicitSize)
{
    if (explicitSize != IntVector2::ZERO)
        return explicitSize;
    const IntVector2 viewportSize = viewportRect.Size();
    return VectorMax(IntVector2::ONE, VectorRoundToInt(viewportSize.ToVector2() * sizeMultiplier));
}

RenderSurface* GetRenderSurfaceFromTexture(Texture* texture, CubeMapFace face = FACE_POSITIVE_X)
{
    if (!texture)
        return nullptr;

    if (texture->GetType() == Texture2D::GetTypeStatic())
        return static_cast<Texture2D*>(texture)->GetRenderSurface();
    else if (texture->GetType() == TextureCube::GetTypeStatic())
        return static_cast<TextureCube*>(texture)->GetRenderSurface(face);
    else
        return nullptr;
}

}

RenderBuffer::RenderBuffer(RenderPipelineInterface* renderPipeline)
    : Object(renderPipeline->GetContext())
    , renderDevice_(GetSubsystem<RenderDevice>())
{
    renderPipeline->OnRenderBegin.Subscribe(this, &RenderBuffer::OnRenderBegin);
    renderPipeline->OnRenderEnd.Subscribe(this, &RenderBuffer::OnRenderEnd);
}

RenderBuffer::~RenderBuffer()
{
}

bool RenderBuffer::CheckIfBufferIsReady() const
{
    if (!bufferIsReady_)
    {
        URHO3D_LOGERROR("RenderBuffer is not available");
        return false;
    }
    return true;
}

TextureRenderBuffer::TextureRenderBuffer(RenderPipelineInterface* renderPipeline,
    const RenderBufferParams& params, const Vector2& size)
    : RenderBuffer(renderPipeline)
    , params_(params)
{
    const bool fixedSize = params.flags_.Test(RenderBufferFlag::FixedTextureSize);
    if (fixedSize)
        fixedSize_ = VectorRoundToInt(size);
    else
        sizeMultiplier_ = size;
}

TextureRenderBuffer::~TextureRenderBuffer()
{
}

void TextureRenderBuffer::OnRenderBegin(const CommonFrameInfo& frameInfo)
{
    if (!isEnabled_)
        return;

    RenderPool* renderPool = renderDevice_->GetRenderPool();

    currentSize_ = CalculateRenderTargetSize(frameInfo.viewportRect_, sizeMultiplier_, fixedSize_);

    const bool noAutoResolve = params_.flags_.Test(RenderBufferFlag::NoMultiSampledAutoResolve);
    const bool isCubemap = params_.flags_.Test(RenderBufferFlag::CubeMap);
    const bool isFiltered = params_.flags_.Test(RenderBufferFlag::BilinearFiltering);
    const bool isPersistent = params_.flags_.Test(RenderBufferFlag::Persistent);
    const bool isDepthStencil = IsDepthTextureFormat(params_.textureFormat_);

    RawTextureParams params;
    params.type_ = isCubemap ? TextureType::TextureCube : TextureType::Texture2D;
    params.format_ = params_.textureFormat_;
    params.flags_.Set(isDepthStencil ? TextureFlag::BindDepthStencil : TextureFlag::BindRenderTarget);
    if (noAutoResolve)
        params.flags_.Set(TextureFlag::NoMultiSampledAutoResolve);

    params.size_ = currentSize_.ToIntVector3();
    params.numLevels_ = 1;
    params.multiSample_ = params_.multiSampleLevel_;

    currentTexture_ = renderPool->GetTexture(params, isPersistent ? this : nullptr);
    currentTexture_->SetSamplerStateDesc(isFiltered ? SamplerStateDesc::Bilinear() : SamplerStateDesc::Nearest());

    bufferIsReady_ = true;
}

void TextureRenderBuffer::OnRenderEnd(const CommonFrameInfo& frameInfo)
{
    currentTexture_ = nullptr;
    bufferIsReady_ = false;
}

RawTexture* TextureRenderBuffer::GetTexture() const
{
    URHO3D_ASSERT(CheckIfBufferIsReady());
    return currentTexture_;
}

RenderTargetView TextureRenderBuffer::GetView(unsigned slice) const
{
    URHO3D_ASSERT(CheckIfBufferIsReady());
    return RenderTargetView::TextureSlice(currentTexture_, slice);
}

RenderTargetView TextureRenderBuffer::GetReadOnlyDepthView(unsigned slice) const
{
    URHO3D_ASSERT(CheckIfBufferIsReady());
    return RenderTargetView::ReadOnlyDepthSlice(currentTexture_, slice);
}

IntRect TextureRenderBuffer::GetViewportRect() const
{
    URHO3D_ASSERT(CheckIfBufferIsReady());
    return {IntVector2::ZERO, currentSize_};
}

ViewportColorRenderBuffer::ViewportColorRenderBuffer(RenderPipelineInterface* renderPipeline)
    : RenderBuffer(renderPipeline)
{
}

RawTexture* ViewportColorRenderBuffer::GetTexture() const
{
    URHO3D_ASSERT(CheckIfBufferIsReady());
    return renderTarget_ ? renderTarget_->GetParentTexture() : nullptr;
}

RenderTargetView ViewportColorRenderBuffer::GetView(unsigned slice) const
{
    URHO3D_ASSERT(CheckIfBufferIsReady());
    return renderTarget_ ? renderTarget_->GetView() : RenderTargetView::SwapChainColor(renderDevice_);
}

RenderTargetView ViewportColorRenderBuffer::GetReadOnlyDepthView(unsigned slice) const
{
    URHO3D_ASSERT(false);
    return GetView(slice);
}

void ViewportColorRenderBuffer::OnRenderBegin(const CommonFrameInfo& frameInfo)
{
    renderTarget_ = frameInfo.renderTarget_;
    viewportRect_ = frameInfo.viewportRect_;
    bufferIsReady_ = true;
}

void ViewportColorRenderBuffer::OnRenderEnd(const CommonFrameInfo& frameInfo)
{
    bufferIsReady_ = false;
}

ViewportDepthStencilRenderBuffer::ViewportDepthStencilRenderBuffer(RenderPipelineInterface* renderPipeline)
    : RenderBuffer(renderPipeline)
{
}

RawTexture* ViewportDepthStencilRenderBuffer::GetTexture() const
{
    URHO3D_ASSERT(CheckIfBufferIsReady());
    return depthStencil_ ? depthStencil_->GetParentTexture() : nullptr;
}

RenderTargetView ViewportDepthStencilRenderBuffer::GetView(unsigned slice) const
{
    URHO3D_ASSERT(CheckIfBufferIsReady());
    return depthStencil_ ? depthStencil_->GetView() : RenderTargetView::SwapChainDepthStencil(renderDevice_);
}

RenderTargetView ViewportDepthStencilRenderBuffer::GetReadOnlyDepthView(unsigned slice) const
{
    URHO3D_ASSERT(CheckIfBufferIsReady());
    return depthStencil_ //
        ? depthStencil_->GetReadOnlyDepthView()
        : RenderTargetView::SwapChainDepthStencil(renderDevice_);
}

void ViewportDepthStencilRenderBuffer::OnRenderBegin(const CommonFrameInfo& frameInfo)
{
    viewportRect_ = frameInfo.viewportRect_;

    if (!frameInfo.renderTarget_)
    {
        // Backbuffer depth
        depthStencil_ = nullptr;
        bufferIsReady_ = true;
    }
    else if (auto depthStencil = frameInfo.renderTarget_->GetLinkedDepthStencil())
    {
        depthStencil_ = depthStencil;
        bufferIsReady_ = true;
    }
    else
    {
        depthStencil_ = nullptr;
        bufferIsReady_ = false;
    }
}

void ViewportDepthStencilRenderBuffer::OnRenderEnd(const CommonFrameInfo& frameInfo)
{
    bufferIsReady_ = false;
}

}
