//
// Copyright (c) 2017-2019 the rbfx project.
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

#pragma once

#include "ToolboxAPI.h"
#include <typeinfo>
#include <ImGui/imgui.h>
#include <Urho3D/Math/Rect.h>
#include <Urho3D/Core/Macros.h>
#include "Urho3D/SystemUI/SystemUI.h"


namespace ImGui
{

enum TransformSelector
{
    TSF_NONE = 0,
    TSF_NOHORIZONTAL = 1,
    TSF_NOVERTICAL = 2,
    TSF_HIDEHANDLES = 4,
};

}

URHO3D_FLAGSET_EX(ImGui, TransformSelector, TransformSelectorFlags);

namespace ImGui
{
/// Helper for running `for` loop just once.
struct ScopeHelper
{
    explicit operator bool()
    {
        check_ ^= true;
        return check_;
    }

    bool check_ = false;
};

/// Keeps Track of ui indentation
struct IndentScope : ScopeHelper
{
    explicit IndentScope(float len)
    {
        len_ = len;
        ImGui::Indent(len);
    }

    ~IndentScope()
    {
        ImGui::Unindent(len_);
    }

    float len_ = 0.f;
};

/// Keeps Track of ui item width
struct ItemWidthScope : ScopeHelper
{
    explicit ItemWidthScope(float len)
    {
        ImGui::PushItemWidth(len);
    }

    ~ItemWidthScope()
    {
        ImGui::PopItemWidth();
    }
};

/// Keeps Track of ui stype vars
struct StyleVarScope : ScopeHelper
{
    explicit StyleVarScope(ImGuiStyleVar var, float value)
    {
        ImGui::PushStyleVar(var, value);
    }

    explicit StyleVarScope(ImGuiStyleVar var, const ImVec2& value)
    {
        ImGui::PushStyleVar(var, value);
    }

    ~StyleVarScope()
    {
        ImGui::PopStyleVar();
    }
};

/// Keeps track of id scopes
struct IdScope : ScopeHelper
{
    template<typename T>
    explicit IdScope(T id)
    {
        ImGui::PushID(id);
    }

    ~IdScope()
    {
        ImGui::PopID();
    }
};

/// Goes up in id scope by specified number and restores id scope on exit.
struct IdScopeGoUp : ScopeHelper
{
    explicit IdScopeGoUp(int count)
    {
        ids_.reserve(count);
        auto* window = ImGui::GetCurrentWindow();
        while (count-- > 0)
        {
            ids_.push_back(window->IDStack.back());
            window->IDStack.pop_back();
        }
    }

    ~IdScopeGoUp()
    {
        auto* window = ImGui::GetCurrentWindow();
        while (!ids_.empty())
        {
            window->IDStack.push_back(ids_.back());
            ids_.pop_back();
        }
    }

