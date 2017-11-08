//
// Copyright (c) 2017 the Urho3D project.
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
#include "Urho3D/Input/InputEvents.h"
#include "Urho3D/Input/Input.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Profiler.h"
#include "Urho3D/Core/Utils.h"
#include "Urho3D/Engine/EngineEvents.h"
#include "Urho3D/Graphics/GraphicsEvents.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "SystemUI.h"
#include "Console.h"
#include <SDL/SDL.h>
#include <ImGuizmo/ImGuizmo.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_freetype.h>


using namespace std::placeholders;
namespace Urho3D
{

SystemUI::SystemUI(Urho3D::Context* context)
    : Object(context)
    , vertexBuffer_(context)
    , indexBuffer_(context)
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = SCANCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_Home] = SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SCANCODE_END;
    io.KeyMap[ImGuiKey_Delete] = SCANCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = SCANCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SCANCODE_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SCANCODE_A;
    io.KeyMap[ImGuiKey_C] = SCANCODE_C;
    io.KeyMap[ImGuiKey_V] = SCANCODE_V;
    io.KeyMap[ImGuiKey_X] = SCANCODE_X;
    io.KeyMap[ImGuiKey_Y] = SCANCODE_Y;
    io.KeyMap[ImGuiKey_Z] = SCANCODE_Z;

    io.RenderDrawListsFn = [](ImDrawData* data) { ((SystemUI*)ImGui::GetIO().UserData)->OnRenderDrawLists(data); };
    io.SetClipboardTextFn = [](void* userData, const char* text) { SDL_SetClipboardText(text); };
    io.GetClipboardTextFn = [](void* userData) -> const char* { return SDL_GetClipboardText(); };

    io.UserData = this;

    io.Fonts->AddFontDefault();
    ReallocateFontTexture();
    UpdateProjectionMatrix();
    // Initializes ImGui. ImGui::Render() can not be called unless imgui is initialized. This call avoids initialization
    // check on every frame in E_ENDRENDERING.
    ImGui::NewFrame();

    // Subscribe to events
    SubscribeToEvent(E_SDLRAWINPUT, std::bind(&SystemUI::OnRawEvent, this, _2));
    SubscribeToEvent(E_SCREENMODE, std::bind(&SystemUI::UpdateProjectionMatrix, this));
    SubscribeToEvent(E_INPUTEND, [&](StringHash, VariantMap&)
    {
        float timeStep = GetTime()->GetTimeStep();
        ImGui::GetIO().DeltaTime = timeStep > 0.0f ? timeStep : 1.0f / 60.0f;
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    });
    SubscribeToEvent(E_ENDRENDERING, [&](StringHash, VariantMap&)
    {
        URHO3D_PROFILE(SystemUiRender);
        OnUpdate();
        ImGui::Render();
    });
}

SystemUI::~SystemUI()
{
    ImGui::Shutdown();
}

void SystemUI::UpdateProjectionMatrix()
{
    // Update screen size
    auto graphics = GetSubsystem<Graphics>();
    ImGui::GetIO().DisplaySize = ImVec2((float)graphics->GetWidth(), (float)graphics->GetHeight());

    // Update projection matrix
    IntVector2 viewSize = graphics->GetViewport().Size();
    Vector2 invScreenSize(1.0f / viewSize.x_, 1.0f / viewSize.y_);
    Vector2 scale(2.0f * invScreenSize.x_, -2.0f * invScreenSize.y_);
    Vector2 offset(-1.0f, 1.0f);

    projection_ = Matrix4(Matrix4::IDENTITY);
    projection_.m00_ = scale.x_ * uiScale_;
    projection_.m03_ = offset.x_;
    projection_.m11_ = scale.y_ * uiScale_;
    projection_.m13_ = offset.y_;
    projection_.m22_ = 1.0f;
    projection_.m23_ = 0.0f;
    projection_.m33_ = 1.0f;
}

