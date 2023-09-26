// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Mutex.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/Core/Timer.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Common/interface/RefCntAutoPtr.hpp>

#include <EASTL/optional.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/tuple.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_set.h>
#include <EASTL/vector.h>

struct SDL_Window;

namespace Diligent
{

struct IDeviceContext;
struct IDeviceContextD3D11;
struct IDeviceContextD3D12;
struct IDeviceContextGL;
struct IDeviceContextVk;
struct IEngineFactory;
struct IEngineFactoryD3D11;
struct IEngineFactoryD3D12;
struct IEngineFactoryOpenGL;
struct IEngineFactoryVk;
struct IRenderDevice;
struct IRenderDeviceD3D11;
struct IRenderDeviceD3D12;
struct IRenderDeviceGL;
struct IRenderDeviceGLES;
struct IRenderDeviceVk;
struct ISwapChain;
struct ISwapChainD3D11;
struct ISwapChainD3D12;
struct ISwapChainGL;
struct ISwapChainVk;
struct ITexture;
struct SwapChainDesc;

} // namespace Diligent

namespace Urho3D
{

class DeviceObject;
class DrawCommandQueue;
class PipelineState;
class RawTexture;
class RenderContext;
class RenderPool;

/// Wrapper for window and GAPI backend.
class URHO3D_API RenderDevice : public Object
{
    URHO3D_OBJECT(RenderDevice, Object);

public:
    /// Android only: handle device loss and restore.
    /// @{
    Signal<void()> OnDeviceLost;
    Signal<void()> OnDeviceRestored;
    /// @}

    /// Initialize the OS window and GAPI.
    /// Throws RuntimeException if unrecoverable error occurs.
    RenderDevice(Context* context, const RenderDeviceSettings& deviceSettings, const WindowSettings& windowSettings);
    ~RenderDevice() override;
    /// Post-initialize, when RenderDevice is visible to the engine.
    void PostInitialize();

    /// Create swap chain for secondary window. It is not supported for some platforms and backends.
    /// @note For OpenGL, unique shared context should be set as current before calling this function.
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> CreateSecondarySwapChain(SDL_Window* sdlWindow, bool hasDepthBuffer);

    /// Update swap chain size according to current dimensions of the window.
    void UpdateSwapChainSize();
    /// Change window settings. Some settings cannot be changed in runtime.
    void UpdateWindowSettings(const WindowSettings& settings);
    /// Change default texture filtering.
    void SetDefaultTextureFilterMode(TextureFilterMode filterMode);
    /// Change default texture anisotropy level.
    void SetDefaultTextureAnisotropy(int anisotropy);
    /// Restore device if is was lost. Only applicable for Android.
    bool Restore();
    /// Emulate device loss and restore. Only applicable for Android.
    bool EmulateLossAndRestore();
    /// Queue pipeline state reload at the end of the frame.
    void QueuePipelineStateReload(PipelineState* pipelineState);

    /// Take the screenshot from current back buffer.
    bool TakeScreenShot(IntVector2& size, ByteVector& data);

    /// Present the frame. Should be called between engine frames.
    void Present();

    /// Check if texture format is supported on hardware.
    bool IsTextureFormatSupported(TextureFormat format) const;
    /// Check if texture format is supported on hardware as render target or depth stencil.
    bool IsRenderTargetFormatSupported(TextureFormat format) const;
    /// Check if texture format is supported as UAV.
    bool IsUnorderedAccessFormatSupported(TextureFormat format) const;
    /// Check if given level of MSAA is supported on hardware.
    bool IsMultiSampleSupported(TextureFormat format, int multiSample) const;
    /// Select supported multi-sample level for given format.
    int GetSupportedMultiSample(TextureFormat format, int multiSample) const;

    /// Getters.
    /// @{
    const RenderBackend GetBackend() const { return deviceSettings_.backend_; }
    const RenderDeviceSettings& GetDeviceSettings() const { return deviceSettings_; }
    const WindowSettings& GetWindowSettings() const { return windowSettings_; }
    const RenderDeviceCaps& GetCaps() const { return caps_; }
    const RenderDeviceStats& GetStats() const { return stats_; }
    const RenderDeviceStats& GetMaxStats() const { return prevMaxStats_; }
    TextureFilterMode GetDefaultTextureFilterMode() const { return defaultTextureFilterMode_; }
    int GetDefaultTextureAnisotropy() const { return defaultTextureAnisotropy_; }
    TextureFormat GetDefaultDepthStencilFormat() const { return defaultDepthStencilFormat_; }
    TextureFormat GetDefaultDepthFormat() const { return defaultDepthFormat_; }
    RenderContext* GetRenderContext() const { return renderContext_; }
    RenderPool* GetRenderPool() const { return renderPool_; }
    SDL_Window* GetSDLWindow() const { return window_.get(); }
    void* GetMetalView() const { return metalView_.get(); }
    Diligent::IEngineFactory* GetFactory() { return factory_.RawPtr(); }
    Diligent::IRenderDevice* GetRenderDevice() { return renderDevice_.RawPtr(); }
    Diligent::IDeviceContext* GetImmediateContext() { return deviceContext_.RawPtr(); }
    Diligent::ISwapChain* GetSwapChain() { return swapChain_.RawPtr(); }
    IntVector2 GetSwapChainSize() const;
    IntVector2 GetWindowSize() const;
    float GetDpiScale() const;
    FrameIndex GetFrameIndex() const { return frameIndex_; }
    RawTexture* GetDefaultTexture(TextureType type) const { return defaultTextures_[type].get(); }
    DrawCommandQueue* GetDefaultQueue() const { return defaultQueue_; }
    /// @}

