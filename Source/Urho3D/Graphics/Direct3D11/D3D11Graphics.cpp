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
#include "../../Graphics/ConstantBuffer.h"
#include "../../Graphics/Geometry.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/IndexBuffer.h"
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

#if UWP
#include <wrl/client.h>
#include <windows.ui.xaml.media.dxinterop.h>
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;
using namespace Platform;
#endif

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

#include "../../DebugNew.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

// Prefer the high-performance GPU on switchable GPU systems
extern "C"
{
__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace Urho3D
{

static const D3D11_COMPARISON_FUNC d3dCmpFunc[] =
{
    D3D11_COMPARISON_ALWAYS,
    D3D11_COMPARISON_EQUAL,
    D3D11_COMPARISON_NOT_EQUAL,
    D3D11_COMPARISON_LESS,
    D3D11_COMPARISON_LESS_EQUAL,
    D3D11_COMPARISON_GREATER,
    D3D11_COMPARISON_GREATER_EQUAL
};

static const DWORD d3dBlendEnable[] =
{
    FALSE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE
};

static const D3D11_BLEND d3dSrcBlend[] =
{
    D3D11_BLEND_ONE,
    D3D11_BLEND_ONE,
    D3D11_BLEND_DEST_COLOR,
    D3D11_BLEND_SRC_ALPHA,
    D3D11_BLEND_SRC_ALPHA,
    D3D11_BLEND_ONE,
    D3D11_BLEND_INV_DEST_ALPHA,
    D3D11_BLEND_ONE,
    D3D11_BLEND_SRC_ALPHA,
};

static const D3D11_BLEND d3dDestBlend[] =
{
    D3D11_BLEND_ZERO,
    D3D11_BLEND_ONE,
    D3D11_BLEND_ZERO,
    D3D11_BLEND_INV_SRC_ALPHA,
    D3D11_BLEND_ONE,
    D3D11_BLEND_INV_SRC_ALPHA,
    D3D11_BLEND_DEST_ALPHA,
    D3D11_BLEND_ONE,
    D3D11_BLEND_ONE
};

static const D3D11_BLEND_OP d3dBlendOp[] =
{
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_REV_SUBTRACT,
    D3D11_BLEND_OP_REV_SUBTRACT
};

static const D3D11_STENCIL_OP d3dStencilOp[] =
{
    D3D11_STENCIL_OP_KEEP,
    D3D11_STENCIL_OP_ZERO,
    D3D11_STENCIL_OP_REPLACE,
    D3D11_STENCIL_OP_INCR,
    D3D11_STENCIL_OP_DECR
};

static const D3D11_CULL_MODE d3dCullMode[] =
{
    D3D11_CULL_NONE,
    D3D11_CULL_BACK,
    D3D11_CULL_FRONT
};

static const D3D11_FILL_MODE d3dFillMode[] =
{
    D3D11_FILL_SOLID,
    D3D11_FILL_WIREFRAME,
    D3D11_FILL_WIREFRAME // Point fill mode not supported
};

struct ClearFramebufferConstantBuffer
{
    Matrix3x4 matrix_;
    Vector4 color_;
};

static void GetD3DPrimitiveType(unsigned elementCount, PrimitiveType type, unsigned& primitiveCount,
    D3D_PRIMITIVE_TOPOLOGY& d3dPrimitiveType)
{
    switch (type)
    {
    case TRIANGLE_LIST:
        primitiveCount = elementCount / 3;
        d3dPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        break;

    case LINE_LIST:
        primitiveCount = elementCount / 2;
        d3dPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        break;

    case POINT_LIST:
        primitiveCount = elementCount;
        d3dPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        break;

    case TRIANGLE_STRIP:
        primitiveCount = elementCount - 2;
        d3dPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        break;

    case LINE_STRIP:
        primitiveCount = elementCount - 1;
        d3dPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        break;

    case TRIANGLE_FAN:
        // Triangle fan is not supported on D3D11
        primitiveCount = 0;
        d3dPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        break;
    }
}

#ifndef UWP
static HWND GetWindowHandle(SDL_Window* window)
{
    SDL_SysWMinfo sysInfo;

    SDL_VERSION(&sysInfo.version);
    SDL_GetWindowWMInfo(window, &sysInfo);
    return sysInfo.info.win.window;
}
#endif

const Vector2 Graphics::pixelUVOffset(0.0f, 0.0f);
bool Graphics::gl3Support = false;

Graphics::Graphics(Context* context) :
    Object(context),
    impl_(new GraphicsImpl()),
    position_(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED),
    shaderPath_("Shaders/HLSL/"),
    shaderExtension_(".hlsl"),
    orientations_("LandscapeLeft LandscapeRight"),
    apiName_("D3D11")
{
    SetTextureUnitMappings();
    ResetCachedState();

    context_->RequireSDL(SDL_INIT_VIDEO);
}

Graphics::~Graphics()
{
    {
        MutexLock lock(gpuObjectMutex_);

        // Release all GPU objects that still exist
        for (auto i = gpuObjects_.begin(); i != gpuObjects_.end(); ++i)
            (*i)->Release();
        gpuObjects_.clear();
    }

    impl_->vertexDeclarations_.clear();
    impl_->allConstantBuffers_.clear();

    for (auto i = impl_->blendStates_.begin(); i != impl_->blendStates_.end(); ++i)
    {
        URHO3D_SAFE_RELEASE(i->second);
    }
    impl_->blendStates_.clear();

    for (auto i = impl_->depthStates_.begin(); i != impl_->depthStates_.end(); ++i)
    {
        URHO3D_SAFE_RELEASE(i->second);
    }
    impl_->depthStates_.clear();

    for (auto i = impl_->rasterizerStates_.begin();
         i != impl_->rasterizerStates_.end(); ++i)
    {
        URHO3D_SAFE_RELEASE(i->second);
    }
    impl_->rasterizerStates_.clear();

    URHO3D_SAFE_RELEASE(impl_->defaultRenderTargetView_);
    URHO3D_SAFE_RELEASE(impl_->defaultDepthStencilView_);
    URHO3D_SAFE_RELEASE(impl_->defaultDepthTexture_);
    URHO3D_SAFE_RELEASE(impl_->resolveTexture_);
    URHO3D_SAFE_RELEASE(impl_->swapChain_);
    URHO3D_SAFE_RELEASE(impl_->deviceContext_);
    URHO3D_SAFE_RELEASE(impl_->device_);

    if (window_)
    {
        SDL_ShowCursor(SDL_TRUE);
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    delete impl_;
    impl_ = nullptr;

    context_->ReleaseSDL();
}

bool Graphics::SetScreenMode(int width, int height, const ScreenModeParams& params, bool maximize)
{
    URHO3D_PROFILE("SetScreenMode");

    // Ensure that parameters are properly filled
    ScreenModeParams newParams = params;
    AdjustScreenMode(width, height, newParams, maximize);

    // Find out the full screen mode display format (match desktop color depth)
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(newParams.monitor_, &mode);
    const DXGI_FORMAT fullscreenFormat = SDL_BITSPERPIXEL(mode.format) == 16 ? DXGI_FORMAT_B5G6R5_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;

    // If nothing changes, do not reset the device
    if (width == width_ && height == height_ && newParams == screenParams_)
        return true;

    SDL_SetHint(SDL_HINT_ORIENTATIONS, orientations_.c_str());

    if (!window_)
    {
        if (!OpenWindow(width, height, newParams.resizable_, newParams.borderless_))
            return false;
    }

    AdjustWindow(width, height, newParams.fullscreen_, newParams.borderless_, newParams.monitor_);

    if (maximize)
    {
        Maximize();
        SDL_GetWindowSize(window_, &width, &height);
    }

    const int oldMultiSample = screenParams_.multiSample_;
    screenParams_ = newParams;

    if (!impl_->device_ || screenParams_.multiSample_ != oldMultiSample)
        CreateDevice(width, height);
    UpdateSwapChain(width, height);

    // Clear the initial window contents to black
    Clear(CLEAR_COLOR);
    impl_->swapChain_->Present(0, 0);

    OnScreenModeChanged();
    return true;
}

void Graphics::SetSRGB(bool enable)
{
    bool newEnable = enable && sRGBWriteSupport_;
    if (newEnable != sRGB_)
    {
        sRGB_ = newEnable;
        if (impl_->swapChain_)
        {
            // Recreate swap chain for the new backbuffer format
            CreateDevice(width_, height_);
            UpdateSwapChain(width_, height_);
        }
    }
}

void Graphics::SetDither(bool enable)
{
    // No effect on Direct3D11
}

void Graphics::SetFlushGPU(bool enable)
{
    flushGPU_ = enable;

    if (impl_->device_)
    {
        IDXGIDevice1* dxgiDevice;
        impl_->device_->QueryInterface(IID_IDXGIDevice1, (void**)&dxgiDevice);
        if (dxgiDevice)
        {
            dxgiDevice->SetMaximumFrameLatency(enable ? 1 : 3);
            dxgiDevice->Release();
        }
    }
}

void Graphics::SetForceGL2(bool enable)
{
    // No effect on Direct3D11
}

void Graphics::Close()
{
    if (window_)
    {
        SDL_ShowCursor(SDL_TRUE);
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

bool Graphics::TakeScreenShot(Image& destImage)
{
    URHO3D_PROFILE("TakeScreenShot");

    if (!impl_->device_)
        return false;

    D3D11_TEXTURE2D_DESC textureDesc;
    memset(&textureDesc, 0, sizeof textureDesc);
    textureDesc.Width = (UINT)width_;
    textureDesc.Height = (UINT)height_;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_STAGING;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* stagingTexture = nullptr;
    HRESULT hr = impl_->device_->CreateTexture2D(&textureDesc, nullptr, &stagingTexture);
    if (FAILED(hr))
    {
        URHO3D_SAFE_RELEASE(stagingTexture);
        URHO3D_LOGD3DERROR("Could not create staging texture for screenshot", hr);
        return false;
    }

    ID3D11Resource* source = nullptr;
    impl_->defaultRenderTargetView_->GetResource(&source);

    if (screenParams_.multiSample_ > 1)
    {
        // If backbuffer is multisampled, need another DEFAULT usage texture to resolve the data to first
        CreateResolveTexture();

        if (!impl_->resolveTexture_)
        {
            stagingTexture->Release();
            source->Release();
            return false;
        }

        impl_->deviceContext_->ResolveSubresource(impl_->resolveTexture_, 0, source, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
        impl_->deviceContext_->CopyResource(stagingTexture, impl_->resolveTexture_);
    }
    else
        impl_->deviceContext_->CopyResource(stagingTexture, source);

    source->Release();

    D3D11_MAPPED_SUBRESOURCE mappedData;
    mappedData.pData = nullptr;
    hr = impl_->deviceContext_->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedData);
    if (FAILED(hr) || !mappedData.pData)
    {
        URHO3D_LOGD3DERROR("Could not map staging texture for screenshot", hr);
        stagingTexture->Release();
        return false;
    }

    destImage.SetSize(width_, height_, 3);
    unsigned char* destData = destImage.GetData();
    for (int y = 0; y < height_; ++y)
    {
        unsigned char* src = (unsigned char*)mappedData.pData + y * mappedData.RowPitch;
        for (int x = 0; x < width_; ++x)
        {
            *destData++ = *src++;
            *destData++ = *src++;
            *destData++ = *src++;
            ++src;
        }
    }

    impl_->deviceContext_->Unmap(stagingTexture, 0);
    stagingTexture->Release();
    return true;
}

bool Graphics::BeginFrame()
{
    if (!IsInitialized())
        return false;

    if (!externalWindow_)
    {
        // To prevent a loop of endless device loss and flicker, do not attempt to render when in fullscreen
        // and the window is minimized
        if (screenParams_.fullscreen_ && (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED))
            return false;
    }

    // Set default rendertarget and depth buffer
    ResetRenderTargets();

    // Cleanup textures from previous frame
    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        SetTexture(i, nullptr);

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
        impl_->swapChain_->Present(screenParams_.vsync_ ? 1 : 0, 0);
    }

    // Clean up too large scratch buffers
    CleanupScratchBuffers();

    // If using an external window, check it for size changes, and reset screen mode if necessary
    if (externalWindow_)
    {
        int width, height;

        SDL_GetWindowSize(window_, &width, &height);
        if (width != width_ || height != height_)
            SetMode(width, height);
    }
}

void Graphics::Clear(ClearTargetFlags flags, const Color& color, float depth, unsigned stencil)
{
    IntVector2 rtSize = GetRenderTargetDimensions();

    bool oldColorWrite = colorWrite_;
    bool oldDepthWrite = depthWrite_;

    // D3D11 clear always clears the whole target regardless of viewport or scissor test settings
    // Emulate partial clear by rendering a quad
    if (!viewport_.left_ && !viewport_.top_ && viewport_.right_ == rtSize.x_ && viewport_.bottom_ == rtSize.y_)
    {
        // Make sure we use the read-write version of the depth stencil
        SetDepthWrite(true);
        PrepareDraw();

        if ((flags & CLEAR_COLOR) && impl_->renderTargetViews_[0])
            impl_->deviceContext_->ClearRenderTargetView(impl_->renderTargetViews_[0], color.Data());

        if ((flags & (CLEAR_DEPTH | CLEAR_STENCIL)) && impl_->depthStencilView_)
        {
            unsigned depthClearFlags = 0;
            if (flags & CLEAR_DEPTH)
                depthClearFlags |= D3D11_CLEAR_DEPTH;
            if (flags & CLEAR_STENCIL)
                depthClearFlags |= D3D11_CLEAR_STENCIL;
            impl_->deviceContext_->ClearDepthStencilView(impl_->depthStencilView_, depthClearFlags, depth, (UINT8)stencil);
        }
    }
    else
    {
        Renderer* renderer = GetSubsystem<Renderer>();
        if (!renderer)
            return;

        Geometry* geometry = renderer->GetQuadGeometry();

        ClearFramebufferConstantBuffer bufferData;
        bufferData.matrix_.m23_ = Clamp(depth, 0.0f, 1.0f);
        bufferData.color_ = color.ToVector4();

        ConstantBufferRange buffers[MAX_SHADER_PARAMETER_GROUPS];
        buffers[0].constantBuffer_ = GetOrCreateConstantBuffer(VS, 0, sizeof(bufferData));
        buffers[0].constantBuffer_->Update(&bufferData);
        buffers[0].size_ = sizeof(bufferData);

        SetBlendMode(BLEND_REPLACE);
        SetColorWrite(flags & CLEAR_COLOR);
        SetCullMode(CULL_NONE);
        SetDepthTest(CMP_ALWAYS);
        SetDepthWrite(flags & CLEAR_DEPTH);
        SetFillMode(FILL_SOLID);
        SetScissorTest(false);
        SetStencilTest(flags & CLEAR_STENCIL, CMP_ALWAYS, OP_REF, OP_KEEP, OP_KEEP, stencil);
        SetShaders(GetShader(VS, "ClearFramebuffer"), GetShader(PS, "ClearFramebuffer"));
        SetShaderConstantBuffers(buffers);

        geometry->Draw(this);

        SetStencilTest(false);
        ClearParameterSources();
    }

    // Restore color & depth write state now
    SetColorWrite(oldColorWrite);
    SetDepthWrite(oldDepthWrite);
}

bool Graphics::ResolveToTexture(Texture2D* destination, const IntRect& viewport)
{
    if (!destination || !destination->GetRenderSurface())
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
            impl_->deviceContext_->CopySubresourceRegion((ID3D11Resource*)destination->GetGPUObject(), 0, 0, 0, 0, source, 0, &srcBox);
    }
    else
    {
        if (!srcBox.left && !srcBox.top && srcBox.right == width_ && srcBox.bottom == height_)
        {
            impl_->deviceContext_->ResolveSubresource((ID3D11Resource*)destination->GetGPUObject(), 0, source, 0, (DXGI_FORMAT)
                destination->GetFormat());
        }
        else
        {
            CreateResolveTexture();

            if (impl_->resolveTexture_)
            {
                impl_->deviceContext_->ResolveSubresource(impl_->resolveTexture_, 0, source, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
                impl_->deviceContext_->CopySubresourceRegion((ID3D11Resource*)destination->GetGPUObject(), 0, 0, 0, 0, impl_->resolveTexture_, 0, &srcBox);
            }
        }
    }

    source->Release();

    return true;
}

bool Graphics::ResolveToTexture(Texture2D* texture)
{
    if (!texture)
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
    return true;
}

bool Graphics::ResolveToTexture(TextureCube* texture)
{
    if (!texture)
        return false;

    texture->SetResolveDirty(false);
    ID3D11Resource* source = (ID3D11Resource*)texture->GetGPUObject();
    ID3D11Resource* dest = (ID3D11Resource*)texture->GetResolveTexture();
    if (!source || !dest)
        return false;

    for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
    {
        // Resolve only the surface(s) that were actually rendered to
        RenderSurface* surface = texture->GetRenderSurface((CubeMapFace)i);
        if (!surface->IsResolveDirty())
            continue;

        surface->SetResolveDirty(false);
        unsigned subResource = D3D11CalcSubresource(0, i, texture->GetLevels());
        impl_->deviceContext_->ResolveSubresource(dest, subResource, source, subResource, (DXGI_FORMAT)texture->GetFormat());
    }

    return true;
}


void Graphics::Draw(PrimitiveType type, unsigned vertexStart, unsigned vertexCount)
{
    if (!vertexCount || !impl_->shaderProgram_)
        return;

    PrepareDraw();

    unsigned primitiveCount;
    D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveType;

    if (fillMode_ == FILL_POINT)
        type = POINT_LIST;

    GetD3DPrimitiveType(vertexCount, type, primitiveCount, d3dPrimitiveType);
    if (d3dPrimitiveType != primitiveType_)
    {
        impl_->deviceContext_->IASetPrimitiveTopology(d3dPrimitiveType);
        primitiveType_ = d3dPrimitiveType;
    }
    impl_->deviceContext_->Draw(vertexCount, vertexStart);

    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount)
{
    if (!impl_->shaderProgram_)
        return;

    PrepareDraw();

    unsigned primitiveCount;
    D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveType;

    if (fillMode_ == FILL_POINT)
        type = POINT_LIST;

    GetD3DPrimitiveType(indexCount, type, primitiveCount, d3dPrimitiveType);
    if (d3dPrimitiveType != primitiveType_)
    {
        impl_->deviceContext_->IASetPrimitiveTopology(d3dPrimitiveType);
        primitiveType_ = d3dPrimitiveType;
    }
    impl_->deviceContext_->DrawIndexed(indexCount, indexStart, 0);

    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex, unsigned vertexCount)
{
    if (!impl_->shaderProgram_)
        return;

    PrepareDraw();

    unsigned primitiveCount;
    D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveType;

    if (fillMode_ == FILL_POINT)
        type = POINT_LIST;

    GetD3DPrimitiveType(indexCount, type, primitiveCount, d3dPrimitiveType);
    if (d3dPrimitiveType != primitiveType_)
    {
        impl_->deviceContext_->IASetPrimitiveTopology(d3dPrimitiveType);
        primitiveType_ = d3dPrimitiveType;
    }
    impl_->deviceContext_->DrawIndexed(indexCount, indexStart, baseVertexIndex);

    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount,
    unsigned instanceCount)
{
    if (!indexCount || !instanceCount || !impl_->shaderProgram_)
        return;

    PrepareDraw();

    unsigned primitiveCount;
    D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveType;

    if (fillMode_ == FILL_POINT)
        type = POINT_LIST;

    GetD3DPrimitiveType(indexCount, type, primitiveCount, d3dPrimitiveType);
    if (d3dPrimitiveType != primitiveType_)
    {
        impl_->deviceContext_->IASetPrimitiveTopology(d3dPrimitiveType);
        primitiveType_ = d3dPrimitiveType;
    }
    impl_->deviceContext_->DrawIndexedInstanced(indexCount, instanceCount, indexStart, 0, 0);

    numPrimitives_ += instanceCount * primitiveCount;
    ++numBatches_;
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex, unsigned vertexCount,
    unsigned instanceCount)
{
    if (!indexCount || !instanceCount || !impl_->shaderProgram_)
        return;

    PrepareDraw();

    unsigned primitiveCount;
    D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveType;

    if (fillMode_ == FILL_POINT)
        type = POINT_LIST;

    GetD3DPrimitiveType(indexCount, type, primitiveCount, d3dPrimitiveType);
    if (d3dPrimitiveType != primitiveType_)
    {
        impl_->deviceContext_->IASetPrimitiveTopology(d3dPrimitiveType);
        primitiveType_ = d3dPrimitiveType;
    }
    impl_->deviceContext_->DrawIndexedInstanced(indexCount, instanceCount, indexStart, baseVertexIndex, 0);

    numPrimitives_ += instanceCount * primitiveCount;
    ++numBatches_;
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
            const ea::vector<VertexElement>& elements = buffer->GetElements();
            // Check if buffer has per-instance data
            bool hasInstanceData = elements.size() && elements[0].perInstance_;
            unsigned offset = hasInstanceData ? instanceOffset * buffer->GetVertexSize() : 0;

            if (buffer != vertexBuffers_[i] || offset != impl_->vertexOffsets_[i])
            {
                vertexBuffers_[i] = buffer;
                impl_->vertexBuffers_[i] = (ID3D11Buffer*)buffer->GetGPUObject();
                impl_->vertexSizes_[i] = buffer->GetVertexSize();
                impl_->vertexOffsets_[i] = offset;
                changed = true;
            }
        }
        else if (vertexBuffers_[i])
        {
            vertexBuffers_[i] = nullptr;
            impl_->vertexBuffers_[i] = nullptr;
            impl_->vertexSizes_[i] = 0;
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

bool Graphics::SetVertexBuffers(const ea::vector<SharedPtr<VertexBuffer> >& buffers, unsigned instanceOffset)
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
            impl_->deviceContext_->IASetIndexBuffer((ID3D11Buffer*)buffer->GetGPUObject(),
                buffer->GetIndexSize() == sizeof(unsigned short) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
        else
            impl_->deviceContext_->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

        indexBuffer_ = buffer;
    }
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
                    URHO3D_LOGERROR("Failed to compile vertex shader " + vs->GetFullName() + ":\n" + vs->GetCompilerOutput());
                    vs = nullptr;
                }
            }
            else
                vs = nullptr;
        }

        impl_->deviceContext_->VSSetShader((ID3D11VertexShader*)(vs ? vs->GetGPUObject() : nullptr), nullptr, 0);
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
                    URHO3D_LOGERROR("Failed to compile pixel shader " + ps->GetFullName() + ":\n" + ps->GetCompilerOutput());
                    ps = nullptr;
                }
            }
            else
                ps = nullptr;
        }

        impl_->deviceContext_->PSSetShader((ID3D11PixelShader*)(ps ? ps->GetGPUObject() : nullptr), nullptr, 0);
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
            ShaderProgram* newProgram = impl_->shaderPrograms_[key] = new ShaderProgram(this, vertexShader_, pixelShader_);
            impl_->shaderProgram_ = newProgram;
        }
    }
    else
        impl_->shaderProgram_ = nullptr;

    // Store shader combination if shader dumping in progress
    if (shaderPrecache_)
        shaderPrecache_->StoreShaders(vertexShader_, pixelShader_);
}

