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

#include "../Precompiled.h"

#include "../Core/ProcessUtils.h"
#include "../Core/Profiler.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Animation.h"
#include "../Graphics/AnimationController.h"
#include "../Graphics/Camera.h"
#include "../Graphics/CameraOperator.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/CustomGeometry.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/DecalSet.h"
#include "../Graphics/GlobalIllumination.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/LightBaker.h"
#include "../Graphics/LightProbeGroup.h"
#include "../Graphics/Material.h"
#include "../Graphics/OcclusionBuffer.h"
#include "../Graphics/Octree.h"
#include "../Graphics/OutlineGroup.h"
#include "../Graphics/ParticleEffect.h"
#include "../Graphics/ParticleEmitter.h"
#include "../Graphics/ReflectionProbe.h"
#include "../Graphics/RibbonTrail.h"
#include "../Graphics/Shader.h"
#include "../Graphics/Skybox.h"
#include "../Graphics/StaticModelGroup.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Terrain.h"
#include "../Graphics/TerrainPatch.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/Texture2DArray.h"
#include "../Graphics/Texture3D.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/Viewport.h"
#include "../Graphics/Zone.h"
#include "../IO/FileSystem.h"
#include "../IO/VirtualFileSystem.h"
#include "../IO/Log.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderContext.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/Resource/ResourceCache.h"

#include <SDL.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

ea::string_view ToString(WindowMode mode)
{
    switch (mode)
    {
    case WindowMode::Windowed:
        return "Windowed";
    case WindowMode::Fullscreen:
        return "Fullscreen";
    case WindowMode::Borderless:
        return "Borderless";
    default:
        return "Unknown";
    }
}

WindowMode ToWindowMode(bool fullscreen, bool borderless)
{
    if (fullscreen)
        return WindowMode::Fullscreen;
    else if (borderless)
        return WindowMode::Borderless;
    else
        return WindowMode::Windowed;
}

}

unsigned Graphics::maxBonesHWSkinned = 0;

Graphics::Graphics(Context* context)
    : Object(context)
    , position_(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED)
    , shaderPath_("Shaders/HLSL/")
    , shaderExtension_(".hlsl")
    , apiName_("Diligent")
{
    // TODO: This can be used to have DPI scaling work on Windows, but it leads to blurry fonts
    // SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
    context_->RequireSDL(SDL_INIT_VIDEO);
}

Graphics::~Graphics()
{
    Close();

    context_->ReleaseSDL();
}

void Graphics::Configure(const GraphicsSettings& settings)
{
    settings_ = settings;

    // Be careful: delete only the files we write ourselves
    if (settings_.discardShaderCache_)
    {
        auto fs = GetSubsystem<FileSystem>();
        auto vfs = GetSubsystem<VirtualFileSystem>();

        StringVector cachedShaders;
        vfs->Scan(cachedShaders, settings_.shaderCacheDir_, "*.bytecode", SCAN_FILES | SCAN_APPEND);
        vfs->Scan(cachedShaders, settings_.shaderCacheDir_, "*.glsl", SCAN_FILES | SCAN_APPEND);

        for (const ea::string& fileName : cachedShaders)
        {
            const ea::string absoluteFileName =
                vfs->GetAbsoluteNameFromIdentifier(settings_.shaderCacheDir_ + fileName);
            if (!absoluteFileName.empty())
                fs->Delete(absoluteFileName);
        }
    }
}

bool Graphics::SetScreenMode(const WindowSettings& windowSettings)
{
    URHO3D_PROFILE("SetScreenMode");

    if (!renderDevice_)
    {
        try
        {
            renderDevice_ = MakeShared<RenderDevice>(context_, settings_, windowSettings);
            context_->RegisterSubsystem(renderDevice_);
        }
        catch (const RuntimeException& ex)
        {
            URHO3D_LOGERROR("Failed to create render device: {}", ex.what());
            return false;
        }

        renderDevice_->PostInitialize();

        renderDevice_->OnDeviceLost.Subscribe(this, [this]() { SendEvent(E_DEVICELOST); });
        renderDevice_->OnDeviceRestored.Subscribe(this, [this]() { SendEvent(E_DEVICERESET); });

        apiName_ = ToString(GetRenderBackend());
    }
    else
    {
        renderDevice_->UpdateWindowSettings(windowSettings);
    }

    window_ = renderDevice_->GetSDLWindow();

    // Clear the initial window contents to black
    RenderContext* renderContext = renderDevice_->GetRenderContext();
    renderContext->SetSwapChainRenderTargets();
    renderContext->ClearRenderTarget(0, Color::BLACK);
    renderDevice_->Present();

    OnScreenModeChanged();
    return true;
}

