//
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

#include "Widgets.h"
#include <ImGui/imgui_internal.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/SystemUI/SystemUI.h>

using namespace Urho3D;

namespace ImGui
{

int DoubleClickSelectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
    if (ui::Selectable(label, p_selected, flags | ImGuiSelectableFlags_AllowDoubleClick, size))
    {
        if (ui::IsMouseDoubleClicked(MOUSEB_LEFT))
            return 2;
        else
            return 1;
    }
    if (ui::IsItemHovered() && ui::IsMouseClicked(MOUSEB_RIGHT))
    {
        *p_selected = true;
        return 1;
    }
    return 0;
}

int DoubleClickSelectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
    return DoubleClickSelectable(label, &selected, flags, size);
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

bool ToolbarButton(const char* label)
{
    auto& g = *ui::GetCurrentContext();
    float dimension = g.FontSize + g.Style.FramePadding.y * 2.0f;
    return ui::ButtonEx(label, {dimension, dimension}, ImGuiButtonFlags_PressedOnClick);
}

void SetHelpTooltip(const char* text, Key requireKey)
{
    unsigned scancode = requireKey & (~SDLK_SCANCODE_MASK);
    if (ui::IsItemHovered() && (requireKey == KEY_UNKNOWN || ui::IsKeyDown(scancode)))
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
    const ImGuiStyle& style = ui::GetStyle();
    ImVec2 pos = ui::GetCursorPos();

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
            if (ui::Button("", {8, 9}))
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
            ui::SetCursorPos({pos.x, pos.y + 9});
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

URHO3D_FLAGSET(TransformResizeType, TransformResizeTypeFlags);

}

