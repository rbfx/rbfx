// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/SystemUI/ImGuiDiligentRendererEx.h"

#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/SystemUI/ImGui.h"

#if D3D11_SUPPORTED
#    include <Diligent/Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>
#endif

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
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain_;
};

ViewportRendererData* GetViewportData(ImGuiViewport* viewport)
{
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
    if (renderBackend != RenderBackend::OpenGL)
        IO.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    createPlatformWindow_ = platformIO.Platform_CreateWindow;

    // clang-format off
    IO.BackendRendererUserData = this;
    platformIO.Platform_CreateWindow = [](ImGuiViewport* viewport) { GetBackendData()->CreatePlatformWindow(viewport); };
    platformIO.Renderer_CreateWindow = [](ImGuiViewport* viewport) { GetBackendData()->CreateWindow(viewport); };
    platformIO.Renderer_DestroyWindow = [](ImGuiViewport* viewport) { GetBackendData()->DestroyWindow(viewport); };
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
    ui::UpdatePlatformWindows();
    ui::RenderPlatformWindowsDefault();
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

void ImGuiDiligentRendererEx::CreateWindow(ImGuiViewport* viewport)
{
    auto userData = new ViewportRendererData{};
    viewport->RendererUserData = userData;

    auto sdlWindow = reinterpret_cast<SDL_Window*>(viewport->PlatformHandle);
    URHO3D_ASSERT(sdlWindow);
    userData->swapChain_ = renderDevice_->CreateSecondarySwapChain(sdlWindow);
}

void ImGuiDiligentRendererEx::DestroyWindow(ImGuiViewport* viewport)
{
    auto userData = GetViewportData(viewport);
    delete userData;
    viewport->RendererUserData = nullptr;
}

void ImGuiDiligentRendererEx::SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiIO& IO = ImGui::GetIO();
    auto userData = GetViewportData(viewport);

    const IntVector2 swapChainSize = VectorRoundToInt(ToVector2(size) * ToVector2(IO.DisplayFramebufferScale));
    userData->swapChain_->Resize(swapChainSize.x_, swapChainSize.y_, userData->swapChain_->GetDesc().PreTransform);
}

void ImGuiDiligentRendererEx::RenderWindow(ImGuiViewport* viewport, void* renderArg)
{
    auto userData = GetViewportData(viewport);

    const Diligent::SwapChainDesc& swapChainDesc = userData->swapChain_->GetDesc();
    Diligent::ImGuiDiligentRenderer::NewFrame(swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.PreTransform);

    // TODO(diligent): Get rid of depth buffer
    Diligent::IDeviceContext* deviceContext = renderDevice_->GetDeviceContext();
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
    auto userData = GetViewportData(viewport);
    userData->swapChain_->Present(0);
}
#endif

} // namespace Urho3D
