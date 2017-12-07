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

#pragma once


#include <typeinfo>
#include <ImGui/imgui.h>


namespace ImGui
{

/// Set custom user pointer storing UI state at given position of id stack. Optionally pass deleter function which is
/// responsible for freeing state object when it is no longer used.
void SetUIStateP(void* state, void(* deleter)(void*) = nullptr);
/// Get custom user pointer storing UI state at given position of id stack. If this function is not called for 30s or
/// longer then state will expire and will be removed.
void* GetUIStateP();
/// Expire custom ui state at given position if id stack, created with SetUIStateP(). It will be freed immediately.
void ExpireUIStateP();
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
int DoubleClickSelectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0,0));
/// Same as Selectable(), except returns 1 when clicked once, 2 when double-clicked, 0 otherwise.
int DoubleClickSelectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0,0));
/// Return true of content was dragged and dropped on last item.
bool DroppedOnItem();
/// Same as ImGui::CollapsingHeader(), except does not draw a frame and background.
bool CollapsingHeaderSimple(const char* label, ImGuiTreeNodeFlags flags=0);
/// Return value scaled according to DisplayFramebufferScale.
float ScaleX(float x);
/// Return value scaled according to DisplayFramebufferScale.
float ScaleY(float y);
/// Return value scaled according to DisplayFramebufferScale.
ImVec2 Scale(ImVec2 value);
/// A button that perfectly fits in menu bar.
bool ToolbarButton(const char* label);
/// Display help tooltip when alt is pressed.
void SetHelpTooltip(const char* text);
/// A square button whose width and height are equal to the height of previous item.
bool IconButton(const char* label);
/// Draw a mask selector widget.
bool MaskSelector(unsigned int* mask);

}
