// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"
#include "Urho3D/RenderAPI/RenderTargetView.h"

#include <Diligent/Common/interface/RefCntAutoPtr.hpp>
#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

class DrawCommandQueue;
class RenderDevice;
class RenderPool;
class RawTexture;

/// Render context that consumes render commands.
class URHO3D_API RenderContext : public Object
{
    URHO3D_OBJECT(RenderContext, Object);

public:
    explicit RenderContext(RenderDevice* renderDevice);
    ~RenderContext() override;

    /// Reset statistics.
    void ResetStats() { stats_ = {}; }

    /// Reset render targets and depth-stencil buffer.
    void ResetRenderTargets();
    /// Set backbuffer render target and associated depth-stencil buffer.
    void SetSwapChainRenderTargets();
    /// Set specified render targets and depth-stencil buffer. Either can be null.
    void SetRenderTargets(OptionalRawTextureRTV depthStencil, ea::span<const RenderTargetView> renderTargets);

    /// Set viewport that covers the entirety of currently bound render target.
    void SetFullViewport();
    /// Set viewport that covers the specified area of currently bound render targets.
    void SetViewport(const IntRect& viewport);

    /// Clear depth-stencil buffer. Viewport and scissor are ignored.
    void ClearDepthStencil(ClearTargetFlags flags, float depth = 1.0f, unsigned stencil = 0);
    /// Clear render target at given index. Viewport and scissor are ignored.
    void ClearRenderTarget(unsigned index, const Color& color);

    /// Set whether the clip plane is enabled. Only used for OpenGL.
    void SetClipPlaneMask(unsigned mask);
    /// Execute draw commands from queue.
    void Execute(DrawCommandQueue* drawQueue);

    /// Check if the texture is already bound as a render target.
    bool IsBoundAsRenderTarget(const RawTexture* texture) const;

    /// Getters.
    /// @{
    Diligent::IDeviceContext* GetHandle() const { return handle_; }
    const RenderDeviceStats& GetStats() const { return stats_; }
    RenderDeviceStats& GetMutableStats() { return stats_; }
    RenderDevice* GetRenderDevice() const { return renderDevice_; }
    RenderPool* GetRenderPool() const { return renderPool_; }
    const IntVector2& GetCurrentRenderTargetSize() const { return currentDimensions_; }
    const PipelineStateOutputDesc& GetCurrentRenderTargetsDesc() const { return currentOutputDesc_; }
    bool IsSwapChainRenderTarget() const { return isSwapChain_; }
    const IntRect& GetCurrentViewport() const { return currentViewport_; }
    /// @}

private:
    void UpdateCurrentRenderTargetInfo();
    void ResetCachedContextState();

    RenderDevice* renderDevice_{};
    SharedPtr<RenderPool> renderPool_;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> handle_;

    RenderDeviceStats stats_;

    /// Current render target info.
    /// @{
    Diligent::ITextureView* currentDepthStencil_{};
    ea::fixed_vector<Diligent::ITextureView*, MaxRenderTargets> currentRenderTargets_;
    PipelineStateOutputDesc currentOutputDesc_;
    bool isSwapChain_{};
    IntVector2 currentDimensions_;
    IntRect currentViewport_;
    /// @}

    struct CachedContextState
    {
        unsigned clipPlaneMask_{};
    } cachedContextState_;
};

} // namespace Urho3D
