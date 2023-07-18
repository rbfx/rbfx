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

#include "../../Precompiled.h"

#include "../../Core/Context.h"
#include "../../Core/ProcessUtils.h"
#include "../../Core/Profiler.h"
#include "../../Graphics/Geometry.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/IndexBuffer.h"
#include "../../RenderAPI/PipelineState.h"
#include "../../Graphics/Renderer.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/Texture2D.h"
#include "../../Graphics/TextureCube.h"
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/File.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"
#include "../../RenderAPI/RenderAPIUtils.h"
#include "../../RenderAPI/RenderContext.h"
#include "../../RenderAPI/RenderDevice.h"

#include <Diligent/Platforms/interface/PlatformDefinitions.h>
#include <Diligent/Primitives/interface/CommonDefinitions.h>
#include <Diligent/Primitives/interface/DebugOutput.h>

#include <EASTL/utility.h>

#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <Diligent/Graphics/GraphicsTools/interface/MapHelper.hpp>

#include <SDL.h>

#include "../../DebugNew.h"

#ifdef _MSC_VER
    #pragma warning(disable : 4355)
#endif

namespace Urho3D
{

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

    numPrimitives_ = 0;
    numBatches_ = 0;

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

    // Clean up too large scratch buffers
    CleanupScratchBuffers();
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

void Graphics::SetDefaultTextureFilterMode(TextureFilterMode mode)
{
    if (mode != defaultTextureFilterMode_)
    {
        defaultTextureFilterMode_ = mode;
    }
}

void Graphics::SetDefaultTextureAnisotropy(unsigned level)
{
    level = Max(level, 1U);

    if (level != defaultTextureAnisotropy_)
    {
        defaultTextureAnisotropy_ = level;
    }
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

bool Graphics::IsDeviceLost() const
{
    // Direct3D11 graphics context is never considered lost
    /// \todo The device could be lost in case of graphics adapters getting disabled during runtime. This is not
    /// currently handled
    return false;
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

} // namespace Urho3D
