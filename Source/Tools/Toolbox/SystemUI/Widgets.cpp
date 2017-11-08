//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "Widgets.h"
#include <ImGui/imgui_internal.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <ThirdParty/SDL/include/SDL_scancode.h>


namespace ImGui
{

const unsigned UISTATE_EXPIRATION_MS = 30000;

struct UIStateWrapper
{
    void Set(void* state, void(*deleter)(void*)=nullptr)
    {
        state_ = state;
        deleter_ = deleter;
    }

    void Unset()
    {
        if (deleter_ && state_)
            deleter_(state_);
        state_ = nullptr;
        deleter_ = nullptr;
    }

    void* Get(bool keepAlive=true)
    {
        if (keepAlive)
            timer_.Reset();
        return state_;
    }

    bool IsExpired()
    {
        return timer_.GetMSec(false) >= UISTATE_EXPIRATION_MS;
    }

protected:
    /// User state pointer.
    void* state_ = nullptr;
    /// Function that handles deleting state object when it becomes unused.
    void(*deleter_)(void* state) = nullptr;
    /// Timer which determines when state expires.
    Urho3D::Timer timer_;
};

Urho3D::HashMap<ImGuiID, UIStateWrapper> uiState_;

void SetUIStateP(void* state, void(*deleter)(void*))
{
    auto id = ui::GetCurrentWindow()->IDStack.back();
    uiState_[id].Set(state, deleter);
}

void* GetUIStateP()
{
    void* result = nullptr;
    auto id = ui::GetCurrentWindow()->IDStack.back();
    auto it = uiState_.Find(id);
    if (it != uiState_.End())
        result = it->second_.Get();

    // Every 30s check all saved states and remove expired ones.
    static Urho3D::Timer gcTimer;
    if (gcTimer.GetMSec(false) > UISTATE_EXPIRATION_MS)
    {
        gcTimer.Reset();

        for (auto jt = uiState_.Begin(); jt != uiState_.End();)
        {
            if (jt->second_.IsExpired())
            {
                jt->second_.Unset();
                jt = uiState_.Erase(jt);
            }
            else
                ++jt;
        }
    }

    return result;
}

void ExpireUIStateP()
{
    auto it = uiState_.Find(ui::GetCurrentWindow()->IDStack.back());
    if (it != uiState_.End())
    {
        it->second_.Unset();
        uiState_.Erase(it);
    }
}

int DoubleClickSelectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
    bool wasSelected = p_selected && *p_selected;
    if (ui::Selectable(label, p_selected, flags | ImGuiSelectableFlags_AllowDoubleClick, size))
    {
        if (wasSelected && ui::IsMouseDoubleClicked(0))
            return 2;
        else
            return 1;
    }
    return 0;
}

int DoubleClickSelectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
    return DoubleClickSelectable(label, &selected, flags, size);
}

bool DroppedOnItem()
{
    auto context = Urho3D::Context::GetContext();
    return ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && context->GetSystemUI()->HasDragData() && !ui::IsMouseDown(0);
}

bool CollapsingHeaderSimple(const char* label, ImGuiTreeNodeFlags flags)
{
    ImGuiWindow* window = ui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ui::PushStyleColor(ImGuiCol_HeaderActive, 0);
    ui::PushStyleColor(ImGuiCol_HeaderHovered, 0);
    ui::PushStyleColor(ImGuiCol_Header, 0);
    bool open = ui::TreeNodeBehavior(window->GetID(label), flags | ImGuiTreeNodeFlags_NoAutoOpenOnLog | ImGuiTreeNodeFlags_NoTreePushOnOpen, label);
    ui::PopStyleColor(3);
    return open;
}

float ScaleX(float x)
{
    return x * ui::GetIO().DisplayFramebufferScale.x;
}

float ScaleY(float y)
{
    return y * ui::GetIO().DisplayFramebufferScale.y;
}

ImVec2 Scale(ImVec2 value)
{
    return value * ui::GetIO().DisplayFramebufferScale;
}

bool ToolbarButton(const char* label)
{
    auto& g = *ui::GetCurrentContext();
    float dimension = g.FontBaseSize + g.Style.FramePadding.y * 2.0f;
    return ui::ButtonEx(label, {dimension, dimension}, ImGuiButtonFlags_PressedOnClick);
}

void SetHelpTooltip(const char* text)
{
    if (ui::IsItemHovered() && ui::IsKeyDown(SDL_SCANCODE_LALT))
        ui::SetTooltip(text);
}

}
