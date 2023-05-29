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
#include "../../Graphics/ComputeDevice.h"
#include "../../Graphics/ConstantBuffer.h"
#include "../../Graphics/Geometry.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/IndexBuffer.h"
#include "../../Graphics/PipelineState.h"
#include "../../Graphics/Renderer.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/ShaderPrecache.h"
#include "../../Graphics/ShaderProgram.h"
#include "../../Graphics/Texture2D.h"
#include "../../Graphics/TextureCube.h"
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/File.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"
#include "../../RenderAPI/OpenGLIncludes.h"
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

#include "DiligentLookupSettings.h"

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

struct ClearFramebufferConstantBuffer
{
    Matrix3x4 matrix_;
    Vector4 color_;
};

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

bool Graphics::gl3Support = false;

Graphics::Graphics(Context* context)
    : Object(context)
    , impl_(new GraphicsImpl())
    , position_(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED)
    , shaderPath_("Shaders/HLSL/")
    , shaderExtension_(".hlsl")
    , orientations_("LandscapeLeft LandscapeRight")
    , apiName_("Diligent")
{
    SetTextureUnitMappings();
    ResetCachedState();

    // TODO(diligent): Revisit this
    //SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
    context_->RequireSDL(SDL_INIT_VIDEO);

    Diligent::SetDebugMessageCallback(&HandleDbgMessageCallbacks);
}

