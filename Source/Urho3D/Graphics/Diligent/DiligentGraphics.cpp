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
#include "../../RenderAPI/PipelineState.h"
#include "../../Graphics/Renderer.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/ShaderPrecache.h"
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

bool Graphics::gl3Support = false;

Graphics::Graphics(Context* context)
    : Object(context)
    , impl_(new GraphicsImpl())
    , position_(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED)
    , shaderPath_("Shaders/HLSL/")
    , shaderExtension_(".hlsl")
    , apiName_("Diligent")
{
    ResetCachedState();

    // TODO(diligent): Revisit this
    //SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
    context_->RequireSDL(SDL_INIT_VIDEO);

    Diligent::SetDebugMessageCallback(&HandleDbgMessageCallbacks);
}

Graphics::~Graphics()
{
    context_->RemoveSubsystem<RenderDevice>();

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

        renderDevice_->OnDeviceLost.Subscribe(this, [this]()
        {
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

        apiName_ = ToString(GetRenderBackend());
    }
    else
    {
        renderDevice_->UpdateWindowSettings(windowSettings);
    }

    window_ = renderDevice_->GetSDLWindow();
    impl_->device_ = renderDevice_->GetRenderDevice();
    impl_->deviceContext_ = renderDevice_->GetImmediateContext();
    impl_->swapChain_ = renderDevice_->GetSwapChain();

    CheckFeatureSupport();

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

    if (!GetExternalWindow())
    {
        // To prevent a loop of endless device loss and flicker, do not attempt to render when in fullscreen
        // and the window is minimized
        if (GetFullscreen() && (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED))
            return false;
    }

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

bool Graphics::ResolveToTexture(Texture2D* destination, const IntRect& viewport)
{
    URHO3D_ASSERT(false);
    return false;
}

bool Graphics::ResolveToTexture(Texture2D* texture)
{
    URHO3D_ASSERT(false);
    return false;
}

bool Graphics::ResolveToTexture(TextureCube* texture)
{
    URHO3D_ASSERT(false);
    return false;
}

void Graphics::Draw(PrimitiveType type, unsigned vertexStart, unsigned vertexCount)
{
    URHO3D_ASSERT(false);
}

void Graphics::Draw(
    PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount)
{
    URHO3D_ASSERT(false);
}

void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex,
    unsigned minVertex, unsigned vertexCount)
{
    URHO3D_ASSERT(false);
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex,
    unsigned vertexCount, unsigned instanceCount)
{
    URHO3D_ASSERT(false);
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex,
    unsigned minVertex, unsigned vertexCount, unsigned instanceCount)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetVertexBuffer(VertexBuffer* buffer)
{
    URHO3D_ASSERT(false);
}

bool Graphics::SetVertexBuffers(const ea::vector<VertexBuffer*>& buffers, unsigned instanceOffset)
{
    URHO3D_ASSERT(false);
    return false;
}

bool Graphics::SetVertexBuffers(const ea::vector<SharedPtr<VertexBuffer>>& buffers, unsigned instanceOffset)
{
    URHO3D_ASSERT(false);
    return false;
}

void Graphics::SetIndexBuffer(IndexBuffer* buffer)
{
    URHO3D_ASSERT(false);
}

ShaderProgramReflection* Graphics::GetShaderProgramLayout(ShaderVariation* vs, ShaderVariation* ps)
{
    URHO3D_ASSERT(false);
    return nullptr;
}

void Graphics::SetShaders(ShaderVariation* vs, ShaderVariation* ps)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderConstantBuffers(ea::span<const ConstantBufferRange> constantBuffers)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, const float data[], unsigned count)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, float value)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, int value)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, bool value)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, const Color& color)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, const Vector2& vector)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3& matrix)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, const Vector3& vector)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, const Matrix4& matrix)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, const Vector4& vector)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3x4& matrix)
{
    URHO3D_ASSERT(false);
}

bool Graphics::NeedParameterUpdate(ShaderParameterGroup group, const void* source)
{
    URHO3D_ASSERT(false);
    return false;
}

bool Graphics::HasShaderParameter(StringHash param)
{
    URHO3D_ASSERT(false);
    return false;
}

bool Graphics::HasTextureUnit(TextureUnit unit)
{
    URHO3D_ASSERT(false);
    return false;
}

void Graphics::ClearParameterSource(ShaderParameterGroup group)
{
    URHO3D_ASSERT(false);
}

void Graphics::ClearParameterSources()
{
    URHO3D_ASSERT(false);
}

void Graphics::ClearTransformSources()
{
    URHO3D_ASSERT(false);
}

void Graphics::SetTexture(unsigned index, Texture* texture)
{
    URHO3D_ASSERT(false);
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
        {
            renderDevice_ = nullptr;
            context_->RemoveSubsystem<RenderDevice>();
        }
    }
}