    ImVector<ImGuiID> ids_;
};

#define UI_INDENT(len) for (ImGui::IndentScope ___indenter_bf655352_0811_4629_b929_051958ca8855##__LINE__(len); static_cast<bool>(___indenter_bf655352_0811_4629_b929_051958ca8855##__LINE__);)
#define UI_ITEMWIDTH(width) for (ImGui::ItemWidthScope ___widthscope_4c0ffd93_e615_43c9_a58e_da6db01d5bb8##__LINE__(width); static_cast<bool>(___widthscope_4c0ffd93_e615_43c9_a58e_da6db01d5bb8##__LINE__);)
#define UI_STYLEVAR(var, ...) for (ImGui::StyleVarScope ___stylevarscope_0652d51d_6ff6_4deb_a1ba_841bbc684e3a##__LINE__(var, __VA_ARGS__); static_cast<bool>(___stylevarscope_0652d51d_6ff6_4deb_a1ba_841bbc684e3a##__LINE__);)
#define UI_ID(id) for (ImGui::IdScope ___idscope_bef482cf_157b_47e9_b8da_0f3d82a7ab3e##__LINE__(id); static_cast<bool>(___idscope_bef482cf_157b_47e9_b8da_0f3d82a7ab3e##__LINE__);)
#define UI_UPIDSCOPE(id) for (ImGui::IdScopeGoUp ___idscopeup_b0614a09_2df3_4258_b1d5_ee0daafcd388##__LINE__(id); static_cast<bool>(___idscopeup_b0614a09_2df3_4258_b1d5_ee0daafcd388##__LINE__);)

/// Set custom user pointer storing UI state at given position of id stack. Optionally pass deleter function which is
/// responsible for freeing state object when it is no longer used.
URHO3D_TOOLBOX_API void SetUIStateP(void* state, void(* deleter)(void*) = nullptr);
/// Get custom user pointer storing UI state at given position of id stack. If this function is not called for 30s or
/// longer then state will expire and will be removed.
URHO3D_TOOLBOX_API void* GetUIStateP();
/// Expire custom ui state at given position if id stack, created with SetUIStateP(). It will be freed immediately.
URHO3D_TOOLBOX_API void ExpireUIStateP();
/// Get custom user iu state at given position of id stack. If state does not exist then state object will be created.
/// Using different type at the same id stack position will return new object of that type. Arguments passed to this
/// function will be passed to constructor of type T.
template<typename T, typename... Args>
T* GetUIState(Args... args)
{
    ImGui::PushID(typeid(T).name());
    T* state = (T*)GetUIStateP();
    if (state == nullptr)
    {
        state = new T(args...);
        SetUIStateP(state, [](void* s) { delete (T*)s; });
    }
    ImGui::PopID();
    return state;
}
/// Expire custom ui state at given position if id stack, created with GetUIState<T>. It will be freed immediately.
template<typename T>
void ExpireUIState()
{
    ImGui::PushID(typeid(T).name());
    ExpireUIStateP();
    ImGui::PopID();
}
/// Same as Selectable(), except returns 1 when clicked once, 2 when double-clicked, 0 otherwise.
URHO3D_TOOLBOX_API int DoubleClickSelectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0,0));
/// Same as Selectable(), except returns 1 when clicked once, 2 when double-clicked, 0 otherwise.
URHO3D_TOOLBOX_API int DoubleClickSelectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0,0));
/// Same as ImGui::CollapsingHeader(), except does not draw a frame and background.
URHO3D_TOOLBOX_API bool CollapsingHeaderSimple(const char* label, ImGuiTreeNodeFlags flags=0);
/// A button that perfectly fits in menu bar.
URHO3D_TOOLBOX_API bool ToolbarButton(const char* label);
/// Display help tooltip when alt is pressed.
URHO3D_TOOLBOX_API void SetHelpTooltip(const char* text, Urho3D::Key requireKey = Urho3D::KEY_ALT);
/// A square button whose width and height are equal to the height of previous item.
URHO3D_TOOLBOX_API bool IconButton(const char* label);
/// Draw a mask selector widget.
URHO3D_TOOLBOX_API bool MaskSelector(unsigned int* mask);
/// Draw a transform rect and allow it's modification by dragging handles with mouse.
URHO3D_TOOLBOX_API bool TransformRect(Urho3D::IntRect& inOut, TransformSelectorFlags flags = TSF_NONE);
/// Draw a transform rect and allow it's modification by dragging handles with mouse.
URHO3D_TOOLBOX_API bool TransformRect(Urho3D::IntRect& inOut, Urho3D::IntRect& delta, TransformSelectorFlags flags = TSF_NONE);
/// Return current SystemUI instance.
URHO3D_TOOLBOX_API Urho3D::SystemUI* GetSystemUI();
/// Create a optionally toggleable toolbar button with a tooltip.
URHO3D_TOOLBOX_API bool EditorToolbarButton(const char* text, const char* tooltip=nullptr, bool active=false);
/// Opens a tree node which was rendered already.
URHO3D_TOOLBOX_API void OpenTreeNode(ImGuiID id);

}