Graphics::~Graphics()
{
    // Reset State
    impl_->deviceContext_->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    // impl_->deviceContext_->SetPipelineState(nullptr);
    impl_->deviceContext_->SetIndexBuffer(nullptr, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    impl_->deviceContext_->SetVertexBuffers(0, 0, nullptr, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    impl_->deviceContext_->Flush();

//    if (impl_->device_)
//    {
//        impl_->device_->IdleGPU();
//
//        if (impl_->swapChain_)
//            impl_->swapChain_->Release();
//
//        impl_->deviceContext_->Release();
//        impl_->device_->Release();
//    }
    //{
    //    MutexLock lock(gpuObjectMutex_);

    //    // Release all GPU objects that still exist
    //    for (auto i = gpuObjects_.begin(); i != gpuObjects_.end(); ++i)
    //        (*i)->Release();
    //    gpuObjects_.clear();
    //}

    // impl_->vertexDeclarations_.clear();
    // impl_->allConstantBuffers_.clear();

    // for (auto i = impl_->blendStates_.begin(); i != impl_->blendStates_.end(); ++i)
    //{
    //     URHO3D_SAFE_RELEASE(i->second);
    // }
    // impl_->blendStates_.clear();

    // for (auto i = impl_->depthStates_.begin(); i != impl_->depthStates_.end(); ++i)
    //{
    //     URHO3D_SAFE_RELEASE(i->second);
    // }
    // impl_->depthStates_.clear();

    // for (auto i = impl_->rasterizerStates_.begin();
    //      i != impl_->rasterizerStates_.end(); ++i)
    //{
    //     URHO3D_SAFE_RELEASE(i->second);
    // }
    // impl_->rasterizerStates_.clear();

    // URHO3D_SAFE_RELEASE(impl_->defaultRenderTargetView_);
    // URHO3D_SAFE_RELEASE(impl_->defaultDepthStencilView_);
    // URHO3D_SAFE_RELEASE(impl_->defaultDepthTexture_);
    // URHO3D_SAFE_RELEASE(impl_->resolveTexture_);
    // URHO3D_SAFE_RELEASE(impl_->swapChain_);
    // URHO3D_SAFE_RELEASE(impl_->deviceContext_);
    // URHO3D_SAFE_RELEASE(impl_->device_);

//#ifdef PLATFORM_MACOS
//    if (impl_->metalView_)
//    {
//        SDL_Metal_DestroyView(impl_->metalView_);
//        impl_->metalView_ = nullptr;
//    }
//#endif
//
//    if (window_)
//    {
//        SDL_ShowCursor(SDL_TRUE);
//        SDL_DestroyWindow(window_);
//        window_ = nullptr;
//    }

    delete impl_;
    impl_ = nullptr;

    context_->ReleaseSDL();
}

bool Graphics::SetScreenMode(const WindowSettings& windowSettings)
{
    URHO3D_PROFILE("SetScreenMode");

    if (!renderDevice_)
    {
        RenderDeviceSettings settings;

        settings.backend_ = GetRenderBackend();
        settings.window_ = windowSettings;
        settings.externalWindowHandle_ = externalWindow_;
        settings.gpuDebug_ = gpuDebug_;
        if (impl_->adapterId_ != M_MAX_UNSIGNED)
            settings.adapterId_ = impl_->adapterId_;

        try
        {
            renderDevice_ = MakeShared<RenderDevice>(context_, settings);
        }
        catch (const RuntimeException& ex)
        {
            URHO3D_LOGERROR("Failed to create render device: {}", ex.what());
            return false;
        }

        renderDevice_->OnDeviceLost.Subscribe(this, [this]()
        {
            impl_->shaderPrograms_.clear();
            for (GPUObject* gpuObject : gpuObjects_)
                gpuObject->OnDeviceLost();
            SendEvent(E_DEVICELOST);
        });
        renderDevice_->OnDeviceRestored.Subscribe(this, [this]()
        {
            for (GPUObject* gpuObject : gpuObjects_)
                gpuObject->OnDeviceReset();
            SendEvent(E_DEVICERESET);
        });
    }
    else
    {
        renderDevice_->UpdateWindowSettings(windowSettings);
    }

    window_ = renderDevice_->GetSDLWindow();
    impl_->device_ = renderDevice_->GetRenderDevice();
    impl_->deviceContext_ = renderDevice_->GetDeviceContext();
    impl_->swapChain_ = renderDevice_->GetSwapChain();

    CheckFeatureSupport();
    ResetRenderTargets();

    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        impl_->renderTargetViews_[i] = nullptr;
    impl_->renderTargetsDirty_ = true;

    //// Clear the initial window contents to black
    Clear(CLEAR_COLOR);
    renderDevice_->Present();

    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        impl_->renderTargetViews_[i] = nullptr;
    impl_->renderTargetsDirty_ = true;

    OnScreenModeChanged();
    return true;
}

void Graphics::Close()
{
    if (impl_->deviceContext_)
        impl_->deviceContext_->Flush();

    //if (window_)
    //{
    //    SDL_ShowCursor(SDL_TRUE);
    //    SDL_DestroyWindow(window_);
    //    window_ = nullptr;
    //}
}

bool Graphics::TakeScreenShot(Image& destImage)
{
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

    if (!externalWindow_)
    {
        // To prevent a loop of endless device loss and flicker, do not attempt to render when in fullscreen
        // and the window is minimized
        if (GetFullscreen() && (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED))
            return false;
    }

    // Set default rendertarget and depth buffer
    ResetRenderTargets();

    // Cleanup textures from previous frame
    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        SetTexture(i, nullptr);

    numPrimitives_ = 0;
    numBatches_ = 0;

    ea::string output = Format("Begin Frame {}\n", impl_->GetDeviceContext()->GetFrameNumber());
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

        for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
            impl_->renderTargetViews_[i] = nullptr;
        impl_->renderTargetsDirty_ = true;
    }

    // Clean up too large scratch buffers
    CleanupScratchBuffers();
}

void Graphics::Clear(ClearTargetFlags flags, const Color& color, float depth, unsigned stencil)
{
    IntVector2 rtSize = GetRenderTargetDimensions();
    // Clear always clears the whole target regardless of viewport or scissor test settings
    // Emulate partial clear by rendering a quad
    if (!viewport_.left_ && !viewport_.top_ && viewport_.right_ == rtSize.x_ && viewport_.bottom_ == rtSize.y_)
    {
        BeginDebug("Clear");
        SetDepthWrite(true);
        PrepareDraw();

        if (flags & CLEAR_COLOR && impl_->renderTargetViews_[0])
            impl_->deviceContext_->ClearRenderTarget(
                impl_->renderTargetViews_[0], color.Data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        if (flags & (CLEAR_DEPTH | CLEAR_STENCIL) && impl_->depthStencilView_)
        {
            CLEAR_DEPTH_STENCIL_FLAGS clearFlags = CLEAR_DEPTH_FLAG_NONE;
            if (flags & CLEAR_DEPTH)
                clearFlags |= CLEAR_DEPTH_FLAG;
            if (flags & CLEAR_STENCIL)
                clearFlags |= CLEAR_STENCIL_FLAG;
            impl_->deviceContext_->ClearDepthStencil(
                impl_->depthStencilView_, clearFlags, depth, (Uint8)stencil, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }
        EndDebug();
    }
    // note: idk if commented case below is really necessary
    // rbfx only reaches this case at startup and never is reached
    // again. all clear operation using blit is handled by RenderBufferManager::DrawQuad()
    //    else {
    //        ClearPipelineDesc clearDesc;
    //        clearDesc.rtTexture_ = impl_->swapChain_->GetDesc().ColorBufferFormat;
    //        clearDesc.colorWrite_ = flags & CLEAR_COLOR;
    //        clearDesc.depthWrite_ = flags & CLEAR_DEPTH;
    //        clearDesc.clearStencil_ = flags & CLEAR_STENCIL;
    //        //SetStencilTest(flags & CLEAR_STENCIL, CMP_ALWAYS, OP_REF, OP_KEEP, OP_KEEP, stencil);
    //        IBuffer* frameCB = impl_->constantBufferManager_->GetOrCreateBuffer(ShaderParameterGroup::SP_FRAME);
    //        auto* pipelineHolder = impl_->commonPipelines_->GetOrCreateClearPipeline(clearDesc);
    //
    //        {
    //            MapHelper<Color> clearColor(impl_->deviceContext_, frameCB, MAP_WRITE, MAP_FLAG_DISCARD);
    //            *clearColor = color;
    //        }
    //        ITextureView* rts[] = { impl_->swapChain_->GetCurrentBackBufferRTV() };
    //        // Vulkan needs to SetRenderTargets because they create a RenderPass.
    //        impl_->deviceContext_->SetRenderTargets(1,
    //            rts,
    //            impl_->swapChain_->GetDepthBufferDSV(),
    //            RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    //        );
    //        impl_->deviceContext_->SetPipelineState(pipelineHolder->GetPipeline());
    //        impl_->deviceContext_->SetStencilRef(stencil);
    //
    //        impl_->deviceContext_->CommitShaderResources(pipelineHolder->GetShaderResourceBinding(),
    //        RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    //
    //        DrawAttribs drawAttribs;
    //        drawAttribs.NumVertices = 4;
    // #ifdef URHO3D_DEBUG
    //        impl_->deviceContext_->BeginDebugGroup("Clear w/ Blit");
    //        impl_->deviceContext_->Draw(drawAttribs);
    //        impl_->deviceContext_->EndDebugGroup();
    // #else
    //        impl_->deviceContext->Draw(drawAttribs);
    // #endif
    //
    //    }
}

bool Graphics::ResolveToTexture(Texture2D* destination, const IntRect& viewport)
{
    assert(0);
    return false;
    /*if (!destination || !destination->GetRenderSurface())
        return false;

    URHO3D_PROFILE("ResolveToTexture");

    IntRect vpCopy = viewport;
    if (vpCopy.right_ <= vpCopy.left_)
        vpCopy.right_ = vpCopy.left_ + 1;
    if (vpCopy.bottom_ <= vpCopy.top_)
        vpCopy.bottom_ = vpCopy.top_ + 1;

    D3D11_BOX srcBox;
    srcBox.left = Clamp(vpCopy.left_, 0, width_);
    srcBox.top = Clamp(vpCopy.top_, 0, height_);
    srcBox.right = Clamp(vpCopy.right_, 0, width_);
    srcBox.bottom = Clamp(vpCopy.bottom_, 0, height_);
    srcBox.front = 0;
    srcBox.back = 1;

    ID3D11Resource* source = nullptr;
    const bool resolve = screenParams_.multiSample_ > 1;
    impl_->defaultRenderTargetView_->GetResource(&source);

    if (!resolve)
    {
        if (!srcBox.left && !srcBox.top && srcBox.right == width_ && srcBox.bottom == height_)
            impl_->deviceContext_->CopyResource((ID3D11Resource*)destination->GetGPUObject(), source);
        else
            impl_->deviceContext_->CopySubresourceRegion((ID3D11Resource*)destination->GetGPUObject(), 0, 0, 0, 0,
    source, 0, &srcBox);
    }
    else
    {
        if (!srcBox.left && !srcBox.top && srcBox.right == width_ && srcBox.bottom == height_)
        {
            impl_->deviceContext_->ResolveSubresource((ID3D11Resource*)destination->GetGPUObject(), 0, source, 0,
    (DXGI_FORMAT) destination->GetFormat());
        }
        else
        {
            CreateResolveTexture();

            if (impl_->resolveTexture_)
            {
                impl_->deviceContext_->ResolveSubresource(impl_->resolveTexture_, 0, source, 0,
    DXGI_FORMAT_R8G8B8A8_UNORM);
                impl_->deviceContext_->CopySubresourceRegion((ID3D11Resource*)destination->GetGPUObject(), 0, 0, 0, 0,
    impl_->resolveTexture_, 0, &srcBox);
            }
        }
    }

    source->Release();

    return true;*/
}

bool Graphics::ResolveToTexture(Texture2D* texture)
{
    assert(0);
    return false;
    /*if (!texture)
        return false;
    RenderSurface* surface = texture->GetRenderSurface();
    if (!surface)
        return false;

    texture->SetResolveDirty(false);
    surface->SetResolveDirty(false);
    ID3D11Resource* source = (ID3D11Resource*)texture->GetGPUObject();
    ID3D11Resource* dest = (ID3D11Resource*)texture->GetResolveTexture();
    if (!source || !dest)
        return false;

    impl_->deviceContext_->ResolveSubresource(dest, 0, source, 0, (DXGI_FORMAT)texture->GetFormat());
    return true;*/
}

bool Graphics::ResolveToTexture(TextureCube* texture)
{
    assert(0);
    return 0;
    // if (!texture)
    //     return false;

    // texture->SetResolveDirty(false);
    // ID3D11Resource* source = (ID3D11Resource*)texture->GetGPUObject();
    // ID3D11Resource* dest = (ID3D11Resource*)texture->GetResolveTexture();
    // if (!source || !dest)
    //     return false;

    // for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
    //{
    //     // Resolve only the surface(s) that were actually rendered to
    //     RenderSurface* surface = texture->GetRenderSurface((CubeMapFace)i);
    //     if (!surface->IsResolveDirty())
    //         continue;

    //    surface->SetResolveDirty(false);
    //    unsigned subResource = D3D11CalcSubresource(0, i, texture->GetLevels());
    //    impl_->deviceContext_->ResolveSubresource(dest, subResource, source, subResource,
    //    (DXGI_FORMAT)texture->GetFormat());
    //}

    // return true;
}

void Graphics::Draw(PrimitiveType type, unsigned vertexStart, unsigned vertexCount)
{
    assert(pipelineState_);
    if (!vertexCount)
        return;
    PrepareDraw();
    unsigned primitiveCount;

    PRIMITIVE_TOPOLOGY primitiveTopology;
    GetPrimitiveType(vertexCount, pipelineState_->GetDesc().primitiveType_, primitiveCount, primitiveTopology);

    DrawAttribs drawAttrs;
    drawAttrs.NumVertices = vertexCount;
    drawAttrs.StartVertexLocation = vertexStart;
    drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    impl_->deviceContext_->Draw(drawAttrs);

    numPrimitives_ += primitiveCount;
    ++numBatches_;
    /*


    unsigned primitiveCount;
    D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveType;

    if (fillMode_ == FILL_POINT)
        type = POINT_LIST;

    GetPrimitiveType(vertexCount, type, primitiveCount, d3dPrimitiveType);
    if (d3dPrimitiveType != primitiveType_)
    {
        impl_->deviceContext_->IASetPrimitiveTopology(d3dPrimitiveType);
        primitiveType_ = d3dPrimitiveType;
    }
    impl_->deviceContext_->Draw(vertexCount, vertexStart);

    */
}

void Graphics::Draw(
    PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount)
{
    if (!indexCount || !pipelineState_)
        return;
    PrepareDraw();

    unsigned primitiveCount;
    PRIMITIVE_TOPOLOGY primitiveTopology;

    GetPrimitiveType(indexCount, type, primitiveCount, primitiveTopology);

    DrawIndexedAttribs drawAttrs;
    drawAttrs.BaseVertex = 0;
    drawAttrs.FirstIndexLocation = indexStart;
    drawAttrs.NumIndices = indexCount;
    drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    drawAttrs.IndexType = DiligentIndexBufferType[IndexBuffer::GetIndexBufferType(indexBuffer_)];

    impl_->deviceContext_->DrawIndexed(drawAttrs);

    numPrimitives_ += primitiveCount;
    ++numBatches_;
    /*


    unsigned primitiveCount;
    D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveType;

    if (fillMode_ == FILL_POINT)
        type = POINT_LIST;

    GetPrimitiveType(indexCount, type, primitiveCount, d3dPrimitiveType);
    if (d3dPrimitiveType != primitiveType_)
    {
        impl_->deviceContext_->IASetPrimitiveTopology(d3dPrimitiveType);
        primitiveType_ = d3dPrimitiveType;
    }
    impl_->deviceContext_->DrawIndexed(indexCount, indexStart, 0);

    numPrimitives_ += primitiveCount;
    ++numBatches_;*/
}

void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex,
    unsigned minVertex, unsigned vertexCount)
{
    if (!impl_->shaderProgram_)
        return;

    PrepareDraw();
    assert(0);
    /*

    unsigned primitiveCount;
    D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveType;

    if (fillMode_ == FILL_POINT)
        type = POINT_LIST;

    GetPrimitiveType(indexCount, type, primitiveCount, d3dPrimitiveType);
    if (d3dPrimitiveType != primitiveType_)
    {
        impl_->deviceContext_->IASetPrimitiveTopology(d3dPrimitiveType);
        primitiveType_ = d3dPrimitiveType;
    }
    impl_->deviceContext_->DrawIndexed(indexCount, indexStart, baseVertexIndex);

    numPrimitives_ += primitiveCount;
    ++numBatches_;*/
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex,
    unsigned vertexCount, unsigned instanceCount)
{
    if (!indexCount || !instanceCount || !pipelineState_)
        return;

    PrepareDraw();

    assert(impl_->vertexBuffers_[0]);
    unsigned primitiveCount;

    PRIMITIVE_TOPOLOGY primitiveTopology;
    GetPrimitiveType(vertexCount, pipelineState_->GetDesc().primitiveType_, primitiveCount, primitiveTopology);

    DrawIndexedAttribs drawAttrs;
    drawAttrs.NumIndices = indexCount;
    drawAttrs.NumInstances = instanceCount;
    drawAttrs.FirstIndexLocation = indexStart;
    drawAttrs.BaseVertex = 0;
    drawAttrs.FirstInstanceLocation = 0;
    drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    drawAttrs.IndexType = DiligentIndexBufferType[IndexBuffer::GetIndexBufferType(indexBuffer_)];

    impl_->deviceContext_->DrawIndexed(drawAttrs);

    numPrimitives_ += instanceCount * primitiveCount;
    ++numBatches_;
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex,
    unsigned minVertex, unsigned vertexCount, unsigned instanceCount)
{
    assert(0);
    /*if (!indexCount || !instanceCount || !impl_->shaderProgram_)
        return;

    PrepareDraw();

    unsigned primitiveCount;
    D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveType;

    if (fillMode_ == FILL_POINT)
        type = POINT_LIST;

    GetPrimitiveType(indexCount, type, primitiveCount, d3dPrimitiveType);
    if (d3dPrimitiveType != primitiveType_)
    {
        impl_->deviceContext_->IASetPrimitiveTopology(d3dPrimitiveType);
        primitiveType_ = d3dPrimitiveType;
    }
    impl_->deviceContext_->DrawIndexedInstanced(indexCount, instanceCount, indexStart, baseVertexIndex, 0);

    numPrimitives_ += instanceCount * primitiveCount;
    ++numBatches_;*/
}

void Graphics::SetVertexBuffer(VertexBuffer* buffer)
{
    // Note: this is not multi-instance safe
    static ea::vector<VertexBuffer*> vertexBuffers(1);
    vertexBuffers[0] = buffer;
    SetVertexBuffers(vertexBuffers);
}

bool Graphics::SetVertexBuffers(const ea::vector<VertexBuffer*>& buffers, unsigned instanceOffset)
{
    if (buffers.size() > MAX_VERTEX_STREAMS)
    {
        URHO3D_LOGERROR("Too many vertex buffers");
        return false;
    }

    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        VertexBuffer* buffer = nullptr;
        bool changed = false;

        buffer = i < buffers.size() ? buffers[i] : nullptr;
        if (buffer)
        {
            // On vulkan backend, if we use a buffer that had been lost
            // diligent will thrown a assert.
            //
            if (buffer->IsDataLost())
                return false;
            const ea::vector<VertexElement>& elements = buffer->GetElements();
            // Check if buffer has per-instance data
            bool hasInstanceData = elements.size() && elements[0].perInstance_;
            unsigned offset = hasInstanceData ? instanceOffset * buffer->GetVertexSize() : 0;

            if (buffer != vertexBuffers_[i] || offset != impl_->vertexOffsets_[i])
            {
                vertexBuffers_[i] = buffer;
                impl_->vertexBuffers_[i] = buffer->GetGPUObject().Cast<Diligent::IBuffer>(Diligent::IID_Buffer);
                impl_->vertexOffsets_[i] = offset;
                changed = true;
            }
        }
        else if (vertexBuffers_[i])
        {
            vertexBuffers_[i] = nullptr;
            impl_->vertexBuffers_[i] = nullptr;
            impl_->vertexOffsets_[i] = 0;
            changed = true;
        }

        if (changed)
        {
            impl_->vertexDeclarationDirty_ = true;

            if (impl_->firstDirtyVB_ == M_MAX_UNSIGNED)
                impl_->firstDirtyVB_ = impl_->lastDirtyVB_ = i;
            else
            {
                if (i < impl_->firstDirtyVB_)
                    impl_->firstDirtyVB_ = i;
                if (i > impl_->lastDirtyVB_)
                    impl_->lastDirtyVB_ = i;
            }
        }
    }

    return true;
}

bool Graphics::SetVertexBuffers(const ea::vector<SharedPtr<VertexBuffer>>& buffers, unsigned instanceOffset)
{
    ea::vector<VertexBuffer*> bufferPointers;
    bufferPointers.reserve(buffers.size());
    for (auto& buffer : buffers)
        bufferPointers.push_back(buffer.Get());
    return SetVertexBuffers(bufferPointers, instanceOffset);
}

void Graphics::SetIndexBuffer(IndexBuffer* buffer)
{
    if (buffer != indexBuffer_)
    {
        if (buffer)
            impl_->deviceContext_->SetIndexBuffer(
                buffer->GetGPUObject().Cast<IBuffer>(IID_Buffer), 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        else
            impl_->deviceContext_->SetIndexBuffer(nullptr, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        indexBuffer_ = buffer;
    }
    /*if (buffer != indexBuffer_)
    {
        if (buffer)
            impl_->deviceContext_->IASetIndexBuffer((ID3D11Buffer*)buffer->GetGPUObject(),
                buffer->GetIndexSize() == sizeof(unsigned short) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
        else
            impl_->deviceContext_->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

        indexBuffer_ = buffer;
    }*/
}

void Graphics::SetPipelineState(PipelineState* pipelineState)
{
    impl_->deviceContext_->SetPipelineState(pipelineState->GetHandle());

    // TODO(diligent): We shouldn't need it cached
    pipelineState_ = pipelineState;
    if (pipelineState_ && pipelineState_->GetDesc().depthCompareFunction_ == CMP_ALWAYS
        && !pipelineState_->GetDesc().depthWriteEnabled_
        && pipelineState_->GetDesc().output_.depthStencilFormat_ == TEX_FORMAT_UNKNOWN)
        impl_->renderTargetsDirty_ = true;

}

ShaderProgramLayout* Graphics::GetShaderProgramLayout(ShaderVariation* vs, ShaderVariation* ps)
{
    const auto combination = ea::make_pair(vs, ps);
    auto iter = impl_->shaderPrograms_.find(combination);
    if (iter != impl_->shaderPrograms_.end())
        return iter->second;

    // TODO: Some overhead due to redundant setting of shader program
    ShaderVariation* prevVertexShader = vertexShader_;
    ShaderVariation* prevPixelShader = pixelShader_;
    SetShaders(vs, ps);
    ShaderProgramLayout* layout = impl_->shaderProgram_;
    SetShaders(prevVertexShader, prevPixelShader);
    return layout;
}

void Graphics::SetShaders(ShaderVariation* vs, ShaderVariation* ps)
{
    if (vs == vertexShader_ && ps == pixelShader_)
        return;

    if (vs != vertexShader_)
    {
        // Create the shader now if not yet created. If already attempted, do not retry
        if (vs && !vs->GetGPUObject())
        {
            if (vs->GetCompilerOutput().empty())
            {
                URHO3D_PROFILE("CompileVertexShader");

                bool success = vs->Create();
                if (!success)
                {
                    URHO3D_LOGERROR(
                        "Failed to compile vertex shader " + vs->GetFullName() + ":\n" + vs->GetCompilerOutput());
                    vs = nullptr;
                }
            }
            else
                vs = nullptr;
        }

        // impl_->deviceContext_->VSSetShader((ID3D11VertexShader*)(vs ? vs->GetGPUObject() : nullptr), nullptr, 0);
        vertexShader_ = vs;
        impl_->vertexDeclarationDirty_ = true;
    }

    if (ps != pixelShader_)
    {
        if (ps && !ps->GetGPUObject())
        {
            if (ps->GetCompilerOutput().empty())
            {
                URHO3D_PROFILE("CompilePixelShader");

                bool success = ps->Create();
                if (!success)
                {
                    URHO3D_LOGERROR(
                        "Failed to compile pixel shader " + ps->GetFullName() + ":\n" + ps->GetCompilerOutput());
                    ps = nullptr;
                }
            }
            else
                ps = nullptr;
        }

        // impl_->deviceContext_->PSSetShader((ID3D11PixelShader*)(ps ? ps->GetGPUObject() : nullptr), nullptr, 0);
        pixelShader_ = ps;
    }

    // Update current shader parameters & constant buffers
    if (vertexShader_ && pixelShader_)
    {
        ea::pair<ShaderVariation*, ShaderVariation*> key = ea::make_pair(vertexShader_, pixelShader_);
        auto i = impl_->shaderPrograms_.find(key);
        if (i != impl_->shaderPrograms_.end())
            impl_->shaderProgram_ = i->second.Get();
        else
        {
            ShaderProgram* newProgram = impl_->shaderPrograms_[key] =
                new ShaderProgram(this, vertexShader_, pixelShader_);
            impl_->shaderProgram_ = newProgram;
        }
    }
    else
        impl_->shaderProgram_ = nullptr;

    // Store shader combination if shader dumping in progress
    if (shaderPrecache_)
        shaderPrecache_->StoreShaders(vertexShader_, pixelShader_);
}

void Graphics::SetShaderConstantBuffers(ea::span<const ConstantBufferRange> constantBuffers)
{
    assert(0);
    /*bool buffersDirty = false;
    ea::vector<ResourceMappingEntry> entries;

    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
    {
        const ConstantBufferRange& range = constantBuffers[i];
        if (range != constantBuffers_[i])
        {
            buffersDirty = true;
            ResourceMappingEntry entry;
            entry.Name = shaderParameterGroupNames[i];
            entry.pObject = static_cast<IBuffer*>(range.constantBuffer_->GetGPUObject());
            entries.push_back(entry);
        }
    }

    if (buffersDirty)
        impl_->constantBufferResMapping_ = impl_->resourceMappingCache_->CreateOrGetResourceMap(entries);*/
}

void Graphics::SetShaderParameter(StringHash param, const float data[], unsigned count)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, float value)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, int value)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, bool value)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, const Color& color)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, const Vector2& vector)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3& matrix)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, const Vector3& vector)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, const Matrix4& matrix)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, const Vector4& vector)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3x4& matrix)
{
    URHO3D_LOGERROR("Graphics::SetShaderParameter is not supported for DX11");
}

