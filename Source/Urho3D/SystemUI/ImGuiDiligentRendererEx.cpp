// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/SystemUI/ImGuiDiligentRendererEx.h"

#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderAPI/RenderScope.h"
#include "Urho3D/SystemUI/ImGui.h"

#include <Diligent/Common/interface/Cast.hpp>
#include <Diligent/Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp>
#if GL_SUPPORTED || GLES_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/DeviceContextGL.h>
    #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/SwapChainGL.h>
#endif

#include <EASTL/optional.h>

#include <SDL.h>

namespace Urho3D
{

namespace
{

// Default sizes are enough for default debug HUD.
static const unsigned initialVertexBufferSize = 2500;
static const unsigned initialIndexBufferSize = 5000;

ImGuiDiligentRendererEx* GetBackendData()
{
    return reinterpret_cast<ImGuiDiligentRendererEx*>(ImGui::GetIO().BackendRendererUserData);
}

struct ViewportRendererData
{
    ea::optional<ImVec2> postponedResize_;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain_;
};

ViewportRendererData* GetViewportData(ImGuiViewport* viewport)
{
    URHO3D_ASSERT(viewport->RendererUserData);
    return static_cast<ViewportRendererData*>(viewport->RendererUserData);
}

bool IsSRGBTextureFormat(TextureFormat format)
{
    return format == TextureFormat::TEX_FORMAT_RGBA8_UNORM_SRGB || format == TextureFormat::TEX_FORMAT_BGRA8_UNORM_SRGB;
}

SharedPtr<PipelineState> CreateRenderPipeline(
    RenderDevice* renderDevice, TextureFormat colorBufferFormat, TextureFormat depthBufferFormat, unsigned multiSample)
{
    auto pipelineStateCache = renderDevice->GetContext()->GetSubsystem<PipelineStateCache>();
    auto graphics = renderDevice->GetContext()->GetSubsystem<Graphics>();

    GraphicsPipelineStateDesc desc;
    desc.debugName_ = Format("ImGUI Render Pipeline (Color: {}, Depth: {})",
        Diligent::GetTextureFormatAttribs(colorBufferFormat).Name,
        Diligent::GetTextureFormatAttribs(depthBufferFormat).Name);

    desc.output_.numRenderTargets_ = 1;
    desc.output_.renderTargetFormats_[0] = colorBufferFormat;
    desc.output_.depthStencilFormat_ = depthBufferFormat;
    desc.output_.multiSample_ = multiSample;

    desc.inputLayout_.size_ = 3;
    desc.inputLayout_.elements_[0].bufferStride_ = sizeof(ImDrawVert);
    desc.inputLayout_.elements_[0].elementSemantic_ = SEM_POSITION;
    desc.inputLayout_.elements_[0].elementType_ = TYPE_VECTOR2;
    desc.inputLayout_.elements_[0].elementOffset_ = 0;
    desc.inputLayout_.elements_[1].bufferStride_ = sizeof(ImDrawVert);
    desc.inputLayout_.elements_[1].elementSemantic_ = SEM_TEXCOORD;
    desc.inputLayout_.elements_[1].elementType_ = TYPE_VECTOR2;
    desc.inputLayout_.elements_[1].elementOffset_ = sizeof(ImVec2);
    desc.inputLayout_.elements_[2].bufferStride_ = sizeof(ImDrawVert);
    desc.inputLayout_.elements_[2].elementSemantic_ = SEM_COLOR;
    desc.inputLayout_.elements_[2].elementType_ = TYPE_UBYTE4_NORM;
    desc.inputLayout_.elements_[2].elementOffset_ = sizeof(ImVec2) + sizeof(ImVec2);
    desc.colorWriteEnabled_ = true;

    ea::string shaderDefines;
    if (IsSRGBTextureFormat(colorBufferFormat))
        shaderDefines += "URHO3D_LINEAR_OUTPUT ";

    desc.vertexShader_ = graphics->GetShader(VS, "v2/X_ImGui", shaderDefines);
    desc.pixelShader_ = graphics->GetShader(PS, "v2/X_ImGui", shaderDefines);

    desc.primitiveType_ = TRIANGLE_LIST;
    desc.depthCompareFunction_ = CMP_ALWAYS;
    desc.depthWriteEnabled_ = false;
    desc.blendMode_ = BLEND_ALPHA;

    desc.samplers_.Add("Texture", SamplerStateDesc::Bilinear(ADDRESS_WRAP));

    return pipelineStateCache->GetGraphicsPipelineState(desc);
}

} // namespace

ImGuiDiligentRendererEx::ImGuiDiligentRendererEx(RenderDevice* renderDevice)
    : Diligent::ImGuiDiligentRenderer(renderDevice->GetRenderDevice(),
        renderDevice->GetSwapChain()->GetDesc().ColorBufferFormat,
        renderDevice->GetSwapChain()->GetDesc().DepthBufferFormat, initialVertexBufferSize, initialIndexBufferSize)
    , renderDevice_{renderDevice}
{
    ImGuiIO& IO = ImGui::GetIO();
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
    createPlatformWindow_ = platformIO.Platform_CreateWindow;

#if URHO3D_PLATFORM_WINDOWS || URHO3D_PLATFORM_LINUX || URHO3D_PLATFORM_MACOS
    const RenderBackend renderBackend = renderDevice->GetBackend();
    IO.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    // clang-format off
    IO.BackendRendererUserData = this;
    platformIO.Platform_CreateWindow = [](ImGuiViewport* viewport) { GetBackendData()->CreatePlatformWindow(viewport); };
    platformIO.Renderer_CreateWindow = [](ImGuiViewport* viewport) { GetBackendData()->CreateRendererWindow(viewport); };
    platformIO.Renderer_DestroyWindow = [](ImGuiViewport* viewport) { GetBackendData()->DestroyRendererWindow(viewport); };
    platformIO.Renderer_SetWindowSize = [](ImGuiViewport* viewport, ImVec2 size) { GetBackendData()->SetWindowSize(viewport, size); };
    platformIO.Renderer_RenderWindow = [](ImGuiViewport* viewport, void* renderArg) { GetBackendData()->RenderWindow(viewport, renderArg); };
    platformIO.Renderer_SwapBuffers = [](ImGuiViewport* viewport, void* renderArg) { GetBackendData()->SwapBuffers(viewport, renderArg); };
    // clang-format on
#endif

    Diligent::ISwapChain* swapChain = renderDevice->GetSwapChain();
    const Diligent::SwapChainDesc& swapChainDesc = swapChain->GetDesc();
    const unsigned multiSample = swapChain->GetCurrentBackBufferRTV()->GetTexture()->GetDesc().SampleCount;
    primaryPipelineState_ = CreateRenderPipeline(
        renderDevice, swapChainDesc.ColorBufferFormat, swapChainDesc.DepthBufferFormat, multiSample);
    secondaryPipelineState_ =
        CreateRenderPipeline(renderDevice, swapChainDesc.ColorBufferFormat, TextureFormat::TEX_FORMAT_UNKNOWN, 1);
}

ImGuiDiligentRendererEx::~ImGuiDiligentRendererEx()
{
#if URHO3D_PLATFORM_WINDOWS || URHO3D_PLATFORM_LINUX || URHO3D_PLATFORM_MACOS
    const auto viewports = ea::move(viewports_);
    for (ImGuiViewport* viewport : viewports)
        DestroyRendererWindow(viewport);

    ImGuiIO& IO = ImGui::GetIO();
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

    IO.BackendRendererUserData = nullptr;
    platformIO.Renderer_CreateWindow = nullptr;
    platformIO.Renderer_DestroyWindow = nullptr;
    platformIO.Renderer_SetWindowSize = nullptr;
    platformIO.Renderer_RenderWindow = nullptr;
    platformIO.Renderer_SwapBuffers = nullptr;
    platformIO.Platform_CreateWindow = createPlatformWindow_;
#endif
}

void ImGuiDiligentRendererEx::NewFrame()
{
    const Diligent::SwapChainDesc& swapChainDesc = renderDevice_->GetSwapChain()->GetDesc();
    Diligent::ImGuiDiligentRenderer::NewFrame(swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.PreTransform);
}

void ImGuiDiligentRendererEx::RenderDrawData(ImDrawData* drawData)
{
    const RenderScope renderScope(renderDevice_->GetRenderContext(), "ImGUI: Render main viewport");

    RenderDrawDataWith(drawData, primaryPipelineState_);
}

void ImGuiDiligentRendererEx::RenderSecondaryWindows()
{
    const RenderScope renderScope(renderDevice_->GetRenderContext(), "ImGUI: Render secondary viewport");

    const bool isOpenGL = renderDevice_->GetBackend() == RenderBackend::OpenGL;
    isCachedStateInvalid_ = false;

#if GL_SUPPORTED || GLES_SUPPORTED
    SDL_Window* backupCurrentWindow = isOpenGL ? SDL_GL_GetCurrentWindow() : nullptr;
    SDL_GLContext backupCurrentContext = isOpenGL ? SDL_GL_GetCurrentContext() : nullptr;
#endif

    ui::UpdatePlatformWindows();
    ui::RenderPlatformWindowsDefault();

#if GL_SUPPORTED || GLES_SUPPORTED
    // On OpenGL, invalidate cached context state after all secondary windows rendered
    if (isCachedStateInvalid_ && isOpenGL)
    {
        SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);

        auto deviceContextGL = ClassPtrCast<Diligent::IDeviceContextGL>(renderDevice_->GetImmediateContext());
        deviceContextGL->InvalidateState();
    }
#endif
}

