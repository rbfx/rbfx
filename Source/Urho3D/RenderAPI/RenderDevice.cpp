// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RenderDevice.h"

#include "Urho3D/Core/ProcessUtils.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/DeviceObject.h"
#include "Urho3D/RenderAPI/DrawCommandQueue.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/RawTexture.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderContext.h"
#include "Urho3D/RenderAPI/RenderPool.h"

#include <Diligent/Common/interface/DefaultRawMemoryAllocator.hpp>
#include <Diligent/Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp>
#include <Diligent/Graphics/GraphicsEngine/include/SwapChainBase.hpp>
#include <Diligent/Graphics/GraphicsEngine/interface/RenderDevice.h>

#include <EASTL/sort.h>

#include <SDL.h>
#include <SDL_syswm.h>

#if D3D11_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineD3D11/interface/DeviceContextD3D11.h>
    #include <Diligent/Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>
    #include <Diligent/Graphics/GraphicsEngineD3D11/interface/RenderDeviceD3D11.h>
    #include <Diligent/Graphics/GraphicsEngineD3D11/interface/SwapChainD3D11.h>
#endif

#if D3D12_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineD3D12/interface/DeviceContextD3D12.h>
    #include <Diligent/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
    #include <Diligent/Graphics/GraphicsEngineD3D12/interface/RenderDeviceD3D12.h>
    #include <Diligent/Graphics/GraphicsEngineD3D12/interface/SwapChainD3D12.h>
#endif

#if VULKAN_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineVulkan/interface/DeviceContextVk.h>
    #include <Diligent/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
    #include <Diligent/Graphics/GraphicsEngineVulkan/interface/RenderDeviceVk.h>
    #include <Diligent/Graphics/GraphicsEngineVulkan/interface/SwapChainVk.h>
#endif

// OpenGL includes
#if GL_SUPPORTED || GLES_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/DeviceContextGL.h>
    #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h>
    #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/RenderDeviceGL.h>
    #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/SwapChainGL.h>
    #if GLES_SUPPORTED && (URHO3D_PLATFORM_WEB || URHO3D_PLATFORM_ANDROID)
        #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/RenderDeviceGLES.h>
    #endif
#endif

#if URHO3D_PLATFORM_MACOS || URHO3D_PLATFORM_IOS || URHO3D_PLATFORM_TVOS
    #include <SDL_metal.h>
#endif

#if URHO3D_PLATFORM_UNIVERSAL_WINDOWS
    #include <windows.ui.core.h>
#endif

#ifdef None
    #undef None
#endif

#ifdef WIN32
// Prefer the high-performance GPU on switchable GPU systems
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace Urho3D
{

namespace
{

template <class T, class U> void CopyOptionalMember(T& dest, const U& src)
{
    if (src)
        dest = *src;
}

void CopyBackendDeviceSettings(Diligent::EngineVkCreateInfo& createInfo, RenderDeviceSettingsVulkan& settings)
{
    CopyOptionalMember(createInfo.MainDescriptorPoolSize, settings.mainDescriptorPoolSize_);
    CopyOptionalMember(createInfo.DynamicDescriptorPoolSize, settings.dynamicDescriptorPoolSize_);
    CopyOptionalMember(createInfo.DeviceLocalMemoryPageSize, settings.deviceLocalMemoryPageSize_);
    CopyOptionalMember(createInfo.HostVisibleMemoryPageSize, settings.hostVisibleMemoryPageSize_);
    CopyOptionalMember(createInfo.DeviceLocalMemoryReserveSize, settings.deviceLocalMemoryReserveSize_);
    CopyOptionalMember(createInfo.HostVisibleMemoryReserveSize, settings.hostVisibleMemoryReserveSize_);
    CopyOptionalMember(createInfo.UploadHeapPageSize, settings.uploadHeapPageSize_);
    CopyOptionalMember(createInfo.DynamicHeapSize, settings.dynamicHeapSize_);
    CopyOptionalMember(createInfo.DynamicHeapPageSize, settings.dynamicHeapPageSize_);
    for (unsigned i = 1; i < Diligent::QUERY_TYPE_NUM_TYPES; ++i)
        CopyOptionalMember(createInfo.QueryPoolSizes[i], settings.queryPoolSizes_[i]);
}

void CopyBackendDeviceSettings(Diligent::EngineD3D12CreateInfo& createInfo, RenderDeviceSettingsD3D12& settings)
{
    for (unsigned i = 0; i < 4; ++i)
        CopyOptionalMember(createInfo.CPUDescriptorHeapAllocationSize[i], settings.cpuDescriptorHeapAllocationSize_[i]);
    for (unsigned i = 0; i < 2; ++i)
    {
        CopyOptionalMember(createInfo.GPUDescriptorHeapSize[i], settings.gpuDescriptorHeapSize_[i]);
        CopyOptionalMember(createInfo.GPUDescriptorHeapDynamicSize[i], settings.gpuDescriptorHeapDynamicSize_[i]);
        CopyOptionalMember(
            createInfo.DynamicDescriptorAllocationChunkSize[i], settings.dynamicDescriptorAllocationChunkSize_[i]);
    }
    CopyOptionalMember(createInfo.DynamicHeapPageSize, settings.dynamicHeapPageSize_);
    CopyOptionalMember(createInfo.NumDynamicHeapPagesToReserve, settings.numDynamicHeapPagesToReserve_);
    for (unsigned i = 1; i < Diligent::QUERY_TYPE_NUM_TYPES; ++i)
        CopyOptionalMember(createInfo.QueryPoolSizes[i], settings.queryPoolSizes_[i]);
}

void DebugMessageCallback(
    Diligent::DEBUG_MESSAGE_SEVERITY severity, const char* msg, const char* func, const char* file, int line)
{
    ea::string message = Format("[diligent] {}", ea::string(msg == nullptr ? "" : msg));
    if (func)
        message += Format(" function: {}", func);
    if (file)
        message += Format(" file: {}", file);
    if (line)
        message += Format(" line: {}", line);

    switch (severity)
    {
    case Diligent::DEBUG_MESSAGE_SEVERITY_INFO: URHO3D_LOGINFO(message); break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_WARNING: URHO3D_LOGWARNING(message); break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_ERROR: URHO3D_LOGERROR(message); break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR: URHO3D_LOGERROR(message); break;
    }
}

void ValidateWindowSettings(WindowSettings& settings)
{
    const PlatformId platform = GetPlatform();

    // iOS and tvOS app always take the fullscreen (and with status bar hidden)
    if (platform == PlatformId::iOS || platform == PlatformId::tvOS)
        settings.mode_ = WindowMode::Fullscreen;

    // Emscripten cannot be truly fullscreen
    // TODO: Maybe it should be only WindowMode::Windowed?
    if (platform == PlatformId::Web)
    {
        if (settings.mode_ == WindowMode::Fullscreen)
            settings.mode_ = WindowMode::Borderless;
    }

    // UWP doesn't support borderless windows
    if (platform == PlatformId::UniversalWindowsPlatform)
    {
        if (settings.mode_ == WindowMode::Borderless)
            settings.mode_ = WindowMode::Fullscreen;
    }

    // Ensure that monitor index is valid
    const int numMonitors = SDL_GetNumVideoDisplays();
    if (settings.monitor_ >= numMonitors || settings.monitor_ < 0)
        settings.monitor_ = 0;

    // Ensure that multisample factor is valid
    settings.multiSample_ = NextPowerOfTwo(Clamp(settings.multiSample_, 1, 16));

    if (platform == PlatformId::iOS)
        settings.resizable_ = true; // iOS window needs to be resizable to handle orientation changes properly
    else if (settings.mode_ != WindowMode::Windowed)
        settings.resizable_ = false; // Only Windowed window can be resizable

    // Deduce window size and refresh rate if not specified
    static const IntVector2 defaultWindowSize{1024, 768};
    SDL_DisplayMode mode;
    if (SDL_GetDesktopDisplayMode(settings.monitor_, &mode) != 0)
    {
        URHO3D_LOGERROR("Failed to get desktop display mode: {}", SDL_GetError());
        settings.mode_ = WindowMode::Windowed;
        settings.size_ = defaultWindowSize;
        settings.refreshRate_ = 60;
    }
    else
    {
        if (settings.size_ == IntVector2::ZERO)
            settings.size_ = settings.mode_ == WindowMode::Windowed ? defaultWindowSize : IntVector2{mode.w, mode.h};

        if (settings.refreshRate_ == 0 || settings.mode_ != WindowMode::Fullscreen)
            settings.refreshRate_ = mode.refresh_rate;
    }

    // If fullscreen, snap to the closest matching mode
    if (settings.mode_ == WindowMode::Fullscreen)
    {
        const FullscreenModeVector modes = RenderDevice::GetFullscreenModes(settings.monitor_);
        if (!modes.empty())
        {
            const FullscreenMode desiredMode{settings.size_, settings.refreshRate_};
            const FullscreenMode closestMode = RenderDevice::GetClosestFullscreenMode(modes, desiredMode);
            settings.size_ = closestMode.size_;
            settings.refreshRate_ = closestMode.refreshRate_;
        }
    }
}

unsigned ToSDLFlag(WindowMode mode)
{
    switch (mode)
    {
    case WindowMode::Fullscreen: return SDL_WINDOW_FULLSCREEN;
    case WindowMode::Borderless: return SDL_WINDOW_FULLSCREEN_DESKTOP;
    default:
    case WindowMode::Windowed: return 0;
    }
}

void SetWindowFullscreen(SDL_Window* window, const WindowSettings& settings)
{
    SDL_DisplayMode* fullscreenDisplayMode = nullptr;
    if (settings.mode_ == WindowMode::Fullscreen)
    {
        static SDL_DisplayMode temp;
        const SDL_DisplayMode desiredMode{
            SDL_PIXELFORMAT_UNKNOWN, settings.size_.x_, settings.size_.y_, settings.refreshRate_, nullptr};
        fullscreenDisplayMode = SDL_GetClosestDisplayMode(settings.monitor_, &desiredMode, &temp);
    }

    SDL_SetWindowFullscreen(window, 0);
    if (fullscreenDisplayMode)
        SDL_SetWindowDisplayMode(window, fullscreenDisplayMode);
    SDL_SetWindowFullscreen(window, ToSDLFlag(settings.mode_));
}

ea::shared_ptr<SDL_Window> CreateEmptyWindow(
    RenderBackend backend, const WindowSettings& settings, void* externalWindowHandle)
{
    unsigned flags = 0;
    if (externalWindowHandle == nullptr)
    {
        if (GetPlatform() != PlatformId::Web)
            flags |= SDL_WINDOW_ALLOW_HIGHDPI;
        if (settings.resizable_)
            flags |= SDL_WINDOW_RESIZABLE; // | SDL_WINDOW_MAXIMIZED;
        if (settings.mode_ == WindowMode::Borderless)
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        if (IsMetalBackend(backend))
        {
            flags |= SDL_WINDOW_METAL;
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
        }
    }

    const int x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(settings.monitor_);
    const int y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(settings.monitor_);
    const int w = settings.size_.x_;
    const int h = settings.size_.y_;

    SDL_SetHint(SDL_HINT_ORIENTATIONS, ea::string::joined(settings.orientations_, " ").c_str());
    SDL_SetHint(SDL_HINT_VIDEO_EXTERNAL_CONTEXT, "1");
    SDL_Window* window = externalWindowHandle == nullptr //
        ? SDL_CreateWindow(settings.title_.c_str(), x, y, w, h, flags)
        : SDL_CreateWindowFrom(externalWindowHandle, flags);

    if (!window)
        throw RuntimeException("Could not create window: {}", SDL_GetError());

    SetWindowFullscreen(window, settings);

    // Window size is off on UWP if it was created with the same size as on previous run.
    // Tweak it a bit to force the correct size.
    if (GetPlatform() == PlatformId::UniversalWindowsPlatform && settings.mode_ == WindowMode::Windowed)
    {
        SDL_SetWindowSize(window, settings.size_.x_ - 1, settings.size_.y_ + 1);
        SDL_SetWindowSize(window, settings.size_.x_, settings.size_.y_);
    }

    return ea::shared_ptr<SDL_Window>(window, SDL_DestroyWindow);
}

ea::shared_ptr<SDL_Window> CreateOpenGLWindow(bool es, const WindowSettings& settings, void* externalWindowHandle)
{
    unsigned flags = SDL_WINDOW_OPENGL;
    if (externalWindowHandle == nullptr)
    {
        flags |= SDL_WINDOW_SHOWN;
        if (GetPlatform() != PlatformId::Web)
            flags |= SDL_WINDOW_ALLOW_HIGHDPI;
        if (settings.resizable_)
            flags |= SDL_WINDOW_RESIZABLE; // | SDL_WINDOW_MAXIMIZED; | SDL_WINDOW_MAXIMIZED;
        if (settings.mode_ == WindowMode::Borderless)
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    const int x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(settings.monitor_);
    const int y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(settings.monitor_);
    const int w = settings.size_.x_;
    const int h = settings.size_.y_;

    SDL_SetHint(SDL_HINT_ORIENTATIONS, ea::string::joined(settings.orientations_, " ").c_str());

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (es)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    }

    for (int colorBits : {8, 1})
    {
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, colorBits);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, colorBits);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, colorBits);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, externalWindowHandle ? 8 : 0);

        for (int depthBits : {24, 16})
        {
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBits);

            for (int stencilBits : {8, 0})
            {
                SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBits);

                for (bool srgb : {true, false})
                {
                    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, srgb);

                    for (int multiSample = settings.multiSample_; multiSample > 0; multiSample /= 2)
                    {
                        if (multiSample > 1)
                        {
                            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
                            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multiSample);
                        }
                        else
                        {
                            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
                            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
                        }

                        SDL_Window* window = externalWindowHandle == nullptr
                            ? SDL_CreateWindow(settings.title_.c_str(), x, y, w, h, flags)
                            : SDL_CreateWindowFrom(externalWindowHandle, flags);

                        if (window)
                        {
                            SetWindowFullscreen(window, settings);

                            return ea::shared_ptr<SDL_Window>(window, SDL_DestroyWindow);
                        }
                    }
                }
            }
        }
    }

    throw RuntimeException("Could not create window: {}", SDL_GetError());
}

