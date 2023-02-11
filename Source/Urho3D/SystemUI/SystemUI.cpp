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
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/GraphicsImpl.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"
#include "../Resource/ResourceCache.h"
#include "../SystemUI/Console.h"

#include <ImGui/imgui_freetype.h>
#include <ImGui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>
#include <SDL.h>

#define IMGUI_IMPL_API IMGUI_API
#include <imgui_impl_sdl.h>
#if defined(URHO3D_OPENGL)
#include <imgui_impl_opengl3.h>
#elif defined(URHO3D_D3D11)
#include <imgui_impl_dx11.h>
#endif

namespace Urho3D
{

SystemUI::SystemUI(Urho3D::Context* context, ImGuiConfigFlags flags)
    : Object(context)
    , vertexBuffer_(context)
    , indexBuffer_(context)
{
    imContext_ = ui::CreateContext();

    ImGuiIO& io = ui::GetIO();
    io.UserData = this;
    // UI subsystem is responsible for managing cursors and that interferes with ImGui.
    io.ConfigFlags |= flags;

    PlatformInitialize();

    // Subscribe to events
    SubscribeToEvent(E_SDLRAWINPUT, [this](StringHash, VariantMap& args) { OnRawEvent(args); });
    SubscribeToEvent(E_SCREENMODE, [this](StringHash, VariantMap& args) { OnScreenMode(args); });
    SubscribeToEvent(E_INPUTBEGIN, [this](StringHash, VariantMap& args) { OnInputBegin(); });
    SubscribeToEvent(E_INPUTEND, [this](StringHash, VariantMap& args) { OnInputEnd(); });
    SubscribeToEvent(E_ENDRENDERING, [this](StringHash, VariantMap&) { OnRenderEnd(); });
    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) { referencedTextures_.clear(); });
    SubscribeToEvent(E_DEVICELOST, [this](StringHash, VariantMap&) { PlatformShutdown(); });
    SubscribeToEvent(E_DEVICERESET, [this](StringHash, VariantMap&) { PlatformInitialize(); });
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
    Graphics* graphics = GetSubsystem<Graphics>();
    ImGuiIO& io = ui::GetIO();
    io.DisplaySize = { static_cast<float>(graphics->GetWidth()), static_cast<float>(graphics->GetHeight()) };
#if URHO3D_OPENGL
    ImGui_ImplSDL2_InitForOpenGL(static_cast<SDL_Window*>(graphics->GetSDLWindow()), graphics->GetImpl()->GetGLContext());
#else
    ImGui_ImplSDL2_InitForD3D(static_cast<SDL_Window*>(graphics->GetSDLWindow()));
#endif
#if URHO3D_OPENGL
#if __APPLE__
#if URHO3D_GLES3
    const char* glslVersion = "#version 300 es";
#else
    const char* glslVersion = "#version 150";
#endif
#else
    const char* glslVersion = nullptr;
#endif
    ImGui_ImplOpenGL3_Init(glslVersion);
#elif URHO3D_D3D11
    ImGui_ImplDX11_Init(graphics->GetImpl()->GetDevice(), graphics->GetImpl()->GetDeviceContext());
#endif
}

void SystemUI::PlatformShutdown()
{
    ImGuiIO& io = ui::GetIO();
    referencedTextures_.clear();
    ClearPerScreenFonts();
#if URHO3D_OPENGL
    ImGui_ImplOpenGL3_Shutdown();
#elif URHO3D_D3D11
    ImGui_ImplDX11_Shutdown();
#endif
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

    ImGuiIO& io = ui::GetIO();
    Graphics* graphics = GetSubsystem<Graphics>();
    Input* input = GetSubsystem<Input>();
    if (graphics && graphics->IsInitialized())
    {
#if URHO3D_OPENGL
        ImGui_ImplOpenGL3_NewFrame();
#elif URHO3D_D3D11
        ImGui_ImplDX11_NewFrame();
#endif
        ImGui_ImplSDL2_NewFrame();
    }

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

#if URHO3D_OPENGL
    ImGui_ImplOpenGL3_RenderDrawData(ui::GetDrawData());
#elif URHO3D_D3D11
    // Resetting render target view required because if last view we rendered into was a texture
    // ImGui would try to render into that texture and we would see no UI on screen.
    auto* graphics = GetSubsystem<Graphics>();
    auto* graphicsImpl = graphics->GetImpl();
    ID3D11RenderTargetView* defaultRenderTargetView = graphicsImpl->GetDefaultRenderTargetView();
    graphicsImpl->GetDeviceContext()->OMSetRenderTargets(1, &defaultRenderTargetView, nullptr);
    graphicsImpl->MarkRenderTargetsDirty();
    ImGui_ImplDX11_RenderDrawData(ui::GetDrawData());
#endif
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
#if URHO3D_OPENGL
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
#endif
        ui::UpdatePlatformWindows();
        ui::RenderPlatformWindowsDefault();
#if URHO3D_OPENGL
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
#elif URHO3D_D3D11
        graphicsImpl->GetDeviceContext()->OMSetRenderTargets(1, &defaultRenderTargetView, nullptr);
        graphicsImpl->MarkRenderTargetsDirty();
#endif
    }
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

    // Store main atlas, imgui expects it.
    io.Fonts->TexID = AllocateFontTexture(io.Fonts);

    // Initialize per-screen font atlases.
    ClearPerScreenFonts();
    io.AllFonts.push_back(io.Fonts);
    for (ImGuiPlatformMonitor& monitor : platform_io.Monitors)
    {
        if (monitor.DpiScale == 1.0f)
            continue; // io.Fonts has default scale.
        ImFontAtlas* atlas = new ImFontAtlas();
        io.Fonts->CloneInto(atlas, monitor.DpiScale);
        io.AllFonts.push_back(atlas);
        atlas->TexID = AllocateFontTexture(atlas);
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

ImTextureID SystemUI::AllocateFontTexture(ImFontAtlas* atlas)
{
    // Create font texture.
    unsigned char* pixels;
    int width, height;
    atlas->ClearTexData();

    const ImFontBuilderIO* fontBuilder = ImGuiFreeType::GetBuilderForFreeType();
    atlas->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
    fontBuilder->FontBuilder_Build(atlas);
    atlas->GetTexDataAsRGBA32(&pixels, &width, &height);

    SharedPtr<Texture2D> fontTexture = MakeShared<Texture2D>(context_);
    fontTexture->SetNumLevels(1);
    fontTexture->SetFilterMode(FILTER_BILINEAR);
    fontTexture->SetSize(width, height, Graphics::GetRGBAFormat());
    fontTexture->SetData(0, 0, 0, width, height, pixels);
    fontTextures_.push_back(fontTexture);

    // Store return texture identifier
#if URHO3D_D3D11
    return fontTexture->GetShaderResourceView();
#else
    return fontTexture->GetGPUObject();
#endif
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
