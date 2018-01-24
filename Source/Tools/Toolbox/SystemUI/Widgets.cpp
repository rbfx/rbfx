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
#include <Urho3D/Input/Input.h>
#include <SDL/SDL_scancode.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <ThirdParty/SDL/include/SDL_scancode.h>

using namespace Urho3D;

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
    Timer timer_;
};

HashMap<ImGuiID, UIStateWrapper> uiState_;

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
    static Timer gcTimer;
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
    return ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ui::GetSystemUI()->HasDragData() && !ui::IsMouseDown(0);
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
        ui::SetTooltip("%s", text);
}

bool IconButton(const char* label)
{
    float size = ui::GetItemRectSize().y;
    return ui::Button(label, {size, size});
}

bool MaskSelector(unsigned int* mask)
{
    bool modified = false;
    auto style = ui::GetStyle();
    auto pos = ui::GetCursorPos();

    for (auto row = 0; row < 2; row++)
    {
        for (auto col = 0; col < 16; col++)
        {
            auto bitPosition = row * 16 + col;
            int bitMask = 1 << bitPosition;
            bool selected = (*mask & bitMask) != 0;
            if (selected)
            {
                ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
                ui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_ButtonActive]);
            }
            else
            {
                ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
                ui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_Button]);
            }

            ui::PushID(bitMask);
            if (ui::Button("", ui::Scale({8, 9})))
            {
                modified = true;
                *mask ^= bitMask;
            }
            if (ui::IsItemHovered())
                ui::SetTooltip("%d", bitPosition);
            ui::PopID();
            ui::SameLine(0, 0);
            ui::PopStyleColor(2);
        }
        ui::NewLine();
        if (row < 1)
            ui::SetCursorPos({pos.x, pos.y + ui::ScaleY(9)});
    }

    return modified;
}

enum TransformResizeType
{
    RESIZE_NONE = 0,
    RESIZE_LEFT = 1,
    RESIZE_RIGHT = 2,
    RESIZE_TOP = 4,
    RESIZE_BOTTOM = 8,
    RESIZE_MOVE = 15,
};

/// Flag manipuation operators.
URHO3D_TO_FLAGS_ENUM(TransformResizeType);
/// Hashing function which enables use of enum type as a HashMap key.
inline unsigned MakeHash(const TransformResizeType& value) { return value; }

bool TransformRect(IntRect& inOut, TransformSelectorFlags flags)
{
    IntRect delta;
    return TransformRect(inOut, delta, flags);
}

