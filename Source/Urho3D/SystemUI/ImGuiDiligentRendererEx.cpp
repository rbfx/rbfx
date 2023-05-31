// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/SystemUI/ImGuiDiligentRendererEx.h"

#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/SystemUI/ImGui.h"

#include <Diligent/Common/interface/Cast.hpp>
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

// TODO(diligent): Revisit
static const unsigned initialVertexBufferSize = 5000;
static const unsigned initialIndexBufferSize  = 10000;

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

} // namespace

ImGuiDiligentRendererEx::ImGuiDiligentRendererEx(RenderDevice* renderDevice)
    : Diligent::ImGuiDiligentRenderer(renderDevice->GetRenderDevice(),
        renderDevice->GetSwapChain()->GetDesc().ColorBufferFormat,
        renderDevice->GetSwapChain()->GetDesc().DepthBufferFormat, initialVertexBufferSize, initialIndexBufferSize)
    , renderDevice_{renderDevice}
{
    ImGuiIO& IO = ImGui::GetIO();
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

#if URHO3D_PLATFORM_WINDOWS || URHO3D_PLATFORM_LINUX || URHO3D_PLATFORM_MACOS
    const RenderBackend renderBackend = renderDevice->GetBackend();
    IO.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    createPlatformWindow_ = platformIO.Platform_CreateWindow;

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
}

ImGuiDiligentRendererEx::~ImGuiDiligentRendererEx()
{
    ImGuiIO& IO = ImGui::GetIO();
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

    IO.BackendRendererUserData = nullptr;
    platformIO.Renderer_CreateWindow = nullptr;
    platformIO.Renderer_DestroyWindow = nullptr;
    platformIO.Renderer_SetWindowSize = nullptr;
    platformIO.Renderer_RenderWindow = nullptr;
    platformIO.Renderer_SwapBuffers = nullptr;
    platformIO.Platform_CreateWindow = createPlatformWindow_;
}

void ImGuiDiligentRendererEx::NewFrame()
{
    const Diligent::SwapChainDesc& swapChainDesc = renderDevice_->GetSwapChain()->GetDesc();
    Diligent::ImGuiDiligentRenderer::NewFrame(swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.PreTransform);
}

void ImGuiDiligentRendererEx::EndFrame()
{
    Diligent::ImGuiDiligentRenderer::EndFrame();
}

void ImGuiDiligentRendererEx::RenderDrawData(ImDrawData* drawData)
{
    Diligent::ImGuiDiligentRenderer::RenderDrawData(renderDevice_->GetDeviceContext(), drawData);
}

void ImGuiDiligentRendererEx::RenderSecondaryWindows()
{
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

        auto deviceContextGL = ClassPtrCast<Diligent::IDeviceContextGL>(renderDevice_->GetDeviceContext());
        deviceContextGL->InvalidateState();
    }
#endif
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
    {
        auto sdlWindow = reinterpret_cast<SDL_Window*>(viewport->PlatformHandle);
        URHO3D_ASSERT(sdlWindow);
        userData->swapChain_ = renderDevice_->CreateSecondarySwapChain(sdlWindow);
    }
}

void ImGuiDiligentRendererEx::DestroyRendererWindow(ImGuiViewport* viewport)
{
    auto userData = GetViewportData(viewport);
    delete userData;
    viewport->RendererUserData = nullptr;
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
    Diligent::IDeviceContext* deviceContext = renderDevice_->GetDeviceContext();

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
    }
#endif

    const Diligent::SwapChainDesc& swapChainDesc = userData->swapChain_->GetDesc();
    Diligent::ImGuiDiligentRenderer::NewFrame(swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.PreTransform);

    // TODO(diligent): Get rid of depth buffer
    Diligent::ITextureView* renderTarget = userData->swapChain_->GetCurrentBackBufferRTV();
    Diligent::ITextureView* depthBuffer = userData->swapChain_->GetDepthBufferDSV();
    deviceContext->SetRenderTargets(1, &renderTarget, depthBuffer, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
    {
        deviceContext->ClearRenderTarget(
            renderTarget, Color::TRANSPARENT_BLACK.Data(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    Diligent::ImGuiDiligentRenderer::RenderDrawData(deviceContext, viewport->DrawData);
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
    userData->swapChain_ = renderDevice_->CreateSecondarySwapChain(sdlWindow);
}

#endif

} // namespace Urho3D