ea::shared_ptr<void> CreateMetalView(SDL_Window* window)
{
    SDL_MetalView metalView = SDL_Metal_CreateView(window);
    if (!metalView)
        throw RuntimeException("Could not create Metal view: {}", SDL_GetError());

    return ea::shared_ptr<void>(metalView, SDL_Metal_DestroyView);
}

/// @note This function is never used for OpenGL backend!
Diligent::NativeWindow GetNativeWindow(SDL_Window* window, void* metalView)
{
    Diligent::NativeWindow result;

#if !URHO3D_PLATFORM_WEB && !URHO3D_PLATFORM_MACOS
    SDL_SysWMinfo sysInfo;
    SDL_VERSION(&sysInfo.version);
    SDL_GetWindowWMInfo(window, &sysInfo);
#endif

#if URHO3D_PLATFORM_WINDOWS
    result.hWnd = sysInfo.info.win.window;
#elif URHO3D_PLATFORM_UNIVERSAL_WINDOWS
    result.pCoreWindow = sysInfo.info.winrt.window;
#elif URHO3D_PLATFORM_LINUX
    result.pDisplay = sysInfo.info.x11.display;
    result.WindowId = sysInfo.info.x11.window;
#elif URHO3D_PLATFORM_MACOS
    result.pNSView = metalView;
#elif URHO3D_PLATFORM_IOS || URHO3D_PLATFORM_TVOS
    result.pCALayer = sysInfo.info.uikit.window;
#elif URHO3D_PLATFORM_ANDROID
    result.pAWindow = sysInfo.info.android.window;
#elif URHO3D_PLATFORM_WEB
    result.pCanvasId = "canvas";
#endif
    return result;
}

unsigned FindBestAdapter(
    Diligent::IEngineFactory* engineFactory, const Diligent::Version& version, ea::optional<unsigned> hintAdapterId)
{
    unsigned numAdapters = 0;
    engineFactory->EnumerateAdapters(version, numAdapters, nullptr);
    ea::vector<Diligent::GraphicsAdapterInfo> adapters(numAdapters);
    engineFactory->EnumerateAdapters(version, numAdapters, adapters.data());

    if (hintAdapterId && *hintAdapterId < numAdapters)
        return *hintAdapterId;

    // Find best quality device
    unsigned result = Diligent::DEFAULT_ADAPTER_ID;
    for (unsigned i = 0; i < numAdapters; ++i)
    {
        const Diligent::GraphicsAdapterInfo& adapter = adapters[i];
        if (adapter.Type == Diligent::ADAPTER_TYPE_INTEGRATED || adapter.Type == Diligent::ADAPTER_TYPE_DISCRETE)
        {
            result = i;
            // Always prefer discrete gpu
            if (adapter.Type == Diligent::ADAPTER_TYPE_DISCRETE)
                break;
        }
    }
    return result;
}

ea::shared_ptr<void> CreateGLContext(SDL_Window* window)
{
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    return glContext ? ea::shared_ptr<void>(glContext, SDL_GL_DeleteContext) : nullptr;
}

TextureFormat SelectDefaultDepthFormat(Diligent::IRenderDevice* device, bool needStencil)
{
    static const TextureFormat depthStencilFormats[] = {
        TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT,
        TextureFormat::TEX_FORMAT_D32_FLOAT_S8X24_UINT,
        TextureFormat::TEX_FORMAT_D32_FLOAT,
        TextureFormat::TEX_FORMAT_D16_UNORM,
    };
    static const TextureFormat depthOnlyFormats[] = {
        TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT,
        TextureFormat::TEX_FORMAT_D32_FLOAT,
        TextureFormat::TEX_FORMAT_D32_FLOAT_S8X24_UINT,
        TextureFormat::TEX_FORMAT_D16_UNORM,
    };

    for (TextureFormat format : (needStencil ? depthStencilFormats : depthOnlyFormats))
    {
        if (device->GetTextureFormatInfoExt(format).BindFlags & Diligent::BIND_DEPTH_STENCIL)
            return format;
    }

    URHO3D_ASSERT(false);
    return TextureFormat::TEX_FORMAT_UNKNOWN;
}

#if GL_SUPPORTED || GLES_SUPPORTED
class ProxySwapChainGL : public Diligent::SwapChainBase<Diligent::ISwapChainGL>
{
public:
    using TBase = Diligent::SwapChainBase<Diligent::ISwapChainGL>;

    ProxySwapChainGL(Diligent::IReferenceCounters* refCounters, Diligent::IRenderDevice* device,
        Diligent::IDeviceContext* deviceContext, const Diligent::SwapChainDesc& swapChainDesc, SDL_Window* window)
        : TBase(refCounters, device, deviceContext, swapChainDesc)
        , window_(window)
    {
        InitializeParameters();
        CreateDummyBuffers();
    }

