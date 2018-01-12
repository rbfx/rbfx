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
#include <IO/Log.h>


using namespace std::placeholders;
namespace Urho3D
{

const float defaultFontSize = 14.f;

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

    SetScale();
    AddFont("Fonts/DejaVuSansMono.ttf", defaultFontSize, nullptr);
    ReallocateFontTexture();
    UpdateProjectionMatrix();
    // Initializes ImGui. ImGui::Render() can not be called unless imgui is initialized. This call avoids initialization
    // check on every frame in E_ENDRENDERING.
    ImGui::NewFrame();
    ImGui::EndFrame();

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
    projection_.m00_ = scale.x_ * uiZoom_;
    projection_.m03_ = offset.x_;
    projection_.m11_ = scale.y_ * uiZoom_;
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
        io.MousePos.x = evt->motion.x / uiZoom_;
        io.MousePos.y = evt->motion.y / uiZoom_;
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
        io.MousePos.x = evt->tfinger.x / uiZoom_;
        io.MousePos.y = evt->tfinger.y / uiZoom_;
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

                IntRect scissor = IntRect(int(cmd->ClipRect.x * uiZoom_), int(cmd->ClipRect.y * uiZoom_),
                                          int(cmd->ClipRect.z * uiZoom_), int(cmd->ClipRect.w * uiZoom_));

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

    fontSizes_.Push(size);

    if (size == 0)
    {
        if (io.Fonts->Fonts.empty())
            return nullptr;
        size = io.Fonts->Fonts.back()->FontSize;
    }
    else
        size *= fontScale_;

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

    if (fontTexture_.Null())
    {
        fontTexture_ = new Texture2D(context_);
        fontTexture_->SetNumLevels(1);
        fontTexture_->SetFilterMode(FILTER_BILINEAR);
    }

    if (fontTexture_->GetWidth() != width || fontTexture_->GetHeight() != height)
        fontTexture_->SetSize(width, height, Graphics::GetRGBAFormat());

    fontTexture_->SetData(0, 0, 0, width, height, pixels);

    // Store our identifier
    io.Fonts->TexID = (void*)fontTexture_.Get();
    io.Fonts->ClearTexData();
}

void SystemUI::SetZoom(float zoom)
{
    if (uiZoom_ == zoom)
        return;
    uiZoom_ = zoom;
    UpdateProjectionMatrix();
}

void SystemUI::SetScale(Vector3 scale)
{
    auto& io = ui::GetIO();
    auto& style = ui::GetStyle();

    if (scale == Vector3::ZERO)
        scale = GetGraphics()->GetDisplayDPI() / 96.f;

    if (scale == Vector3::ZERO)
    {
        URHO3D_LOGWARNING("SystemUI failed to set font scaling, DPI unknown.");
        return;
    }

    io.DisplayFramebufferScale = {scale.x_, scale.y_};
    fontScale_ = scale.z_;

    float prevSize = defaultFontSize;
    for (auto i = 0; i < io.Fonts->Fonts.size(); i++)
    {
        float sizePixels = fontSizes_[i];
        if (sizePixels == 0)
            sizePixels = prevSize;
        io.Fonts->ConfigData[i].SizePixels = sizePixels * fontScale_;
    }

    if (io.Fonts->Fonts.size() > 0)
        ReallocateFontTexture();
}

void SystemUI::ApplyStyleDefault(bool darkStyle, float alpha)
{
    ImGuiStyle& style = ImGui::GetStyle();
    if (darkStyle)
        ui::StyleColorsDark(&style);
    else
        ui::StyleColorsLight(&style);
    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.ScaleAllSizes(GetFontScale());
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
            ui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
            ui::Begin("SystemUI drag", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings);
            ui::TextUnformatted(dragData_.ToString().CString());
            ui::End();
            ui::PopStyleVar();
        }
        else
            dragData_.Clear();
    }
}

}