bool Graphics::NeedParameterUpdate(ShaderParameterGroup group, const void* source)
{
    URHO3D_LOGERROR("Graphics::NeedParameterUpdate is not supported for DX11");
    return false;
}

bool Graphics::HasShaderParameter(StringHash param)
{
    URHO3D_LOGERROR("Graphics::HasShaderParameter is not supported for DX11");
    return false;
}

bool Graphics::HasTextureUnit(TextureUnit unit)
{
    return false;
//    return (vertexShader_ && vertexShader_->HasTextureUnit(unit))
//        || (pixelShader_ && pixelShader_->HasTextureUnit(unit));
}

void Graphics::ClearParameterSource(ShaderParameterGroup group)
{
    shaderParameterSources_[group] = reinterpret_cast<const void*>(M_MAX_UNSIGNED);
}

void Graphics::ClearParameterSources()
{
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        shaderParameterSources_[i] = reinterpret_cast<const void*>(M_MAX_UNSIGNED);
}

void Graphics::ClearTransformSources()
{
    shaderParameterSources_[SP_CAMERA] = reinterpret_cast<const void*>(M_MAX_UNSIGNED);
    shaderParameterSources_[SP_OBJECT] = reinterpret_cast<const void*>(M_MAX_UNSIGNED);
}

void Graphics::SetTexture(unsigned index, Texture* texture)
{
    if (index >= MAX_TEXTURE_UNITS)
        return;

    // Check if texture is currently bound as a rendertarget. In that case, use its backup texture, or blank if not
    // defined
    if (texture)
    {
        if (renderTargets_[0] && renderTargets_[0]->GetParentTexture() == texture)
            texture = texture->GetBackupTexture();
        else
        {
            // Resolve multisampled texture now as necessary
            if (texture->GetMultiSample() > 1 && texture->GetAutoResolve() && texture->IsResolveDirty())
            {
                if (texture->GetType() == Texture2D::GetTypeStatic())
                    ResolveToTexture(static_cast<Texture2D*>(texture));
                if (texture->GetType() == TextureCube::GetTypeStatic())
                    ResolveToTexture(static_cast<TextureCube*>(texture));
            }
        }

        if (texture && texture->GetLevelsDirty())
            texture->RegenerateLevels();
    }

    if (texture && texture->GetParametersDirty())
    {
        texture->UpdateParameters();
        textures_[index] = nullptr; // Force reassign
    }

    if (texture != textures_[index])
    {
        if (impl_->firstDirtyTexture_ == M_MAX_UNSIGNED)
            impl_->firstDirtyTexture_ = impl_->lastDirtyTexture_ = index;
        else
        {
            if (index < impl_->firstDirtyTexture_)
                impl_->firstDirtyTexture_ = index;
            if (index > impl_->lastDirtyTexture_)
                impl_->lastDirtyTexture_ = index;
        }

        textures_[index] = texture;
        impl_->shaderResourceViews_[index] = texture ? (ITextureView*)texture->GetShaderResourceView() : nullptr;
        impl_->samplers_[index] = texture ? (ISampler*)texture->GetSampler() : nullptr;
        impl_->texturesDirty_ = true;
    }
}

