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

#include "../Core/Profiler.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Animation.h"
#include "../Graphics/AnimationController.h"
#include "../Graphics/Camera.h"
#include "../Graphics/ConstantBuffer.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/CustomGeometry.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/DecalSet.h"
#include "../Graphics/GlobalIllumination.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/GraphicsImpl.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/LightBaker.h"
#include "../Graphics/LightProbeGroup.h"
#include "../Graphics/Material.h"
#include "../Graphics/OcclusionBuffer.h"
#include "../Graphics/Octree.h"
#include "../Graphics/ParticleEffect.h"
#include "../Graphics/ParticleEmitter.h"
#include "../Graphics/RibbonTrail.h"
#include "../Graphics/Shader.h"
#include "../Graphics/ShaderPrecache.h"
#include "../Graphics/Skybox.h"
#include "../Graphics/StaticModelGroup.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Terrain.h"
#include "../Graphics/TerrainPatch.h"
#ifdef _WIN32
#include "../Graphics/Texture2D.h"
#endif
#include "../Graphics/Texture2DArray.h"
#include "../Graphics/Texture3D.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/View.h"
#include "../Graphics/Viewport.h"
#include "../Graphics/Zone.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"

#include <SDL/SDL.h>

#include "../DebugNew.h"

namespace Urho3D
{

GraphicsCaps Graphics::caps;

void Graphics::SetExternalWindow(void* window)
{
    if (!window_)
        externalWindow_ = window;
    else
        URHO3D_LOGERROR("Window already opened, can not set external window");
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

void Graphics::SetOrientations(const ea::string& orientations)
{
    orientations_ = orientations.trimmed();
    SDL_SetHint(SDL_HINT_ORIENTATIONS, orientations_.c_str());
}

bool Graphics::SetScreenMode(int width, int height)
{
    return SetScreenMode(width, height, screenParams_);
}

bool Graphics::SetWindowModes(const WindowModeParams& windowMode, const WindowModeParams& secondaryWindowMode, bool maximize)
{
    primaryWindowMode_ = windowMode;
    secondaryWindowMode_ = secondaryWindowMode;
    return SetScreenMode(primaryWindowMode_.width_, primaryWindowMode_.height_, primaryWindowMode_.screenParams_, maximize);
}

bool Graphics::SetDefaultWindowModes(int width, int height, const ScreenModeParams& params)
{
    // Fill window mode to be applied now
    WindowModeParams primaryWindowMode;
    primaryWindowMode.width_ = width;
    primaryWindowMode.height_ = height;
    primaryWindowMode.screenParams_ = params;

    // Fill window mode to be applied on Graphics::ToggleFullscreen
    WindowModeParams secondaryWindowMode = primaryWindowMode;

    // Pick resolution automatically
    secondaryWindowMode.width_ = 0;
    secondaryWindowMode.height_ = 0;

    if (params.fullscreen_ || params.borderless_)
    {
        secondaryWindowMode.screenParams_.fullscreen_ = false;
        secondaryWindowMode.screenParams_.borderless_ = false;
    }
    else
    {
        secondaryWindowMode.screenParams_.borderless_ = true;
    }

    const bool maximize = (!width || !height) && !params.fullscreen_ && !params.borderless_ && params.resizable_;
    return SetWindowModes(primaryWindowMode, secondaryWindowMode, maximize);
}

bool Graphics::SetMode(int width, int height, bool fullscreen, bool borderless, bool resizable,
    bool highDPI, bool vsync, bool tripleBuffer, int multiSample, int monitor, int refreshRate, bool gpuDebug)
{
    ScreenModeParams params;
    params.fullscreen_ = fullscreen;
    params.borderless_ = borderless;
    params.resizable_ = resizable;
    params.highDPI_ = highDPI;
    params.vsync_ = vsync;
    params.tripleBuffer_ = tripleBuffer;
    params.multiSample_ = multiSample;
    params.monitor_ = monitor;
    params.refreshRate_ = refreshRate;
    params.gpuDebug_ = gpuDebug;

    return SetDefaultWindowModes(width, height, params);
}

bool Graphics::SetMode(int width, int height)
{
    return SetDefaultWindowModes(width, height, screenParams_);
}

bool Graphics::ToggleFullscreen()
{
    ea::swap(primaryWindowMode_, secondaryWindowMode_);
    return SetScreenMode(primaryWindowMode_.width_, primaryWindowMode_.height_, primaryWindowMode_.screenParams_);
}

void Graphics::SetShaderParameter(StringHash param, const Variant& value)
{
    switch (value.GetType())
    {
    case VAR_BOOL:
        SetShaderParameter(param, value.GetBool());
        break;

    case VAR_INT:
        SetShaderParameter(param, value.GetInt());
        break;

    case VAR_FLOAT:
    case VAR_DOUBLE:
        SetShaderParameter(param, value.GetFloat());
        break;

    case VAR_VECTOR2:
        SetShaderParameter(param, value.GetVector2());
        break;

    case VAR_VECTOR3:
        SetShaderParameter(param, value.GetVector3());
        break;

    case VAR_VECTOR4:
        SetShaderParameter(param, value.GetVector4());
        break;

    case VAR_COLOR:
        SetShaderParameter(param, value.GetColor());
        break;

    case VAR_MATRIX3:
        SetShaderParameter(param, value.GetMatrix3());
        break;

    case VAR_MATRIX3X4:
        SetShaderParameter(param, value.GetMatrix3x4());
        break;

    case VAR_MATRIX4:
        SetShaderParameter(param, value.GetMatrix4());
        break;

    case VAR_BUFFER:
        {
            const ea::vector<unsigned char>& buffer = value.GetBuffer();
            if (buffer.size() >= sizeof(float))
                SetShaderParameter(param, reinterpret_cast<const float*>(&buffer[0]), buffer.size() / sizeof(float));
        }
        break;

    default:
        // Unsupported parameter type, do nothing
        break;
    }
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
    return IntVector2(width_, height_);
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

void Graphics::BeginDumpShaders(const ea::string& fileName)
{
    shaderPrecache_ = new ShaderPrecache(context_, fileName);
}

void Graphics::EndDumpShaders()
{
    shaderPrecache_.Reset();
}

void Graphics::PrecacheShaders(Deserializer& source)
{
    URHO3D_PROFILE("PrecacheShaders");

    ShaderPrecache::LoadShaders(this, source);
}

void Graphics::SetGlobalShaderDefines(const ea::string& globalShaderDefines)
{
    globalShaderDefines_ = globalShaderDefines;
    globalShaderDefinesHash_ = globalShaderDefines_;
}

void Graphics::SetShaderCacheDir(const ea::string& path)
{
    ea::string trimmedPath = path.trimmed();
    if (trimmedPath.length())
        shaderCacheDir_ = AddTrailingSlash(trimmedPath);
}

void Graphics::AddGPUObject(GPUObject* object)
{
    MutexLock lock(gpuObjectMutex_);

    gpuObjects_.push_back(object);
}

void Graphics::RemoveGPUObject(GPUObject* object)
{
    MutexLock lock(gpuObjectMutex_);

    gpuObjects_.erase_first(object);
}

void* Graphics::ReserveScratchBuffer(unsigned size)
{
    if (!size)
        return nullptr;

    if (size > maxScratchBufferRequest_)
        maxScratchBufferRequest_ = size;

    // First check for a free buffer that is large enough
    for (auto i = scratchBuffers_.begin(); i != scratchBuffers_.end(); ++i)
    {
        if (!i->reserved_ && i->size_ >= size)
        {
            i->reserved_ = true;
            return i->data_.get();
        }
    }

    // Then check if a free buffer can be resized
    for (auto i = scratchBuffers_.begin(); i != scratchBuffers_.end(); ++i)
    {
        if (!i->reserved_)
        {
            i->data_.reset(new unsigned char[size]);
            i->size_ = size;
            i->reserved_ = true;

            URHO3D_LOGTRACE("Resized scratch buffer to size " + ea::to_string(size));

            return i->data_.get();
        }
    }

    // Finally allocate a new buffer
    ScratchBuffer newBuffer;
    newBuffer.data_.reset(new unsigned char[size]);
    newBuffer.size_ = size;
    newBuffer.reserved_ = true;
    scratchBuffers_.push_back(newBuffer);

    URHO3D_LOGDEBUG("Allocated scratch buffer with size " + ea::to_string(size));

    return newBuffer.data_.get();
}

void Graphics::FreeScratchBuffer(void* buffer)
{
    if (!buffer)
        return;

    for (auto i = scratchBuffers_.begin(); i != scratchBuffers_.end(); ++i)
    {
        if (i->reserved_ && i->data_.get() == buffer)
        {
            i->reserved_ = false;
            return;
        }
    }

    URHO3D_LOGWARNING("Reserved scratch buffer " + ToStringHex((unsigned)(size_t)buffer) + " not found");
}

void Graphics::CleanupScratchBuffers()
{
    for (auto i = scratchBuffers_.begin(); i != scratchBuffers_.end(); ++i)
    {
        if (!i->reserved_ && i->size_ > maxScratchBufferRequest_ * 2 && i->size_ >= 1024 * 1024)
        {
            i->data_.reset(maxScratchBufferRequest_ > 0 ? (new unsigned char[maxScratchBufferRequest_]) : nullptr);
            i->size_ = maxScratchBufferRequest_;

            URHO3D_LOGTRACE("Resized scratch buffer to size " + ea::to_string(maxScratchBufferRequest_));
        }
    }

    maxScratchBufferRequest_ = 0;
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

void Graphics::AdjustScreenMode(int& newWidth, int& newHeight, ScreenModeParams& params, bool& maximize) const
{
    // High DPI is supported only for OpenGL backend
#ifndef URHO3D_OPENGL
    params.highDPI_ = false;
#endif

#if defined(IOS) || defined(TVOS)
    // iOS and tvOS app always take the fullscreen (and with status bar hidden)
    params.fullscreen_ = true;
#endif

    // Make sure monitor index is not bigger than the currently detected monitors
    const int numMonitors = SDL_GetNumVideoDisplays();
    if (params.monitor_ >= numMonitors || params.monitor_ < 0)
        params.monitor_ = 0; // this monitor is not present, use first monitor

    // Fullscreen or Borderless can not be resizable and cannot be maximized
    if (params.fullscreen_ || params.borderless_)
    {
        params.resizable_ = false;
        maximize = false;
    }

    // Borderless cannot be fullscreen, they are mutually exclusive
    if (params.borderless_)
        params.fullscreen_ = false;

    // On iOS window needs to be resizable to handle orientation changes properly
#ifdef IOS
    if (!externalWindow_)
        params.resizable_ = true;
#endif

    // Ensure that multisample factor is in valid range
    params.multiSample_ = NextPowerOfTwo(Clamp(params.multiSample_, 1, 16));

    // If zero dimensions in windowed mode, set windowed mode to maximize and set a predefined default restored window size.
    // If zero in fullscreen, use desktop mode
    if (!newWidth || !newHeight)
    {
        if (params.fullscreen_ || params.borderless_)
        {
            SDL_DisplayMode mode;
            SDL_GetDesktopDisplayMode(params.monitor_, &mode);
            newWidth = mode.w;
            newHeight = mode.h;
        }
        else
        {
            newWidth = 1024;
            newHeight = 768;
        }
    }

    // Check fullscreen mode validity (desktop only). Use a closest match if not found
#ifdef DESKTOP_GRAPHICS
    if (params.fullscreen_)
    {
        const ea::vector<IntVector3> resolutions = GetResolutions(params.monitor_);
        if (!resolutions.empty())
        {
            const unsigned bestResolution = FindBestResolutionIndex(params.monitor_,
                newWidth, newHeight, params.refreshRate_);
            newWidth = resolutions[bestResolution].x_;
            newHeight = resolutions[bestResolution].y_;
            params.refreshRate_ = resolutions[bestResolution].z_;
        }
    }
    else
    {
        // If windowed, use the same refresh rate as desktop
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(params.monitor_, &mode);
        params.refreshRate_ = mode.refresh_rate;
    }
#endif
}

void Graphics::OnScreenModeChanged()
{
#ifdef URHO3D_LOGGING
    ea::string msg;
    msg.append_sprintf("Set screen mode %dx%d rate %d Hz %s monitor %d", width_, height_, screenParams_.refreshRate_,
        (screenParams_.fullscreen_ ? "fullscreen" : "windowed"), screenParams_.monitor_);
    if (screenParams_.borderless_)
        msg.append(" borderless");
    if (screenParams_.resizable_)
        msg.append(" resizable");
    if (screenParams_.highDPI_)
        msg.append(" highDPI");
    if (screenParams_.multiSample_ > 1)
        msg.append_sprintf(" multisample %d", screenParams_.multiSample_);
    URHO3D_LOGINFO(msg);
#endif

    using namespace ScreenMode;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_WIDTH] = width_;
    eventData[P_HEIGHT] = height_;
    eventData[P_FULLSCREEN] = screenParams_.fullscreen_;
    eventData[P_BORDERLESS] = screenParams_.borderless_;
    eventData[P_RESIZABLE] = screenParams_.resizable_;
    eventData[P_HIGHDPI] = screenParams_.highDPI_;
    eventData[P_MONITOR] = screenParams_.monitor_;
    eventData[P_REFRESHRATE] = screenParams_.refreshRate_;
    SendEvent(E_SCREENMODE, eventData);
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
    Zone::RegisterObject(context);
    VertexBuffer::RegisterObject(context);
    IndexBuffer::RegisterObject(context);
    Geometry::RegisterObject(context);
    ConstantBuffer::RegisterObject(context);
    View::RegisterObject(context);
    Viewport::RegisterObject(context);
    OcclusionBuffer::RegisterObject(context);
}


}
