//
// Copyright (c) 2008-2022 the Urho3D project.
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

#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/span.h>

#include "../Core/Mutex.h"
#include "../Core/Object.h"
#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/ShaderVariation.h"
#include "../IO/FileIdentifier.h"
#include "../Math/Color.h"
#include "../Math/Plane.h"
#include "../Math/Rect.h"
#include "../Resource/Image.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

struct SDL_Window;

namespace Urho3D
{

class ShaderProgramReflection;
class File;
class Image;
class IndexBuffer;
class RenderSurface;
class Shader;
class ShaderVariation;
class Texture;
class Texture2D;
class Texture2DArray;
class TextureCube;
class Vector3;
class Vector4;
class VertexBuffer;
class PipelineState;
class ShaderResourceBinding;
class RenderDevice;
class RenderContext;

/// Graphics settings that should be configured before initialization.
struct GraphicsSettings : public RenderDeviceSettings
{
    /// Current shader translation policy.
    ShaderTranslationPolicy shaderTranslationPolicy_{};

    /// Directory to store cached compiled shaders and logged shader sources.
    FileIdentifier shaderCacheDir_;
    /// Whether to log all compiled shaders.
    bool logShaderSources_{};
    /// Whether the shader validation is enabled.
    bool validateShaders_{};
    /// Whether to discard shader cache on the disk.
    bool discardShaderCache_{};
    /// Whether to cache shaders compiled during this run on the disk.
    bool cacheShaders_{};
};

/// %Graphics subsystem. Manages the application window, rendering state and GPU resources.
class URHO3D_API Graphics : public Object
{
    URHO3D_OBJECT(Graphics, Object);

public:
    /// Construct.
    explicit Graphics(Context* context);
    /// Destruct. Release the Direct3D11 device and close the window.
    ~Graphics() override;

    /// Configure before initial setup.
    void Configure(const GraphicsSettings& settings);

    /// Set window title.
    /// @property
    void SetWindowTitle(const ea::string& windowTitle);
    /// Set window icon.
    /// @property
    void SetWindowIcon(Image* windowIcon);
    /// Set window position. Sets initial position if window is not created yet.
    /// @property
    void SetWindowPosition(const IntVector2& position);
    /// Set window position. Sets initial position if window is not created yet.
    void SetWindowPosition(int x, int y);
    /// Set screen mode. Return true if successful.
    /// Don't use SetScreenMode if ToggleFullscreen is used directly or indirectly.
    bool SetScreenMode(const WindowSettings& windowSettings);
    /// Set window modes to be rotated by ToggleFullscreen. Apply primary window settings immediately.
    /// Return true if successful.
    bool SetWindowModes(const WindowSettings& primarySettings, const WindowSettings& secondarySettings);
    /// Set default window modes. Return true if successful.
    bool SetDefaultWindowModes(const WindowSettings& commonSettings);
    /// Set default window modes. Deprecated. Return true if successful.
    bool SetMode(int width, int height, bool fullscreen, bool borderless, bool resizable,
        bool highDPI, bool vsync, bool tripleBuffer, int multiSample, int monitor, int refreshRate);
    /// Set screen resolution only. Deprecated. Return true if successful.
    bool SetMode(int width, int height);

    /// Initialize pipeline state cache.
    /// Should be called after GPU is initialized and before pipeline states are created.
    void InitializePipelineStateCache(const FileIdentifier& fileName);
    /// Save pipeline state cache.
    void SavePipelineStateCache(const FileIdentifier& fileName);

    /// Toggle between full screen and windowed mode. Return true if successful.
    bool ToggleFullscreen();
    /// Close the window.
    void Close();
    /// Take a screenshot. Return true if successful.
    bool TakeScreenShot(Image& destImage);
    /// Begin frame rendering. Return true if device available and can render.
    bool BeginFrame();
    /// End frame rendering and swap buffers.
    void EndFrame();
    /// Clear any or all of rendertarget, depth buffer and stencil buffer.
    void Clear(ClearTargetFlags flags, const Color& color = Color::TRANSPARENT_BLACK, float depth = 1.0f, unsigned stencil = 0);