    /// Implement ISwapChainGL
    /// @{
    void DILIGENT_CALL_TYPE Present(Diligent::Uint32 syncInterval) override { SDL_GL_SwapWindow(window_); }

    void DILIGENT_CALL_TYPE SetFullscreenMode(const Diligent::DisplayModeAttribs& displayMode) override
    {
        URHO3D_ASSERT(false, "Fullscreen mode cannot be set through the proxy swap chain");
    }

    void DILIGENT_CALL_TYPE SetWindowedMode() override
    {
        URHO3D_ASSERT(false, "Windowed mode cannot be set through the proxy swap chain");
    }

    void DILIGENT_CALL_TYPE Resize(
        Diligent::Uint32 newWidth, Diligent::Uint32 newHeight, Diligent::SURFACE_TRANSFORM newPreTransform) override
    {
        if (newPreTransform == Diligent::SURFACE_TRANSFORM_OPTIMAL)
            newPreTransform = Diligent::SURFACE_TRANSFORM_IDENTITY;
        URHO3D_ASSERT(newPreTransform == Diligent::SURFACE_TRANSFORM_IDENTITY, "Unsupported pre-transform");

        if (TBase::Resize(newWidth, newHeight, newPreTransform))
        {
            CreateDummyBuffers();
        }
    }

    GLuint DILIGENT_CALL_TYPE GetDefaultFBO() const override { return defaultFBO_; }

    Diligent::ITextureView* DILIGENT_CALL_TYPE GetCurrentBackBufferRTV() override { return renderTargetView_; }
    Diligent::ITextureView* DILIGENT_CALL_TYPE GetDepthBufferDSV() override { return depthStencilView_; }
    /// @}

private:
    void InitializeParameters()
    {
        if (m_SwapChainDesc.PreTransform == Diligent::SURFACE_TRANSFORM_OPTIMAL)
            m_SwapChainDesc.PreTransform = Diligent::SURFACE_TRANSFORM_IDENTITY;

        // Get default framebuffer for iOS platforms
        const PlatformId platform = GetPlatform();
        if (platform == PlatformId::iOS || platform == PlatformId::tvOS)
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&defaultFBO_));

        // Get swap chain parameters
        int width{};
        int height{};
        SDL_GL_GetDrawableSize(window_, &width, &height);
        m_SwapChainDesc.Width = static_cast<Diligent::Uint32>(width);
        m_SwapChainDesc.Height = static_cast<Diligent::Uint32>(height);

        m_SwapChainDesc.ColorBufferFormat =
            IsSRGB() ? TextureFormat::TEX_FORMAT_RGBA8_UNORM_SRGB : TextureFormat::TEX_FORMAT_RGBA8_UNORM;
        m_SwapChainDesc.DepthBufferFormat = GetDepthStencilFormat();
    }

    void CreateDummyBuffers()
    {
        if (m_SwapChainDesc.Width == 0 || m_SwapChainDesc.Height == 0)
            return;

        Diligent::RefCntAutoPtr<Diligent::IRenderDeviceGL> deviceGL(m_pRenderDevice, Diligent::IID_RenderDeviceGL);

        Diligent::TextureDesc dummyTexDesc;
        dummyTexDesc.Name = "Back buffer proxy";
        dummyTexDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        dummyTexDesc.Format = m_SwapChainDesc.ColorBufferFormat;
        dummyTexDesc.Width = m_SwapChainDesc.Width;
        dummyTexDesc.Height = m_SwapChainDesc.Height;
        dummyTexDesc.BindFlags = Diligent::BIND_RENDER_TARGET;
        Diligent::RefCntAutoPtr<Diligent::ITexture> dummyRenderTarget;
        deviceGL->CreateDummyTexture(dummyTexDesc, Diligent::RESOURCE_STATE_RENDER_TARGET, &dummyRenderTarget);
        renderTargetView_ = dummyRenderTarget->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);

        dummyTexDesc.Name = "Depth buffer proxy";
        dummyTexDesc.Format = m_SwapChainDesc.DepthBufferFormat;
        dummyTexDesc.BindFlags = Diligent::BIND_DEPTH_STENCIL;
        Diligent::RefCntAutoPtr<Diligent::ITexture> dummyDepthBuffer;
        deviceGL->CreateDummyTexture(dummyTexDesc, Diligent::RESOURCE_STATE_DEPTH_WRITE, &dummyDepthBuffer);
        depthStencilView_ = dummyDepthBuffer->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
    }

    bool IsSRGB() const
    {
        int effectiveSRGB{};
        if (SDL_GL_GetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, &effectiveSRGB) != 0)
            return false;
        return effectiveSRGB != 0;
    }

    Diligent::TEXTURE_FORMAT GetDepthStencilFormat() const
    {
        static const Diligent::TEXTURE_FORMAT defaultFormat = TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT;

        int effectiveDepthBits{};
        if (SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &effectiveDepthBits) != 0)
            return defaultFormat;
        int effectiveStencilBits{};
        if (SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &effectiveStencilBits) != 0)
            return defaultFormat;

        if (effectiveDepthBits == 16 && effectiveStencilBits == 0)
            return TextureFormat::TEX_FORMAT_D16_UNORM;
        else if (effectiveDepthBits == 24 && effectiveStencilBits == 0)
            return TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT;
        else if (effectiveDepthBits == 24 && effectiveStencilBits == 8)
            return TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT;
        else if (effectiveDepthBits == 32 && effectiveStencilBits == 0)
            return TextureFormat::TEX_FORMAT_D32_FLOAT;
        else if (effectiveDepthBits == 32 && effectiveStencilBits == 8)
            return TextureFormat::TEX_FORMAT_D32_FLOAT_S8X24_UINT;
        else
            return defaultFormat;
    }

    SDL_Window* window_{};

    Diligent::RefCntAutoPtr<Diligent::ITextureView> renderTargetView_;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> depthStencilView_;

    GLuint defaultFBO_{};
};
#endif

class ProxySwapChainMS : public Diligent::ObjectBase<Diligent::ISwapChain>
{
public:
    using TBase = Diligent::ObjectBase<Diligent::ISwapChain>;

    ProxySwapChainMS(Diligent::IReferenceCounters* refCounters, Diligent::IRenderDevice* device,
        Diligent::IDeviceContext* deviceContext, Diligent::ISwapChain* nativeSwapChain, TextureFormat depthFormat,
        unsigned multiSample)
        : TBase(refCounters)
        , depthFormat_(depthFormat)
        , multiSample_(multiSample)
        , nativeSwapChain_(nativeSwapChain)
        , renderDevice_(device)
        , immediateContext_(deviceContext)
    {
        CreateDepthStencil();
        CreateRenderTarget();
        UpdateDesc();
    }

    /// Implement ISwapChainGL
    /// @{
    void DILIGENT_CALL_TYPE Present(Diligent::Uint32 SyncInterval) override
    {
        if (msaaRenderTarget_)
        {
            Diligent::ITexture* currentBackBuffer = nativeSwapChain_->GetCurrentBackBufferRTV()->GetTexture();

            Diligent::ResolveTextureSubresourceAttribs resolveAttribs;
            resolveAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            resolveAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
            immediateContext_->ResolveTextureSubresource(msaaRenderTarget_, currentBackBuffer, resolveAttribs);
        }

        nativeSwapChain_->Present(SyncInterval);
    }

    const Diligent::SwapChainDesc& DILIGENT_CALL_TYPE GetDesc() const override { return swapChainDesc_; }

    void DILIGENT_CALL_TYPE Resize(
        Diligent::Uint32 newWidth, Diligent::Uint32 newHeight, Diligent::SURFACE_TRANSFORM NewTransform) override
    {
        const Diligent::SwapChainDesc& swapChainDesc = nativeSwapChain_->GetDesc();
        const Diligent::Uint32 oldWidth = swapChainDesc.Width;
        const Diligent::Uint32 oldHeight = swapChainDesc.Height;

        nativeSwapChain_->Resize(newWidth, newHeight, NewTransform);
        UpdateDesc();

        if (swapChainDesc.Width != oldWidth || swapChainDesc.Height != oldHeight)
        {
            CreateDepthStencil();
            CreateRenderTarget();
        }
    }

    void DILIGENT_CALL_TYPE SetFullscreenMode(const Diligent::DisplayModeAttribs& DisplayMode) override
    {
        URHO3D_ASSERT(false, "Fullscreen mode cannot be set through the proxy swap chain");
    }

    void DILIGENT_CALL_TYPE SetWindowedMode() override
    {
        URHO3D_ASSERT(false, "Fullscreen mode cannot be set through the proxy swap chain");
    }

    void DILIGENT_CALL_TYPE SetMaximumFrameLatency(Diligent::Uint32 MaxLatency) override
    {
        nativeSwapChain_->SetMaximumFrameLatency(MaxLatency);
    }

    Diligent::ITextureView* DILIGENT_CALL_TYPE GetCurrentBackBufferRTV() override
    {
        return multiSample_ > 1 ? msaaRenderTargetView_ : nativeSwapChain_->GetCurrentBackBufferRTV();
    }

    Diligent::ITextureView* DILIGENT_CALL_TYPE GetDepthBufferDSV() override { return depthBufferView_; }
    /// @}

private:
    void CreateDepthStencil()
    {
        const Diligent::SwapChainDesc& swapChainDesc = nativeSwapChain_->GetDesc();

        Diligent::TextureDesc desc;
        desc.Name = "Main depth buffer";
        desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        desc.Width = swapChainDesc.Width;
        desc.Height = swapChainDesc.Height;
        desc.Format = depthFormat_;
        desc.SampleCount = multiSample_;
        desc.Usage = Diligent::USAGE_DEFAULT;
        desc.BindFlags = Diligent::BIND_DEPTH_STENCIL;

        Diligent::RefCntAutoPtr<Diligent::ITexture> depthBuffer;
        renderDevice_->CreateTexture(desc, nullptr, &depthBuffer);

        depthBufferView_ = depthBuffer->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
    }