void Graphics::Close()
{
    context_->RemoveSubsystem<RenderDevice>();
    renderDevice_ = nullptr;
}

bool Graphics::TakeScreenShot(Image& destImage)
{
    URHO3D_PROFILE("TakeScreenShot");
    if (!IsInitialized())
        return false;

    IntVector2 size;
    ByteVector data;
    if (!renderDevice_->TakeScreenShot(size, data))
        return false;

    destImage.SetSize(size.x_, size.y_, 4);
    destImage.SetData(data.data());
    return true;
}

bool Graphics::BeginFrame()
{
    if (!IsInitialized())
        return false;

    if (!GetExternalWindow())
    {
        // To prevent a loop of endless device loss and flicker, do not attempt to render when in fullscreen
        // and the window is minimized
        if (GetFullscreen() && (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED))
            return false;
    }

    SendEvent(E_BEGINRENDERING);
    return true;
}

void Graphics::EndFrame()
{
    if (!IsInitialized())
        return;

    {
        URHO3D_PROFILE("Present");

        SendEvent(E_ENDRENDERING);

        renderDevice_->Present();
    }
}

void Graphics::SetWindowTitle(const ea::string& windowTitle)
{
    windowTitle_ = windowTitle;
    if (window_)
        SDL_SetWindowTitle(window_, windowTitle_.c_str());
}

void Graphics::SetWindowIcon(Image* windowIcon)
{
    windowIcon_ = windowIcon;
    if (window_)
        CreateWindowIcon();
}

void Graphics::SetWindowPosition(const IntVector2& position)
{
    if (window_)
        SDL_SetWindowPosition(window_, position.x_, position.y_);
    else
        position_ = position; // Sets as initial position for OpenWindow()
}

void Graphics::SetWindowPosition(int x, int y)
{
    SetWindowPosition(IntVector2(x, y));
}

bool Graphics::SetWindowModes(const WindowSettings& primarySettings, const WindowSettings& secondarySettings)
{
    primaryWindowSettings_ = primarySettings;
    secondaryWindowSettings_ = secondarySettings;
    return SetScreenMode(primaryWindowSettings_);
}

bool Graphics::SetDefaultWindowModes(const WindowSettings& commonSettings)
{
    // Fill window mode to be applied now
    WindowSettings primaryWindowSettings = commonSettings;

    // Fill window mode to be applied on Graphics::ToggleFullscreen
    WindowSettings secondaryWindowSettings = commonSettings;

    // Pick resolution automatically
    secondaryWindowSettings.size_ = IntVector2::ZERO;

    // Use the opposite of the specified window mode
    if (primaryWindowSettings.mode_ == WindowMode::Windowed)
        secondaryWindowSettings.mode_ = WindowMode::Borderless;
    else
        secondaryWindowSettings.mode_ = WindowMode::Windowed;

    return SetWindowModes(primaryWindowSettings, secondaryWindowSettings);
}

bool Graphics::SetMode(int width, int height, bool fullscreen, bool borderless, bool resizable,
    bool /*highDPI*/, bool vsync, bool /*tripleBuffer*/, int multiSample, int monitor, int refreshRate)
{
    WindowSettings params;
    params.size_ = {width, height};
    params.mode_ = ToWindowMode(fullscreen, borderless);
    params.resizable_ = resizable;
    params.vSync_ = vsync;
    params.multiSample_ = multiSample;
    params.monitor_ = monitor;
    params.refreshRate_ = refreshRate;

    return SetDefaultWindowModes(params);
}

bool Graphics::SetMode(int width, int height)
{
    WindowSettings params = GetWindowSettings();
    params.size_ = {width, height};
    return SetDefaultWindowModes(params);
}