    /// Reset all rendertargets, depth-stencil surface and viewport.
    void ResetRenderTargets();

    /// Return whether rendering initialized.
    /// @property
    bool IsInitialized() const;

    /// Return OS-specific external window handle. Null if not in use.
    void* GetExternalWindow() const { return settings_.externalWindowHandle_; }

    /// Return SDL window.
    SDL_Window* GetWindow() const { return window_; }

    /// Return window title.
    /// @property
    const ea::string& GetWindowTitle() const { return windowTitle_; }

    /// Return graphics API name.
    /// @property
    const ea::string& GetApiName() const { return apiName_; }

    /// Return window position.
    /// @property
    IntVector2 GetWindowPosition() const;

    /// Return screen mode parameters.
    const WindowSettings& GetWindowSettings() const;

    /// Return swap chain size.
    const IntVector2 GetSwapChainSize() const;

    /// Return window width in pixels.
    /// @property
    int GetWidth() const { return GetSwapChainSize().x_; }

    /// Return window height in pixels.
    /// @property
    int GetHeight() const { return GetSwapChainSize().y_; }

    /// Return multisample mode (1 = no multisampling).
    /// @property
    int GetMultiSample() const { return GetWindowSettings().multiSample_; }

    /// Return window size in pixels.
    /// @property
    IntVector2 GetSize() const { return GetSwapChainSize(); }

    /// Return whether window is fullscreen.
    /// @property
    bool GetFullscreen() const { return GetWindowSettings().mode_ == WindowMode::Fullscreen; }

    /// Return whether window is borderless.
    /// @property
    bool GetBorderless() const { return GetWindowSettings().mode_ == WindowMode::Borderless; }

    /// Return whether window is resizable.
    /// @property
    bool GetResizable() const { return GetWindowSettings().resizable_; }

    /// Return whether vertical sync is on.
    /// @property
    bool GetVSync() const { return GetWindowSettings().vSync_; }

    /// Return refresh rate when using vsync in fullscreen
    int GetRefreshRate() const { return GetWindowSettings().refreshRate_; }

    /// Return the current monitor index. Effective on in fullscreen
    int GetMonitor() const { return GetWindowSettings().monitor_; }

    /// Return whether the main window is using sRGB conversion on write.
    /// @property
    bool GetSRGB() const { return GetWindowSettings().sRGB_; }

    /// Return dummy color texture format for shadow maps. Is "NULL" (consume no video memory) if supported.
    TextureFormat GetDummyColorFormat() const { return TextureFormat::TEX_FORMAT_UNKNOWN; }

    /// Return shadow map depth texture format, or 0 if not supported.
    TextureFormat GetShadowMapFormat() const { return TextureFormat::TEX_FORMAT_UNKNOWN; }

    /// Return 24-bit shadow map depth texture format, or 0 if not supported.
    TextureFormat GetHiresShadowMapFormat() const { return TextureFormat::TEX_FORMAT_UNKNOWN; }

    /// Return whether hardware instancing is supported.
    /// @property
    bool GetInstancingSupport() const { return true; }

    /// Return whether shadow map depth compare is done in hardware.
    /// @property
    bool GetHardwareShadowSupport() const { return true; }

    /// Return supported fullscreen resolutions (third component is refreshRate). Will be empty if listing the resolutions is not supported on the platform (e.g. Web).
    /// @property
    ea::vector<IntVector3> GetResolutions(int monitor) const;
    /// Return index of the best resolution for requested width, height and refresh rate.
    unsigned FindBestResolutionIndex(int monitor, int width, int height, int refreshRate) const;
    /// Return the desktop resolution.
    /// @property
    IntVector2 GetDesktopResolution(int monitor) const;
    /// Return the number of currently connected monitors.
    /// @property
    int GetMonitorCount() const;
    /// Returns the index of the display containing the center of the window on success or a negative error code on failure.
    /// @property
    int GetCurrentMonitor() const;
    /// Returns true if window is maximized or runs in full screen mode.
    /// @property
    bool GetMaximized() const;
    /// Return display dpi information: (hdpi, vdpi, ddpi). On failure returns zero vector.
    /// @property
    Vector3 GetDisplayDPI(int monitor=0) const;

