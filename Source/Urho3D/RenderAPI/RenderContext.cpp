// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RenderContext.h"

#include "Urho3D/RenderAPI/DrawCommandQueue.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/RawTexture.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderAPI/RenderPool.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

IntVector2 GetTextureDimensions(Diligent::ITexture* texture)
{
    const Diligent::TextureDesc& desc = texture->GetDesc();
    return {static_cast<int>(desc.GetWidth()), static_cast<int>(desc.GetHeight())};
}

} // namespace

RenderContext::RenderContext(RenderDevice* renderDevice)
    : Object(renderDevice->GetContext())
    , renderDevice_(renderDevice)
    , renderPool_(renderDevice->GetRenderPool())
    , handle_(renderDevice->GetImmediateContext())
{
    renderDevice_->OnDeviceLost.Subscribe(this, &RenderContext::ResetCachedContextState);
}

RenderContext::~RenderContext()
{
}

void RenderContext::ResetRenderTargets()
{
    SetRenderTargets(ea::nullopt, {});
}

void RenderContext::SetSwapChainRenderTargets()
{
    const RenderTargetView depthStencil = RenderTargetView::SwapChainDepthStencil(renderDevice_);
    const RenderTargetView renderTargets[] = {RenderTargetView::SwapChainColor(renderDevice_)};
    SetRenderTargets(depthStencil, renderTargets);
}