    void CreateRenderTarget()
    {
        if (multiSample_ <= 1)
            return;

        const Diligent::SwapChainDesc& swapChainDesc = nativeSwapChain_->GetDesc();

        Diligent::TextureDesc desc;
        desc.Name = "Main depth buffer";
        desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
        desc.Width = swapChainDesc.Width;
        desc.Height = swapChainDesc.Height;
        desc.Format = swapChainDesc.ColorBufferFormat;
        desc.SampleCount = multiSample_;
        desc.Usage = Diligent::USAGE_DEFAULT;
        desc.BindFlags = Diligent::BIND_RENDER_TARGET;

        Diligent::RefCntAutoPtr<Diligent::ITexture> renderTarget;
        renderDevice_->CreateTexture(desc, nullptr, &renderTarget);

        msaaRenderTarget_ = renderTarget;
        msaaRenderTargetView_ = msaaRenderTarget_->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
    }

    void UpdateDesc()
    {
        swapChainDesc_ = nativeSwapChain_->GetDesc();
        swapChainDesc_.DepthBufferFormat = depthFormat_;
    }

    const TextureFormat depthFormat_{};
    const unsigned multiSample_{};

    const Diligent::RefCntAutoPtr<Diligent::ISwapChain> nativeSwapChain_;
    const Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice_;
    const Diligent::RefCntAutoPtr<Diligent::IDeviceContext> immediateContext_;

    Diligent::RefCntAutoPtr<Diligent::ITextureView> depthBufferView_;
    Diligent::RefCntAutoPtr<Diligent::ITexture> msaaRenderTarget_;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> msaaRenderTargetView_;

    Diligent::SwapChainDesc swapChainDesc_;
};

#if URHO3D_PLATFORM_UNIVERSAL_WINDOWS
IntVector2 CalculateSwapChainSize(SDL_Window* window)
{
    SDL_SysWMinfo sysInfo;
    SDL_VERSION(&sysInfo.version);
    SDL_GetWindowWMInfo(window, &sysInfo);

    const auto displayInfo = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
    const float dpiScale = displayInfo->LogicalDpi / 96.0f;

    const auto coreWindow = reinterpret_cast<Windows::UI::Core::CoreWindow ^>(sysInfo.info.winrt.window);
    const float width = coreWindow->Bounds.Width * dpiScale;
    const float height = coreWindow->Bounds.Height * dpiScale;
    return VectorCeilToInt(Vector2{width, height});
}
#endif

} // namespace

RenderDevice::RenderDevice(
    Context* context, const RenderDeviceSettings& deviceSettings, const WindowSettings& windowSettings)
    : Object(context)
    , deviceSettings_(deviceSettings)
    , windowSettings_(windowSettings)
    , renderPool_(MakeShared<RenderPool>(this))
    , defaultQueue_(MakeShared<DrawCommandQueue>(this))
{
    Diligent::SetDebugMessageCallback(&DebugMessageCallback);

    if (deviceSettings_.externalWindowHandle_)
        windowSettings_.mode_ = WindowMode::Windowed;

    ValidateWindowSettings(windowSettings_);
    InitializeWindow();
    InitializeFactory();
    InitializeDevice();
    InitializeCaps();

    const Diligent::SwapChainDesc& desc = swapChain_->GetDesc();
    URHO3D_LOGINFO("RenderDevice is initialized for {}: size={}x{}px ({}x{}dp), color={}, depth={}",
        ToString(deviceSettings_.backend_), desc.Width, desc.Height, windowSettings_.size_.x_, windowSettings_.size_.y_,
        Diligent::GetTextureFormatAttribs(desc.ColorBufferFormat).Name,
        Diligent::GetTextureFormatAttribs(desc.DepthBufferFormat).Name);
}

void RenderDevice::PostInitialize()
{
    InitializeDefaultObjects();
}

RenderDevice::~RenderDevice()
{
    SendDeviceObjectEvent(DeviceObjectEvent::Destroy);
    deviceContext_->WaitForIdle();
}

void RenderDevice::InitializeWindow()
{
    if (deviceSettings_.backend_ == RenderBackend::OpenGL)
    {
        window_ = CreateOpenGLWindow(
            IsOpenGLESBackend(deviceSettings_.backend_), windowSettings_, deviceSettings_.externalWindowHandle_);

        glContext_ = CreateGLContext(window_.get());
        if (!glContext_)
            throw RuntimeException("Could not create OpenGL context: {}", SDL_GetError());

        int effectiveMultiSample{};
        if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &effectiveMultiSample) == 0)
            windowSettings_.multiSample_ = ea::max(1, effectiveMultiSample);

        int effectiveSRGB{};
        if (SDL_GL_GetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, &effectiveSRGB) == 0)
            windowSettings_.sRGB_ = effectiveSRGB != 0;

        SDL_GL_SetSwapInterval(windowSettings_.vSync_ ? 1 : 0);
    }
    else
    {
        window_ = CreateEmptyWindow(deviceSettings_.backend_, windowSettings_, deviceSettings_.externalWindowHandle_);
        if (IsMetalBackend(deviceSettings_.backend_))
            metalView_ = CreateMetalView(window_.get());
    }

    SDL_GetWindowSize(window_.get(), &windowSettings_.size_.x_, &windowSettings_.size_.y_);
}

void RenderDevice::InitializeFactory()
{
    switch (deviceSettings_.backend_)
    {
#if D3D11_SUPPORTED
    case RenderBackend::D3D11:
        factoryD3D11_ = Diligent::GetEngineFactoryD3D11();
        factory_ = factoryD3D11_;
        break;
#endif
#if D3D12_SUPPORTED
    case RenderBackend::D3D12:
        factoryD3D12_ = Diligent::GetEngineFactoryD3D12();
        if (!factoryD3D12_->LoadD3D12())
            throw RuntimeException("Could not load D3D12 runtime");
        factory_ = factoryD3D12_;
        break;
#endif
#if GL_SUPPORTED || GLES_SUPPORTED
    case RenderBackend::OpenGL:
        factoryOpenGL_ = Diligent::GetEngineFactoryOpenGL();
        factory_ = factoryOpenGL_;
        break;
#endif
#if VULKAN_SUPPORTED
    case RenderBackend::Vulkan:
        factoryVulkan_ = Diligent::GetEngineFactoryVk();
        factory_ = factoryVulkan_;
        break;
#endif
    default: throw RuntimeException("Unsupported render backend");
    }
}