void SetTextureForUpdate(Texture* texture)
{
    // No-op on Direct3D11
}

void Graphics::SetDefaultTextureFilterMode(TextureFilterMode mode)
{
    if (mode != defaultTextureFilterMode_)
    {
        defaultTextureFilterMode_ = mode;
        SetTextureParametersDirty();
    }
}

void Graphics::SetDefaultTextureAnisotropy(unsigned level)
{
    level = Max(level, 1U);

    if (level != defaultTextureAnisotropy_)
    {
        defaultTextureAnisotropy_ = level;
        SetTextureParametersDirty();
    }
}

void Graphics::Restore()
{
    if (renderDevice_)
    {
        if (!renderDevice_->Restore())
            renderDevice_ = nullptr;
    }
}

void Graphics::SetTextureParametersDirty()
{
    MutexLock lock(gpuObjectMutex_);

    for (auto i = gpuObjects_.begin(); i != gpuObjects_.end(); ++i)
    {
        Texture* texture = dynamic_cast<Texture*>(*i);
        if (texture)
            texture->SetParametersDirty();
    }
}

void Graphics::ResetRenderTargets()
{
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        SetRenderTarget(i, (RenderSurface*)nullptr);
    SetDepthStencil((RenderSurface*)nullptr);
    SetViewport(IntRect(0, 0, GetWidth(), GetHeight()));
}

void Graphics::ResetRenderTarget(unsigned index)
{
    SetRenderTarget(index, (RenderSurface*)nullptr);
}