namespace ImGui
{

bool TransformRect(ImRect& inOut, TransformSelectorFlags flags)
{
    ImRect delta;
    return TransformRect(inOut, delta, flags);
}

bool TransformRect(ImRect& inOut, ImRect& delta, TransformSelectorFlags flags)
{
    struct State
    {
        /// A flag indicating type of resize action currently in progress
        TransformResizeTypeFlags resizing_ = RESIZE_NONE;
    };

    auto renderHandle = [&](ImVec2 screenPos, float wh) -> bool {
        ImRect rect(
            screenPos.x - wh / 2.0f,
            screenPos.y - wh / 2.0f,
            screenPos.x + wh / 2.0f,
            screenPos.y + wh / 2.0f
        );

        if (!(flags & TSF_HIDEHANDLES))
        {
            ui::GetWindowDrawList()->AddRectFilled(rect.Min, rect.Max, ui::GetColorU32(ToImGui(Color::RED)));
        }

        const ImGuiIO& io = ui::GetIO();
        return rect.Contains(io.MousePos);
    };

    auto size = inOut.GetSize();
    auto handleSize = Max(Min(Min(size.x / 4, size.y / 4), 8), 2);
    bool modified = false;

    auto* s = ui::GetUIState<State>();
    auto id = ui::GetID(s);

    // Extend rect to cover resize handles that are sticking out of ui element boundaries.
    auto extendedRect = inOut + ImRect(-handleSize / 2, -handleSize / 2, handleSize / 2, handleSize / 2);
    ui::ItemSize(inOut);
    if (ui::ItemAdd(extendedRect, id))
    {
        TransformResizeTypeFlags resizing = RESIZE_NONE;
        if (renderHandle(inOut.Min + size / 2, handleSize))
            resizing = RESIZE_MOVE;

        bool canResizeHorizontal = !(flags & TSF_NOHORIZONTAL);
        bool canResizeVertical = !(flags & TSF_NOVERTICAL);

        if (canResizeHorizontal && canResizeVertical)
        {
            if (renderHandle(inOut.Min, handleSize))
                resizing = RESIZE_LEFT | RESIZE_TOP;
            if (renderHandle(inOut.Min + ImVec2(0, size.y), handleSize))
                resizing = RESIZE_LEFT | RESIZE_BOTTOM;
            if (renderHandle(inOut.Min + ImVec2(size.x, 0), handleSize))
                resizing = RESIZE_TOP | RESIZE_RIGHT;
            if (renderHandle(inOut.Max, handleSize))
                resizing = RESIZE_BOTTOM | RESIZE_RIGHT;
        }

        if (canResizeHorizontal)
        {
            if (renderHandle(inOut.Min + ImVec2(0, size.y / 2), handleSize))
                resizing = RESIZE_LEFT;
            if (renderHandle(inOut.Min + ImVec2(size.x, size.y / 2), handleSize))
                resizing = RESIZE_RIGHT;
        }

        if (canResizeVertical)
        {
            if (renderHandle(inOut.Min + ImVec2(size.x / 2, 0), handleSize))
                resizing = RESIZE_TOP;
            if (renderHandle(inOut.Min + ImVec2(size.x / 2, size.y), handleSize))
                resizing = RESIZE_BOTTOM;
        }

        // Draw rect around selected element
        ui::GetWindowDrawList()->AddRect(inOut.Min, inOut.Max, ui::GetColorU32(ToImGui(Color::RED)));

        // Set mouse cursor if handle is hovered or if we are resizing
        if (resizing & RESIZE_TOP && resizing & RESIZE_LEFT && resizing & RESIZE_BOTTOM && resizing & RESIZE_RIGHT)
            ui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        else if ((resizing & RESIZE_TOP && resizing & RESIZE_RIGHT) || (resizing & RESIZE_BOTTOM && resizing & RESIZE_LEFT))
            ui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
        else if ((resizing & RESIZE_TOP && resizing & RESIZE_LEFT) || (resizing & RESIZE_BOTTOM && resizing & RESIZE_RIGHT))
            ui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        else if (resizing & RESIZE_LEFT || resizing & RESIZE_RIGHT)
            ui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        else if (resizing & RESIZE_TOP || resizing & RESIZE_BOTTOM)
            ui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

        // Prevent interaction when something else blocks inactive transform.
        if (s->resizing_ != RESIZE_NONE || (ui::IsItemHovered(ImGuiHoveredFlags_RectOnly) &&
            (!ui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ui::IsWindowHovered())))
        {
            // Begin resizing
            if (ui::IsMouseClicked(0))
                s->resizing_ = resizing;

            ImVec2 d = ui::GetIO().MouseDelta;
            if (s->resizing_ != RESIZE_NONE)
            {
                ui::SetActiveID(id, ui::GetCurrentWindow());
                if (!ui::IsMouseDown(0))
                    s->resizing_ = RESIZE_NONE;
                else if (d != ImVec2(0, 0))
                {
                    delta = ImRect(0, 0, 0, 0);

                    if (s->resizing_ == RESIZE_MOVE)
                    {
                        delta.Min.x += d.x;
                        delta.Max.x += d.x;
                        delta.Min.y += d.y;
                        delta.Max.y += d.y;
                        modified = true;
                    }
                    else
                    {
                        if (s->resizing_ & RESIZE_LEFT)
                        {
                            delta.Min.x += d.x;
                            modified = true;
                        }
                        else if (s->resizing_ & RESIZE_RIGHT)
                        {
                            delta.Max.x += d.x;
                            modified = true;
                        }

                        if (s->resizing_ & RESIZE_TOP)
                        {
                            delta.Min.y += d.y;
                            modified = true;
                        }
                        else if (s->resizing_ & RESIZE_BOTTOM)
                        {
                            delta.Max.y += d.y;
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

void OpenTreeNode(ImGuiID id)
{
    auto& storage = ui::GetCurrentWindow()->DC.StateStorage;
    if (!storage->GetInt(id))
    {
        storage->SetInt(id, true);
        ui::TreePushOverrideID(id);
    }
}

void BeginButtonGroup()
{
    auto* storage = ui::GetStateStorage();
    auto* lists = ui::GetWindowDrawList();
    ImVec2 pos = ui::GetCursorScreenPos();
    storage->SetFloat(ui::GetID("button-group-x"), pos.x);
    storage->SetFloat(ui::GetID("button-group-y"), pos.y);
    lists->ChannelsSplit(2);
    lists->ChannelsSetCurrent(1);
}

void EndButtonGroup()
{
    auto& style = ui::GetStyle();
    auto* lists = ui::GetWindowDrawList();
    auto* storage = ui::GetStateStorage();
    ImVec2 min(
        storage->GetFloat(ui::GetID("button-group-x")),
        storage->GetFloat(ui::GetID("button-group-y"))
              );
    lists->ChannelsSetCurrent(0);
    lists->AddRectFilled(min, ui::GetItemRectMax(), ImColor(style.Colors[ImGuiCol_Button]), style.FrameRounding);
    lists->ChannelsMerge();
}

void TextElided(const char* text, float width)
{
    float x = ui::GetCursorPosX();
    if (ui::CalcTextSize(text).x <= width)
    {
        ui::TextUnformatted(text);
        ui::SameLine(0, 0);
        ui::SetCursorPosX(x + width);
        ui::NewLine();
        return;
    }
    else
    {
        float w = ui::CalcTextSize("...").x;
        for (const char* c = text; *c; c++)
        {
            w += ui::CalcTextSize(c, c + 1).x;
            if (w >= width)
            {
                ui::TextUnformatted(text, c > text ? c - 1 : c);
                ui::SameLine(0, 0);
                ui::TextUnformatted("...");
                ui::SameLine(0, 0);
                ui::SetCursorPosX(x + width);
                ui::NewLine();
                return;
            }
        }
    }
    ui::SetCursorPosX(x + width);
    ui::NewLine();
}

}