void Graphics::SetShaderConstantBuffers(ea::span<const ConstantBufferRange, MAX_SHADER_PARAMETER_GROUPS> constantBuffers)
{
    bool buffersDirty = false;
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
    {
        const ConstantBufferRange& range = constantBuffers[i];
        if (range != constantBuffers_[i])
        {
            buffersDirty = true;
            impl_->constantBuffers_[i] = reinterpret_cast<ID3D11Buffer*>(range.constantBuffer_->GetGPUObject());
            impl_->constantBuffersStartSlots_[i] = range.offset_ / 16;
            impl_->constantBuffersNumSlots_[i] = (range.size_ / 16 + 15) / 16 * 16;
        }
    }

    if (buffersDirty)
    {
        // TODO: Optimize unused buffers
        impl_->deviceContext_->VSSetConstantBuffers1(0, MAX_SHADER_PARAMETER_GROUPS,
            impl_->constantBuffers_, impl_->constantBuffersStartSlots_, impl_->constantBuffersNumSlots_);
        impl_->deviceContext_->PSSetConstantBuffers1(0, MAX_SHADER_PARAMETER_GROUPS,
            impl_->constantBuffers_, impl_->constantBuffersStartSlots_, impl_->constantBuffersNumSlots_);
    }
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
    return (vertexShader_ && vertexShader_->HasTextureUnit(unit)) || (pixelShader_ && pixelShader_->HasTextureUnit(unit));
}

