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

// TODO(diligent): This is Web-only, revisit
// @{
#include "../../Input/Input.h"
#include "../../UI/Cursor.h"
#include "../../UI/UI.h"
#ifdef URHO3D_RMLUI
    #include "../../RmlUI/RmlUI.h"
#endif
// @}

#include <Diligent/Platforms/interface/PlatformDefinitions.h>
#include <Diligent/Primitives/interface/CommonDefinitions.h>
#include <Diligent/Primitives/interface/DebugOutput.h>

#include <EASTL/utility.h>

#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <Diligent/Graphics/GraphicsTools/interface/MapHelper.hpp>

#include <SDL.h>

#if URHO3D_PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#endif

#include "../../DebugNew.h"

#ifdef _MSC_VER
    #pragma warning(disable : 4355)
#endif

#ifdef WIN32
// Prefer the high-performance GPU on switchable GPU systems
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#if URHO3D_PLATFORM_WEB
static void JSCanvasSize(int width, int height, bool fullscreen, float scale)
{
    URHO3D_LOGINFOF("JSCanvasSize: width=%d height=%d fullscreen=%d ui scale=%f", width, height, fullscreen, scale);

    using namespace Urho3D;

    auto context = Context::GetInstance();
    if (context)
    {
        bool uiCursorVisible = false;
        bool systemCursorVisible = false;
        MouseMode mouseMode{};

        // Detect current system pointer state
        Input* input = context->GetSubsystem<Input>();
        if (input)
        {
            systemCursorVisible = input->IsMouseVisible();
            mouseMode = input->GetMouseMode();
        }

        UI* ui = context->GetSubsystem<UI>();
        if (ui)
        {
            ui->SetScale(scale);

            // Detect current UI pointer state
            Cursor* cursor = ui->GetCursor();
            if (cursor)
                uiCursorVisible = cursor->IsVisible();
        }

#ifdef URHO3D_RMLUI
        if (RmlUI* ui = context->GetSubsystem<RmlUI>())
            ui->SetScale(scale);
#endif

        // Apply new resolution
        context->GetSubsystem<Graphics>()->SetMode(width, height);

        // Reset the pointer state as it was before resolution change
        if (input)
        {
            if (uiCursorVisible)
                input->SetMouseVisible(false);
            else
                input->SetMouseVisible(systemCursorVisible);

            input->SetMouseMode(mouseMode);
        }

        if (ui)
        {
            Cursor* cursor = ui->GetCursor();
            if (cursor)
            {
                cursor->SetVisible(uiCursorVisible);

                IntVector2 pos = input->GetMousePosition();
                pos = ui->ConvertSystemToUI(pos);

                cursor->SetPosition(pos);
            }
        }
    }
}

using namespace emscripten;
EMSCRIPTEN_BINDINGS(Module)
{
    function("JSCanvasSize", &JSCanvasSize);
}
#endif