void RenderDevice::InitializeDevice()
{
    Diligent::NativeWindow nativeWindow = GetNativeWindow(window_.get(), metalView_.get());

    const Diligent::TEXTURE_FORMAT colorFormats[2][2] = {
        {TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFormat::TEX_FORMAT_RGBA8_UNORM_SRGB},
        {TextureFormat::TEX_FORMAT_BGRA8_UNORM, TextureFormat::TEX_FORMAT_BGRA8_UNORM_SRGB},
    };

    // Don't bother with deducing the format for now
    const bool isBGRA = false;

    Diligent::SwapChainDesc swapChainDesc{};
    swapChainDesc.ColorBufferFormat = colorFormats[isBGRA][windowSettings_.sRGB_];
    swapChainDesc.DepthBufferFormat = TextureFormat::TEX_FORMAT_UNKNOWN;
#if URHO3D_PLATFORM_UNIVERSAL_WINDOWS
    const IntVector2 swapChainSize = CalculateSwapChainSize(window_.get());
    swapChainDesc.Width = swapChainSize.x_;
    swapChainDesc.Height = swapChainSize.y_;
#endif

    Diligent::FullScreenModeDesc fullscreenDesc{};
    fullscreenDesc.Fullscreen = windowSettings_.mode_ == WindowMode::Fullscreen;
    fullscreenDesc.RefreshRateNumerator = windowSettings_.refreshRate_;
    fullscreenDesc.RefreshRateDenominator = 1;

    switch (deviceSettings_.backend_)
    {
#if D3D11_SUPPORTED
    case RenderBackend::D3D11:
    {
        Diligent::EngineD3D11CreateInfo createInfo;
        createInfo.GraphicsAPIVersion = Diligent::Version{11, 0};
        createInfo.AdapterId = FindBestAdapter(factory_, createInfo.GraphicsAPIVersion, deviceSettings_.adapterId_);
        createInfo.EnableValidation = true;
        createInfo.D3D11ValidationFlags = Diligent::D3D11_VALIDATION_FLAG_VERIFY_COMMITTED_RESOURCE_RELEVANCE;

        factoryD3D11_->CreateDeviceAndContextsD3D11(createInfo, &renderDevice_, &deviceContext_);

        Diligent::RefCntAutoPtr<Diligent::ISwapChain> nativeSwapChain;
        factoryD3D11_->CreateSwapChainD3D11(
            renderDevice_, deviceContext_, swapChainDesc, fullscreenDesc, nativeWindow, &nativeSwapChain);
        InitializeMultiSampleSwapChain(nativeSwapChain);

        renderDeviceD3D11_ =
            Diligent::RefCntAutoPtr<Diligent::IRenderDeviceD3D11>(renderDevice_, Diligent::IID_RenderDeviceD3D11);
        deviceContextD3D11_ =
            Diligent::RefCntAutoPtr<Diligent::IDeviceContextD3D11>(deviceContext_, Diligent::IID_DeviceContextD3D11);
        swapChainD3D11_ =
            Diligent::RefCntAutoPtr<Diligent::ISwapChainD3D11>(nativeSwapChain, Diligent::IID_SwapChainD3D11);
        break;
    }
#endif
#if D3D12_SUPPORTED
    case RenderBackend::D3D12:
    {
        Diligent::EngineD3D12CreateInfo createInfo;
        CopyBackendDeviceSettings(createInfo, deviceSettings_.d3d12_);

        createInfo.GraphicsAPIVersion = Diligent::Version{11, 0};
        createInfo.AdapterId = FindBestAdapter(factory_, createInfo.GraphicsAPIVersion, deviceSettings_.adapterId_);

        factoryD3D12_->CreateDeviceAndContextsD3D12(createInfo, &renderDevice_, &deviceContext_);

        Diligent::RefCntAutoPtr<Diligent::ISwapChain> nativeSwapChain;
        factoryD3D12_->CreateSwapChainD3D12(
            renderDevice_, deviceContext_, swapChainDesc, fullscreenDesc, nativeWindow, &nativeSwapChain);
        InitializeMultiSampleSwapChain(nativeSwapChain);

        renderDeviceD3D12_ =
            Diligent::RefCntAutoPtr<Diligent::IRenderDeviceD3D12>(renderDevice_, Diligent::IID_RenderDeviceD3D12);
        deviceContextD3D12_ =
            Diligent::RefCntAutoPtr<Diligent::IDeviceContextD3D12>(deviceContext_, Diligent::IID_DeviceContextD3D12);
        swapChainD3D12_ =
            Diligent::RefCntAutoPtr<Diligent::ISwapChainD3D12>(nativeSwapChain, Diligent::IID_SwapChainD3D12);
        break;
    }
#endif
#if VULKAN_SUPPORTED
    case RenderBackend::Vulkan:
    {
        Diligent::EngineVkCreateInfo createInfo;
        CopyBackendDeviceSettings(createInfo, deviceSettings_.vulkan_);

        const auto instanceExtensionsCStr = ToCStringVector(deviceSettings_.vulkan_.instanceExtensions_);
        const auto deviceExtensionsCStr = ToCStringVector(deviceSettings_.vulkan_.deviceExtensions_);
        createInfo.InstanceExtensionCount = instanceExtensionsCStr.size();
        createInfo.ppInstanceExtensionNames = instanceExtensionsCStr.data();
        createInfo.DeviceExtensionCount = deviceExtensionsCStr.size();
        createInfo.ppDeviceExtensionNames = deviceExtensionsCStr.data();

        const char* const ppIgnoreDebugMessages[] = {
            // Validation Performance Warning: [ UNASSIGNED-CoreValidation-Shader-OutputNotConsumed ]
            // vertex shader writes to output location 1.0 which is not consumed by fragment shader
            "UNASSIGNED-CoreValidation-Shader-OutputNotConsumed" //
        };
        createInfo.Features = Diligent::DeviceFeatures{Diligent::DEVICE_FEATURE_STATE_OPTIONAL};
        createInfo.Features.TransferQueueTimestampQueries = Diligent::DEVICE_FEATURE_STATE_DISABLED;
        createInfo.ppIgnoreDebugMessageNames = ppIgnoreDebugMessages;
        createInfo.IgnoreDebugMessageCount = _countof(ppIgnoreDebugMessages);
        createInfo.AdapterId = FindBestAdapter(factory_, createInfo.GraphicsAPIVersion, deviceSettings_.adapterId_);

        factoryVulkan_->CreateDeviceAndContextsVk(createInfo, &renderDevice_, &deviceContext_);

        Diligent::RefCntAutoPtr<Diligent::ISwapChain> nativeSwapChain;
        factoryVulkan_->CreateSwapChainVk(renderDevice_, deviceContext_, swapChainDesc, nativeWindow, &nativeSwapChain);
        InitializeMultiSampleSwapChain(nativeSwapChain);

        renderDeviceVulkan_ =
            Diligent::RefCntAutoPtr<Diligent::IRenderDeviceVk>(renderDevice_, Diligent::IID_RenderDeviceVk);
        deviceContextVulkan_ =
            Diligent::RefCntAutoPtr<Diligent::IDeviceContextVk>(deviceContext_, Diligent::IID_DeviceContextVk);
        swapChainVulkan_ = Diligent::RefCntAutoPtr<Diligent::ISwapChainVk>(nativeSwapChain, Diligent::IID_SwapChainVk);
        break;
    }
#endif
#if GL_SUPPORTED || GLES_SUPPORTED
    case RenderBackend::OpenGL:
    {
        Diligent::EngineGLCreateInfo createInfo;
        createInfo.AdapterId = FindBestAdapter(factory_, createInfo.GraphicsAPIVersion, deviceSettings_.adapterId_);

        factoryOpenGL_->AttachToActiveGLContext(createInfo, &renderDevice_, &deviceContext_);

        renderDeviceGL_ =
            Diligent::RefCntAutoPtr<Diligent::IRenderDeviceGL>(renderDevice_, Diligent::IID_RenderDeviceGL);
        deviceContextGL_ =
            Diligent::RefCntAutoPtr<Diligent::IDeviceContextGL>(deviceContext_, Diligent::IID_DeviceContextGL);
    #if GLES_SUPPORTED && (URHO3D_PLATFORM_WEB || URHO3D_PLATFORM_ANDROID)
        renderDeviceGLES_ =
            Diligent::RefCntAutoPtr<Diligent::IRenderDeviceGLES>(renderDevice_, Diligent::IID_RenderDeviceGLES);
    #endif

        auto& defaultAllocator = Diligent::DefaultRawMemoryAllocator::GetAllocator();
        swapChainGL_ = NEW_RC_OBJ(defaultAllocator, "ProxySwapChainGL instance", ProxySwapChainGL)(
            renderDevice_, deviceContext_, swapChainDesc, window_.get());
        defaultDepthStencilFormat_ = swapChainGL_->GetDesc().DepthBufferFormat;
        defaultDepthFormat_ = SelectDefaultDepthFormat(renderDevice_, false);
        deviceContextGL_->SetSwapChain(swapChainGL_);

        swapChain_ = swapChainGL_;
        break;
    }
#endif
    default: throw RuntimeException("Unsupported render backend");
    }

    renderContext_ = MakeShared<RenderContext>(this);
}

void RenderDevice::InitializeMultiSampleSwapChain(Diligent::ISwapChain* nativeSwapChain)
{
    defaultDepthStencilFormat_ = SelectDefaultDepthFormat(renderDevice_, true);
    defaultDepthFormat_ = SelectDefaultDepthFormat(renderDevice_, false);

    const TextureFormat colorFormat = nativeSwapChain->GetDesc().ColorBufferFormat;
    const unsigned multiSample = GetSupportedMultiSample(colorFormat, windowSettings_.multiSample_);

    auto& defaultAllocator = Diligent::DefaultRawMemoryAllocator::GetAllocator();
    swapChain_ = NEW_RC_OBJ(defaultAllocator, "ProxySwapChainMS instance", ProxySwapChainMS)(
        renderDevice_, deviceContext_, nativeSwapChain, defaultDepthStencilFormat_, multiSample);
    windowSettings_.multiSample_ = multiSample;
}

void RenderDevice::InitializeCaps()
{
    const Diligent::GraphicsAdapterInfo& adapterInfo = renderDevice_->GetAdapterInfo();

    caps_.computeShaders_ = adapterInfo.Features.ComputeShaders == Diligent::DEVICE_FEATURE_STATE_ENABLED;
    caps_.drawBaseVertex_ = (adapterInfo.DrawCommand.CapFlags & Diligent::DRAW_COMMAND_CAP_FLAG_BASE_VERTEX) != 0;
    // OpenGL ES and some MacOS versions don't have base instance draw.
    caps_.drawBaseInstance_ = !IsOpenGLESBackend(deviceSettings_.backend_) && GetPlatform() != PlatformId::MacOS;

    // OpenGL does not have clear specification when it is allowed
    // to bind read-only depth texture both as depth-stencil view and as shader resource.
    // It certainly does not work in WebGL.
    caps_.readOnlyDepth_ = GetPlatform() != PlatformId::Web;

    caps_.srgbOutput_ = IsRenderTargetFormatSupported(TextureFormat::TEX_FORMAT_RGBA8_UNORM_SRGB)
        || IsRenderTargetFormatSupported(TextureFormat::TEX_FORMAT_BGRA8_UNORM_SRGB);
    caps_.hdrOutput_ = IsRenderTargetFormatSupported(TextureFormat::TEX_FORMAT_RGBA16_FLOAT);

    caps_.constantBufferOffsetAlignment_ = adapterInfo.Buffer.ConstantBufferOffsetAlignment;
    caps_.maxTextureSize_ = adapterInfo.Texture.MaxTexture2DDimension;
    // TODO: Add constant to Diligent.
    // - OpenGL: GL_MAX_VIEWPORT_DIMS
    // - Vulkan: VkPhysicalDeviceLimits::maxViewportDimensions
    caps_.maxRenderTargetSize_ = adapterInfo.Texture.MaxTexture2DDimension;

    ea::unordered_set<ea::string> supportedExtensions;
#if GL_SUPPORTED || GLES_SUPPORTED
    if (deviceSettings_.backend_ == RenderBackend::OpenGL)
    {
        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        for (int i = 0; i < numExtensions; ++i)
        {
            const auto extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
            supportedExtensions.emplace(extension);
        }
    }
#endif

    if (IsOpenGLESBackend(deviceSettings_.backend_))
    {
        caps_.clipDistance_ = supportedExtensions.contains("GL_EXT_clip_cull_distance");
    }
    else
    {
        caps_.clipDistance_ = true;
    }
}