void Graphics::ClearParameterSource(ShaderParameterGroup group)
{
    shaderParameterSources_[group] = (const void*)M_MAX_UNSIGNED;
}

void Graphics::ClearParameterSources()
{
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        shaderParameterSources_[i] = (const void*)M_MAX_UNSIGNED;
}

void Graphics::ClearTransformSources()
{
    shaderParameterSources_[SP_CAMERA] = (const void*)M_MAX_UNSIGNED;
    shaderParameterSources_[SP_OBJECT] = (const void*)M_MAX_UNSIGNED;
}

void Graphics::SetTexture(unsigned index, Texture* texture)
{
    if (index >= MAX_TEXTURE_UNITS)
        return;

    // Check if texture is currently bound as a rendertarget. In that case, use its backup texture, or blank if not defined
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
        impl_->shaderResourceViews_[index] = texture ? (ID3D11ShaderResourceView*)texture->GetShaderResourceView() : nullptr;
        impl_->samplers_[index] = texture ? (ID3D11SamplerState*)texture->GetSampler() : nullptr;
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
    // No-op on Direct3D11
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
    SetViewport(IntRect(0, 0, width_, height_));
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
    // Constant depth bias depends on the bitdepth
    impl_->rasterizerStateDirty_ = true;
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

    static D3D11_VIEWPORT d3dViewport;
    d3dViewport.TopLeftX = (float)rectCopy.left_;
    d3dViewport.TopLeftY = (float)rectCopy.top_;
    d3dViewport.Width = (float)(rectCopy.right_ - rectCopy.left_);
    d3dViewport.Height = (float)(rectCopy.bottom_ - rectCopy.top_);
    d3dViewport.MinDepth = 0.0f;
    d3dViewport.MaxDepth = 1.0f;

    impl_->deviceContext_->RSSetViewports(1, &d3dViewport);

    viewport_ = rectCopy;

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
    if (enable != depthWrite_)
    {
        depthWrite_ = enable;
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

void Graphics::SetStencilTest(bool enable, CompareMode mode, StencilOp pass, StencilOp fail, StencilOp zFail, unsigned stencilRef,
    unsigned compareMask, unsigned writeMask)
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
    return window_ != nullptr && impl_->GetDevice() != nullptr;
}

ea::vector<int> Graphics::GetMultiSampleLevels() const
{
    ea::vector<int> ret;
    ret.emplace_back(1);

    if (impl_->device_)
    {
        for (unsigned i = 2; i <= 16; ++i)
        {
            if (impl_->CheckMultiSampleSupport(sRGB_ ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, i))
                ret.emplace_back(i);
        }
    }

    return ret;
}

unsigned Graphics::GetFormat(CompressedFormat format) const
{
    switch (format)
    {
    case CF_RGBA:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case CF_DXT1:
        return DXGI_FORMAT_BC1_UNORM;

    case CF_DXT3:
        return DXGI_FORMAT_BC2_UNORM;

    case CF_DXT5:
        return DXGI_FORMAT_BC3_UNORM;

    default:
        return 0;
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
        width = width_;
        height = height_;
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
    /// \todo The device could be lost in case of graphics adapters getting disabled during runtime. This is not currently handled
    return false;
}

void Graphics::OnWindowResized()
{
    if (!impl_->device_ || !window_)
        return;

    int newWidth, newHeight;

    SDL_GetWindowSize(window_, &newWidth, &newHeight);
    if (newWidth == width_ && newHeight == height_)
        return;

    UpdateSwapChain(newWidth, newHeight);

    // Reset rendertargets and viewport for the new screen size
    ResetRenderTargets();

    URHO3D_LOGTRACEF("Window was resized to %dx%d", width_, height_);

    using namespace ScreenMode;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_WIDTH] = width_;
    eventData[P_HEIGHT] = height_;
    eventData[P_FULLSCREEN] = screenParams_.fullscreen_;
    eventData[P_RESIZABLE] = screenParams_.resizable_;
    eventData[P_BORDERLESS] = screenParams_.borderless_;
    eventData[P_HIGHDPI] = screenParams_.highDPI_;
    SendEvent(E_SCREENMODE, eventData);
}

void Graphics::OnWindowMoved()
{
    if (!impl_->device_ || !window_ || screenParams_.fullscreen_)
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
        SharedPtr<ConstantBuffer> newConstantBuffer(context_->CreateObject<ConstantBuffer>());
        newConstantBuffer->SetSize(size);
        impl_->allConstantBuffers_[key] = newConstantBuffer;
        return newConstantBuffer.Get();
    }
}