void Graphics::SetTextureParametersDirty()
{
    MutexLock lock(gpuObjectMutex_);

    for (auto i = gpuObjects_.begin(); i != gpuObjects_.end(); ++i)
    {
        Texture* texture = dynamic_cast<Texture*>(*i);
        if (texture)
            ;//texture->SetParametersDirty();
    }
}

void Graphics::ResetRenderTargets()
{
    URHO3D_ASSERT(renderDevice_);

    RenderContext* renderContext = renderDevice_->GetRenderContext();
    renderContext->SetSwapChainRenderTargets();
    renderContext->SetFullViewport();
}

void Graphics::ResetRenderTarget(unsigned index)
{
    URHO3D_ASSERT(false);
}

void Graphics::ResetDepthStencil()
{
    URHO3D_ASSERT(false);
}

void Graphics::SetRenderTarget(unsigned index, RenderSurface* renderTarget)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetRenderTarget(unsigned index, Texture2D* texture)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetDepthStencil(RenderSurface* depthStencil)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetDepthStencil(Texture2D* texture)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetViewport(const IntRect& rect)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetBlendMode(BlendMode mode, bool alphaToCoverage)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetColorWrite(bool enable)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetCullMode(CullMode mode)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetDepthBias(float constantBias, float slopeScaledBias)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetDepthTest(CompareMode mode)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetDepthWrite(bool enable)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetFillMode(FillMode mode)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetLineAntiAlias(bool enable)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetScissorTest(bool enable, const Rect& rect, bool borderInclusive)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetScissorTest(bool enable, const IntRect& rect)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetStencilTest(bool enable, CompareMode mode, StencilOp pass, StencilOp fail, StencilOp zFail,
    unsigned stencilRef, unsigned compareMask, unsigned writeMask)
{
    URHO3D_ASSERT(false);
}

void Graphics::SetClipPlane(bool enable, const Plane& clipPlane, const Matrix3x4& view, const Matrix4& projection)
{
    URHO3D_ASSERT(false);
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

VertexBuffer* Graphics::GetVertexBuffer(unsigned index) const
{
    URHO3D_ASSERT(false);
    return nullptr;
}

RenderSurface* Graphics::GetRenderTarget(unsigned index) const
{
    URHO3D_ASSERT(false);
    return nullptr;
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

RenderBackend Graphics::GetRenderBackend() const
{
    return renderDevice_ ? renderDevice_->GetBackend() : RenderBackend::OpenGL;
}

TextureFormat Graphics::GetAlphaFormat()
{
    return TextureFormat::TEX_FORMAT_R8_UNORM;
}

TextureFormat Graphics::GetLuminanceFormat()
{
    return TextureFormat::TEX_FORMAT_R8_UNORM;
}

TextureFormat Graphics::GetLuminanceAlphaFormat()
{
    return TextureFormat::TEX_FORMAT_RG8_UNORM;
}

TextureFormat Graphics::GetRGBFormat()
{
    return TextureFormat::TEX_FORMAT_RGBA8_UNORM;
}

TextureFormat Graphics::GetRGBAFormat()
{
    return TextureFormat::TEX_FORMAT_RGBA8_UNORM;
}

TextureFormat Graphics::GetRGBA16Format()
{
    return TextureFormat::TEX_FORMAT_RGBA16_UNORM;
}

TextureFormat Graphics::GetRGBAFloat16Format()
{
    return TextureFormat::TEX_FORMAT_RGBA16_FLOAT;
}

TextureFormat Graphics::GetRGBAFloat32Format()
{
    return TextureFormat::TEX_FORMAT_RGBA32_FLOAT;
}

TextureFormat Graphics::GetRG16Format()
{
    return TextureFormat::TEX_FORMAT_RG16_UNORM;
}

TextureFormat Graphics::GetRGFloat16Format()
{
    return TextureFormat::TEX_FORMAT_RG16_FLOAT;
}

TextureFormat Graphics::GetRGFloat32Format()
{
    return TextureFormat::TEX_FORMAT_RG32_FLOAT;
}

TextureFormat Graphics::GetFloat16Format()
{
    return TextureFormat::TEX_FORMAT_R16_FLOAT;
}

TextureFormat Graphics::GetFloat32Format()
{
    return TextureFormat::TEX_FORMAT_R32_FLOAT;
}

TextureFormat Graphics::GetLinearDepthFormat()
{
    return TextureFormat::TEX_FORMAT_D32_FLOAT;
}

TextureFormat Graphics::GetDepthStencilFormat()
{
    return TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT;
}

TextureFormat Graphics::GetReadableDepthFormat()
{
    return TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT;
}

TextureFormat Graphics::GetReadableDepthStencilFormat()
{
    return TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT;
}

TextureFormat Graphics::GetFormat(const ea::string& formatName)
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
    /// User-specified number of bones
    if (maxBonesHWSkinned)
        return maxBonesHWSkinned;

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

    caps = renderDevice_->GetCaps();

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

const IntRect& Graphics::GetViewport() const
{
    URHO3D_ASSERT(false);
    return IntRect::ZERO;
}

} // namespace Urho3D
