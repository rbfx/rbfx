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
#pragma once

#include "../Core/Variant.h"
#include "../Graphics/Texture2D.h"
#include "../SystemUI/ImGui.h"

#include <EASTL/optional.h>

namespace Urho3D
{

namespace Widgets
{

/// Return best size of small square button with one icon.
URHO3D_API float GetSmallButtonSize();
/// Render toolbar button with optional tooltip. May be toggled on.
URHO3D_API bool ToolbarButton(const char* label, const char* tooltip = nullptr, bool active = false);
/// Render a bit of space in toolbar between buttons.
URHO3D_API void ToolbarSeparator();
/// Render a label for next item. Label may be on the left or on the right, depending on flags.
URHO3D_API void ItemLabel(ea::string_view title, const ea::optional<Color>& color = ea::nullopt, bool isLeft = true);

}

}

// TODO(editor): Remove or move up
namespace ImGui
{

URHO3D_API bool SetDragDropVariant(const ea::string& types, const Urho3D::Variant& variant, ImGuiCond cond = 0);
URHO3D_API const Urho3D::Variant& AcceptDragDropVariant(const ea::string& type, ImGuiDragDropFlags flags = 0);
URHO3D_API void Image(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
/// Render an image which is also an item that can be activated.
URHO3D_API void ImageItem(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
URHO3D_API bool ImageButton(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4 & bg_col = ImVec4(0, 0, 0, 0), const ImVec4 & tint_col = ImVec4(1, 1, 1, 1));
/// Activate last item if specified mouse button is pressed and held over it, deactivate when released.
URHO3D_API bool ItemMouseActivation(Urho3D::MouseButton button, unsigned flags = ImGuiItemMouseActivation_Click);
URHO3D_API void HideCursorWhenActive(Urho3D::MouseButton button, bool on_drag = false);

}