unsigned Graphics::GetAlphaFormat()
{
    return DXGI_FORMAT_A8_UNORM;
}

unsigned Graphics::GetLuminanceFormat()
{
    // Note: not same sampling behavior as on D3D9; need to sample the R channel only
    return DXGI_FORMAT_R8_UNORM;
}

unsigned Graphics::GetLuminanceAlphaFormat()
{
    // Note: not same sampling behavior as on D3D9; need to sample the RG channels
    return DXGI_FORMAT_R8G8_UNORM;
}

unsigned Graphics::GetRGBFormat()
{
    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

unsigned Graphics::GetRGBAFormat()
{
    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

unsigned Graphics::GetRGBA16Format()
{
    return DXGI_FORMAT_R16G16B16A16_UNORM;
}

unsigned Graphics::GetRGBAFloat16Format()
{
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
}

unsigned Graphics::GetRGBAFloat32Format()
{
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
}

unsigned Graphics::GetRG16Format()
{
    return DXGI_FORMAT_R16G16_UNORM;
}

unsigned Graphics::GetRGFloat16Format()
{
    return DXGI_FORMAT_R16G16_FLOAT;
}

unsigned Graphics::GetRGFloat32Format()
{
    return DXGI_FORMAT_R32G32_FLOAT;
}

unsigned Graphics::GetFloat16Format()
{
    return DXGI_FORMAT_R16_FLOAT;
}

unsigned Graphics::GetFloat32Format()
{
    return DXGI_FORMAT_R32_FLOAT;
}

unsigned Graphics::GetLinearDepthFormat()
{
    return DXGI_FORMAT_R32_FLOAT;
}

unsigned Graphics::GetDepthStencilFormat()
{
    return DXGI_FORMAT_R24G8_TYPELESS;
}

unsigned Graphics::GetReadableDepthFormat()
{
    return DXGI_FORMAT_R24G8_TYPELESS;
}

unsigned Graphics::GetReadableDepthStencilFormat()
{
    return DXGI_FORMAT_R24G8_TYPELESS;
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

bool Graphics::OpenWindow(int width, int height, bool resizable, bool borderless)
{
    if (!externalWindow_)
    {
        unsigned flags = 0;
        if (resizable)
            flags |= SDL_WINDOW_RESIZABLE;
        if (borderless)
            flags |= SDL_WINDOW_BORDERLESS;

        window_ = SDL_CreateWindow(windowTitle_.c_str(), position_.x_, position_.y_, width, height, flags);
    }
    else
        window_ = SDL_CreateWindowFrom(externalWindow_, 0);

    if (!window_)
    {
        URHO3D_LOGERRORF("Could not create window, root cause: '%s'", SDL_GetError());
        return false;
    }

    SDL_GetWindowPosition(window_, &position_.x_, &position_.y_);

    CreateWindowIcon();

    return true;
}

void Graphics::AdjustWindow(int& newWidth, int& newHeight, bool& newFullscreen, bool& newBorderless, int& monitor)
{
    if (!externalWindow_)
    {
        // Keep current window position because it may change in intermediate callbacks
        const IntVector2 oldPosition = position_;
        bool reposition = false;
        bool resizePostponed = false;
        if (!newWidth || !newHeight)
        {
            SDL_MaximizeWindow(window_);
            SDL_GetWindowSize(window_, &newWidth, &newHeight);
        }
        else
        {
            SDL_Rect display_rect;
            SDL_GetDisplayBounds(monitor, &display_rect);

            reposition = newFullscreen || (newBorderless && newWidth >= display_rect.w && newHeight >= display_rect.h);
            if (reposition)
            {
                // Reposition the window on the specified monitor if it's supposed to cover the entire monitor
                SDL_SetWindowPosition(window_, display_rect.x, display_rect.y);
            }

            // Postpone window resize if exiting fullscreen to avoid redundant resolution change
            if (!newFullscreen && screenParams_.fullscreen_)
                resizePostponed = true;
            else
                SDL_SetWindowSize(window_, newWidth, newHeight);
        }

        // Turn off window fullscreen mode so it gets repositioned to the correct monitor
        SDL_SetWindowFullscreen(window_, SDL_FALSE);
        // Hack fix: on SDL 2.0.4 a fullscreen->windowed transition results in a maximized window when the D3D device is reset, so hide before
        if (!newFullscreen) SDL_HideWindow(window_);
        SDL_SetWindowFullscreen(window_, newFullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        SDL_SetWindowBordered(window_, newBorderless ? SDL_FALSE : SDL_TRUE);
        if (!newFullscreen) SDL_ShowWindow(window_);

        // Resize now if was postponed
        if (resizePostponed)
            SDL_SetWindowSize(window_, newWidth, newHeight);

        // Ensure that window keeps its position
        if (!reposition)
            SDL_SetWindowPosition(window_, oldPosition.x_, oldPosition.y_);
        else
            position_ = oldPosition;
    }
    else
    {
        // If external window, must ask its dimensions instead of trying to set them
        SDL_GetWindowSize(window_, &newWidth, &newHeight);
        newFullscreen = false;
    }
}

bool Graphics::CreateDevice(int width, int height)
{
    // Device needs only to be created once
    if (!impl_->device_)
    {
        UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        if (screenParams_.gpuDebug_)
        {
            // Enable the debug layer if requested.
            deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
        }
        ID3D11DeviceContext* deviceContext = nullptr;
        const D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            //D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            //D3D_FEATURE_LEVEL_10_0,
        };
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            deviceFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &impl_->device_,
            nullptr,
            &deviceContext
        );

        if (FAILED(hr))
        {
            URHO3D_SAFE_RELEASE(impl_->device_);
            URHO3D_SAFE_RELEASE(deviceContext);
            URHO3D_LOGD3DERROR("Failed to create D3D11 device", hr);
            return false;
        }

        deviceContext->QueryInterface(IID_ID3D11DeviceContext1, reinterpret_cast<void**>(&impl_->deviceContext_));

        CheckFeatureSupport();
        // Set the flush mode now as the device has been created
        SetFlushGPU(flushGPU_);
    }

    // Check that multisample level is supported
    ea::vector<int> multiSampleLevels = GetMultiSampleLevels();
    if (!multiSampleLevels.contains(screenParams_.multiSample_))
        screenParams_.multiSample_ = 1;

    // Create swap chain. Release old if necessary
    if (impl_->swapChain_)
    {
        impl_->swapChain_->Release();
        impl_->swapChain_ = nullptr;
    }

    IDXGIDevice* dxgiDevice = nullptr;
    if (FAILED(impl_->device_->QueryInterface(IID_PPV_ARGS(&dxgiDevice))))
        return false;
    IDXGIAdapter* dxgiAdapter = nullptr;
    if (FAILED(dxgiDevice->GetParent(IID_PPV_ARGS(&dxgiAdapter))))
    {
        dxgiDevice->Release();
        return false;
    }
    IDXGIFactory2* dxgiFactory = nullptr;
    if (FAILED(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))))
    {
        dxgiAdapter->Release();
        dxgiDevice->Release();
        return false;
    }

#ifndef UWP
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc{};
    swapChainFullScreenDesc.Windowed = TRUE;
    IDXGIOutput* dxgiOutput = nullptr;
    UINT numModes = 0;
    dxgiAdapter->EnumOutputs(screenParams_.monitor_, &dxgiOutput);
    dxgiOutput->GetDisplayModeList(sRGB_ ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, 0);

    // find the best matching refresh rate with the specified resolution
    if (numModes > 0)
    {
        DXGI_MODE_DESC* modes = new DXGI_MODE_DESC[numModes];
        dxgiOutput->GetDisplayModeList(sRGB_ ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, modes);
        unsigned bestMatchingRateIndex = -1;
        unsigned bestError = M_MAX_UNSIGNED;
        for (unsigned i = 0; i < numModes; ++i)
        {
            if (width != modes[i].Width || height != modes[i].Height)
                continue;

            float rate = (float)modes[i].RefreshRate.Numerator / modes[i].RefreshRate.Denominator;
            unsigned error = (unsigned)(Abs(rate - screenParams_.refreshRate_));
            if (error < bestError)
            {
                bestMatchingRateIndex = i;
                bestError = error;
            }
        }
        if (bestMatchingRateIndex != -1)
        {
            swapChainFullScreenDesc.RefreshRate.Numerator = modes[bestMatchingRateIndex].RefreshRate.Numerator;
            swapChainFullScreenDesc.RefreshRate.Denominator = modes[bestMatchingRateIndex].RefreshRate.Denominator;
        }
        delete[] modes;
    }

    dxgiOutput->Release();
#endif

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = (UINT)width;
    swapChainDesc.Height = (UINT)height;
    swapChainDesc.Format = sRGB_ ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SampleDesc.Count = static_cast<UINT>(screenParams_.multiSample_);
    swapChainDesc.SampleDesc.Quality = impl_->GetMultiSampleQuality(swapChainDesc.Format, screenParams_.multiSample_);
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Stereo = false;
#if UWP
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    Windows::UI::Core::CoreWindow^ coreWindow = Windows::UI::Core::CoreWindow::GetForCurrentThread();
    HRESULT hr = dxgiFactory->CreateSwapChainForCoreWindow(impl_->device_, reinterpret_cast<IUnknown*>(coreWindow), &swapChainDesc, nullptr, &impl_->swapChain_);
#else
    swapChainDesc.BufferCount = 1;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(impl_->device_, GetWindowHandle(window_), &swapChainDesc, &swapChainFullScreenDesc, nullptr, &impl_->swapChain_);
    // After creating the swap chain, disable automatic Alt-Enter fullscreen/windowed switching
    // (the application will switch manually if it wants to)
    dxgiFactory->MakeWindowAssociation(GetWindowHandle(window_), DXGI_MWA_NO_ALT_ENTER);
#endif

#ifdef URHO3D_LOGGING
    DXGI_ADAPTER_DESC desc;
    dxgiAdapter->GetDesc(&desc);
    ea::string adapterDesc = WideToMultiByte(desc.Description);
    URHO3D_LOGINFO("Adapter used " + adapterDesc);
#endif

    dxgiFactory->Release();
    dxgiAdapter->Release();
    dxgiDevice->Release();

    if (FAILED(hr))
    {
        URHO3D_SAFE_RELEASE(impl_->swapChain_);
        URHO3D_LOGD3DERROR("Failed to create D3D11 swap chain", hr);
        return false;
    }

    return true;
}