void ImGuiDiligentRendererEx::RenderDrawDataWith(ImDrawData* drawData, PipelineState* pipelineState)
{
    pipelineState->Restore();
    if (!pipelineState->IsValid())
        return;

    Diligent::IShaderResourceBinding* srb = pipelineState->GetShaderResourceBinding();
    Diligent::IShaderResourceVariable* textureVar = srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "sTexture");
    Diligent::IShaderResourceVariable* constantsVar = srb->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "Camera");
    Diligent::ImGuiDiligentRenderer::RenderDrawData(
        renderDevice_->GetImmediateContext(), drawData, pipelineState->GetHandle(), srb, textureVar, constantsVar);
}

#if URHO3D_PLATFORM_WINDOWS || URHO3D_PLATFORM_LINUX || URHO3D_PLATFORM_MACOS
void ImGuiDiligentRendererEx::CreatePlatformWindow(ImGuiViewport* viewport)
{
    const bool isMetalBackend = IsMetalBackend(renderDevice_->GetBackend());
    // On Metal backend, we need to use implicit SDL_WINDOW_METAL flag
    // because ImGui_ImplSDL2_CreateWindow does not set it.
    if (isMetalBackend)
        SDL_SetHint(SDL_HINT_VIDEO_EXTERNAL_CONTEXT, "0");
    createPlatformWindow_(viewport);
    if (isMetalBackend)
        SDL_SetHint(SDL_HINT_VIDEO_EXTERNAL_CONTEXT, "1");
}

