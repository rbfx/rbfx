// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/Graphics/RenderSurface.h"
#include "Urho3D/Graphics/Texture.h"
#include "Urho3D/RenderAPI/RawTexture.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderAPI/RenderTargetView.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

RenderSurface::RenderSurface(Texture* parentTexture, unsigned slice)
    : parentTexture_(parentTexture)
    , slice_(slice)
{
}

RenderSurface::~RenderSurface()
{
}

void RenderSurface::SetNumViewports(unsigned num)
{
    viewports_.resize(num);
}

void RenderSurface::SetViewport(unsigned index, Viewport* viewport)
{
    if (index >= viewports_.size())
        viewports_.resize(index + 1);

    viewports_[index] = viewport;
}

void RenderSurface::SetUpdateMode(RenderSurfaceUpdateMode mode)
{
    updateMode_ = mode;
}

void RenderSurface::SetLinkedRenderTarget(RenderSurface* renderTarget)
{
    if (renderTarget != this)
        linkedRenderTarget_ = renderTarget;
}

void RenderSurface::SetLinkedDepthStencil(RenderSurface* depthStencil)
{
    if (depthStencil != this)
        linkedDepthStencil_ = depthStencil;
}

void RenderSurface::QueueUpdate()
{
    updateQueued_.store(true, std::memory_order_relaxed);
}

void RenderSurface::ResetUpdateQueued()
{
    updateQueued_.store(false, std::memory_order_relaxed);
}

int RenderSurface::GetWidth() const
{
    return parentTexture_->GetWidth();
}

int RenderSurface::GetHeight() const
{
    return parentTexture_->GetHeight();
}

IntVector2 RenderSurface::GetSize() const
{
    return { GetWidth(), GetHeight() };
}

int RenderSurface::GetMultiSample() const
{
    return parentTexture_->GetMultiSample();
}

bool RenderSurface::GetAutoResolve() const
{
    return parentTexture_->GetAutoResolve();
}

Viewport* RenderSurface::GetViewport(unsigned index) const
{
    return index < viewports_.size() ? viewports_[index] : nullptr;
}

IntVector2 RenderSurface::GetSize(Graphics* graphics, const RenderSurface* renderSurface)
{
    return renderSurface ? renderSurface->GetSize() : graphics->GetSize();
}

IntRect RenderSurface::GetRect(Graphics* graphics, const RenderSurface* renderSurface)
{
    return { IntVector2::ZERO, GetSize(graphics, renderSurface) };
}

TextureFormat RenderSurface::GetColorFormat(Graphics* graphics, const RenderSurface* renderSurface)
{
    auto renderDevice = graphics->GetSubsystem<RenderDevice>();
    const auto rtv = renderSurface ? renderSurface->GetView() : RenderTargetView::SwapChainColor(renderDevice);
    return rtv.GetFormat();
}

TextureFormat RenderSurface::GetDepthFormat(Graphics* graphics, const RenderSurface* renderSurface)
{
    auto renderDevice = graphics->GetSubsystem<RenderDevice>();
    const auto rtv = renderSurface ? renderSurface->GetView() : RenderTargetView::SwapChainDepthStencil(renderDevice);
    return rtv.GetFormat();
}

int RenderSurface::GetMultiSample(Graphics* graphics, const RenderSurface* renderSurface)
{
    return renderSurface ? renderSurface->GetMultiSample() : graphics->GetMultiSample();
}

bool RenderSurface::GetSRGB(Graphics* graphics, const RenderSurface* renderSurface)
{
    return renderSurface ? renderSurface->GetParentTexture()->GetSRGB() : graphics->GetSRGB();
}

RenderTargetView RenderSurface::GetView() const
{
    return RenderTargetView::TextureSlice(parentTexture_, slice_);
}

RenderTargetView RenderSurface::GetReadOnlyDepthView() const
{
    return RenderTargetView::ReadOnlyDepthSlice(parentTexture_, slice_);
}

bool RenderSurface::IsRenderTarget() const
{
    return parentTexture_->IsRenderTarget();
}

bool RenderSurface::IsDepthStencil() const
{
    return parentTexture_->IsDepthStencil();
}

}