bool Graphics::UpdateSwapChain(int width, int height)
{
    bool success = true;

    ID3D11RenderTargetView* nullView = nullptr;
    impl_->deviceContext_->OMSetRenderTargets(1, &nullView, nullptr);
    if (impl_->defaultRenderTargetView_)
    {
        impl_->defaultRenderTargetView_->Release();
        impl_->defaultRenderTargetView_ = nullptr;
    }
    if (impl_->defaultDepthStencilView_)
    {
        impl_->defaultDepthStencilView_->Release();
        impl_->defaultDepthStencilView_ = nullptr;
    }
    if (impl_->defaultDepthTexture_)
    {
        impl_->defaultDepthTexture_->Release();
        impl_->defaultDepthTexture_ = nullptr;
    }
    if (impl_->resolveTexture_)
    {
        impl_->resolveTexture_->Release();
        impl_->resolveTexture_ = nullptr;
    }

    impl_->depthStencilView_ = nullptr;
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        impl_->renderTargetViews_[i] = nullptr;
    impl_->renderTargetsDirty_ = true;

#if UWP
    int bufferCount = 2;
#else
    int bufferCount = 1;
#endif
    impl_->swapChain_->ResizeBuffers(bufferCount, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

    // Create default rendertarget view representing the backbuffer
    ID3D11Texture2D* backbufferTexture;
    HRESULT hr = impl_->swapChain_->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backbufferTexture);
    if (FAILED(hr))
    {
        URHO3D_SAFE_RELEASE(backbufferTexture);
        URHO3D_LOGD3DERROR("Failed to get backbuffer texture", hr);
        success = false;
    }
    else
    {
        hr = impl_->device_->CreateRenderTargetView(backbufferTexture, nullptr, &impl_->defaultRenderTargetView_);
        backbufferTexture->Release();
        if (FAILED(hr))
        {
            URHO3D_SAFE_RELEASE(impl_->defaultRenderTargetView_);
            URHO3D_LOGD3DERROR("Failed to create backbuffer rendertarget view", hr);
            success = false;
        }
    }

    // Create default depth-stencil texture and view
    D3D11_TEXTURE2D_DESC depthDesc;
    memset(&depthDesc, 0, sizeof depthDesc);
    depthDesc.Width = (UINT)width;
    depthDesc.Height = (UINT)height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = static_cast<UINT>(screenParams_.multiSample_);
    depthDesc.SampleDesc.Quality = impl_->GetMultiSampleQuality(depthDesc.Format, screenParams_.multiSample_);
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;
    hr = impl_->device_->CreateTexture2D(&depthDesc, nullptr, &impl_->defaultDepthTexture_);
    if (FAILED(hr))
    {
        URHO3D_SAFE_RELEASE(impl_->defaultDepthTexture_);
        URHO3D_LOGD3DERROR("Failed to create backbuffer depth-stencil texture", hr);
        success = false;
    }
    else
    {
        hr = impl_->device_->CreateDepthStencilView(impl_->defaultDepthTexture_, nullptr, &impl_->defaultDepthStencilView_);
        if (FAILED(hr))
        {
            URHO3D_SAFE_RELEASE(impl_->defaultDepthStencilView_);
            URHO3D_LOGD3DERROR("Failed to create backbuffer depth-stencil view", hr);
            success = false;
        }
    }

    // Update internally held backbuffer size
    width_ = width;
    height_ = height;

    ResetRenderTargets();
    return success;
}

