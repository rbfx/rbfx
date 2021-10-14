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
#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Core/Macros.h"
#include "../Engine/EngineEvents.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsImpl.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"
#include "../IO/Log.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../SystemUI/SystemUI.h"
#include "../SystemUI/Console.h"
#include <SDL/SDL.h>
#include <ImGuizmo/ImGuizmo.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_freetype.h>

#define IMGUI_IMPL_API IMGUI_API
#include <imgui_impl_sdl.h>
#if defined(URHO3D_OPENGL)
#include <imgui_impl_opengl3.h>
#elif defined(URHO3D_D3D11)
#include <imgui_impl_dx11.h>
#else
#include <imgui_impl_dx9.h>
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
    SubscribeToEvent(E_INPUTEND, [this](StringHash, VariantMap& args) { OnInputEnd(args); });
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
#else
    ImGui_ImplDX9_Init(graphics->GetImpl()->GetDevice());
#endif
}

void SystemUI::PlatformShutdown()
{
    ImGuiIO& io = ui::GetIO();
    referencedTextures_.clear();
    ClearPerScreenFonts();
#if URHO3D_OPENGL
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
#elif URHO3D_D3D11
    ImGui_ImplDX11_InvalidateDeviceObjects();
#else
    ImGui_ImplDX9_InvalidateDeviceObjects();
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

void SystemUI::OnInputEnd(VariantMap& args)
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
#else
        ImGui_ImplDX9_NewFrame();
#endif
        ImGui_ImplSDL2_NewFrame(graphics->GetWindow());
    }

    ui::NewFrame();

    if (!input->IsMouseVisible())
        ui::SetMouseCursor(ImGuiMouseCursor_None);

    ImGuizmo::BeginFrame();
}

void SystemUI::SetMouseWrapping(bool enabled, bool revertMousePositionOnDisable)
{
    ImGuiWindow* currentWindow = ui::GetCurrentWindowRead();
    const ImGuiContext& g = *ui::GetCurrentContext();
    if (!enabled || !currentWindow)
    {
        enableWrapping_ = false;
        return;
    }

    enableWrapping_ = true;
    const ImVec2& windowPosition = currentWindow->Pos;
    const ImVec2& windowSize = currentWindow->Size;
    minWrapBound_ = windowPosition;
    maxWrapBound_ = windowPosition + windowSize - ImVec2{ 1, 1 };

    const unsigned monitorIndex = currentWindow->Viewport->PlatformMonitor;
    if (monitorIndex < g.PlatformIO.Monitors.size())
    {
        const ImGuiPlatformMonitor& currentMonitor = g.PlatformIO.Monitors[monitorIndex];
        const ImVec2& monitorPosition = currentMonitor.WorkPos;
        const ImVec2& monitorSize = currentMonitor.WorkSize;
        const ImVec2 minMonitorBound = monitorPosition;
        const ImVec2 maxMonitorBound = monitorPosition + monitorSize - ImVec2{ 1, 1 };
        minWrapBound_ = ImClamp(minWrapBound_, minMonitorBound, maxMonitorBound);
        maxWrapBound_ = ImClamp(maxWrapBound_, minMonitorBound, maxMonitorBound);
    }

    const ImVec2 wrapRegionSize = maxWrapBound_ - minWrapBound_;
    if (wrapRegionSize.x <= 3 || wrapRegionSize.y <= 3)
        enableWrapping_ = false;

    revertMousePositionOnDisable_ = revertMousePositionOnDisable;
    revertMousePosition_ = g.IO.MousePos;
}