void Graphics::InitializePipelineStateCache(const FileIdentifier& fileName)
{
    auto psoCache = context_->RegisterSubsystem<PipelineStateCache>();

    ByteVector cachedData;
    if (fileName)
    {
        auto vfs = GetSubsystem<VirtualFileSystem>();
        if (vfs->Exists(fileName))
        {
            if (const AbstractFilePtr file = vfs->OpenFile(fileName, FILE_READ))
            {
                cachedData.resize(file->GetSize());
                file->Read(cachedData.data(), cachedData.size());
            }
        }
    }

    psoCache->Initialize(cachedData);
}

void Graphics::SavePipelineStateCache(const FileIdentifier& fileName)
{
    if (!fileName)
        return;

    auto psoCache = GetSubsystem<PipelineStateCache>();
    if (!psoCache)
        return;

    const auto cachedData = psoCache->GetCachedData();

    auto vfs = GetSubsystem<VirtualFileSystem>();
    if (const AbstractFilePtr file = vfs->OpenFile(fileName, FILE_WRITE))
        file->Write(cachedData.data(), cachedData.size());
}

bool Graphics::ToggleFullscreen()
{
    ea::swap(primaryWindowSettings_, secondaryWindowSettings_);
    return SetScreenMode(primaryWindowSettings_);
}

IntVector2 Graphics::GetWindowPosition() const
{
    if (window_)
    {
        IntVector2 position;
        SDL_GetWindowPosition(window_, &position.x_, &position.y_);
        return position;
    }
    return position_;
}

const WindowSettings& Graphics::GetWindowSettings() const
{
    if (renderDevice_)
        return renderDevice_->GetWindowSettings();
    else
        return primaryWindowSettings_;
}

const IntVector2 Graphics::GetSwapChainSize() const
{
    if (renderDevice_)
        return renderDevice_->GetSwapChainSize();
    else
        return primaryWindowSettings_.size_;
}

ea::vector<IntVector3> Graphics::GetResolutions(int monitor) const
{
    ea::vector<IntVector3> ret;
    // Emscripten is not able to return a valid list
#ifndef __EMSCRIPTEN__
    auto numModes = (unsigned)SDL_GetNumDisplayModes(monitor);

    for (unsigned i = 0; i < numModes; ++i)
    {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(monitor, i, &mode);
        int width = mode.w;
        int height = mode.h;
        int rate = mode.refresh_rate;

        // Store mode if unique
        bool unique = true;
        for (unsigned j = 0; j < ret.size(); ++j)
        {
            if (ret[j].x_ == width && ret[j].y_ == height && ret[j].z_ == rate)
            {
                unique = false;
                break;
            }
        }

        if (unique)
            ret.push_back(IntVector3(width, height, rate));
    }
#endif

    return ret;
}

unsigned Graphics::FindBestResolutionIndex(int monitor, int width, int height, int refreshRate) const
{
    const ea::vector<IntVector3> resolutions = GetResolutions(monitor);
    if (resolutions.empty())
        return M_MAX_UNSIGNED;

    unsigned best = 0;
    unsigned bestError = M_MAX_UNSIGNED;

    for (unsigned i = 0; i < resolutions.size(); ++i)
    {
        auto error = static_cast<unsigned>(Abs(resolutions[i].x_ - width) + Abs(resolutions[i].y_ - height));
        if (refreshRate != 0)
            error += static_cast<unsigned>(Abs(resolutions[i].z_ - refreshRate));
        if (error < bestError)
        {
            best = i;
            bestError = error;
        }
    }

    return best;
}

IntVector2 Graphics::GetDesktopResolution(int monitor) const
{
#if !defined(__ANDROID__) && !defined(IOS) && !defined(TVOS)
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(monitor, &mode);
    return IntVector2(mode.w, mode.h);
#else
    // SDL_GetDesktopDisplayMode() may not work correctly on mobile platforms. Rather return the window size
    return GetSize();
#endif
}

int Graphics::GetMonitorCount() const
{
    return SDL_GetNumVideoDisplays();
}

int Graphics::GetCurrentMonitor() const
{
    return window_ ? SDL_GetWindowDisplayIndex(window_) : 0;
}