void RenderContext::SetRenderTargets(OptionalRawTextureRTV depthStencil, ea::span<const RenderTargetView> renderTargets)
{
    const bool isDepthStencilSwapChain = depthStencil && depthStencil->IsSwapChain();
    const bool isRenderTargetsSwapChain = (renderTargets.size() == 1) && renderTargets[0].IsSwapChain();
    const bool isSingleView = (renderTargets.size() + (depthStencil ? 1 : 0)) == 1;

    if (isDepthStencilSwapChain != isRenderTargetsSwapChain && !isSingleView)
    {
        URHO3D_ASSERTLOG(false, "Cannot mix swap chain and non-swap chain views");
        return;
    }

    // It's pointless to mark swap chain views as dirty because they cannot be read.
    isSwapChain_ = isDepthStencilSwapChain || isRenderTargetsSwapChain;
    if (!isSwapChain_)
    {
        if (depthStencil)
            depthStencil->MarkDirty();
        for (const RenderTargetView& renderTarget : renderTargets)
            renderTarget.MarkDirty();
    }

    currentDepthStencil_ = depthStencil ? depthStencil->GetView() : nullptr;
    currentRenderTargets_.clear();
    for (const RenderTargetView& renderTarget : renderTargets)
        currentRenderTargets_.push_back(renderTarget.GetView());

    UpdateCurrentRenderTargetInfo();

    handle_->SetRenderTargets(currentRenderTargets_.size(), currentRenderTargets_.data(), currentDepthStencil_,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void RenderContext::SetFullViewport()
{
    const IntVector2 currentSize = GetCurrentRenderTargetSize();
    if (currentSize == IntVector2::ZERO)
    {
        URHO3D_ASSERTLOG(false, "Depth-stencil buffer or render target must be bound to call SetFullViewport");
        return;
    }

    SetViewport({IntVector2::ZERO, currentSize});
}

void RenderContext::SetViewport(const IntRect& viewport)
{
    const IntVector2 viewportMin = viewport.Min();
    const IntVector2 viewportMax = viewport.Max();
    currentViewport_ = IntRect{VectorMax(viewportMin, IntVector2::ZERO), VectorMin(viewportMax, currentDimensions_)};

    Diligent::Viewport viewportDesc;
    viewportDesc.TopLeftX = currentViewport_.left_;
    viewportDesc.TopLeftY = currentViewport_.top_;
    viewportDesc.Width = currentViewport_.Width();
    viewportDesc.Height = currentViewport_.Height();
    handle_->SetViewports(1, &viewportDesc, 0, 0);
}

void RenderContext::ClearDepthStencil(ClearTargetFlags flags, float depth, unsigned stencil)
{
    if (!flags.Test(CLEAR_DEPTH) && !flags.Test(CLEAR_STENCIL))
    {
        URHO3D_ASSERTLOG(false, "At least one of CLEAR_DEPTH or CLEAR_STENCIL must be set to call ClearDepthStencil");
        return;
    }
    if (!currentDepthStencil_)
    {
        URHO3D_ASSERTLOG(false, "Depth-stencil buffer must be bound to call ClearDepthStencil");
        return;
    }

    Diligent::CLEAR_DEPTH_STENCIL_FLAGS internalFlags;
    if (flags.Test(CLEAR_DEPTH))
        internalFlags |= Diligent::CLEAR_DEPTH_FLAG;
    if (flags.Test(CLEAR_STENCIL) && IsStencilTextureFormat(currentDepthStencil_->GetTexture()->GetDesc().Format))
        internalFlags |= Diligent::CLEAR_STENCIL_FLAG;
    handle_->ClearDepthStencil(
        currentDepthStencil_, internalFlags, depth, stencil, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void RenderContext::ClearRenderTarget(unsigned index, const Color& color)
{
    if (index >= currentRenderTargets_.size())
    {
        URHO3D_ASSERTLOG(false, "Render target must be bound to call ClearRenderTarget");
        return;
    }

    handle_->ClearRenderTarget(
        currentRenderTargets_[index], color.Data(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void RenderContext::SetClipPlaneEnabled(bool enable)
{
    if (cachedContextState_.clipPlaneEnabled_ == enable)
        return;

    if (!renderDevice_->GetCaps().clipDistance_)
        return;

    cachedContextState_.clipPlaneEnabled_ = enable;
    if (renderDevice_->GetBackend() == RenderBackend::OpenGL)
    {
#if GL_SUPPORTED
        if (enable)
            glEnable(GL_CLIP_DISTANCE0);
        else
            glDisable(GL_CLIP_DISTANCE0);
#elif GLES_SUPPORTED
        if (enable)
            glEnable(GL_CLIP_DISTANCE0_EXT);
        else
            glDisable(GL_CLIP_DISTANCE0_EXT);
#endif
    }
}

void RenderContext::Execute(DrawCommandQueue* drawQueue)
{
    drawQueue->ExecuteInContext(this);
}

void RenderContext::UpdateCurrentRenderTargetInfo()
{
    currentOutputDesc_.depthStencilFormat_ =
        currentDepthStencil_ ? currentDepthStencil_->GetTexture()->GetDesc().Format : Diligent::TEX_FORMAT_UNKNOWN;
    currentOutputDesc_.numRenderTargets_ = currentRenderTargets_.size();
    for (unsigned i = 0; i < currentRenderTargets_.size(); ++i)
        currentOutputDesc_.renderTargetFormats_[i] = currentRenderTargets_[i]->GetTexture()->GetDesc().Format;

    Diligent::ITextureView* view = !currentRenderTargets_.empty() ? currentRenderTargets_[0] : currentDepthStencil_;
    currentDimensions_ = view ? GetTextureDimensions(view->GetTexture()) : IntVector2::ZERO;
    currentOutputDesc_.multiSample_ = view ? view->GetTexture()->GetDesc().SampleCount : 1;
}

void RenderContext::ResetCachedContextState()
{
    cachedContextState_ = {};
}

bool RenderContext::IsBoundAsRenderTarget(const RawTexture* texture) const
{
    if (!texture)
        return false;

    Diligent::ITexture* handle = texture->GetHandles().texture_;
    const auto isBound = [&](Diligent::ITextureView* view) { return view->GetTexture() == handle; };
    return ea::any_of(currentRenderTargets_.begin(), currentRenderTargets_.end(), isBound);
}

} // namespace Urho3D