void ImGuiDiligentRendererEx::CreateRendererWindow(ImGuiViewport* viewport)
{
    auto userData = new ViewportRendererData{};
    viewport->RendererUserData = userData;

    // Postpone SwapChain creation until we have a valid shared OpenGL context
    if (renderDevice_->GetBackend() != RenderBackend::OpenGL)
        CreateSwapChainForViewport(viewport);

    viewports_.push_back(viewport);
}

void ImGuiDiligentRendererEx::DestroyRendererWindow(ImGuiViewport* viewport)
{
    auto userData = GetViewportData(viewport);
    delete userData;
    viewport->RendererUserData = nullptr;

    viewports_.erase_first(viewport);
}

void ImGuiDiligentRendererEx::SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiIO& IO = ImGui::GetIO();
    auto userData = GetViewportData(viewport);

    if (userData->swapChain_)
    {
        const IntVector2 swapChainSize = VectorRoundToInt(ToVector2(size) * ToVector2(IO.DisplayFramebufferScale));
        userData->swapChain_->Resize(swapChainSize.x_, swapChainSize.y_, userData->swapChain_->GetDesc().PreTransform);
    }
    else
    {
        userData->postponedResize_ = size;
    }
}

void ImGuiDiligentRendererEx::RenderWindow(ImGuiViewport* viewport, void* renderArg)
{
    auto userData = GetViewportData(viewport);
    Diligent::IDeviceContext* deviceContext = renderDevice_->GetImmediateContext();

    // Delayed initialization for OpenGL
    if (!userData->swapChain_)
    {
        CreateSwapChainForViewport(viewport);
        if (userData->postponedResize_)
        {
            SetWindowSize(viewport, *userData->postponedResize_);
            userData->postponedResize_ = ea::nullopt;
        }
    }

    #if GL_SUPPORTED || GLES_SUPPORTED
    // On OpenGL, set swap chain and invalidate cached context state
    if (renderDevice_->GetBackend() == RenderBackend::OpenGL)
    {
        isCachedStateInvalid_ = true;

        auto deviceContextGL = ClassPtrCast<Diligent::IDeviceContextGL>(deviceContext);
        auto swapChainGL = ClassPtrCast<Diligent::ISwapChainGL>(userData->swapChain_.RawPtr());
        deviceContextGL->SetSwapChain(swapChainGL);
        deviceContextGL->InvalidateState();

        glEnable(GL_FRAMEBUFFER_SRGB);
    }
    #endif

    const Diligent::SwapChainDesc& swapChainDesc = userData->swapChain_->GetDesc();
    Diligent::ImGuiDiligentRenderer::NewFrame(swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.PreTransform);

    Diligent::ITextureView* renderTarget = userData->swapChain_->GetCurrentBackBufferRTV();
    deviceContext->SetRenderTargets(1, &renderTarget, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
    {
        deviceContext->ClearRenderTarget(
            renderTarget, Color::TRANSPARENT_BLACK.Data(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    RenderDrawDataWith(viewport->DrawData, secondaryPipelineState_);
}

void ImGuiDiligentRendererEx::SwapBuffers(ImGuiViewport* viewport, void* renderArg)
{
    #if GL_SUPPORTED || GLES_SUPPORTED
    // On OpenGL, swap chain is presented automatically
    if (renderDevice_->GetBackend() == RenderBackend::OpenGL)
        return;
    #endif

    auto userData = GetViewportData(viewport);
    userData->swapChain_->Present(0);
}

void ImGuiDiligentRendererEx::CreateSwapChainForViewport(ImGuiViewport* viewport)
{
    auto userData = GetViewportData(viewport);
    auto sdlWindow = reinterpret_cast<SDL_Window*>(viewport->PlatformHandle);
    URHO3D_ASSERT(sdlWindow);
    userData->swapChain_ = renderDevice_->CreateSecondarySwapChain(sdlWindow, false);
}

#endif

} // namespace Urho3D