void SystemUI::OnRenderEnd()
{
    // When SystemUI subsystem is recreated during runtime this method may be called without UI being rendered.
    assert(imContext_ != nullptr);
    if (!imContext_->WithinFrameScope)
        return;

    URHO3D_PROFILE("SystemUiRender");
    SendEvent(E_ENDRENDERINGSYSTEMUI);

    // Disable mouse wrapping automatically if none of mouse buttons are down
    if (!ui::IsAnyMouseDown())
        enableWrapping_ = false;

    ImGuiIO& io = ui::GetIO();
    ui::Render();

    // Apply mouse wrapping in the same way ImGUI controls mouse
    if (enableWrapping_)
    {
        ImVec2 mousePos = io.MousePos;

        if (mousePos.x <= minWrapBound_.x)
            mousePos.x = maxWrapBound_.x - 1;
        if (mousePos.x >= maxWrapBound_.x)
            mousePos.x = minWrapBound_.x + 1;
        if (mousePos.y <= minWrapBound_.y)
            mousePos.y = maxWrapBound_.y - 1;
        if (mousePos.y >= maxWrapBound_.y)
            mousePos.y = minWrapBound_.y + 1;

        if (mousePos != io.MousePos)
        {
            for (int btn = 0; btn < ImGuiMouseButton_COUNT; btn++)
            {
                if (io.MouseDown[btn])
                    io.MouseClickedPos[btn] += mousePos - io.MousePos;
            }

            io.MousePos = mousePos;
            io.MousePosPrev = mousePos;
            io.WantSetMousePos = true;
        }
    }
    else if (revertMousePositionOnDisable_)
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
#else
    ImGui_ImplDX9_RenderDrawData(ui::GetDrawData());
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
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    // Store main atlas, imgui expects it.
    io.Fonts->TexID = AllocateFontTexture(io.Fonts);

#if URHO3D_SYSTEMUI_VIEWPORTS
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
#endif
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

    SharedPtr<Texture2D> fontTexture = context_->CreateObject<Texture2D>();
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

int ToImGui(MouseButton button)
{
    switch (button)
    {
    case MOUSEB_LEFT:
        return ImGuiMouseButton_Left;
    case MOUSEB_MIDDLE:
        return ImGuiMouseButton_Middle;
    case MOUSEB_RIGHT:
        return ImGuiMouseButton_Right;
    case MOUSEB_X1:
        return 3;
    case MOUSEB_X2:
        return 4;
    default:
        return -1;
    }
}

}

bool ui::IsMouseDown(Urho3D::MouseButton button)
{
    return ui::IsMouseDown(Urho3D::ToImGui(button));
}

bool ui::IsMouseDoubleClicked(Urho3D::MouseButton button)
{
    return ui::IsMouseDoubleClicked(Urho3D::ToImGui(button));
}

bool ui::IsMouseDragging(Urho3D::MouseButton button, float lock_threshold)
{
    return ui::IsMouseDragging(Urho3D::ToImGui(button), lock_threshold);
}

bool ui::IsMouseReleased(Urho3D::MouseButton button)
{
    return ui::IsMouseReleased(Urho3D::ToImGui(button));
}

bool ui::IsMouseClicked(Urho3D::MouseButton button, bool repeat)
{
    return ui::IsMouseClicked(Urho3D::ToImGui(button), repeat);
}

bool ui::IsItemClicked(Urho3D::MouseButton button)
{
    return ui::IsItemClicked(Urho3D::ToImGui(button));
}

ImVec2 ui::GetMouseDragDelta(Urho3D::MouseButton button, float lock_threshold)
{
    return ui::GetMouseDragDelta(Urho3D::ToImGui(button), lock_threshold);
}

void ui::ResetMouseDragDelta(Urho3D::MouseButton button)
{
    ResetMouseDragDelta(Urho3D::ToImGui(button));
}

bool ui::SetDragDropVariant(const ea::string& types, const Urho3D::Variant& variant, ImGuiCond cond)
{
    if (SetDragDropPayload(types.c_str(), nullptr, 0, cond))
    {
        auto* systemUI = static_cast<Urho3D::SystemUI*>(GetIO().UserData);
        systemUI->GetContext()->SetGlobalVar("SystemUI_Drag&Drop_Value", variant);
        return true;
    }
    return false;
}