Diligent::RefCntAutoPtr<Diligent::ISwapChain> RenderDevice::CreateSecondarySwapChain(
    SDL_Window* sdlWindow, bool hasDepthBuffer)
{
    const auto metalView = IsMetalBackend(deviceSettings_.backend_) ? CreateMetalView(sdlWindow) : nullptr;
    Diligent::NativeWindow nativeWindow = GetNativeWindow(sdlWindow, metalView.get());
    Diligent::SwapChainDesc swapChainDesc{};
    swapChainDesc.IsPrimary = false;
    swapChainDesc.ColorBufferFormat = swapChain_->GetDesc().ColorBufferFormat;
    swapChainDesc.DepthBufferFormat =
        hasDepthBuffer ? swapChain_->GetDesc().DepthBufferFormat : TextureFormat::TEX_FORMAT_UNKNOWN;
    Diligent::FullScreenModeDesc fullscreenDesc{};

    switch (deviceSettings_.backend_)
    {
#if D3D11_SUPPORTED
    case RenderBackend::D3D11:
    {
        Diligent::RefCntAutoPtr<Diligent::ISwapChain> secondarySwapChain;
        factoryD3D11_->CreateSwapChainD3D11(
            renderDevice_, deviceContext_, swapChainDesc, fullscreenDesc, nativeWindow, &secondarySwapChain);
        return secondarySwapChain;
    }
#endif
#if D3D12_SUPPORTED
    case RenderBackend::D3D12:
    {
        Diligent::RefCntAutoPtr<Diligent::ISwapChain> secondarySwapChain;
        factoryD3D12_->CreateSwapChainD3D12(
            renderDevice_, deviceContext_, swapChainDesc, fullscreenDesc, nativeWindow, &secondarySwapChain);
        return secondarySwapChain;
    }
#endif
#if VULKAN_SUPPORTED
    case RenderBackend::Vulkan:
    {
        Diligent::RefCntAutoPtr<Diligent::ISwapChain> secondarySwapChain;
        factoryVulkan_->CreateSwapChainVk(
            renderDevice_, deviceContext_, swapChainDesc, nativeWindow, &secondarySwapChain);
        return secondarySwapChain;
    }
#endif
#if GL_SUPPORTED || GLES_SUPPORTED
    case RenderBackend::OpenGL:
    {
        SDL_GLContext currentContext = SDL_GL_GetCurrentContext();
        URHO3D_ASSERT(currentContext && currentContext != glContext_.get());

        auto& defaultAllocator = Diligent::DefaultRawMemoryAllocator::GetAllocator();
        auto secondarySwapChain = NEW_RC_OBJ(defaultAllocator, "Secondary ProxySwapChainGL instance", ProxySwapChainGL)(
            renderDevice_, deviceContext_, swapChainDesc, sdlWindow);

        return Diligent::RefCntAutoPtr<Diligent::ISwapChain>{secondarySwapChain};
    }
#endif
    default:
    {
        URHO3D_ASSERT(false, "Unsupported render backend");
        return {};
    }
    }
}

void RenderDevice::UpdateSwapChainSize()
{
    const IntVector2 oldWindowSize = windowSettings_.size_;
    const IntVector2 oldSwapChainSize = GetSwapChainSize();

    SDL_GetWindowSize(window_.get(), &windowSettings_.size_.x_, &windowSettings_.size_.y_);

    switch (deviceSettings_.backend_)
    {
#if GL_SUPPORTED || GLES_SUPPORTED
    case RenderBackend::OpenGL:
    {
        // OpenGL is managed by SDL, use SDL_GL_GetDrawableSize to get the actual size
        int width{};
        int height{};
        SDL_GL_GetDrawableSize(window_.get(), &width, &height);

        swapChain_->Resize(width, height);
        break;
    }
#endif
#if VULKAN_SUPPORTED
    case RenderBackend::Vulkan:
    {
        const VkPhysicalDevice physicalDevice = renderDeviceVulkan_->GetVkPhysicalDevice();
        const VkSurfaceKHR surface = swapChainVulkan_->GetVkSurface();

        VkSurfaceCapabilitiesKHR surfCapabilities = {};
        const VkResult err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCapabilities);
        if (err == VK_SUCCESS && surfCapabilities.currentExtent.width != 0xFFFFFFFF)
        {
            swapChain_->Resize(surfCapabilities.currentExtent.width, surfCapabilities.currentExtent.height);
        }
        else
        {
            URHO3D_LOGERROR("Cannot resize Vulkan swap chain");
        }

        break;
    }
#endif
#if D3D11_SUPPORTED || D3D12_SUPPORTED
    case RenderBackend::D3D11:
    case RenderBackend::D3D12:
    {
    #if URHO3D_PLATFORM_WINDOWS
        // Use client rect to get the actual size on Windows
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window_.get(), &wmInfo);

        RECT rect;
        GetClientRect(wmInfo.info.win.window, &rect);
        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        swapChain_->Resize(width, height);
    #elif URHO3D_PLATFORM_UNIVERSAL_WINDOWS
        const IntVector2 swapChainSize = CalculateSwapChainSize(window_.get());
        swapChain_->Resize(swapChainSize.x_, swapChainSize.y_);
    #else
        URHO3D_ASSERT(false, "Unsupported render backend");
    #endif
        break;
    }
#endif
    default:
    {
        URHO3D_ASSERT(false, "Unsupported render backend");
        break;
    }
    }

    const IntVector2 newSwapChainSize = GetSwapChainSize();
    if (oldWindowSize != windowSettings_.size_ || oldSwapChainSize != newSwapChainSize)
    {
        URHO3D_LOGINFO("Swap chain is resized to {}x{}px ({}x{}dp)", newSwapChainSize.x_, newSwapChainSize.y_,
            windowSettings_.size_.x_, windowSettings_.size_.y_);
    }
}

void RenderDevice::UpdateWindowSettings(const WindowSettings& settings)
{
    WindowSettings& oldSettings = windowSettings_;
    WindowSettings newSettings = settings;
    ValidateWindowSettings(newSettings);

    const bool sizeChanged = oldSettings.size_ != newSettings.size_;
    if (sizeChanged || oldSettings.mode_ != newSettings.mode_ || oldSettings.refreshRate_ != newSettings.refreshRate_)
    {
        oldSettings.size_ = newSettings.size_;
        oldSettings.mode_ = newSettings.mode_;
        oldSettings.refreshRate_ = newSettings.refreshRate_;

        if (sizeChanged && oldSettings.mode_ == WindowMode::Windowed)
        {
            if (GetPlatform() != PlatformId::UniversalWindowsPlatform)
                SDL_SetWindowSize(window_.get(), oldSettings.size_.x_, oldSettings.size_.y_);
            else
                URHO3D_LOGWARNING("Window resize by application is not supported in UWP.");
        }
        SetWindowFullscreen(window_.get(), oldSettings);

        UpdateSwapChainSize();
    }

    if (oldSettings.monitor_ != newSettings.monitor_)
    {
        oldSettings.monitor_ = newSettings.monitor_;

        const int x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(newSettings.monitor_);
        const int y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(newSettings.monitor_);
        SDL_SetWindowPosition(window_.get(), x, y);
    }

    if (oldSettings.title_ != newSettings.title_)
    {
        oldSettings.title_ = newSettings.title_;

        SDL_SetWindowTitle(window_.get(), newSettings.title_.c_str());
    }

    if (oldSettings.resizable_ != newSettings.resizable_)
    {
        oldSettings.resizable_ = newSettings.resizable_;

        SDL_SetWindowResizable(window_.get(), newSettings.resizable_ ? SDL_TRUE : SDL_FALSE);
    }

    if (oldSettings.vSync_ != newSettings.vSync_)
    {
        oldSettings.vSync_ = newSettings.vSync_;

        if (deviceSettings_.backend_ == RenderBackend::OpenGL)
            SDL_GL_SetSwapInterval(windowSettings_.vSync_ ? 1 : 0);
    }
}

void RenderDevice::SetDefaultTextureFilterMode(TextureFilterMode filterMode)
{
    URHO3D_ASSERT(filterMode != FILTER_DEFAULT, "Invalid texture filter mode");

    if (defaultTextureFilterMode_ == filterMode)
        return;

    defaultTextureFilterMode_ = filterMode;
    defaultTextureParametersDirty_ = true;
}

void RenderDevice::SetDefaultTextureAnisotropy(int anisotropy)
{
    anisotropy = ea::max(1, anisotropy);

    if (defaultTextureAnisotropy_ == anisotropy)
        return;

    defaultTextureAnisotropy_ = anisotropy;
    defaultTextureParametersDirty_ = true;
}

bool RenderDevice::Restore()
{
#if URHO3D_PLATFORM_ANDROID
    if (deviceSettings_.backend_ == RenderBackend::Vulkan)
    {
        // We don't have to handle context loss on Android Vulkan.
        // Mandatory resource recreation will be handled by Diligent,
        // and optional memory optimization is not implemented yet.
        return true;
    }
    else if (deviceSettings_.backend_ == RenderBackend::OpenGL)
    {
        if (!SDL_GL_GetCurrentContext())
        {
            InvalidateGLESContext();
            return RestoreGLESContext();
        }
        return true;
    }
    else
    {
        URHO3D_ASSERT(false, "Unsupported render backend");
        return true;
    }
#endif
    return true;
}