void SystemUI::OnRawEvent(VariantMap& args)
{
    auto evt = static_cast<SDL_Event*>(args[SDLRawInput::P_SDLEVENT].Get<void*>());
    auto& io = ImGui::GetIO();
    switch (evt->type)
    {
    case SDL_KEYUP:
    case SDL_KEYDOWN:
    {
        auto code = evt->key.keysym.scancode;
        auto down = evt->type == SDL_KEYDOWN;
        if (code < 512)
            io.KeysDown[code] = down;
        if (evt->key.keysym.sym == SDLK_LCTRL || evt->key.keysym.sym == SDLK_RCTRL)
            io.KeyCtrl = down;
        else if (evt->key.keysym.sym == SDLK_LSHIFT || evt->key.keysym.sym == SDLK_RSHIFT)
            io.KeyShift = down;
        else if (evt->key.keysym.sym == SDLK_LALT || evt->key.keysym.sym == SDLK_RALT)
            io.KeyAlt = down;
        else if (evt->key.keysym.sym == SDLK_LGUI || evt->key.keysym.sym == SDLK_RGUI)
            io.KeySuper = down;
        break;
    }
    case SDL_MOUSEWHEEL:
        io.MouseWheel = evt->wheel.y;
        break;
    URHO3D_FALLTHROUGH
    case SDL_MOUSEBUTTONUP:
    URHO3D_FALLTHROUGH
    case SDL_MOUSEBUTTONDOWN:
        io.MouseDown[evt->button.button - 1] = evt->type == SDL_MOUSEBUTTONDOWN;
    case SDL_MOUSEMOTION:
        io.MousePos.x = evt->motion.x / uiScale_;
        io.MousePos.y = evt->motion.y / uiScale_;
        break;
    URHO3D_FALLTHROUGH
    case SDL_FINGERUP:
        io.MouseDown[0] = false;
        io.MousePos.x = -1;
        io.MousePos.y = -1;
    URHO3D_FALLTHROUGH
    case SDL_FINGERDOWN:
        io.MouseDown[0] = true;
    case SDL_FINGERMOTION:
        io.MousePos.x = evt->tfinger.x / uiScale_;
        io.MousePos.y = evt->tfinger.y / uiScale_;
        break;
    case SDL_TEXTINPUT:
        ImGui::GetIO().AddInputCharactersUTF8(evt->text.text);
        break;
    default:
        break;
    }
}

void SystemUI::OnRenderDrawLists(ImDrawData* data)
{
    auto graphics = GetGraphics();
    // Engine does not render when window is closed or device is lost
    assert(graphics && graphics->IsInitialized() && !graphics->IsDeviceLost());

    ImGuiIO& io = ImGui::GetIO();
    int fbWidth = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fbHeight = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fbWidth == 0 || fbHeight == 0)
        return;
    data->ScaleClipRects(io.DisplayFramebufferScale);

    for (int n = 0; n < data->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = data->CmdLists[n];
        unsigned int idxBufferOffset = 0;

        // Resize vertex and index buffers on the fly. Once buffer becomes too small for data that is to be rendered
        // we reallocate buffer to be twice as big as we need now. This is done in order to minimize memory reallocation
        // in rendering loop.
        if (cmdList->VtxBuffer.Size > vertexBuffer_.GetVertexCount())
        {
            PODVector<VertexElement> elems = {VertexElement(TYPE_VECTOR2, SEM_POSITION),
                                              VertexElement(TYPE_VECTOR2, SEM_TEXCOORD),
                                              VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR)
            };
            vertexBuffer_.SetSize((unsigned int)(cmdList->VtxBuffer.Size * 2), elems, true);
        }
        if (cmdList->IdxBuffer.Size > indexBuffer_.GetIndexCount())
            indexBuffer_.SetSize((unsigned int)(cmdList->IdxBuffer.Size * 2), false, true);

