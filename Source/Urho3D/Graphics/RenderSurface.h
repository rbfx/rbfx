// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/Graphics/Viewport.h"

#include <Diligent/Common/interface/RefCntAutoPtr.hpp>
#include <Diligent/Graphics/GraphicsEngine/interface/TextureView.h>

#include <atomic>

namespace Urho3D
{

class RenderTargetView;
class Texture;

/// %Color or depth-stencil surface that can be rendered into.
class URHO3D_API RenderSurface : public RefCounted
{
public:
    RenderSurface(Texture* parentTexture, unsigned slice);
    ~RenderSurface() override;

    /// Internal. Restore GPU resource.
    void Restore(Diligent::ITextureView* view) { renderTargetView_ = view; }
    /// Internal. Invalidate GPU resource.
    void Invalidate() { renderTargetView_ = nullptr; }

    /// Set number of viewports.
    /// @property
    void SetNumViewports(unsigned num);
    /// Set viewport.
    /// @property{set_viewports}
    void SetViewport(unsigned index, Viewport* viewport);
    /// Set viewport update mode. Default is to update when visible.
    /// @property
    void SetUpdateMode(RenderSurfaceUpdateMode mode);
    /// Set linked color rendertarget.
    /// @property
    void SetLinkedRenderTarget(RenderSurface* renderTarget);
    /// Set linked depth-stencil surface.
    /// @property
    void SetLinkedDepthStencil(RenderSurface* depthStencil);
    /// Queue manual update of the viewport(s).
    void QueueUpdate();

    /// Return width.
    /// @property
    int GetWidth() const;

    /// Return height.
    /// @property
    int GetHeight() const;

    /// Return size.
    IntVector2 GetSize() const;

    /// Return multisampling level.
    int GetMultiSample() const;

    /// Return multisampling autoresolve mode.
    bool GetAutoResolve() const;

    /// Return number of viewports.
    /// @property
    unsigned GetNumViewports() const { return viewports_.size(); }

    /// Return viewport by index.
    /// @property{get_viewports}
    Viewport* GetViewport(unsigned index) const;

    /// Return viewport update mode.
    /// @property
    RenderSurfaceUpdateMode GetUpdateMode() const { return updateMode_; }

    /// Return linked color rendertarget.
    /// @property
    RenderSurface* GetLinkedRenderTarget() const { return linkedRenderTarget_; }

    /// Return linked depth-stencil surface.
    /// @property
    RenderSurface* GetLinkedDepthStencil() const { return linkedDepthStencil_; }

    /// Return whether manual update queued. Called internally.
    bool IsUpdateQueued() const { return updateQueued_.load(std::memory_order_relaxed); }

    /// Reset update queued flag. Called internally.
    void ResetUpdateQueued();

    /// Return parent texture.
    /// @property
    Texture* GetParentTexture() const { return parentTexture_; }

    /// Return slice of the parent texture.
    unsigned GetSlice() const { return slice_; }

    RenderTargetView GetView() const;
    RenderTargetView GetReadOnlyDepthView() const;
    bool IsRenderTarget() const;
    bool IsDepthStencil() const;

    /// Return whether multisampled rendertarget needs resolve.
    /// @property
    bool IsResolveDirty() const { return resolveDirty_; }

    /// Set or clear the need resolve flag. Called internally by Graphics.
    void SetResolveDirty(bool enable) { resolveDirty_ = enable; }

    /// Property getters that can work with null RenderSurface corresponding to main viewport
    /// @{
    static IntVector2 GetSize(Graphics* graphics, const RenderSurface* renderSurface);
    static IntRect GetRect(Graphics* graphics, const RenderSurface* renderSurface);
    static TextureFormat GetColorFormat(Graphics* graphics, const RenderSurface* renderSurface);
    static TextureFormat GetDepthFormat(Graphics* graphics, const RenderSurface* renderSurface);
    static int GetMultiSample(Graphics* graphics, const RenderSurface* renderSurface);
    static bool GetSRGB(Graphics* graphics, const RenderSurface* renderSurface);
    /// @}

private:
    /// Parent texture.
    const WeakPtr<Texture> parentTexture_;
    /// Slice of the parent texture.
    const unsigned slice_{};

    /// Diligent rendertarget or depth-stencil view.
    Diligent::RefCntAutoPtr<Diligent::ITextureView> renderTargetView_;

    /// Viewports.
    ea::vector<SharedPtr<Viewport> > viewports_;
    /// Linked color buffer.
    WeakPtr<RenderSurface> linkedRenderTarget_;
    /// Linked depth buffer.
    WeakPtr<RenderSurface> linkedDepthStencil_;
    /// Update mode for viewports.
    RenderSurfaceUpdateMode updateMode_{SURFACE_UPDATEVISIBLE};
    /// Update queued flag.
    std::atomic_bool updateQueued_{ false };
    /// Multisampled resolve dirty flag.
    bool resolveDirty_{};
};

}