void RenderDevice::InvalidateDeviceState()
{
    ReleaseDefaultObjects();
    SendDeviceObjectEvent(DeviceObjectEvent::Invalidate);
    OnDeviceLost(this);
}

void RenderDevice::RestoreDeviceState()
{
    SendDeviceObjectEvent(DeviceObjectEvent::Restore);
    OnDeviceRestored(this);
    InitializeDefaultObjects();
}

bool RenderDevice::EmulateLossAndRestore()
{
    static const int delayMs = 250;
    if (deviceSettings_.backend_ == RenderBackend::Vulkan)
    {
        InvalidateVulkanContext();
        URHO3D_LOGINFO("Emulated context lost");
        SDL_Delay(delayMs);
        return RestoreVulkanContext();
    }
    else if (GetPlatform() == PlatformId::Android)
    {
        URHO3D_ASSERT(deviceSettings_.backend_ == RenderBackend::OpenGL);

        InvalidateGLESContext();
        URHO3D_LOGINFO("Emulated context lost");
        SDL_Delay(delayMs);
        return RestoreGLESContext();
    }
    else
    {
        InvalidateDeviceState();
        URHO3D_LOGINFO("Emulated context lost");
        RestoreDeviceState();
        return true;
    }
}

void RenderDevice::InvalidateGLESContext()
{
#if URHO3D_PLATFORM_ANDROID && GLES_SUPPORTED
    URHO3D_LOGINFO("OpenGL context is lost");
    InvalidateDeviceState();
    deviceContextGL_->InvalidateState();
    renderDeviceGLES_->Invalidate();
    glContext_ = nullptr;
#else
    URHO3D_LOGWARNING("RenderDevice::InvalidateGLESContext is supported only for Android platform");
#endif
}

bool RenderDevice::RestoreGLESContext()
{
#if URHO3D_PLATFORM_ANDROID && GLES_SUPPORTED
    glContext_ = CreateGLContext(window_.get());
    if (!glContext_)
    {
        URHO3D_LOGERROR("Could not restore OpenGL context: {}", SDL_GetError());
        return false;
    }

    renderDeviceGLES_->Resume(nullptr);
    RestoreDeviceState();
    URHO3D_LOGINFO("OpenGL context is restored");
    return true;
#else
    URHO3D_LOGWARNING("RenderDevice::RestoreGLESContext is supported only for Android platform");
    return true;
#endif
}

void RenderDevice::InvalidateVulkanContext()
{
#if VULKAN_SUPPORTED
    URHO3D_LOGINFO("Vulkan context is lost");
    InvalidateDeviceState();

    Diligent::RefCntWeakPtr<Diligent::ISwapChain> oldSwapChain{swapChain_};
    Diligent::RefCntWeakPtr<Diligent::ISwapChain> oldSwapChainVulkan{swapChainVulkan_};

    oldNativeSwapChainDesc_ = ea::make_unique<Diligent::SwapChainDesc>(swapChainVulkan_->GetDesc());
    swapChain_ = nullptr;
    swapChainVulkan_ = nullptr;

    URHO3D_ASSERT(!oldSwapChain.IsValid() && !oldSwapChainVulkan.IsValid());
#else
    URHO3D_LOGWARNING("RenderDevice::InvalidateVulkanContext is supported only for Vulkan backend");
#endif
}

bool RenderDevice::RestoreVulkanContext()
{
#if VULKAN_SUPPORTED
    Diligent::NativeWindow nativeWindow = GetNativeWindow(window_.get(), metalView_.get());

    Diligent::SwapChainDesc swapChainDesc{};
    swapChainDesc.ColorBufferFormat = oldNativeSwapChainDesc_->ColorBufferFormat;
    swapChainDesc.DepthBufferFormat = oldNativeSwapChainDesc_->DepthBufferFormat;
    swapChainDesc.Width = oldNativeSwapChainDesc_->Width;
    swapChainDesc.Height = oldNativeSwapChainDesc_->Height;
    swapChainDesc.PreTransform = oldNativeSwapChainDesc_->PreTransform;

    Diligent::RefCntAutoPtr<Diligent::ISwapChain> nativeSwapChain;
    factoryVulkan_->CreateSwapChainVk(renderDevice_, deviceContext_, swapChainDesc, nativeWindow, &nativeSwapChain);
    if (!nativeSwapChain)
    {
        URHO3D_LOGERROR("Failed to restore swap chain");
        return false;
    }

    InitializeMultiSampleSwapChain(nativeSwapChain);
    swapChain_->Resize(swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.PreTransform);

    swapChainVulkan_ = Diligent::RefCntAutoPtr<Diligent::ISwapChainVk>(nativeSwapChain, Diligent::IID_SwapChainVk);
    oldNativeSwapChainDesc_ = nullptr;

    RestoreDeviceState();
    URHO3D_LOGINFO("Vulkan context is restored");
    return true;
#else
    URHO3D_LOGWARNING("RenderDevice::RestoreVulkanContext is supported only for Vulkan backend");
    return true;
#endif
}

void RenderDevice::QueuePipelineStateReload(PipelineState* pipelineState)
{
    pipelineStatesToReload_.emplace_back(pipelineState);
}

bool RenderDevice::TakeScreenShot(IntVector2& size, ByteVector& data)
{
    const bool flipY = deviceSettings_.backend_ == RenderBackend::OpenGL;

    const auto resolvedBackBuffer = GetResolvedBackBuffer();
    if (!resolvedBackBuffer)
    {
        URHO3D_LOGERROR("Failed to create resolve texture for RenderDevice::TakeScreenShot");
        return false;
    }

    const auto stagingTexture = ReadTextureToStaging(resolvedBackBuffer);
    if (!stagingTexture)
    {
        URHO3D_LOGERROR("Failed to create staging texture for RenderDevice::TakeScreenShot");
        return false;
    }

    Diligent::MappedTextureSubresource mappedData;
    deviceContext_->MapTextureSubresource(
        stagingTexture, 0, 0, Diligent::MAP_READ, Diligent::MAP_FLAG_NONE, nullptr, mappedData);

    if (!mappedData.pData)
    {
        URHO3D_LOGERROR("Failed to map staging texture for RenderDevice::TakeScreenShot");
        return false;
    }

    const Diligent::TextureDesc& desc = stagingTexture->GetDesc();
    const auto& formatInfo = Diligent::GetTextureFormatAttribs(desc.Format);
    if (formatInfo.GetElementSize() != 4 || formatInfo.ComponentType == Diligent::COMPONENT_TYPE_COMPRESSED)
    {
        URHO3D_LOGERROR("Unexpected backbuffer for RenderDevice::TakeScreenShot");
        return false;
    }

    size = {static_cast<int>(desc.Width), static_cast<int>(desc.Height)};

    const unsigned elementSize = formatInfo.GetElementSize();
    const unsigned rowSize = desc.Width * elementSize;
    data.resize(desc.Width * desc.Height * elementSize);

    const auto srcBuffer = reinterpret_cast<const unsigned char*>(mappedData.pData);
    auto destBuffer = reinterpret_cast<unsigned char*>(data.data());
    for (unsigned i = 0; i < size.y_; ++i)
    {
        const unsigned row = flipY ? size.y_ - i - 1 : i;
        memcpy(destBuffer, srcBuffer + row * mappedData.Stride, rowSize);
        destBuffer += rowSize;
    }

    deviceContext_->UnmapTextureSubresource(stagingTexture, 0, 0);
    return true;
}

Diligent::RefCntAutoPtr<Diligent::ITexture> RenderDevice::GetResolvedBackBuffer()
{
    Diligent::ITexture* backBuffer = swapChain_->GetCurrentBackBufferRTV()->GetTexture();
    const Diligent::TextureDesc& backBufferDesc = backBuffer->GetDesc();

    if (backBufferDesc.SampleCount == 1)
        return Diligent::RefCntAutoPtr<Diligent::ITexture>{backBuffer};

    Diligent::TextureDesc textureDesc;
    textureDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
    textureDesc.Name = "RenderDevice::TakeScreenShot resolve texture";
    textureDesc.Usage = Diligent::USAGE_DEFAULT;
    textureDesc.Format = backBufferDesc.Format;
    textureDesc.Width = backBufferDesc.Width;
    textureDesc.Height = backBufferDesc.Height;
    textureDesc.BindFlags = Diligent::BIND_RENDER_TARGET;

    Diligent::RefCntAutoPtr<Diligent::ITexture> resolvedBackBuffer;
    renderDevice_->CreateTexture(textureDesc, nullptr, &resolvedBackBuffer);
    if (!resolvedBackBuffer)
        return {};

    Diligent::ResolveTextureSubresourceAttribs attribs;
    attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    deviceContext_->ResolveTextureSubresource(backBuffer, resolvedBackBuffer, attribs);

    return resolvedBackBuffer;
}

Diligent::RefCntAutoPtr<Diligent::ITexture> RenderDevice::ReadTextureToStaging(Diligent::ITexture* sourceTexture)
{
    const Diligent::TextureDesc& sourceTextureDesc = sourceTexture->GetDesc();

    Diligent::TextureDesc textureDesc;
    textureDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
    textureDesc.Name = "RenderDevice::TakeScreenShot staging texture";
    textureDesc.Usage = Diligent::USAGE_STAGING;
    textureDesc.CPUAccessFlags = Diligent::CPU_ACCESS_READ;
    textureDesc.Format = sourceTextureDesc.Format;
    textureDesc.Width = sourceTextureDesc.Width;
    textureDesc.Height = sourceTextureDesc.Height;

    Diligent::RefCntAutoPtr<Diligent::ITexture> stagingTexture;
    renderDevice_->CreateTexture(textureDesc, nullptr, &stagingTexture);
    if (!stagingTexture)
        return {};

    Diligent::CopyTextureAttribs attribs;
    attribs.pSrcTexture = sourceTexture;
    attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    attribs.pDstTexture = stagingTexture;
    attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    deviceContext_->CopyTexture(attribs);
    deviceContext_->WaitForIdle();

    return stagingTexture;
}