#if (defined(_WIN32) && !defined(URHO3D_D3D11) && !defined(URHO3D_OPENGL)) || defined(URHO3D_D3D9)
        for (int i = 0; i < cmdList->VtxBuffer.Size; i++)
        {
            ImDrawVert& v = cmdList->VtxBuffer.Data[i];
            v.pos.x += 0.5f;
            v.pos.y += 0.5f;
        }
#endif

        vertexBuffer_.SetDataRange(cmdList->VtxBuffer.Data, 0, (unsigned int)cmdList->VtxBuffer.Size, true);
        indexBuffer_.SetDataRange(cmdList->IdxBuffer.Data, 0, (unsigned int)cmdList->IdxBuffer.Size, true);

        graphics->ClearParameterSources();
        graphics->SetColorWrite(true);
        graphics->SetCullMode(CULL_NONE);
        graphics->SetDepthTest(CMP_ALWAYS);
        graphics->SetDepthWrite(false);
        graphics->SetFillMode(FILL_SOLID);
        graphics->SetStencilTest(false);
        graphics->SetVertexBuffer(&vertexBuffer_);
        graphics->SetIndexBuffer(&indexBuffer_);

        for (const ImDrawCmd* cmd = cmdList->CmdBuffer.begin(); cmd != cmdList->CmdBuffer.end(); cmd++)
        {
            if (cmd->UserCallback)
                cmd->UserCallback(cmdList, cmd);
            else
            {
                ShaderVariation* ps;
                ShaderVariation* vs;

                Texture2D* texture = static_cast<Texture2D*>(cmd->TextureId);
                if (!texture)
                {
                    ps = graphics->GetShader(PS, "Basic", "VERTEXCOLOR");
                    vs = graphics->GetShader(VS, "Basic", "VERTEXCOLOR");
                }
                else
                {
                    // If texture contains only an alpha channel, use alpha shader (for fonts)
                    vs = graphics->GetShader(VS, "Basic", "DIFFMAP VERTEXCOLOR");
                    if (texture->GetFormat() == Graphics::GetAlphaFormat())
                        ps = graphics->GetShader(PS, "Basic", "ALPHAMAP VERTEXCOLOR");
                    else
                        ps = graphics->GetShader(PS, "Basic", "DIFFMAP VERTEXCOLOR");
                }

                graphics->SetShaders(vs, ps);
                if (graphics->NeedParameterUpdate(SP_OBJECT, this))
                    graphics->SetShaderParameter(VSP_MODEL, Matrix3x4::IDENTITY);
                if (graphics->NeedParameterUpdate(SP_CAMERA, this))
                    graphics->SetShaderParameter(VSP_VIEWPROJ, projection_);
                if (graphics->NeedParameterUpdate(SP_MATERIAL, this))
                    graphics->SetShaderParameter(PSP_MATDIFFCOLOR, Color(1.0f, 1.0f, 1.0f, 1.0f));

                float elapsedTime = GetSubsystem<Time>()->GetElapsedTime();
                graphics->SetShaderParameter(VSP_ELAPSEDTIME, elapsedTime);
                graphics->SetShaderParameter(PSP_ELAPSEDTIME, elapsedTime);

                IntRect scissor = IntRect(int(cmd->ClipRect.x * uiScale_), int(cmd->ClipRect.y * uiScale_),
                                          int(cmd->ClipRect.z * uiScale_), int(cmd->ClipRect.w * uiScale_));

                graphics->SetBlendMode(BLEND_ALPHA);
                graphics->SetScissorTest(true, scissor);
                graphics->SetTexture(0, texture);
                graphics->Draw(TRIANGLE_LIST, idxBufferOffset, cmd->ElemCount, 0, 0,
                                vertexBuffer_.GetVertexCount());
                idxBufferOffset += cmd->ElemCount;
            }
        }
    }
    graphics->SetScissorTest(false);
}