bool Graphics::GetMaximized() const
{
    return window_? static_cast<bool>(SDL_GetWindowFlags(window_) & SDL_WINDOW_MAXIMIZED) : false;
}

Vector3 Graphics::GetDisplayDPI(int monitor) const
{
    Vector3 result;
    SDL_GetDisplayDPI(monitor, &result.z_, &result.x_, &result.y_);
    return result;
}

void Graphics::Maximize()
{
    if (!window_)
        return;

    SDL_MaximizeWindow(window_);
}

void Graphics::Minimize()
{
    if (!window_)
        return;

    SDL_MinimizeWindow(window_);
}

void Graphics::Raise() const
{
    if (!window_)
        return;

    SDL_RaiseWindow(window_);
}

void Graphics::CreateWindowIcon()
{
    if (windowIcon_)
    {
        SDL_Surface* surface = windowIcon_->GetSDLSurface();
        if (surface)
        {
            SDL_SetWindowIcon(window_, surface);
            SDL_FreeSurface(surface);
        }
    }
}

void Graphics::OnScreenModeChanged()
{
    URHO3D_LOGINFO("Set screen mode: {}x{} pixels at {} Hz at monitor {} [{}]{}{}", GetWidth(), GetHeight(),
        GetRefreshRate(), GetMonitor(), ToString(GetWindowSettings().mode_), GetResizable() ? " [Resizable]" : "",
        GetMultiSample() > 1 ? Format(" [{}x MSAA]", GetMultiSample()) : "");

    using namespace ScreenMode;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_WIDTH] = GetWidth();
    eventData[P_HEIGHT] = GetHeight();
    eventData[P_FULLSCREEN] = GetFullscreen();
    eventData[P_BORDERLESS] = GetBorderless();
    eventData[P_RESIZABLE] = GetResizable();
    eventData[P_MONITOR] = GetMonitor();
    eventData[P_REFRESHRATE] = GetRefreshRate();
    SendEvent(E_SCREENMODE, eventData);
}

void Graphics::SetMaxBones(unsigned numBones)
{
    maxBonesHWSkinned = numBones;
}

void Graphics::Clear(ClearTargetFlags flags, const Color& color, float depth, unsigned stencil)
{
    URHO3D_ASSERT(renderDevice_);

    RenderContext* renderContext = renderDevice_->GetRenderContext();
    if (flags.Test(CLEAR_COLOR))
        renderContext->ClearRenderTarget(0, color);
    if (flags.Test(CLEAR_DEPTH) || flags.Test(CLEAR_STENCIL))
        renderContext->ClearDepthStencil(flags, depth, stencil);
}

void Graphics::Restore()
{
    if (renderDevice_)
    {
        if (!renderDevice_->Restore())
            Close();
    }
}

void Graphics::ResetRenderTargets()
{
    URHO3D_ASSERT(renderDevice_);

    RenderContext* renderContext = renderDevice_->GetRenderContext();
    renderContext->SetSwapChainRenderTargets();
    renderContext->SetFullViewport();
}

bool Graphics::IsInitialized() const
{
    return renderDevice_ != nullptr;
}

TextureFormat Graphics::GetFormat(CompressedFormat format) const
{
    switch (format)
    {
    case CF_RGBA: return TextureFormat::TEX_FORMAT_RGBA8_UNORM;
    case CF_DXT1: return TextureFormat::TEX_FORMAT_BC1_UNORM;
    case CF_DXT3: return TextureFormat::TEX_FORMAT_BC2_UNORM;
    case CF_DXT5: return TextureFormat::TEX_FORMAT_BC3_UNORM;
    default: return TextureFormat::TEX_FORMAT_UNKNOWN;
    }
}

ShaderVariation* Graphics::GetShader(ShaderType type, const ea::string& name, const ea::string& defines) const
{
    return GetShader(type, name.c_str(), defines.c_str());
}

