// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Common/interface/RefCntAutoPtr.hpp>

#include <EASTL/optional.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/tuple.h>
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

} // namespace Diligent

namespace Urho3D
{

struct FullscreenMode
{
    IntVector2 size_{};
    int refreshRate_{};

    /// Operators.
    /// @{
    const auto Tie() const { return ea::tie(size_.x_, size_.y_, refreshRate_); }
    bool operator==(const FullscreenMode& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const FullscreenMode& rhs) const { return Tie() != rhs.Tie(); }
    bool operator<(const FullscreenMode& rhs) const { return Tie() < rhs.Tie(); }
    /// @}
};
using FullscreenModeVector = ea::vector<FullscreenMode>;

struct WindowSettings
{
    /// Type of window (windowed, borderless fullscreen, native fullscreen).
    WindowMode mode_{};
    /// Windowed: size of the window in units. May be different from the size in pixels due to DPI scale.
    /// Fullscreen: display resolution in pixels.
    /// Borderless: ignored.
    /// Set to 0 to pick automatically.
    IntVector2 size_;
    /// Window title.
    ea::string title_;

    /// Windowed only: whether the window can be resized.
    bool resizable_{};
    /// Fullscreen and Borderless only: index of the monitor.
    int monitor_{};

    /// Whether to enable vertical synchronization.
    bool vSync_{};
    /// Refresh rate. 0 to pick automatically.
    int refreshRate_{};
    /// Multi-sampling level.
    int multiSample_{1};
    /// Whether to use sRGB framebuffer.
    bool sRGB_{};

    /// Mobiles: orientation hints.
    /// Could be any combination of "LandscapeLeft", "LandscapeRight", "Portrait" and "PortraitUpsideDown".
    StringVector orientations_{"LandscapeLeft", "LandscapeRight"};
};

struct RenderDeviceSettings
{
    /// Render backend to use.
    RenderBackend backend_{};

    /// Initial window settings.
    WindowSettings window_;
    /// Whether to enable debug mode on GPU if possible.
    bool gpuDebug_{};
    /// Adapter ID.
    ea::optional<unsigned> adapterId_;
};

/// Wrapper for window and GAPI backend.
class URHO3D_API RenderDevice : public Object
{
    URHO3D_OBJECT(RenderDevice, Object);

public:
    Signal<void()> OnDeviceLost;
    Signal<void()> OnDeviceRestored;

    /// Initialize the OS window and GAPI.
    /// Throws RuntimeException if unrecoverable error occurs.
    RenderDevice(Context* context, const RenderDeviceSettings& settings);
    ~RenderDevice() override;

    /// Update swap chain size according to current dimensions of the window.
    void UpdateSwapChainSize();
    /// Change window settings. Some settings cannot be changed in runtime.
    void UpdateWindowSettings(const WindowSettings& settings);
    /// Restore device if is was lost. Only applicable for Android.
    bool Restore();
    /// Emulate device loss and restore. Only applicable for Android.
    bool EmulateLossAndRestore();

    /// Present the frame. Should be called between engine frames.
    void Present();

    /// Getters.
    /// @{
    const RenderDeviceSettings& GetSettings() const { return settings_; }
    SDL_Window* GetSDLWindow() const { return window_.get(); }
    void* GetMetalView() const { return metalView_.get(); }
    Diligent::IEngineFactory* GetFactory() { return factory_.RawPtr(); }
    Diligent::IRenderDevice* GetRenderDevice() { return renderDevice_.RawPtr(); }
    Diligent::IDeviceContext* GetDeviceContext() { return deviceContext_.RawPtr(); }
    Diligent::ISwapChain* GetSwapChain() { return swapChain_.RawPtr(); }
    IntVector2 GetSwapChainSize() const;
    IntVector2 GetWindowSize() const;
    float GetDpiScale() const;
    /// @}

    /// Static utilities.
    /// @{
    static FullscreenModeVector GetFullscreenModes(int monitor);
    static unsigned GetClosestFullscreenModeIndex(const FullscreenModeVector& modes, FullscreenMode desiredMode);
    static FullscreenMode GetClosestFullscreenMode(const FullscreenModeVector& modes, FullscreenMode desiredMode);
    /// @}

private:
    void InitializeWindow();
    void InitializeFactory();
    void InitializeDevice();

    void InvalidateGLESContext();
    bool RestoreGLESContext();

    RenderDeviceSettings settings_;

    ea::shared_ptr<SDL_Window> window_;
    ea::shared_ptr<void> metalView_;
    ea::shared_ptr<void> glContext_;

    Diligent::RefCntAutoPtr<Diligent::IEngineFactory> factory_;
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice_;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> deviceContext_;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain_;

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