void Graphics::ResetDepthStencil()
{
    SetDepthStencil((RenderSurface*)nullptr);
}

void Graphics::SetRenderTarget(unsigned index, RenderSurface* renderTarget)
{
    if (index >= MAX_RENDERTARGETS)
        return;

    if (renderTarget != renderTargets_[index])
    {
        renderTargets_[index] = renderTarget;
        impl_->renderTargetsDirty_ = true;

        // If the rendertarget is also bound as a texture, replace with backup texture or null
        if (renderTarget)
        {
            Texture* parentTexture = renderTarget->GetParentTexture();

            for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
            {
                if (textures_[i] == parentTexture)
                    SetTexture(i, textures_[i]->GetBackupTexture());
            }

            // If multisampled, mark the texture & surface needing resolve
            if (parentTexture->GetMultiSample() > 1 && parentTexture->GetAutoResolve())
            {
                parentTexture->SetResolveDirty(true);
                renderTarget->SetResolveDirty(true);
            }

            // If mipmapped, mark the levels needing regeneration
            if (parentTexture->GetLevels() > 1)
                parentTexture->SetLevelsDirty();
        }
    }
}

void Graphics::SetRenderTarget(unsigned index, Texture2D* texture)
{
    RenderSurface* renderTarget = nullptr;
    if (texture)
        renderTarget = texture->GetRenderSurface();

    SetRenderTarget(index, renderTarget);
}

void Graphics::SetDepthStencil(RenderSurface* depthStencil)
{
    if (depthStencil != depthStencil_)
    {
        depthStencil_ = depthStencil;
        impl_->renderTargetsDirty_ = true;
    }
}

void Graphics::SetDepthStencil(Texture2D* texture)
{
    RenderSurface* depthStencil = nullptr;
    if (texture)
        depthStencil = texture->GetRenderSurface();

    SetDepthStencil(depthStencil);
}

void Graphics::SetViewport(const IntRect& rect)
{
    IntVector2 size = GetRenderTargetDimensions();

    IntRect rectCopy = rect;

    if (rectCopy.right_ <= rectCopy.left_)
        rectCopy.right_ = rectCopy.left_ + 1;
    if (rectCopy.bottom_ <= rectCopy.top_)
        rectCopy.bottom_ = rectCopy.top_ + 1;
    rectCopy.left_ = Clamp(rectCopy.left_, 0, size.x_);
    rectCopy.top_ = Clamp(rectCopy.top_, 0, size.y_);
    rectCopy.right_ = Clamp(rectCopy.right_, 0, size.x_);
    rectCopy.bottom_ = Clamp(rectCopy.bottom_, 0, size.y_);

    viewport_ = rectCopy;
    impl_->viewportDirty_ = true;

    // Disable scissor test, needs to be re-enabled by the user
    SetScissorTest(false);
}

void Graphics::SetBlendMode(BlendMode mode, bool alphaToCoverage)
{
    if (mode != blendMode_ || alphaToCoverage != alphaToCoverage_)
    {
        blendMode_ = mode;
        alphaToCoverage_ = alphaToCoverage;
        impl_->blendStateDirty_ = true;
    }
}

void Graphics::SetColorWrite(bool enable)
{
    if (enable != colorWrite_)
    {
        colorWrite_ = enable;
        impl_->blendStateDirty_ = true;
    }
}

void Graphics::SetCullMode(CullMode mode)
{
    if (mode != cullMode_)
    {
        cullMode_ = mode;
        impl_->rasterizerStateDirty_ = true;
    }
}

void Graphics::SetDepthBias(float constantBias, float slopeScaledBias)
{
    if (constantBias != constantDepthBias_ || slopeScaledBias != slopeScaledDepthBias_)
    {
        constantDepthBias_ = constantBias;
        slopeScaledDepthBias_ = slopeScaledBias;
        impl_->rasterizerStateDirty_ = true;
    }
}

void Graphics::SetDepthTest(CompareMode mode)
{
    if (mode != depthTestMode_)
    {
        depthTestMode_ = mode;
        impl_->depthStateDirty_ = true;
    }
}

void Graphics::SetDepthWrite(bool enable)
{
    if (!pipelineState_)
    {
        impl_->depthStateDirty_ = true;
        impl_->depthStateDirty_ = true;
        return;
    }
    if (enable != pipelineState_->GetDesc().depthWriteEnabled_)
    {
        impl_->depthStateDirty_ = true;
        // Also affects whether a read-only version of depth-stencil should be bound, to allow sampling
        impl_->renderTargetsDirty_ = true;
    }
}

void Graphics::SetFillMode(FillMode mode)
{
    if (mode != fillMode_)
    {
        fillMode_ = mode;
        impl_->rasterizerStateDirty_ = true;
    }
}

void Graphics::SetLineAntiAlias(bool enable)
{
    if (enable != lineAntiAlias_)
    {
        lineAntiAlias_ = enable;
        impl_->rasterizerStateDirty_ = true;
    }
}

void Graphics::SetScissorTest(bool enable, const Rect& rect, bool borderInclusive)
{
    // During some light rendering loops, a full rect is toggled on/off repeatedly.
    // Disable scissor in that case to reduce state changes
    if (rect.min_.x_ <= 0.0f && rect.min_.y_ <= 0.0f && rect.max_.x_ >= 1.0f && rect.max_.y_ >= 1.0f)
        enable = false;

    if (enable)
    {
        IntVector2 rtSize(GetRenderTargetDimensions());
        IntVector2 viewSize(viewport_.Size());
        IntVector2 viewPos(viewport_.left_, viewport_.top_);
        IntRect intRect;
        int expand = borderInclusive ? 1 : 0;

        intRect.left_ = Clamp((int)((rect.min_.x_ + 1.0f) * 0.5f * viewSize.x_) + viewPos.x_, 0, rtSize.x_ - 1);
        intRect.top_ = Clamp((int)((-rect.max_.y_ + 1.0f) * 0.5f * viewSize.y_) + viewPos.y_, 0, rtSize.y_ - 1);
        intRect.right_ = Clamp((int)((rect.max_.x_ + 1.0f) * 0.5f * viewSize.x_) + viewPos.x_ + expand, 0, rtSize.x_);
        intRect.bottom_ = Clamp((int)((-rect.min_.y_ + 1.0f) * 0.5f * viewSize.y_) + viewPos.y_ + expand, 0, rtSize.y_);

        if (intRect.right_ == intRect.left_)
            intRect.right_++;
        if (intRect.bottom_ == intRect.top_)
            intRect.bottom_++;

        if (intRect.right_ < intRect.left_ || intRect.bottom_ < intRect.top_)
            enable = false;

        if (enable && intRect != scissorRect_)
        {
            scissorRect_ = intRect;
            impl_->scissorRectDirty_ = true;
        }
    }

    if (enable != scissorTest_)
    {
        scissorTest_ = enable;
        impl_->rasterizerStateDirty_ = true;
    }
}

void Graphics::SetScissorTest(bool enable, const IntRect& rect)
{
    IntVector2 rtSize(GetRenderTargetDimensions());
    IntVector2 viewPos(viewport_.left_, viewport_.top_);

    if (enable)
    {
        IntRect intRect;
        intRect.left_ = Clamp(rect.left_ + viewPos.x_, 0, rtSize.x_ - 1);
        intRect.top_ = Clamp(rect.top_ + viewPos.y_, 0, rtSize.y_ - 1);
        intRect.right_ = Clamp(rect.right_ + viewPos.x_, 0, rtSize.x_);
        intRect.bottom_ = Clamp(rect.bottom_ + viewPos.y_, 0, rtSize.y_);

        if (intRect.right_ == intRect.left_)
            intRect.right_++;
        if (intRect.bottom_ == intRect.top_)
            intRect.bottom_++;

        if (intRect.right_ < intRect.left_ || intRect.bottom_ < intRect.top_)
            enable = false;

        if (enable && intRect != scissorRect_)
        {
            scissorRect_ = intRect;
            impl_->scissorRectDirty_ = true;
        }
    }

    if (enable != scissorTest_)
    {
        scissorTest_ = enable;
        impl_->rasterizerStateDirty_ = true;
    }
}