    /// Static utilities.
    /// @{
    static FullscreenModeVector GetFullscreenModes(int monitor);
    static unsigned GetClosestFullscreenModeIndex(const FullscreenModeVector& modes, FullscreenMode desiredMode);
    static FullscreenMode GetClosestFullscreenMode(const FullscreenModeVector& modes, FullscreenMode desiredMode);
    /// @}

    /// Internal.
    /// @{
    void AddDeviceObject(DeviceObject* object);
    void RemoveDeviceObject(DeviceObject* object);
    /// @}

private:
    void InitializeWindow();
    void InitializeFactory();
    void InitializeDevice();
    void InitializeMultiSampleSwapChain(Diligent::ISwapChain* nativeSwapChain);
    void InitializeCaps();

    void InitializeDefaultObjects();
    void ReleaseDefaultObjects();

    void InvalidateGLESContext();
    bool RestoreGLESContext();

    void InvalidateVulkanContext();
    bool RestoreVulkanContext();

    void InvalidateDeviceState();
    void RestoreDeviceState();

    void SendDeviceObjectEvent(DeviceObjectEvent event);

    Diligent::RefCntAutoPtr<Diligent::ITexture> GetResolvedBackBuffer();
    Diligent::RefCntAutoPtr<Diligent::ITexture> ReadTextureToStaging(Diligent::ITexture* sourceTexture);

    RenderDeviceSettings deviceSettings_;
    WindowSettings windowSettings_;

    bool defaultTextureParametersDirty_{};
    TextureFilterMode defaultTextureFilterMode_{FILTER_TRILINEAR};
    int defaultTextureAnisotropy_{4};

    RenderDeviceCaps caps_;
    TextureFormat defaultDepthStencilFormat_{};
    TextureFormat defaultDepthFormat_{};

    ea::shared_ptr<SDL_Window> window_;
    ea::shared_ptr<void> metalView_;
    ea::shared_ptr<void> glContext_;

    Diligent::RefCntAutoPtr<Diligent::IEngineFactory> factory_;
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice_;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> deviceContext_;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain_;

    SharedPtr<RenderContext> renderContext_;

    FrameIndex frameIndex_{FrameIndex::First};

    ea::unordered_set<DeviceObject*> deviceObjects_;
    Mutex deviceObjectsMutex_;

    EnumArray<ea::unique_ptr<RawTexture>, TextureType> defaultTextures_;
    SharedPtr<RenderPool> renderPool_;
    SharedPtr<DrawCommandQueue> defaultQueue_;

    ea::unique_ptr<Diligent::SwapChainDesc> oldNativeSwapChainDesc_;
    ea::vector<WeakPtr<PipelineState>> pipelineStatesToReload_;

    static constexpr unsigned StatsPeriodMs = 333;
    Timer statsTimer_;
    RenderDeviceStats stats_;
    RenderDeviceStats maxStats_;
    RenderDeviceStats prevMaxStats_;

    // Keep aliases at the end to ensure they are destroyed first and don't affect real order of destruction.
#if D3D11_SUPPORTED
    Diligent::RefCntAutoPtr<Diligent::IEngineFactoryD3D11> factoryD3D11_;
    Diligent::RefCntAutoPtr<Diligent::IRenderDeviceD3D11> renderDeviceD3D11_;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContextD3D11> deviceContextD3D11_;
    Diligent::RefCntAutoPtr<Diligent::ISwapChainD3D11> swapChainD3D11_;
#endif
#if D3D12_SUPPORTED
    Diligent::RefCntAutoPtr<Diligent::IEngineFactoryD3D12> factoryD3D12_;
    Diligent::RefCntAutoPtr<Diligent::IRenderDeviceD3D12> renderDeviceD3D12_;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContextD3D12> deviceContextD3D12_;
    Diligent::RefCntAutoPtr<Diligent::ISwapChainD3D12> swapChainD3D12_;
#endif
#if GL_SUPPORTED || GLES_SUPPORTED
    Diligent::RefCntAutoPtr<Diligent::IEngineFactoryOpenGL> factoryOpenGL_;
    Diligent::RefCntAutoPtr<Diligent::IRenderDeviceGL> renderDeviceGL_;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContextGL> deviceContextGL_;
    Diligent::RefCntAutoPtr<Diligent::ISwapChainGL> swapChainGL_;
    #if GLES_SUPPORTED && (URHO3D_PLATFORM_WEB || URHO3D_PLATFORM_ANDROID)
    Diligent::RefCntAutoPtr<Diligent::IRenderDeviceGLES> renderDeviceGLES_;
    #endif
#endif
#if VULKAN_SUPPORTED
    Diligent::RefCntAutoPtr<Diligent::IEngineFactoryVk> factoryVulkan_;
    Diligent::RefCntAutoPtr<Diligent::IRenderDeviceVk> renderDeviceVulkan_;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContextVk> deviceContextVulkan_;
    Diligent::RefCntAutoPtr<Diligent::ISwapChainVk> swapChainVulkan_;
#endif
};

} // namespace Urho3D