void RenderDevice::Present()
{
    swapChain_->Present(windowSettings_.vSync_ ? 1 : 0);

    // If using an external window, check it for size changes, and reset screen mode if necessary
    if (deviceSettings_.externalWindowHandle_ != nullptr)
    {
        IntVector2 currentSize;
        SDL_GetWindowSize(window_.get(), &currentSize.x_, &currentSize.y_);

        if (windowSettings_.size_ != currentSize)
            UpdateSwapChainSize();
    }

    // Execute postponed work
    renderPool_->OnFrameEnd();

    for (PipelineState* pipelineState : pipelineStatesToReload_)
    {
        if (pipelineState)
            pipelineState->Restore();
    }
    pipelineStatesToReload_.clear();

    if (defaultTextureParametersDirty_)
    {
        defaultTextureParametersDirty_ = false;

        MutexLock lock(deviceObjectsMutex_);
        for (DeviceObject* object : deviceObjects_)
        {
            if (auto pipelineState = dynamic_cast<PipelineState*>(object))
            {
                pipelineState->Invalidate();
                pipelineState->Restore();
            }
        }
    }

    stats_ = renderContext_->GetStats();
    renderContext_->ResetStats();

    if (statsTimer_.GetMSec(false) >= StatsPeriodMs)
    {
        statsTimer_.Reset();
        prevMaxStats_ = maxStats_;
        maxStats_ = stats_;
    }
    else
    {
        maxStats_.numPrimitives_ = ea::max(maxStats_.numPrimitives_, stats_.numPrimitives_);
        maxStats_.numDraws_ = ea::max(maxStats_.numDraws_, stats_.numDraws_);
        maxStats_.numDispatches_ = ea::max(maxStats_.numDispatches_, stats_.numDispatches_);
    }

    // Increment frame index
    frameIndex_ = static_cast<FrameIndex>(static_cast<long long>(frameIndex_) + 1);
    URHO3D_ASSERT(frameIndex_ > FrameIndex::None, "How did you exhaust 2^63 frames?");
}

IntVector2 RenderDevice::GetSwapChainSize() const
{
    if (!swapChain_)
        return IntVector2::ZERO;
    const Diligent::SwapChainDesc& desc = swapChain_->GetDesc();
    return {static_cast<int>(desc.Width), static_cast<int>(desc.Height)};
}

IntVector2 RenderDevice::GetWindowSize() const
{
    return windowSettings_.size_;
}

float RenderDevice::GetDpiScale() const
{
#if defined(URHO3D_PLATFORM_WINDOWS)
    float logicalDpi{};
    if (SDL_GetDisplayDPI(windowSettings_.monitor_, nullptr, &logicalDpi, nullptr) != 0)
        return 1.0f;

    return logicalDpi / 96.0f;
#else
    const Vector2 ratio = GetSwapChainSize().ToVector2() / GetWindowSize().ToVector2();
    // This is just a hack to get rid of possible rounding errors
    return SnapRound((ratio.x_ + ratio.y_) / 2.0f, 0.05f);
#endif
}

ea::vector<FullscreenMode> RenderDevice::GetFullscreenModes(int monitor)
{
    ea::vector<FullscreenMode> result;
#if URHO3D_PLATFORM_WEB
    // Emscripten is not able to return a valid list
#else
    const int numModes = SDL_GetNumDisplayModes(monitor);
    for (int i = 0; i < numModes; ++i)
    {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(monitor, i, &mode);

        result.push_back(FullscreenMode{{mode.w, mode.h}, mode.refresh_rate});
    }

    ea::sort(result.begin(), result.end());
    result.erase(ea::unique(result.begin(), result.end()), result.end());
#endif
    return result;
}

unsigned RenderDevice::GetClosestFullscreenModeIndex(const FullscreenModeVector& modes, FullscreenMode desiredMode)
{
    URHO3D_ASSERT(!modes.empty());

    // 1. Try to find exact match
    {
        const auto iterMatch = ea::find(modes.begin(), modes.end(), desiredMode);
        if (iterMatch != modes.end())
            return static_cast<unsigned>(ea::distance(modes.begin(), iterMatch));
    }

    // 2. Try to find exact resolution match with different refresh rate
    const auto iter = ea::upper_bound(modes.begin(), modes.end(), FullscreenMode{desiredMode.size_, M_MAX_INT});
    if (iter != modes.begin())
    {
        const auto iterMatch = ea::prev(iter);
        if (iterMatch->size_ == desiredMode.size_)
            return static_cast<unsigned>(ea::distance(modes.begin(), iterMatch));
    }

    // 3. Try to find better mode
    if (iter != modes.end())
    {
        const auto isBetter = [&](const FullscreenMode& mode) { return mode.refreshRate_ >= desiredMode.refreshRate_; };
        const auto iterBetterMatch = ea::find_if(iter, modes.end(), isBetter);
        const auto iterMatch = iterBetterMatch != modes.end() ? iterBetterMatch : iter;
        return static_cast<unsigned>(ea::distance(modes.begin(), iterMatch));
    }

    // 4. Pick the best mode
    return modes.size() - 1;
}

FullscreenMode RenderDevice::GetClosestFullscreenMode(const FullscreenModeVector& modes, FullscreenMode desiredMode)
{
    const unsigned index = GetClosestFullscreenModeIndex(modes, desiredMode);
    return modes[index];
}

void RenderDevice::AddDeviceObject(DeviceObject* object)
{
    MutexLock lock(deviceObjectsMutex_);
    deviceObjects_.emplace(object);
}

void RenderDevice::RemoveDeviceObject(DeviceObject* object)
{
    MutexLock lock(deviceObjectsMutex_);
    deviceObjects_.erase(object);
}

void RenderDevice::SendDeviceObjectEvent(DeviceObjectEvent event)
{
    MutexLock lock(deviceObjectsMutex_);
    for (DeviceObject* object : deviceObjects_)
        object->ProcessDeviceObjectEvent(event);
}

bool RenderDevice::IsTextureFormatSupported(TextureFormat format) const
{
    return renderDevice_->GetTextureFormatInfoExt(format).BindFlags != Diligent::BIND_NONE;
}

bool RenderDevice::IsRenderTargetFormatSupported(TextureFormat format) const
{
    const Diligent::TextureFormatInfoExt& info = renderDevice_->GetTextureFormatInfoExt(format);
    return (info.BindFlags & (Diligent::BIND_RENDER_TARGET | Diligent::BIND_DEPTH_STENCIL)) != 0;
}

bool RenderDevice::IsUnorderedAccessFormatSupported(TextureFormat format) const
{
    const Diligent::TextureFormatInfoExt& info = renderDevice_->GetTextureFormatInfoExt(format);
    return (info.BindFlags & Diligent::BIND_UNORDERED_ACCESS) != 0;
}

bool RenderDevice::IsMultiSampleSupported(TextureFormat format, int multiSample) const
{
    const Diligent::TextureFormatInfoExt& info = renderDevice_->GetTextureFormatInfoExt(format);
    return (info.SampleCounts & multiSample) != 0;
}

int RenderDevice::GetSupportedMultiSample(TextureFormat format, int multiSample) const
{
    multiSample = NextPowerOfTwo(Clamp(multiSample, 1, 16));

    const Diligent::TextureFormatInfoExt& formatInfo = renderDevice_->GetTextureFormatInfoExt(format);
    while (multiSample > 1 && ((formatInfo.SampleCounts & multiSample) == 0))
        multiSample >>= 1;
    return ea::max(1, multiSample);
}

void RenderDevice::InitializeDefaultObjects()
{
    const auto createDefaultTexture = [&](const TextureType type, Diligent::RESOURCE_DIMENSION_SUPPORT flag)
    {
        const TextureFormat format = TextureFormat::TEX_FORMAT_RGBA8_UNORM;
        if (!(renderDevice_->GetTextureFormatInfoExt(format).Dimensions & flag))
            return;

        RawTextureParams params;
        params.type_ = type;
        params.format_ = format;
        params.size_ = {1, 1, 1};
        defaultTextures_[type] = ea::make_unique<RawTexture>(context_, params);

        const unsigned char data[4]{};
        defaultTextures_[type]->Update(0, IntVector3::ZERO, IntVector3::ONE, 0, data);
    };

    createDefaultTexture(TextureType::Texture2D, Diligent::RESOURCE_DIMENSION_SUPPORT_TEX_2D);
    createDefaultTexture(TextureType::TextureCube, Diligent::RESOURCE_DIMENSION_SUPPORT_TEX_CUBE);
    createDefaultTexture(TextureType::Texture3D, Diligent::RESOURCE_DIMENSION_SUPPORT_TEX_3D);
    createDefaultTexture(TextureType::Texture2DArray, Diligent::RESOURCE_DIMENSION_SUPPORT_TEX_2D_ARRAY);
    renderPool_->Restore();
}

void RenderDevice::ReleaseDefaultObjects()
{
    defaultTextures_ = {};
    renderPool_->Invalidate();
}

} // namespace Urho3D