void Graphics::SetStencilTest(bool enable, CompareMode mode, StencilOp pass, StencilOp fail, StencilOp zFail,
    unsigned stencilRef, unsigned compareMask, unsigned writeMask)
{
    if (enable != stencilTest_)
    {
        stencilTest_ = enable;
        impl_->depthStateDirty_ = true;
    }

    if (enable)
    {
        if (mode != stencilTestMode_)
        {
            stencilTestMode_ = mode;
            impl_->depthStateDirty_ = true;
        }
        if (pass != stencilPass_)
        {
            stencilPass_ = pass;
            impl_->depthStateDirty_ = true;
        }
        if (fail != stencilFail_)
        {
            stencilFail_ = fail;
            impl_->depthStateDirty_ = true;
        }
        if (zFail != stencilZFail_)
        {
            stencilZFail_ = zFail;
            impl_->depthStateDirty_ = true;
        }
        if (compareMask != stencilCompareMask_)
        {
            stencilCompareMask_ = compareMask;
            impl_->depthStateDirty_ = true;
        }
        if (writeMask != stencilWriteMask_)
        {
            stencilWriteMask_ = writeMask;
            impl_->depthStateDirty_ = true;
        }
        if (stencilRef != stencilRef_)
        {
            stencilRef_ = stencilRef;
            impl_->stencilRefDirty_ = true;
            impl_->depthStateDirty_ = true;
        }
    }
}

void Graphics::SetClipPlane(bool enable, const Plane& clipPlane, const Matrix3x4& view, const Matrix4& projection)
{
    // Basically no-op for DX11, clip plane has to be managed in user code
    useClipPlane_ = enable;
}

bool Graphics::IsInitialized() const
{
    return window_ != nullptr && impl_->device_ != nullptr;
}

ea::vector<int> Graphics::GetMultiSampleLevels() const
{
    using namespace Diligent;
    ea::vector<int> ret;
    ret.emplace_back(1);

    if (!impl_->device_)
        return ret;
    const TextureFormatInfoExt& colorFmtInfo =
        impl_->device_->GetTextureFormatInfoExt(GetSRGB() ? TEX_FORMAT_RGBA8_UNORM_SRGB : TEX_FORMAT_RGBA8_UNORM);
    if (colorFmtInfo.SampleCounts & SAMPLE_COUNT_64)
        ret.emplace_back(64);
    else if (colorFmtInfo.SampleCounts & SAMPLE_COUNT_32)
        ret.emplace_back(32);
    else if (colorFmtInfo.SampleCounts & SAMPLE_COUNT_16)
        ret.emplace_back(16);
    else if (colorFmtInfo.SampleCounts & SAMPLE_COUNT_8)
        ret.emplace_back(8);
    else if (colorFmtInfo.SampleCounts & SAMPLE_COUNT_4)
        ret.emplace_back(4);
    else if (colorFmtInfo.SampleCounts & SAMPLE_COUNT_2)
        ret.emplace_back(2);

    return ret;
}