bool TransformRect(Urho3D::IntRect& inOut, Urho3D::IntRect& delta, TransformSelectorFlags flags)
{
    struct State
    {
        /// A flag indicating type of resize action currently in progress
        TransformResizeType resizing_ = RESIZE_NONE;
        /// A cache of system cursors
        HashMap<TransformResizeType, SDL_Cursor*> cursors_;
        /// Default cursor shape
        SDL_Cursor* cursorArrow_;
        /// Flag indicating that this selector set cursor handle
        bool ownsCursor_ = false;

        State()
        {
            cursors_[RESIZE_MOVE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
            cursors_[RESIZE_LEFT] = cursors_[RESIZE_RIGHT] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
            cursors_[RESIZE_BOTTOM] = cursors_[RESIZE_TOP] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
            cursors_[RESIZE_TOP | RESIZE_LEFT] = cursors_[RESIZE_BOTTOM | RESIZE_RIGHT] = SDL_CreateSystemCursor(
                SDL_SYSTEM_CURSOR_SIZENWSE);
            cursors_[RESIZE_TOP | RESIZE_RIGHT] = cursors_[RESIZE_BOTTOM | RESIZE_LEFT] = SDL_CreateSystemCursor(
                SDL_SYSTEM_CURSOR_SIZENESW);
            cursorArrow_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        }

        ~State()
        {
            SDL_FreeCursor(cursorArrow_);
            for (const auto& it : cursors_)
                SDL_FreeCursor(it.second_);
        }
    };

    Input* input = ui::GetSystemUI()->GetSubsystem<Input>();

    auto renderHandle = [&](IntVector2 screenPos, int wh) -> bool {
        IntRect rect(
            screenPos.x_ - wh / 2,
            screenPos.y_ - wh / 2,
            screenPos.x_ + wh / 2,
            screenPos.y_ + wh / 2
        );

        if (!(flags & TSF_HIDEHANDLES))
        {
            ui::GetWindowDrawList()->AddRectFilled(ToImGui(rect.Min()), ToImGui(rect.Max()),
                ui::GetColorU32(ToImGui(Color::RED)));
        }

        return rect.IsInside(input->GetMousePosition()) == INSIDE;
    };

    auto size = inOut.Size();
    auto handleSize = Max(Min(Min(size.x_ / 4, size.y_ / 4), 8), 2);
    bool modified = false;

    State* s = ui::GetUIState<State>();
    auto id = ui::GetID(s);

    // Extend rect to cover resize handles that are sticking out of ui element boundaries.
    auto extendedRect = inOut + IntRect(-handleSize / 2, -handleSize / 2, handleSize / 2, handleSize / 2);
    ui::ItemSize(ToImGui(inOut));
    if (ui::ItemAdd(ToImGui(extendedRect), id))
    {
        TransformResizeType resizing = RESIZE_NONE;
        if (renderHandle(inOut.Min() + size / 2, handleSize))
            resizing = RESIZE_MOVE;

        bool canResizeHorizontal = !(flags & TSF_NOHORIZONTAL);
        bool canResizeVertical = !(flags & TSF_NOVERTICAL);

        if (canResizeHorizontal && canResizeVertical)
        {
            if (renderHandle(inOut.Min(), handleSize))
                resizing = RESIZE_LEFT | RESIZE_TOP;
            if (renderHandle(inOut.Min() + IntVector2(0, size.y_), handleSize))
                resizing = RESIZE_LEFT | RESIZE_BOTTOM;
            if (renderHandle(inOut.Min() + IntVector2(size.x_, 0), handleSize))
                resizing = RESIZE_TOP | RESIZE_RIGHT;
            if (renderHandle(inOut.Max(), handleSize))
                resizing = RESIZE_BOTTOM | RESIZE_RIGHT;
        }

        if (canResizeHorizontal)
        {
            if (renderHandle(inOut.Min() + IntVector2(0, size.y_ / 2), handleSize))
                resizing = RESIZE_LEFT;
            if (renderHandle(inOut.Min() + IntVector2(size.x_, size.y_ / 2), handleSize))
                resizing = RESIZE_RIGHT;
        }

        if (canResizeVertical)
        {
            if (renderHandle(inOut.Min() + IntVector2(size.x_ / 2, 0), handleSize))
                resizing = RESIZE_TOP;
            if (renderHandle(inOut.Min() + IntVector2(size.x_ / 2, size.y_), handleSize))
                resizing = RESIZE_BOTTOM;
        }

        // Draw rect around selected element
        ui::GetWindowDrawList()->AddRect(ToImGui(inOut.Min()), ToImGui(inOut.Max()),
            ui::GetColorU32(ToImGui(Color::RED)));

        // Reset mouse cursor if we are not hovering any handle and are not resizing
        if (resizing == RESIZE_NONE && s->resizing_ == RESIZE_NONE && s->ownsCursor_)
        {
            SDL_SetCursor(s->cursorArrow_);
            s->ownsCursor_ = false;
        }

        // Prevent interaction when something else blocks inactive transform.
        if (s->resizing_ != RESIZE_NONE || (ui::IsItemHovered(ImGuiHoveredFlags_RectOnly) &&
            (!ui::IsAnyWindowHovered() || ui::IsWindowHovered())))
        {
            // Set mouse cursor if handle is hovered or if we are resizing
            if (resizing != RESIZE_NONE && !s->ownsCursor_)
            {
                SDL_SetCursor(s->cursors_[resizing]);
                s->ownsCursor_ = true;
            }

            // Begin resizing
            if (ui::IsMouseClicked(0))
                s->resizing_ = resizing;

            IntVector2 d = ToIntVector2(ui::GetIO().MouseDelta);
            if (s->resizing_ != RESIZE_NONE)
            {
                ui::SetActiveID(id, ui::GetCurrentWindow());
                if (!ui::IsMouseDown(0))
                    s->resizing_ = RESIZE_NONE;
                else if (d != IntVector2::ZERO)
                {
                    delta = IntRect::ZERO;

                    if (s->resizing_ == RESIZE_MOVE)
                    {
                        delta.left_ += d.x_;
                        delta.right_ += d.x_;
                        delta.top_ += d.y_;
                        delta.bottom_ += d.y_;
                        modified = true;
                    }
                    else
                    {
                        if (s->resizing_ & RESIZE_LEFT)
                        {
                            delta.left_ += d.x_;
                            modified = true;
                        }
                        else if (s->resizing_ & RESIZE_RIGHT)
                        {
                            delta.right_ += d.x_;
                            modified = true;
                        }

                        if (s->resizing_ & RESIZE_TOP)
                        {
                            delta.top_ += d.y_;
                            modified = true;
                        }
                        else if (s->resizing_ & RESIZE_BOTTOM)
                        {
                            delta.bottom_ += d.y_;
                            modified = true;
                        }
                    }
                }
            }
            else if (ui::IsItemActive())
                ui::SetActiveID(0, ui::GetCurrentWindow());

            if (modified)
                inOut += delta;
        }
        else if (ui::IsItemActive())
            ui::SetActiveID(0, ui::GetCurrentWindow());
    }
    return modified;
}

SystemUI* GetSystemUI()
{
    return static_cast<SystemUI*>(ui::GetIO().UserData);
}

ImVec2 GetPixelPerfectDPIScale()
{
    auto nearestPowerOf2 = [](int num) -> int {
        int n = 1;
        while (n < num)
            n <<= 1;
        return n;
    };
    ImVec2 ppScale = ui::GetIO().DisplayFramebufferScale;   // DPI scale
    ppScale.x = nearestPowerOf2(RoundToInt(ppScale.x));
    ppScale.y = nearestPowerOf2(RoundToInt(ppScale.y));
    return ppScale;
}

bool EditorToolbarButton(const char* text, const char* tooltip, bool active)
{
    const auto& style = ui::GetStyle();
    if (active)
        ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    else
        ui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
    bool result = ui::ToolbarButton(text);
    ui::PopStyleColor();
    ui::SameLine(0, 0);
    if (ui::IsItemHovered() && tooltip)
        ui::SetTooltip("%s", tooltip);
    return result;
}

}