const Urho3D::Variant& ui::AcceptDragDropVariant(const ea::string& types, ImGuiDragDropFlags flags)
{
    using namespace Urho3D;

    if (const ImGuiPayload* payload = GetDragDropPayload())
    {
        bool accepted = false;
        for (const ea::string& type : types.split(','))
        {
            const char* t = payload->DataType;
            while (t < payload->DataType + URHO3D_ARRAYSIZE(payload->DataType))
            {
                t = strstr(t, type.c_str());
                if (t == nullptr)
                    break;

                const char* tEnd = strstr(t, ",");
                tEnd = tEnd ? Min(t + strlen(t), tEnd) : t + strlen(t);
                if ((t == payload->DataType || t[-1] == ',') && (*tEnd == 0 || *tEnd == ','))
                    accepted = true;
                t = tEnd;
            }
        }

        if (AcceptDragDropPayload(accepted ? payload->DataType : "Smth that won't be accepted.", flags))
        {
            SystemUI* systemUI = static_cast<SystemUI*>(GetIO().UserData);
            return systemUI->GetContext()->GetGlobalVar("SystemUI_Drag&Drop_Value");
        }
    }

    return Urho3D::Variant::EMPTY;
}

void ui::Image(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
    auto* systemUI = static_cast<Urho3D::SystemUI*>(GetIO().UserData);
    systemUI->ReferenceTexture(user_texture_id);
#if URHO3D_D3D11
    void* texture_id = user_texture_id->GetShaderResourceView();
#else
    void* texture_id = user_texture_id->GetGPUObject();
#endif
    Image(texture_id, size, uv0, uv1, tint_col, border_col);
}

void ui::ImageItem(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
    ImGuiWindow* window = ui::GetCurrentWindow();
    ImGuiID id = window->GetID(user_texture_id);
    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ui::Image(user_texture_id, size, uv0, uv1, tint_col, border_col);
    ui::ItemAdd(bb, id);
}

bool ui::ImageButton(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
{
    auto* systemUI = static_cast<Urho3D::SystemUI*>(GetIO().UserData);
    systemUI->ReferenceTexture(user_texture_id);
#if URHO3D_D3D11
    void* texture_id = user_texture_id->GetShaderResourceView();
#else
    void* texture_id = user_texture_id->GetGPUObject();
#endif
    return ImageButton(texture_id, size, uv0, uv1, frame_padding, bg_col, tint_col);
}

bool ui::IsKeyDown(Urho3D::Key key)
{
    return IsKeyDown(SDL_GetScancodeFromKey(key));
}

bool ui::IsKeyPressed(Urho3D::Key key, bool repeat)
{
    return IsKeyPressed(SDL_GetScancodeFromKey(key), repeat);
}

bool ui::IsKeyReleased(Urho3D::Key key)
{
    return IsKeyReleased(SDL_GetScancodeFromKey(key));
}

int ui::GetKeyPressedAmount(Urho3D::Key key, float repeat_delay, float rate)
{
    return GetKeyPressedAmount(SDL_GetScancodeFromKey(key), repeat_delay, rate);
}

bool ui::ItemMouseActivation(Urho3D::MouseButton button, unsigned flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    bool activated = !ui::IsItemActive() && ui::IsItemHovered();
    if (flags == ImGuiItemMouseActivation_Dragging)
        activated &= ui::IsMouseDragging(button);
    else
        activated &= ui::IsMouseClicked(button);

    if (activated)
        ui::SetActiveID(g.LastItemData.ID, window);
    else if (ui::IsItemActive() && !ui::IsMouseDown(button))
        ui::ClearActiveID();
    return ui::IsItemActive();
}

void ui::HideCursorWhenActive(Urho3D::MouseButton button, bool on_drag)
{
    using namespace Urho3D;
    ImGuiContext& g = *GImGui;
    SystemUI* systemUI = reinterpret_cast<SystemUI*>(g.IO.UserData);
    if (ui::IsItemActive())
    {
        if (!on_drag || ui::IsMouseDragging(button))
        {
            Input* input = systemUI->GetSubsystem<Input>();
            if (input->IsMouseVisible())
            {
                systemUI->SetMouseWrapping(true, true);
                input->SetMouseVisible(false);
            }
        }
    }
    else if (ui::IsItemDeactivated())
    {
        systemUI->SetMouseWrapping(false, true);
        systemUI->GetSubsystem<Input>()->SetMouseVisible(true);
    }
}