    /// Return hardware format for a compressed image format, or 0 if unsupported.
    TextureFormat GetFormat(CompressedFormat format) const;
    /// Return a shader variation by name and defines.
    ShaderVariation* GetShader(ShaderType type, const ea::string& name, const ea::string& defines = EMPTY_STRING) const;
    /// Return a shader variation by name and defines.
    ShaderVariation* GetShader(ShaderType type, const char* name, const char* defines) const;

    /// Return current rendertarget width and height.
    IntVector2 GetRenderTargetDimensions() const;

    /// Window was resized through user interaction. Called by Input subsystem.
    void OnWindowResized();
    /// Window was moved through user interaction. Called by Input subsystem.
    void OnWindowMoved();
    /// Restore GPU objects and reinitialize state. Requires an open window. Used only on OpenGL.
    /// @nobind
    void Restore();
    /// Maximize the window.
    void Maximize();
    /// Minimize the window.
    void Minimize();
    /// Raises window if it was minimized.
    void Raise() const;

    /// Sets the maximum number of supported bones for hardware skinning. Check GPU capabilities before setting.
    static void SetMaxBones(unsigned maxBones);
    /// Return maximum number of supported bones for skinning.
    static unsigned GetMaxBones();
    /// Return whether is using an OpenGL 3 context. Return always false on DirectX 11.
    static bool GetGL3Support() { return true; }

    /// Get the SDL_Window as a void* to avoid having to include the graphics implementation
    void* GetSDLWindow() { return window_; }

    /// Getters.
    /// @{
    RenderBackend GetRenderBackend() const;
    const GraphicsSettings& GetSettings() const { return settings_; }
    /// @}

private:
    /// Create the application window icon.
    void CreateWindowIcon();
    /// Called when screen mode is successfully changed by the backend.
    void OnScreenModeChanged();

    /// SDL window.
    SDL_Window* window_{};
    /// Window title.
    ea::string windowTitle_;
    /// Window icon image.
    WeakPtr<Image> windowIcon_;
    /// Most recently applied window settings. It may not represent actual window state
    /// if window was resized by user or Graphics::SetScreenMode was explicitly called.
    WindowSettings primaryWindowSettings_;
    /// Secondary window mode to be applied on Graphics::ToggleFullscreen.
    WindowSettings secondaryWindowSettings_;
    /// Window position.
    IntVector2 position_;
    /// ETC1 format support flag.
    bool etcTextureSupport_{};
    /// ETC2 format support flag.
    bool etc2TextureSupport_{};
    /// PVRTC formats support flag.
    bool pvrtcTextureSupport_{};
    /// Base directory for shaders.
    ea::string shaderPath_;
    /// Shader name prefix for universal shaders.
    ea::string universalShaderNamePrefix_{ "v2/" };
    /// Format string for universal shaders.
    ea::string universalShaderPath_{ "Shaders/GLSL/{}.glsl" };
    /// File extension for shaders.
    ea::string shaderExtension_;
    /// Last used shader in shader variation query.
    mutable WeakPtr<Shader> lastShader_;
    /// Last used shader name in shader variation query.
    mutable ea::string lastShaderName_;
    /// Graphics API name.
    ea::string apiName_;

    GraphicsSettings settings_;

    SharedPtr<RenderDevice> renderDevice_;

    /// Max number of bones which can be skinned on GPU. Zero means default value.
    static unsigned maxBonesHWSkinned;
};

/// Register Graphics library objects.
/// @nobind
void URHO3D_API RegisterGraphicsLibrary(Context* context);

}