unsigned Graphics::GetFormat(CompressedFormat format) const
{
    switch (format)
    {
    case CF_RGBA: return TEX_FORMAT_RGBA8_UNORM;

    case CF_DXT1: return TEX_FORMAT_BC1_UNORM;

    case CF_DXT3: return TEX_FORMAT_BC2_UNORM;

    case CF_DXT5: return TEX_FORMAT_BC3_UNORM;

    default: return 0;
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

VertexBuffer* Graphics::GetVertexBuffer(unsigned index) const
{
    return index < MAX_VERTEX_STREAMS ? vertexBuffers_[index] : nullptr;
}

ShaderProgram* Graphics::GetShaderProgram() const
{
    return impl_->shaderProgram_;
}

TextureUnit Graphics::GetTextureUnit(const ea::string& name)
{
    auto i = textureUnits_.find(name);
    if (i != textureUnits_.end())
        return i->second;
    else
        return MAX_TEXTURE_UNITS;
}

const ea::string& Graphics::GetTextureUnitName(TextureUnit unit)
{
    for (auto i = textureUnits_.begin(); i != textureUnits_.end(); ++i)
    {
        if (i->second == unit)
            return i->first;
    }
    return EMPTY_STRING;
}

Texture* Graphics::GetTexture(unsigned index) const
{
    return index < MAX_TEXTURE_UNITS ? textures_[index] : nullptr;
}

RenderSurface* Graphics::GetRenderTarget(unsigned index) const
{
    return index < MAX_RENDERTARGETS ? renderTargets_[index] : nullptr;
}

IntVector2 Graphics::GetRenderTargetDimensions() const
{
    int width, height;

    if (renderTargets_[0])
    {
        width = renderTargets_[0]->GetWidth();
        height = renderTargets_[0]->GetHeight();
    }
    else if (depthStencil_) // Depth-only rendering
    {
        width = depthStencil_->GetWidth();
        height = depthStencil_->GetHeight();
    }
    else
    {
        width = GetWidth();
        height = GetHeight();
    }

    return IntVector2(width, height);
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

    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        impl_->renderTargetViews_[i] = nullptr;
    impl_->renderTargetsDirty_ = true;

    // Reset rendertargets and viewport for the new screen size
    ResetRenderTargets();

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
    if (!impl_->device_ || !window_ || GetFullscreen())
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

void Graphics::CleanupShaderPrograms(ShaderVariation* variation)
{
    for (auto i = impl_->shaderPrograms_.begin(); i != impl_->shaderPrograms_.end();)
    {
        if (i->first.first == variation || i->first.second == variation)
            i = impl_->shaderPrograms_.erase(i);
        else
            ++i;
    }

    if (vertexShader_ == variation || pixelShader_ == variation)
        impl_->shaderProgram_ = nullptr;
}

void Graphics::CleanupRenderSurface(RenderSurface* surface)
{
    // No-op on Direct3D11
}

ConstantBuffer* Graphics::GetOrCreateConstantBuffer(ShaderType type, unsigned index, unsigned size)
{
    // Ensure that different shader types and index slots get unique buffers, even if the size is same
    unsigned key = type | (index << 1) | (size << 4);
    auto i = impl_->allConstantBuffers_.find(key);
    if (i != impl_->allConstantBuffers_.end())
        return i->second.Get();
    else
    {
        SharedPtr<ConstantBuffer> newConstantBuffer(MakeShared<ConstantBuffer>(context_));
        newConstantBuffer->SetSize(size);
        impl_->allConstantBuffers_[key] = newConstantBuffer;
        return newConstantBuffer.Get();
    }
}

RenderBackend Graphics::GetRenderBackend() const
{
    return impl_->renderBackend_;
}
void Graphics::SetRenderBackend(RenderBackend renderBackend)
{
    if (impl_->device_)
    {
        URHO3D_LOGERROR("Render Backend cannot be change after graphics initialization.");
        return;
    }
    impl_->renderBackend_ = renderBackend;
}
unsigned Graphics::GetAdapterId() const
{
    return impl_->adapterId_;
}
void Graphics::SetAdapterId(unsigned adapterId)
{
    if (impl_->device_)
    {
        URHO3D_LOGERROR("Cannot change Adapter ID after graphics initialization.");
        return;
    }
    impl_->adapterId_ = adapterId;
}
unsigned Graphics::GetSwapChainRTFormat()
{
    return impl_->swapChain_->GetDesc().ColorBufferFormat;
}
unsigned Graphics::GetSwapChainDepthFormat()
{
    return impl_->swapChain_->GetDesc().DepthBufferFormat;
}

PipelineStateOutputDesc Graphics::GetSwapChainOutputDesc() const
{
    const Diligent::SwapChainDesc& swapChainDesc = impl_->swapChain_->GetDesc();

    PipelineStateOutputDesc result;
    result.depthStencilFormat_ = swapChainDesc.DepthBufferFormat;
    result.numRenderTargets_ = 1;
    result.renderTargetFormats_[0] = swapChainDesc.ColorBufferFormat;
    return result;
}

PipelineStateOutputDesc Graphics::GetCurrentOutputDesc() const
{
    // TODO(diligent): Cache PipelineStateOutputDesc
    PipelineStateOutputDesc result;

    // TODO(diligent): Revisit this mess
    // @{
    Diligent::ITextureView* depthStencil = (depthStencil_ && depthStencil_->GetUsage() == TEXTURE_DEPTHSTENCIL)
        ? (ITextureView*)depthStencil_->GetRenderTargetView()
        : impl_->swapChain_->GetDepthBufferDSV();

    Diligent::ITextureView* renderTargets[MAX_RENDERTARGETS]{};
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        renderTargets[i] = (renderTargets_[i] && renderTargets_[i]->GetUsage() == TEXTURE_RENDERTARGET)
            ? (ITextureView*)renderTargets_[i]->GetRenderTargetView()
            : nullptr;
    if (!renderTargets_[0]
        && (!depthStencil_
            || (depthStencil_ && depthStencil_->GetWidth() == GetWidth() && depthStencil_->GetHeight() == GetHeight())))
        renderTargets[0] = impl_->swapChain_->GetCurrentBackBufferRTV();
    unsigned rtCount = 0;
    while (impl_->renderTargetViews_[rtCount] != nullptr)
        ++rtCount;
    // @}

    result.depthStencilFormat_ = depthStencil->GetDesc().Format;
    result.numRenderTargets_ = rtCount;
    for (unsigned i = 0; i < rtCount; ++i)
        result.renderTargetFormats_[i] = renderTargets[i]->GetDesc().Format;

    return result;
}

unsigned Graphics::GetAlphaFormat()
{
    // TODO(diligent): Revisit
    using namespace Diligent;
    return TEX_FORMAT_R8_UNORM;
}

unsigned Graphics::GetLuminanceFormat()
{
    // Note: not same sampling behavior as on D3D9; need to sample the R channel only
    using namespace Diligent;
    return TEX_FORMAT_R8_UNORM;
}

unsigned Graphics::GetLuminanceAlphaFormat()
{
    // Note: not same sampling behavior as on D3D9; need to sample the RG channels
    using namespace Diligent;
    return TEX_FORMAT_RG8_UNORM;
}

unsigned Graphics::GetRGBFormat()
{
    using namespace Diligent;
    return TEX_FORMAT_RGBA8_UNORM;
}

unsigned Graphics::GetRGBAFormat()
{
    using namespace Diligent;
    return TEX_FORMAT_RGBA8_UNORM;
}

unsigned Graphics::GetRGBA16Format()
{
    using namespace Diligent;
    return TEX_FORMAT_RGBA16_UNORM;
}

unsigned Graphics::GetRGBAFloat16Format()
{
    using namespace Diligent;
    return TEX_FORMAT_RGBA16_FLOAT;
}

unsigned Graphics::GetRGBAFloat32Format()
{
    using namespace Diligent;
    return TEX_FORMAT_RGBA32_FLOAT;
}

unsigned Graphics::GetRG16Format()
{
    using namespace Diligent;
    return TEX_FORMAT_RG16_UNORM;
}

unsigned Graphics::GetRGFloat16Format()
{
    using namespace Diligent;
    return TEX_FORMAT_RG16_UNORM;
}

unsigned Graphics::GetRGFloat32Format()
{
    using namespace Diligent;
    return TEX_FORMAT_RG32_FLOAT;
}

unsigned Graphics::GetFloat16Format()
{
    using namespace Diligent;
    return TEX_FORMAT_R16_FLOAT;
}

unsigned Graphics::GetFloat32Format()
{
    using namespace Diligent;
    return TEX_FORMAT_R32_FLOAT;
}

unsigned Graphics::GetLinearDepthFormat()
{
    using namespace Diligent;
    return TEX_FORMAT_D32_FLOAT;
}

unsigned Graphics::GetDepthStencilFormat()
{
    using namespace Diligent;
    return TEX_FORMAT_D24_UNORM_S8_UINT;
}

unsigned Graphics::GetReadableDepthFormat()
{
    using namespace Diligent;
    return TEX_FORMAT_D24_UNORM_S8_UINT;
}

unsigned Graphics::GetReadableDepthStencilFormat()
{
    using namespace Diligent;
    return TEX_FORMAT_D24_UNORM_S8_UINT;
}

unsigned Graphics::GetFormat(const ea::string& formatName)
{
    ea::string nameLower = formatName.to_lower();
    nameLower.trim();

    if (nameLower == "a")
        return GetAlphaFormat();
    if (nameLower == "l")
        return GetLuminanceFormat();
    if (nameLower == "la")
        return GetLuminanceAlphaFormat();
    if (nameLower == "rgb")
        return GetRGBFormat();
    if (nameLower == "rgba")
        return GetRGBAFormat();
    if (nameLower == "rgba16")
        return GetRGBA16Format();
    if (nameLower == "rgba16f")
        return GetRGBAFloat16Format();
    if (nameLower == "rgba32f")
        return GetRGBAFloat32Format();
    if (nameLower == "rg16")
        return GetRG16Format();
    if (nameLower == "rg16f")
        return GetRGFloat16Format();
    if (nameLower == "rg32f")
        return GetRGFloat32Format();
    if (nameLower == "r16f")
        return GetFloat16Format();
    if (nameLower == "r32f" || nameLower == "float")
        return GetFloat32Format();
    if (nameLower == "lineardepth" || nameLower == "depth")
        return GetLinearDepthFormat();
    if (nameLower == "d24s8")
        return GetDepthStencilFormat();
    if (nameLower == "readabledepth" || nameLower == "hwdepth")
        return GetReadableDepthFormat();

    return GetRGBFormat();
}

unsigned Graphics::GetMaxBones()
{
    return 128;
}

bool Graphics::GetGL3Support()
{
    return gl3Support;
}

void Graphics::CheckFeatureSupport()
{
    anisotropySupport_ = true;
    dxtTextureSupport_ = true;
    lightPrepassSupport_ = true;
    deferredSupport_ = true;
    hardwareShadowSupport_ = true;
    instancingSupport_ = true;
    shadowMapFormat_ = TEX_FORMAT_D16_UNORM;
    hiresShadowMapFormat_ = TEX_FORMAT_D24_UNORM_S8_UINT;
    dummyColorFormat_ = TEX_FORMAT_UNKNOWN;
    sRGBSupport_ = true;
    sRGBWriteSupport_ = true;

    auto deviceFeatures = impl_->device_->GetDeviceInfo().Features;
    caps.maxVertexShaderUniforms_ = 4096;
    caps.maxPixelShaderUniforms_ = 4096;
    caps.constantBuffersSupported_ = true;
    caps.constantBufferOffsetAlignment_ = impl_->device_->GetAdapterInfo().Buffer.ConstantBufferOffsetAlignment;
    caps.maxTextureSize_ = impl_->device_->GetAdapterInfo().Texture.MaxTexture2DDimension;
    caps.maxRenderTargetSize_ = impl_->device_->GetAdapterInfo().Texture.MaxTexture2DDimension;
    caps.maxNumRenderTargets_ = MAX_RENDER_TARGETS;

#ifdef URHO3D_COMPUTE
    computeSupport_ = true;
#endif
}

void Graphics::ResetCachedState()
{
    for (auto& constantBuffer : constantBuffers_)
        constantBuffer = {};

    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        vertexBuffers_[i] = nullptr;
        impl_->vertexBuffers_[i] = nullptr;
        // impl_->vertexSizes_[i] = 0;
        impl_->vertexOffsets_[i] = 0;
    }

    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
    {
        textures_[i] = nullptr;
        impl_->shaderResourceViews_[i] = nullptr;
        impl_->samplers_[i] = nullptr;
    }

    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
    {
        renderTargets_[i] = nullptr;
        impl_->renderTargetViews_[i] = nullptr;
    }

    ea::fill(ea::begin(impl_->constantBuffers_), ea::end(impl_->constantBuffers_), nullptr);
    ea::fill(ea::begin(impl_->constantBuffersStartSlots_), ea::end(impl_->constantBuffersStartSlots_), 0u);
    ea::fill(ea::begin(impl_->constantBuffersNumSlots_), ea::end(impl_->constantBuffersNumSlots_), 0u);

    depthStencil_ = nullptr;
    // impl_->depthStencilView_ = nullptr;
    viewport_ = IntRect(0, 0, GetWidth(), GetHeight());

    indexBuffer_ = nullptr;
    vertexDeclarationHash_ = 0;
    primitiveType_ = 0;
    vertexShader_ = nullptr;
    pixelShader_ = nullptr;
    blendMode_ = BLEND_REPLACE;
    alphaToCoverage_ = false;
    colorWrite_ = true;
    cullMode_ = CULL_CCW;
    constantDepthBias_ = 0.0f;
    slopeScaledDepthBias_ = 0.0f;
    depthTestMode_ = CMP_LESSEQUAL;
    depthWrite_ = true;
    fillMode_ = FILL_SOLID;
    lineAntiAlias_ = false;
    scissorTest_ = false;
    scissorRect_ = IntRect::ZERO;
    stencilTest_ = false;
    stencilTestMode_ = CMP_ALWAYS;
    stencilPass_ = OP_KEEP;
    stencilFail_ = OP_KEEP;
    stencilZFail_ = OP_KEEP;
    stencilRef_ = 0;
    stencilCompareMask_ = M_MAX_UNSIGNED;
    stencilWriteMask_ = M_MAX_UNSIGNED;
    useClipPlane_ = false;
    impl_->shaderProgram_ = nullptr;
    impl_->renderTargetsDirty_ = true;
    impl_->texturesDirty_ = true;
    impl_->vertexDeclarationDirty_ = true;
    impl_->blendStateDirty_ = true;
    impl_->depthStateDirty_ = true;
    impl_->rasterizerStateDirty_ = true;
    impl_->scissorRectDirty_ = true;
    impl_->stencilRefDirty_ = true;
    impl_->blendStateHash_ = M_MAX_UNSIGNED;
    impl_->depthStateHash_ = M_MAX_UNSIGNED;
    impl_->rasterizerStateHash_ = M_MAX_UNSIGNED;
    impl_->firstDirtyTexture_ = impl_->lastDirtyTexture_ = M_MAX_UNSIGNED;
    impl_->firstDirtyVB_ = impl_->lastDirtyVB_ = M_MAX_UNSIGNED;
}

void Graphics::PrepareDraw()
{
    // TODO(diligent): This is ALL terrible. Refactor.
    if (impl_->renderTargetsDirty_)
    {
        impl_->depthStencilView_ = (depthStencil_ && depthStencil_->GetUsage() == TEXTURE_DEPTHSTENCIL)
            ? (ITextureView*)depthStencil_->GetRenderTargetView()
            : impl_->swapChain_->GetDepthBufferDSV();
        if (pipelineState_ && pipelineState_->GetDesc().depthCompareFunction_ == CMP_ALWAYS
            && !pipelineState_->GetDesc().depthWriteEnabled_
            && pipelineState_->GetDesc().output_.depthStencilFormat_ == TEX_FORMAT_UNKNOWN)
            impl_->depthStencilView_ = nullptr;

        // If possible, bind a read-only depth stencil view to allow reading depth in shader
        if (!depthWrite_ && depthStencil_ && depthStencil_->GetReadOnlyView())
            impl_->depthStencilView_ = (ITextureView*)depthStencil_->GetReadOnlyView();

        //assert(impl_->depthStencilView_);

        for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
            impl_->renderTargetViews_[i] = (renderTargets_[i] && renderTargets_[i]->GetUsage() == TEXTURE_RENDERTARGET)
                ? (ITextureView*)renderTargets_[i]->GetRenderTargetView()
                : nullptr;
        // If rendertarget 0 is null and not doing depth-only rendering, render to the backbuffer
        // Special case: if rendertarget 0 is null and depth stencil has same size as backbuffer, assume the intention
        // is to do backbuffer rendering with a custom depth stencil
        if (!renderTargets_[0]
            && (!depthStencil_
                || (depthStencil_ && depthStencil_->GetWidth() == GetWidth() && depthStencil_->GetHeight() == GetHeight())))
            impl_->renderTargetViews_[0] = impl_->swapChain_->GetCurrentBackBufferRTV();

        unsigned rtCount = 0;
        while (impl_->renderTargetViews_[rtCount] != nullptr)
            ++rtCount;
        impl_->deviceContext_->SetRenderTargets(rtCount, &impl_->renderTargetViews_[0], impl_->depthStencilView_,
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        impl_->renderTargetsDirty_ = false;
        // When RenderTarget is changed, Diligent forces Viewport by size of Render Target.
        impl_->viewportDirty_ = true;
    }

    if (impl_->firstDirtyVB_ < M_MAX_UNSIGNED)
    {
        impl_->deviceContext_->SetVertexBuffers(impl_->firstDirtyVB_, impl_->lastDirtyVB_ - impl_->firstDirtyVB_ + 1,
            &impl_->vertexBuffers_[impl_->firstDirtyVB_], &impl_->vertexOffsets_[impl_->firstDirtyVB_],
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_NONE);
        impl_->firstDirtyVB_ = impl_->lastDirtyVB_ = M_MAX_UNSIGNED;
    }

    static const float blendFactors[] = {1.f, 1.f, 1.f, 1.f};
    impl_->deviceContext_->SetBlendFactors(blendFactors);

    if (impl_->viewportDirty_)
    {
        Diligent::Viewport viewport;
        viewport.TopLeftX = viewport_.left_;
        viewport.TopLeftY = viewport_.top_;
        viewport.Width = (viewport_.right_ - viewport_.left_);
        viewport.Height = (viewport_.bottom_ - viewport_.top_);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        impl_->GetDeviceContext()->SetViewports(1, &viewport, 0, 0);
        impl_->viewportDirty_ = false;
    }

    if (impl_->scissorRectDirty_)
    {
        Diligent::Rect rect;
        rect.left = scissorRect_.left_;
        rect.top = scissorRect_.top_;
        rect.right = scissorRect_.right_;
        rect.bottom = scissorRect_.bottom_;

        impl_->deviceContext_->SetScissorRects(
            1, &rect, impl_->swapChain_->GetDesc().Width, impl_->swapChain_->GetDesc().Height);
        impl_->scissorRectDirty_ = false;
    }
}

void Graphics::BeginDebug(const ea::string_view& debugName)
{
    impl_->deviceContext_->BeginDebugGroup(debugName.data());
}
void Graphics::BeginDebug(const ea::string& debugName)
{
    impl_->deviceContext_->BeginDebugGroup(debugName.data());
}
void Graphics::BeginDebug(const char* debugName)
{
    impl_->deviceContext_->BeginDebugGroup(debugName);
}
void Graphics::EndDebug()
{
    impl_->deviceContext_->EndDebugGroup();
}

void Graphics::CreateResolveTexture()
{
    assert(0);
    /*if (impl_->resolveTexture_)
        return;

    D3D11_TEXTURE2D_DESC textureDesc;
    memset(&textureDesc, 0, sizeof textureDesc);
    textureDesc.Width = (UINT)width_;
    textureDesc.Height = (UINT)height_;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.CPUAccessFlags = 0;

    HRESULT hr = impl_->device_->CreateTexture2D(&textureDesc, nullptr, &impl_->resolveTexture_);
    if (FAILED(hr))
    {
        URHO3D_SAFE_RELEASE(impl_->resolveTexture_);
        URHO3D_LOGD3DERROR("Could not create resolve texture", hr);
    }*/
}

void Graphics::SetTextureUnitMappings()
{
    textureUnits_["DiffMap"] = TU_DIFFUSE;
    textureUnits_["DiffCubeMap"] = TU_DIFFUSE;
    textureUnits_["NormalMap"] = TU_NORMAL;
    textureUnits_["SpecMap"] = TU_SPECULAR;
    textureUnits_["EmissiveMap"] = TU_EMISSIVE;
    textureUnits_["EnvMap"] = TU_ENVIRONMENT;
    textureUnits_["EnvCubeMap"] = TU_ENVIRONMENT;
    textureUnits_["LightRampMap"] = TU_LIGHTRAMP;
    textureUnits_["LightSpotMap"] = TU_LIGHTSHAPE;
    textureUnits_["LightCubeMap"] = TU_LIGHTSHAPE;
    textureUnits_["ShadowMap"] = TU_SHADOWMAP;
#ifdef DESKTOP_GRAPHICS
    textureUnits_["FaceSelectCubeMap"] = TU_FACESELECT;
    textureUnits_["IndirectionCubeMap"] = TU_INDIRECTION;
    textureUnits_["VolumeMap"] = TU_VOLUMEMAP;
    textureUnits_["ZoneCubeMap"] = TU_ZONE;
    textureUnits_["ZoneVolumeMap"] = TU_ZONE;
#endif
}

void Graphics::SetTextureForUpdate(Texture* texture)
{
}

void Graphics::MarkFBODirty()
{
}

void Graphics::SetVBO(unsigned object)
{
}

void Graphics::SetUBO(unsigned object)
{
}

} // namespace Urho3D