ImFont* SystemUI::AddFont(const String& fontPath, float size, const unsigned short* ranges, bool merge)
{
    auto io = ImGui::GetIO();

    if (size == 0)
    {
        if (io.Fonts->Fonts.empty())
            return nullptr;
        size = io.Fonts->Fonts.back()->FontSize;
    }

    if (auto fontFile = GetSubsystem<ResourceCache>()->GetFile(fontPath))
    {
        PODVector<uint8_t> data;
        data.Resize(fontFile->GetSize());
        auto bytesLen = fontFile->Read(&data.Front(), data.Size());
        ImFontConfig cfg;
        cfg.MergeMode = merge;
        cfg.FontDataOwnedByAtlas = false;
        cfg.PixelSnapH = true;
        if (auto newFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(&data.Front(), bytesLen, size, &cfg, ranges))
        {
            ReallocateFontTexture();
            return newFont;
        }
    }
    return nullptr;
}

ImFont* SystemUI::AddFont(const Urho3D::String& fontPath, float size,
    const std::initializer_list<unsigned short>& ranges, bool merge)
{
    return AddFont(fontPath, size, ranges.size() ? &*ranges.begin() : nullptr, merge);
}

void SystemUI::ReallocateFontTexture()
{
    auto io = ImGui::GetIO();
    // Create font texture.
    unsigned char* pixels;
    int width, height;

    ImGuiFreeType::BuildFontAtlas(io.Fonts, ImGuiFreeType::ForceAutoHint);
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    if (!fontTexture_ || width != fontTexture_->GetWidth() || height != fontTexture_->GetHeight())
    {
        fontTexture_ = new Texture2D(context_);
        fontTexture_->SetNumLevels(1);
        fontTexture_->SetSize(width, height, Graphics::GetRGBAFormat());
        fontTexture_->SetData(0, 0, 0, width, height, pixels);
        fontTexture_->SetFilterMode(FILTER_BILINEAR);

        // Store our identifier
        io.Fonts->TexID = (void*)fontTexture_.Get();
        io.Fonts->ClearTexData();
    }
}

void SystemUI::SetScale(float scale)
{
    if (uiScale_ == scale)
        return;
    uiScale_ = scale;
    UpdateProjectionMatrix();
}

void SystemUI::ApplyStyleDefault(bool darkStyle, float alpha)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
    style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
//    style.Colors[ImGuiCol_Column]                = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
//    style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
//    style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
    style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    if( darkStyle )
    {
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            ImVec4& col = style.Colors[i];
            float H, S, V;
            ImGui::ColorConvertRGBtoHSV( col.x, col.y, col.z, H, S, V );

            if( S < 0.1f )
            {
                V = 1.0f - V;
            }
            ImGui::ColorConvertHSVtoRGB( H, S, V, col.x, col.y, col.z );
            if( col.w < 1.00f )
            {
                col.w *= alpha;
            }
        }
    }
    else
    {
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            ImVec4& col = style.Colors[i];
            if( col.w < 1.00f )
            {
                col.x *= alpha;
                col.y *= alpha;
                col.z *= alpha;
                col.w *= alpha;
            }
        }
    }
}

bool SystemUI::IsAnyItemActive() const
{
    return ui::IsAnyItemActive();
}

bool SystemUI::IsAnyItemHovered() const
{
    return ui::IsAnyItemHovered() || ui::IsAnyWindowHovered();
}

void SystemUI::OnUpdate()
{
    // Draw dragged item
    if (dragData_.GetType() != VAR_NONE)
    {
        if (ui::IsMouseDown(0))
        {
            ImGuiIO& io = ImGui::GetIO();
            ui::SetNextWindowPos(ui::GetMousePos() - ImVec2(10, 10), ImGuiCond_Always);
            ui::SetNextWindowFocus();
            ui::Begin("SystemUI drag", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ShowBorders);
            ui::TextUnformatted(dragData_.ToString().CString());
            ui::End();
        }
        else
            dragData_.Clear();
    }
}

}