ShaderVariation* Graphics::GetShader(ShaderType type, const char* name, const char* defines) const
{
    // Return cached shader
    if (lastShaderName_ == name && lastShader_)
        return lastShader_->GetVariation(type, defines);

    auto cache = context_->GetSubsystem<ResourceCache>();
    lastShader_ = nullptr;

    // Try to load universal shader
    if (strncmp(universalShaderNamePrefix_.c_str(), name, universalShaderNamePrefix_.size()) == 0)
    {
        const ea::string universalShaderName = Format(universalShaderPath_, name);
        if (cache->Exists(universalShaderName))
        {
            lastShader_ = cache->GetResource<Shader>(universalShaderName);
            lastShaderName_ = name;
        }
    }

    // Try to load native shader
    if (!lastShader_)
    {
        const ea::string fullShaderName = shaderPath_ + name + shaderExtension_;
        // Try to reduce repeated error log prints because of missing shaders
        if (lastShaderName_ != name || cache->Exists(fullShaderName))
        {
            lastShader_ = cache->GetResource<Shader>(fullShaderName);
            lastShaderName_ = name;
        }
    }

    return lastShader_ ? lastShader_->GetVariation(type, defines) : nullptr;
}

IntVector2 Graphics::GetRenderTargetDimensions() const
{
    URHO3D_ASSERT(false);
    return IntVector2::ZERO;
}

void Graphics::OnWindowResized()
{
    if (!renderDevice_ || GetPlatform() == PlatformId::Web)
        return;

    renderDevice_->UpdateSwapChainSize();

    using namespace ScreenMode;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_WIDTH] = GetWidth();
    eventData[P_HEIGHT] = GetWidth();
    eventData[P_FULLSCREEN] = GetFullscreen();
    eventData[P_BORDERLESS] = GetBorderless();
    eventData[P_RESIZABLE] = GetResizable();
    SendEvent(E_SCREENMODE, eventData);
}

void Graphics::OnWindowMoved()
{
    if (!renderDevice_ || !window_ || GetFullscreen())
        return;

    int newX, newY;

    SDL_GetWindowPosition(window_, &newX, &newY);
    if (newX == position_.x_ && newY == position_.y_)
        return;

    position_.x_ = newX;
    position_.y_ = newY;

    URHO3D_LOGTRACEF("Window was moved to %d,%d", position_.x_, position_.y_);

    using namespace WindowPos;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_X] = position_.x_;
    eventData[P_Y] = position_.y_;
    SendEvent(E_WINDOWPOS, eventData);
}

RenderBackend Graphics::GetRenderBackend() const
{
    return renderDevice_ ? renderDevice_->GetBackend() : RenderBackend::OpenGL;
}

unsigned Graphics::GetMaxBones()
{
    /// User-specified number of bones
    if (maxBonesHWSkinned)
        return maxBonesHWSkinned;

    return 128;
}

void RegisterGraphicsLibrary(Context* context)
{
    Animation::RegisterObject(context);
    Material::RegisterObject(context);
    Model::RegisterObject(context);
    Shader::RegisterObject(context);
    Technique::RegisterObject(context);
    Texture2D::RegisterObject(context);
    Texture2DArray::RegisterObject(context);
    Texture3D::RegisterObject(context);
    TextureCube::RegisterObject(context);
    Camera::RegisterObject(context);
    CameraOperator::RegisterObject(context);
    Drawable::RegisterObject(context);
    Light::RegisterObject(context);
    LightBaker::RegisterObject(context);
    LightProbeGroup::RegisterObject(context);
    GlobalIllumination::RegisterObject(context);
    StaticModel::RegisterObject(context);
    StaticModelGroup::RegisterObject(context);
    Skybox::RegisterObject(context);
    AnimatedModel::RegisterObject(context);
    AnimationController::RegisterObject(context);
    BillboardSet::RegisterObject(context);
    ParticleEffect::RegisterObject(context);
    ParticleEmitter::RegisterObject(context);
    RibbonTrail::RegisterObject(context);
    CustomGeometry::RegisterObject(context);
    DecalSet::RegisterObject(context);
    Terrain::RegisterObject(context);
    TerrainPatch::RegisterObject(context);
    DebugRenderer::RegisterObject(context);
    Octree::RegisterObject(context);
    OutlineGroup::RegisterObject(context);
    Zone::RegisterObject(context);
    Geometry::RegisterObject(context);
    Viewport::RegisterObject(context);
    OcclusionBuffer::RegisterObject(context);
    ReflectionProbe::RegisterObject(context);
    ReflectionProbeManager::RegisterObject(context);
}


}
