//
// Copyright (c) 2008-2017 the Urho3D project.
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../SystemUI/SystemUI.h"

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Macros.h"
#include "../Core/Profiler.h"
#include "../Engine/EngineEvents.h"
#include "../Graphics/GraphicsEvents.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"
#include "../Resource/ResourceCache.h"
#include "../SystemUI/Console.h"
#include "Urho3D/RenderAPI/RenderContext.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/SystemUI/ImGuiDiligentRendererEx.h"

#include <ImGui/imgui_freetype.h>
#include <ImGui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

#include <SDL.h>

#define IMGUI_IMPL_API IMGUI_API
#include <imgui_impl_sdl.h>

namespace Urho3D
{

SystemUI::SystemUI(Urho3D::Context* context, ImGuiConfigFlags flags)
    : Object(context)
{
    imContext_ = ui::CreateContext();

    ImGuiIO& io = ui::GetIO();
    io.UserData = this;
    // UI subsystem is responsible for managing cursors and that interferes with ImGui.
    io.ConfigFlags |= flags;

    PlatformInitialize();

    // Subscribe to events
    SubscribeToEvent(E_SDLRAWINPUT, &SystemUI::OnRawEvent);
    SubscribeToEvent(E_SCREENMODE, &SystemUI::OnScreenMode);
    SubscribeToEvent(E_INPUTBEGIN, &SystemUI::OnInputBegin);
    SubscribeToEvent(E_INPUTEND, &SystemUI::OnInputEnd);
    SubscribeToEvent(E_ENDRENDERING, &SystemUI::OnRenderEnd);
    SubscribeToEvent(E_ENDFRAME, [this] { referencedTextures_.clear(); });
    SubscribeToEvent(E_DEVICELOST, &SystemUI::PlatformShutdown);
    SubscribeToEvent(E_DEVICERESET, &SystemUI::PlatformInitialize);
    SubscribeToEvent(E_MOUSEVISIBLECHANGED, &SystemUI::OnMouseVisibilityChanged);
}

SystemUI::~SystemUI()
{
    PlatformShutdown();
    ui::DestroyContext(imContext_);
    imContext_ = nullptr;
}

void SystemUI::PlatformInitialize()
{
    static const unsigned defaultNumVertices = 16 * 1024;
    static const unsigned defaultNumIndices = 32 * 1024;

    auto renderDevice = GetSubsystem<RenderDevice>();

    ImGuiIO& io = ui::GetIO();
    io.DisplaySize = ToImGui(renderDevice->GetSwapChainSize());

    switch (renderDevice->GetBackend())
    {
    case RenderBackend::OpenGL:
    {
        ImGui_ImplSDL2_InitForOpenGL(renderDevice->GetSDLWindow(), SDL_GL_GetCurrentContext());
        break;
    }
    case RenderBackend::Vulkan:
    {
        // Diligent manages Vulkan on its own.
        ImGui_ImplSDL2_InitForSDLRenderer(renderDevice->GetSDLWindow());
        break;
    }
    case RenderBackend::D3D11:
    case RenderBackend::D3D12:
    {
        ImGui_ImplSDL2_InitForD3D(renderDevice->GetSDLWindow());
        break;
    }
    default:
    {
        URHO3D_ASSERT(false, "Not implemented");
        break;
    }
    }

    impl_ = ea::make_unique<ImGuiDiligentRendererEx>(renderDevice);

    // Ensure that swap chain is initialized
    impl_->NewFrame();
}

void SystemUI::PlatformShutdown()
{
    ImGuiIO& io = ui::GetIO();
    referencedTextures_.clear();
    ClearPerScreenFonts();

    impl_ = nullptr;
    ImGui_ImplSDL2_Shutdown();
}

void SystemUI::OnRawEvent(VariantMap& args)
{
    assert(imContext_ != nullptr);
    using namespace SDLRawInput;

    auto* evt = static_cast<SDL_Event*>(args[P_SDLEVENT].Get<void*>());
    ImGuiContext& g = *ui::GetCurrentContext();
    ImGuiIO& io = ui::GetIO();
    switch (evt->type)
    {
    case SDL_MOUSEMOTION:
        if (!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        {
            // No viewports - mouse is relative to the window. When viewports are enabled we get global mouse position
            // on every frame.
            io.MousePos.x = evt->motion.x;
            io.MousePos.y = evt->motion.y;
        }
        relativeMouseMove_.x_ += evt->motion.xrel;
        relativeMouseMove_.y_ += evt->motion.yrel;
        break;
    case SDL_FINGERUP:
        io.MouseDown[0] = false;
        io.MousePos.x = -1;
        io.MousePos.y = -1;
        URHO3D_FALLTHROUGH;
    case SDL_FINGERDOWN:
        io.MouseDown[0] = true;
    case SDL_FINGERMOTION:
        io.MousePos.x = evt->tfinger.x;
        io.MousePos.y = evt->tfinger.y;
        break;
    default:
        ImGui_ImplSDL2_ProcessEvent(evt);
        break;
    }

    // Consume events handled by imgui, unless explicitly told not to.
    if (!passThroughEvents_)
    {
        switch (evt->type)
        {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            args[P_CONSUMED] = io.WantCaptureKeyboard;
            break;
        case SDL_TEXTINPUT:
            args[P_CONSUMED] = io.WantTextInput;
            break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
        case SDL_FINGERDOWN:
        case SDL_FINGERUP:
        case SDL_FINGERMOTION:
            args[P_CONSUMED] = io.WantCaptureMouse;
            break;
        default:
            break;
        }
    }
}

void SystemUI::OnScreenMode(VariantMap& args)
{
    assert(imContext_ != nullptr);

    using namespace ScreenMode;
    ImGuiIO& io = ui::GetIO();
    io.DisplaySize = {args[P_WIDTH].GetFloat(), args[P_HEIGHT].GetFloat()};
}

void SystemUI::OnInputBegin()
{
    relativeMouseMove_ = Vector2::ZERO;
}

void SystemUI::OnInputEnd()
{
    assert(imContext_ != nullptr);

    if (imContext_->WithinFrameScope)
    {
        ui::EndFrame();
        ui::UpdatePlatformWindows();
    }

    auto input = GetSubsystem<Input>();
    auto renderDevice = GetSubsystem<RenderDevice>();
    if (!renderDevice)
        return;

    if (fontTextures_.empty())
       ReallocateFontTexture();

    // ImTextureID may be transient, make sure to tag all used textures every frame
    ImGuiIO& io = ui::GetIO();
    URHO3D_ASSERT(fontTextures_.size() >= io.AllFonts.size());
    io.Fonts->TexID = ToImTextureID(fontTextures_[0]);
    for (int i = 1; i < io.AllFonts.size(); ++i)
        io.AllFonts[i]->TexID = ToImTextureID(fontTextures_[i]);

    impl_->NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ui::NewFrame();

    if (!input->IsMouseVisible())
        ui::SetMouseCursor(ImGuiMouseCursor_None);

    ImGuizmo::BeginFrame();
}

void SystemUI::SetRelativeMouseMove(bool enabled, bool revertMousePositionOnDisable)
{
    ImGuiWindow* currentWindow = ui::GetCurrentWindowRead();
    const ImGuiContext& g = *ui::GetCurrentContext();
    if (!enabled || !currentWindow)
    {
        enableRelativeMouseMove_ = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
        return;
    }

    enableRelativeMouseMove_ = true;
    SDL_SetRelativeMouseMode(SDL_TRUE);

    revertMousePositionOnDisable_ = revertMousePositionOnDisable;
    revertMousePosition_ = g.IO.MousePos;
}

const Vector2 SystemUI::GetRelativeMouseMove() const
{
    return relativeMouseMove_;
}

void SystemUI::OnRenderEnd()
{
    // When SystemUI subsystem is recreated during runtime this method may be called without UI being rendered.
    assert(imContext_ != nullptr);
    if (!imContext_->WithinFrameScope)
        return;

    URHO3D_PROFILE("SystemUiRender");
    SendEvent(E_ENDRENDERINGSYSTEMUI);

    // Disable relative mouse movement automatically if none of mouse buttons are down
    if (enableRelativeMouseMove_ && !ui::IsAnyMouseDown())
    {
        enableRelativeMouseMove_ = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    ImGuiIO& io = ui::GetIO();
    if (imContext_->WithinFrameScope)
        ui::Render();

    // Revert mouse position after relative movement
    if (!enableRelativeMouseMove_ && revertMousePositionOnDisable_)
    {
        revertMousePositionOnDisable_ = false;
        io.MousePos = revertMousePosition_;
        io.MousePosPrev = revertMousePosition_;
        io.WantSetMousePos = true;
    }

    auto renderDevice = GetSubsystem<RenderDevice>();
    RenderContext* renderContext = renderDevice->GetRenderContext();
    renderContext->SetSwapChainRenderTargets();
    renderContext->SetFullViewport();

    impl_->RenderDrawData(ui::GetDrawData());
    impl_->RenderSecondaryWindows();
}

void SystemUI::OnMouseVisibilityChanged(StringHash, VariantMap& args)
{
    using namespace MouseVisibleChanged;
    ui::SetMouseCursor(args[P_VISIBLE].GetBool() ? ImGuiMouseCursor_Arrow : ImGuiMouseCursor_None);
}

ImFont* SystemUI::AddFont(const ea::string& fontPath, const ImWchar* ranges, float size, bool merge)
{
    float previousSize = fontSizes_.empty() ? SYSTEMUI_DEFAULT_FONT_SIZE : fontSizes_.back();
    fontSizes_.push_back(size);
    size = (size == 0 ? previousSize : size);

    if (auto fontFile = GetSubsystem<ResourceCache>()->GetFile(fontPath))
    {
        ea::vector<uint8_t> data;
        data.resize(fontFile->GetSize());
        auto bytesLen = fontFile->Read(&data.front(), data.size());
        return AddFont(data.data(), bytesLen, GetFileName(fontPath).c_str(), ranges, size, merge);
    }
    return nullptr;
}

ImFont* SystemUI::AddFont(const void* data, unsigned dsize, const char* name, const ImWchar* ranges, float size, bool merge)
{
    float previousSize = fontSizes_.empty() ? SYSTEMUI_DEFAULT_FONT_SIZE : fontSizes_.back();
    fontSizes_.push_back(size);
    size = (size == 0 ? previousSize : size);

    ImFontConfig cfg;
    cfg.MergeMode = merge;
    cfg.FontDataOwnedByAtlas = false;
    cfg.PixelSnapH = true;
    ImFormatString(cfg.Name, sizeof(cfg.Name), "%s (%.02f)", name, size);
    if (auto* newFont = ui::GetIO().Fonts->AddFontFromMemoryTTF((void*)data, dsize, size, &cfg, ranges))
    {
        ReallocateFontTexture();
        return newFont;
    }
    return nullptr;
}

ImFont* SystemUI::AddFontCompressed(const void* data, unsigned dsize, const char* name, const ImWchar* ranges, float size, bool merge)
{
    float previousSize = fontSizes_.empty() ? SYSTEMUI_DEFAULT_FONT_SIZE : fontSizes_.back();
    fontSizes_.push_back(size);
    size = (size == 0 ? previousSize : size);

    ImFontConfig cfg;
    cfg.MergeMode = merge;
    cfg.FontDataOwnedByAtlas = false;
    cfg.PixelSnapH = true;
    ImFormatString(cfg.Name, sizeof(cfg.Name), "%s (%.02f)", name, size);
    if (auto* newFont = ui::GetIO().Fonts->AddFontFromMemoryCompressedTTF((void*)data, dsize, size, &cfg, ranges))
    {
        ReallocateFontTexture();
        return newFont;
    }
    return nullptr;
}

void SystemUI::ReallocateFontTexture()
{
    ImGuiIO& io = ui::GetIO();
    ImGuiPlatformIO& platform_io = ui::GetPlatformIO();

    // Initialize per-screen font atlases.
    ClearPerScreenFonts();

    // Store main atlas, imgui expects it.
    fontTextures_.push_back(AllocateFontTexture(io.Fonts));
    io.AllFonts.push_back(io.Fonts);

    for (ImGuiPlatformMonitor& monitor : platform_io.Monitors)
    {
        if (monitor.DpiScale == 1.0f)
            continue; // io.Fonts has default scale.
        ImFontAtlas* atlas = new ImFontAtlas();
        io.Fonts->CloneInto(atlas, monitor.DpiScale);

        fontTextures_.push_back(AllocateFontTexture(atlas));
        io.AllFonts.push_back(atlas);
    }
}

void SystemUI::ClearPerScreenFonts()
{
    ImGuiIO& io = ui::GetIO();
    fontTextures_.clear();
    for (int i = 1; i < io.AllFonts.Size; i++) // First atlas (which is io.Fonts) is not deleted because it is handled separately by library itself.
        delete io.AllFonts[i];
    io.AllFonts.clear();
}

SharedPtr<Texture2D> SystemUI::AllocateFontTexture(ImFontAtlas* atlas) const
{
    // Create font texture.
    unsigned char* pixels;
    int width, height;

    if (atlas->ConfigData.Size > 0)
    {
        atlas->ClearTexData();

        const ImFontBuilderIO* fontBuilder = ImGuiFreeType::GetBuilderForFreeType();
        atlas->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
        fontBuilder->FontBuilder_Build(atlas);
    }
    atlas->GetTexDataAsRGBA32(&pixels, &width, &height);

    SharedPtr<Texture2D> fontTexture = MakeShared<Texture2D>(context_);
    fontTexture->SetNumLevels(1);
    fontTexture->SetFilterMode(FILTER_BILINEAR);
    fontTexture->SetSize(width, height, TextureFormat::TEX_FORMAT_RGBA8_UNORM);
    fontTexture->SetData(0, 0, 0, width, height, pixels);

    return fontTexture;
}

void SystemUI::ApplyStyleDefault(bool darkStyle, float alpha)
{
    ImGuiStyle& style = ui::GetStyleTemplate();
    style.ScrollbarSize = 10.f;
    if (darkStyle)
        ui::StyleColorsDark(&style);
    else
        ui::StyleColorsLight(&style);
    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
}

}