void Graphics::CheckFeatureSupport()
{
    anisotropySupport_ = true;
    dxtTextureSupport_ = true;
    lightPrepassSupport_ = true;
    deferredSupport_ = true;
    hardwareShadowSupport_ = true;
    instancingSupport_ = true;
    shadowMapFormat_ = DXGI_FORMAT_R16_TYPELESS;
    hiresShadowMapFormat_ = DXGI_FORMAT_R32_TYPELESS;
    dummyColorFormat_ = DXGI_FORMAT_UNKNOWN;
    sRGBSupport_ = true;
    sRGBWriteSupport_ = true;

    caps.maxVertexShaderUniforms_ = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT;
    caps.maxPixelShaderUniforms_ = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT;
    caps.constantBuffersSupported_ = true;
    caps.constantBufferOffsetAlignment_ = 256;
    caps.maxTextureSize_ = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    caps.maxRenderTargetSize_ = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    caps.maxNumRenderTargets_ = 8;
}

void Graphics::ResetCachedState()
{
    for (auto& constantBuffer : constantBuffers_)
        constantBuffer = {};

    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        vertexBuffers_[i] = nullptr;
        impl_->vertexBuffers_[i] = nullptr;
        impl_->vertexSizes_[i] = 0;
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
    impl_->depthStencilView_ = nullptr;
    viewport_ = IntRect(0, 0, width_, height_);

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
    if (impl_->renderTargetsDirty_)
    {
        impl_->depthStencilView_ =
            (depthStencil_ && depthStencil_->GetUsage() == TEXTURE_DEPTHSTENCIL) ?
                (ID3D11DepthStencilView*)depthStencil_->GetRenderTargetView() : impl_->defaultDepthStencilView_;

        // If possible, bind a read-only depth stencil view to allow reading depth in shader
        if (!depthWrite_ && depthStencil_ && depthStencil_->GetReadOnlyView())
            impl_->depthStencilView_ = (ID3D11DepthStencilView*)depthStencil_->GetReadOnlyView();

        for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
            impl_->renderTargetViews_[i] =
                (renderTargets_[i] && renderTargets_[i]->GetUsage() == TEXTURE_RENDERTARGET) ?
                    (ID3D11RenderTargetView*)renderTargets_[i]->GetRenderTargetView() : nullptr;
        // If rendertarget 0 is null and not doing depth-only rendering, render to the backbuffer
        // Special case: if rendertarget 0 is null and depth stencil has same size as backbuffer, assume the intention is to do
        // backbuffer rendering with a custom depth stencil
        if (!renderTargets_[0] &&
            (!depthStencil_ || (depthStencil_ && depthStencil_->GetWidth() == width_ && depthStencil_->GetHeight() == height_)))
            impl_->renderTargetViews_[0] = impl_->defaultRenderTargetView_;

        impl_->deviceContext_->OMSetRenderTargets(MAX_RENDERTARGETS, &impl_->renderTargetViews_[0], impl_->depthStencilView_);
        impl_->renderTargetsDirty_ = false;
    }

    if (impl_->texturesDirty_ && impl_->firstDirtyTexture_ < M_MAX_UNSIGNED)
    {
        // Set also VS textures to enable vertex texture fetch to work the same way as on OpenGL
        impl_->deviceContext_->VSSetShaderResources(impl_->firstDirtyTexture_, impl_->lastDirtyTexture_ - impl_->firstDirtyTexture_ + 1,
            &impl_->shaderResourceViews_[impl_->firstDirtyTexture_]);
        impl_->deviceContext_->VSSetSamplers(impl_->firstDirtyTexture_, impl_->lastDirtyTexture_ - impl_->firstDirtyTexture_ + 1,
            &impl_->samplers_[impl_->firstDirtyTexture_]);
        impl_->deviceContext_->PSSetShaderResources(impl_->firstDirtyTexture_, impl_->lastDirtyTexture_ - impl_->firstDirtyTexture_ + 1,
            &impl_->shaderResourceViews_[impl_->firstDirtyTexture_]);
        impl_->deviceContext_->PSSetSamplers(impl_->firstDirtyTexture_, impl_->lastDirtyTexture_ - impl_->firstDirtyTexture_ + 1,
            &impl_->samplers_[impl_->firstDirtyTexture_]);

        impl_->firstDirtyTexture_ = impl_->lastDirtyTexture_ = M_MAX_UNSIGNED;
        impl_->texturesDirty_ = false;
    }

    if (impl_->vertexDeclarationDirty_ && vertexShader_ && vertexShader_->GetByteCode().size())
    {
        if (impl_->firstDirtyVB_ < M_MAX_UNSIGNED)
        {
            impl_->deviceContext_->IASetVertexBuffers(impl_->firstDirtyVB_, impl_->lastDirtyVB_ - impl_->firstDirtyVB_ + 1,
                &impl_->vertexBuffers_[impl_->firstDirtyVB_], &impl_->vertexSizes_[impl_->firstDirtyVB_], &impl_->vertexOffsets_[impl_->firstDirtyVB_]);

            impl_->firstDirtyVB_ = impl_->lastDirtyVB_ = M_MAX_UNSIGNED;
        }

        unsigned long long newVertexDeclarationHash = 0;
        for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
        {
            if (vertexBuffers_[i])
                newVertexDeclarationHash |= vertexBuffers_[i]->GetBufferHash(i);
        }
        // Do not create input layout if no vertex buffers / elements
        if (newVertexDeclarationHash)
        {
            /// \todo Using a 64bit total hash for vertex shader and vertex buffer elements hash may not guarantee uniqueness
            newVertexDeclarationHash += vertexShader_->GetElementHash();
            if (newVertexDeclarationHash != vertexDeclarationHash_)
            {
                auto i = impl_->vertexDeclarations_.find(newVertexDeclarationHash);
                if (i == impl_->vertexDeclarations_.end())
                {
                    SharedPtr<VertexDeclaration> newVertexDeclaration(new VertexDeclaration(this, vertexShader_, vertexBuffers_));
                    i = impl_->vertexDeclarations_.insert(ea::make_pair(newVertexDeclarationHash, newVertexDeclaration)).first;
                }
                impl_->deviceContext_->IASetInputLayout((ID3D11InputLayout*)i->second->GetInputLayout());
                vertexDeclarationHash_ = newVertexDeclarationHash;
            }
        }

        impl_->vertexDeclarationDirty_ = false;
    }

    if (impl_->blendStateDirty_)
    {
        unsigned newBlendStateHash = (unsigned)((colorWrite_ ? 1 : 0) | (alphaToCoverage_ ? 2 : 0) | (blendMode_ << 2));
        if (newBlendStateHash != impl_->blendStateHash_)
        {
            auto i = impl_->blendStates_.find(newBlendStateHash);
            if (i == impl_->blendStates_.end())
            {
                URHO3D_PROFILE("CreateBlendState");

                D3D11_BLEND_DESC stateDesc;
                memset(&stateDesc, 0, sizeof stateDesc);
                stateDesc.AlphaToCoverageEnable = alphaToCoverage_ ? TRUE : FALSE;
                stateDesc.IndependentBlendEnable = false;
                stateDesc.RenderTarget[0].BlendEnable = d3dBlendEnable[blendMode_];
                stateDesc.RenderTarget[0].SrcBlend = d3dSrcBlend[blendMode_];
                stateDesc.RenderTarget[0].DestBlend = d3dDestBlend[blendMode_];
                stateDesc.RenderTarget[0].BlendOp = d3dBlendOp[blendMode_];
                stateDesc.RenderTarget[0].SrcBlendAlpha = d3dSrcBlend[blendMode_];
                stateDesc.RenderTarget[0].DestBlendAlpha = d3dDestBlend[blendMode_];
                stateDesc.RenderTarget[0].BlendOpAlpha = d3dBlendOp[blendMode_];
                stateDesc.RenderTarget[0].RenderTargetWriteMask = colorWrite_ ? D3D11_COLOR_WRITE_ENABLE_ALL : 0x0;

                ID3D11BlendState* newBlendState = nullptr;
                HRESULT hr = impl_->device_->CreateBlendState(&stateDesc, &newBlendState);
                if (FAILED(hr))
                {
                    URHO3D_SAFE_RELEASE(newBlendState);
                    URHO3D_LOGD3DERROR("Failed to create blend state", hr);
                }

                i = impl_->blendStates_.insert(ea::make_pair(newBlendStateHash, newBlendState)).first;
            }

            impl_->deviceContext_->OMSetBlendState(i->second, nullptr, M_MAX_UNSIGNED);
            impl_->blendStateHash_ = newBlendStateHash;
        }

        impl_->blendStateDirty_ = false;
    }

    if (impl_->depthStateDirty_)
    {
        unsigned newDepthStateHash =
            (depthWrite_ ? 1 : 0) | (stencilTest_ ? 2 : 0) | (depthTestMode_ << 2) | ((stencilCompareMask_ & 0xff) << 5) |
            ((stencilWriteMask_ & 0xff) << 13) | (stencilTestMode_ << 21) |
            ((stencilFail_ + stencilZFail_ * 5 + stencilPass_ * 25) << 24);
        if (newDepthStateHash != impl_->depthStateHash_ || impl_->stencilRefDirty_)
        {
            auto i = impl_->depthStates_.find(newDepthStateHash);
            if (i == impl_->depthStates_.end())
            {
                URHO3D_PROFILE("CreateDepthState");

                D3D11_DEPTH_STENCIL_DESC stateDesc;
                memset(&stateDesc, 0, sizeof stateDesc);
                stateDesc.DepthEnable = TRUE;
                stateDesc.DepthWriteMask = depthWrite_ ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
                stateDesc.DepthFunc = d3dCmpFunc[depthTestMode_];
                stateDesc.StencilEnable = stencilTest_ ? TRUE : FALSE;
                stateDesc.StencilReadMask = (unsigned char)stencilCompareMask_;
                stateDesc.StencilWriteMask = (unsigned char)stencilWriteMask_;
                stateDesc.FrontFace.StencilFailOp = d3dStencilOp[stencilFail_];
                stateDesc.FrontFace.StencilDepthFailOp = d3dStencilOp[stencilZFail_];
                stateDesc.FrontFace.StencilPassOp = d3dStencilOp[stencilPass_];
                stateDesc.FrontFace.StencilFunc = d3dCmpFunc[stencilTestMode_];
                stateDesc.BackFace.StencilFailOp = d3dStencilOp[stencilFail_];
                stateDesc.BackFace.StencilDepthFailOp = d3dStencilOp[stencilZFail_];
                stateDesc.BackFace.StencilPassOp = d3dStencilOp[stencilPass_];
                stateDesc.BackFace.StencilFunc = d3dCmpFunc[stencilTestMode_];

                ID3D11DepthStencilState* newDepthState = nullptr;
                HRESULT hr = impl_->device_->CreateDepthStencilState(&stateDesc, &newDepthState);
                if (FAILED(hr))
                {
                    URHO3D_SAFE_RELEASE(newDepthState);
                    URHO3D_LOGD3DERROR("Failed to create depth state", hr);
                }

                i = impl_->depthStates_.insert(ea::make_pair(newDepthStateHash, newDepthState)).first;
            }

            impl_->deviceContext_->OMSetDepthStencilState(i->second, stencilRef_);
            impl_->depthStateHash_ = newDepthStateHash;
        }

        impl_->depthStateDirty_ = false;
        impl_->stencilRefDirty_ = false;
    }

    if (impl_->rasterizerStateDirty_)
    {
        unsigned depthBits = 24;
        if (depthStencil_ && depthStencil_->GetParentTexture()->GetFormat() == DXGI_FORMAT_R16_TYPELESS)
            depthBits = 16;
        int scaledDepthBias = (int)(constantDepthBias_ * (1 << depthBits));

        unsigned newRasterizerStateHash =
            (scissorTest_ ? 1 : 0) | (lineAntiAlias_ ? 2 : 0) | (fillMode_ << 2) | (cullMode_ << 4) |
            ((scaledDepthBias & 0x1fff) << 6) | (((int)(slopeScaledDepthBias_ * 100.0f) & 0x1fff) << 19);
        if (newRasterizerStateHash != impl_->rasterizerStateHash_)
        {
            auto i = impl_->rasterizerStates_.find(newRasterizerStateHash);
            if (i == impl_->rasterizerStates_.end())
            {
                URHO3D_PROFILE("CreateRasterizerState");

                D3D11_RASTERIZER_DESC stateDesc;
                memset(&stateDesc, 0, sizeof stateDesc);
                stateDesc.FillMode = d3dFillMode[fillMode_];
                stateDesc.CullMode = d3dCullMode[cullMode_];
                stateDesc.FrontCounterClockwise = FALSE;
                stateDesc.DepthBias = scaledDepthBias;
                stateDesc.DepthBiasClamp = M_INFINITY;
                stateDesc.SlopeScaledDepthBias = slopeScaledDepthBias_;
                stateDesc.DepthClipEnable = TRUE;
                stateDesc.ScissorEnable = scissorTest_ ? TRUE : FALSE;
                stateDesc.MultisampleEnable = lineAntiAlias_ ? FALSE : TRUE;
                stateDesc.AntialiasedLineEnable = lineAntiAlias_ ? TRUE : FALSE;

                ID3D11RasterizerState* newRasterizerState = nullptr;
                HRESULT hr = impl_->device_->CreateRasterizerState(&stateDesc, &newRasterizerState);
                if (FAILED(hr))
                {
                    URHO3D_SAFE_RELEASE(newRasterizerState);
                    URHO3D_LOGD3DERROR("Failed to create rasterizer state", hr);
                }

                i = impl_->rasterizerStates_.insert(ea::make_pair(newRasterizerStateHash, newRasterizerState)).first;
            }

            impl_->deviceContext_->RSSetState(i->second);
            impl_->rasterizerStateHash_ = newRasterizerStateHash;
        }

        impl_->rasterizerStateDirty_ = false;
    }

    if (impl_->scissorRectDirty_)
    {
        D3D11_RECT d3dRect;
        d3dRect.left = scissorRect_.left_;
        d3dRect.top = scissorRect_.top_;
        d3dRect.right = scissorRect_.right_;
        d3dRect.bottom = scissorRect_.bottom_;
        impl_->deviceContext_->RSSetScissorRects(1, &d3dRect);
        impl_->scissorRectDirty_ = false;
    }
}

void Graphics::CreateResolveTexture()
{
    if (impl_->resolveTexture_)
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
    }
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
    textureUnits_["FaceSelectCubeMap"] = TU_FACESELECT;
    textureUnits_["IndirectionCubeMap"] = TU_INDIRECTION;
    textureUnits_["VolumeMap"] = TU_VOLUMEMAP;
    textureUnits_["ZoneCubeMap"] = TU_ZONE;
    textureUnits_["ZoneVolumeMap"] = TU_ZONE;
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

}