namespace Urho3D
{
using namespace Diligent;

static const Diligent::VALUE_TYPE DiligentIndexBufferType[] = {
    Diligent::VT_UNDEFINED, Diligent::VT_UINT16, Diligent::VT_UINT32};

static void GetPrimitiveType(unsigned elementCount, PrimitiveType type, unsigned& primitiveCount,
    Diligent::PRIMITIVE_TOPOLOGY& primitiveTopology)
{
    switch (type)
    {
    case TRIANGLE_LIST:
        primitiveCount = elementCount / 3;
        primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;

    case LINE_LIST:
        primitiveCount = elementCount / 2;
        primitiveTopology = PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;

    case POINT_LIST:
        primitiveCount = elementCount;
        primitiveTopology = PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;

    case TRIANGLE_STRIP:
        primitiveCount = elementCount - 2;
        primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        break;

    case LINE_STRIP:
        primitiveCount = elementCount - 1;
        primitiveTopology = PRIMITIVE_TOPOLOGY_LINE_STRIP;
        break;

    case TRIANGLE_FAN:
        // Triangle fan is not supported on D3D11
        primitiveCount = 0;
        primitiveTopology = PRIMITIVE_TOPOLOGY_UNDEFINED;
        break;
    }
}

static void HandleDbgMessageCallbacks(
    Diligent::DEBUG_MESSAGE_SEVERITY severity, const char* msg, const char* func, const char* file, int line)
{
    ea::string logMsg = Format("(diligent) {}", ea::string(msg == nullptr ? "" : msg));
    ea::vector<ea::pair<ea::string, ea::string>> additionalInfo;

    ea::string lineStr = Format("{}", line);
    if (func)
        additionalInfo.push_back(ea::make_pair<ea::string, ea::string>(ea::string("function"), ea::string(func)));
    if (file)
        additionalInfo.push_back(ea::make_pair<ea::string, ea::string>(ea::string("file"), ea::string(file)));
    if (line)
        additionalInfo.push_back(ea::make_pair<ea::string, ea::string>(ea::string("line"), ea::string(lineStr)));

    if (additionalInfo.size() > 0)
    {
        logMsg.append("\n");
        for (uint8_t i = 0; i < additionalInfo.size(); ++i)
        {
            logMsg.append(additionalInfo[i].first);
            logMsg.append(": ");
            logMsg.append(additionalInfo[i].second);
            if (i < additionalInfo.size() - 1)
                logMsg.append(" | ");
        }
    }

    switch (severity)
    {
    case Diligent::DEBUG_MESSAGE_SEVERITY_INFO: URHO3D_LOGINFO(logMsg); break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_WARNING: URHO3D_LOGWARNING(logMsg); break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_ERROR: URHO3D_LOGERROR(logMsg); break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR: URHO3D_LOGERROR("[fatal]{}", logMsg.c_str()); break;
    }
}

Graphics::Graphics(Context* context)
    : Object(context)
    , position_(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED)
    , shaderPath_("Shaders/HLSL/")
    , shaderExtension_(".hlsl")
    , apiName_("Diligent")
{
    // TODO(diligent): Revisit this
    //SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
    context_->RequireSDL(SDL_INIT_VIDEO);

    Diligent::SetDebugMessageCallback(&HandleDbgMessageCallbacks);
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
    // TODO(diligent): Implement
    URHO3D_PROFILE("TakeScreenShot");
    if (!IsInitialized())
        return false;

    assert(0);
    return false;
    // if (!impl_->device_)
    //     return false;

    // D3D11_TEXTURE2D_DESC textureDesc;
    // memset(&textureDesc, 0, sizeof textureDesc);
    // textureDesc.Width = (UINT)width_;
    // textureDesc.Height = (UINT)height_;
    // textureDesc.MipLevels = 1;
    // textureDesc.ArraySize = 1;
    // textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // textureDesc.SampleDesc.Count = 1;
    // textureDesc.SampleDesc.Quality = 0;
    // textureDesc.Usage = D3D11_USAGE_STAGING;
    // textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    // ID3D11Texture2D* stagingTexture = nullptr;
    // HRESULT hr = impl_->device_->CreateTexture2D(&textureDesc, nullptr, &stagingTexture);
    // if (FAILED(hr))
    //{
    //     URHO3D_SAFE_RELEASE(stagingTexture);
    //     URHO3D_LOGD3DERROR("Could not create staging texture for screenshot", hr);
    //     return false;
    // }

    // ID3D11Resource* source = nullptr;
    // impl_->defaultRenderTargetView_->GetResource(&source);

    // if (screenParams_.multiSample_ > 1)
    //{
    //     // If backbuffer is multisampled, need another DEFAULT usage texture to resolve the data to first
    //     CreateResolveTexture();

    //    if (!impl_->resolveTexture_)
    //    {
    //        stagingTexture->Release();
    //        source->Release();
    //        return false;
    //    }

    //    impl_->deviceContext_->ResolveSubresource(impl_->resolveTexture_, 0, source, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    //    impl_->deviceContext_->CopyResource(stagingTexture, impl_->resolveTexture_);
    //}
    // else
    //    impl_->deviceContext_->CopyResource(stagingTexture, source);

    // source->Release();

    // D3D11_MAPPED_SUBRESOURCE mappedData;
    // mappedData.pData = nullptr;
    // hr = impl_->deviceContext_->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedData);
    // if (FAILED(hr) || !mappedData.pData)
    //{
    //     URHO3D_LOGD3DERROR("Could not map staging texture for screenshot", hr);
    //     stagingTexture->Release();
    //     return false;
    // }

    // destImage.SetSize(width_, height_, 3);
    // unsigned char* destData = destImage.GetData();
    // for (int y = 0; y < height_; ++y)
    //{
    //     unsigned char* src = (unsigned char*)mappedData.pData + y * mappedData.RowPitch;
    //     for (int x = 0; x < width_; ++x)
    //     {
    //         *destData++ = *src++;
    //         *destData++ = *src++;
    //         *destData++ = *src++;
    //         ++src;
    //     }
    // }

    // impl_->deviceContext_->Unmap(stagingTexture, 0);
    // stagingTexture->Release();
    // return true;
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

    BeginDebug("Clear");
    RenderContext* renderContext = renderDevice_->GetRenderContext();
    if (flags.Test(CLEAR_COLOR))
        renderContext->ClearRenderTarget(0, color);
    if (flags.Test(CLEAR_DEPTH) || flags.Test(CLEAR_STENCIL))
        renderContext->ClearDepthStencil(flags, depth, stencil);
    EndDebug();
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
    case CF_RGBA: return TEX_FORMAT_RGBA8_UNORM;

    case CF_DXT1: return TEX_FORMAT_BC1_UNORM;

    case CF_DXT3: return TEX_FORMAT_BC2_UNORM;

    case CF_DXT5: return TEX_FORMAT_BC3_UNORM;

    default: return TEX_FORMAT_UNKNOWN;
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

bool Graphics::GetDither() const
{
    return false;
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

void Graphics::BeginDebug(const ea::string_view& debugName)
{
    Diligent::IDeviceContext* deviceContext = renderDevice_->GetImmediateContext();
    deviceContext->BeginDebugGroup(debugName.data());
}

void Graphics::BeginDebug(const ea::string& debugName)
{
    Diligent::IDeviceContext* deviceContext = renderDevice_->GetImmediateContext();
    deviceContext->BeginDebugGroup(debugName.data());
}

void Graphics::BeginDebug(const char* debugName)
{
    Diligent::IDeviceContext* deviceContext = renderDevice_->GetImmediateContext();
    deviceContext->BeginDebugGroup(debugName);
}

void Graphics::EndDebug()
{
    Diligent::IDeviceContext* deviceContext = renderDevice_->GetImmediateContext();
    deviceContext->EndDebugGroup();
}

} // namespace Urho3D
