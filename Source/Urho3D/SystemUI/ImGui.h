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

#include "../Input/InputConstants.h"
#include "../Math/Color.h"
#include "../Math/Matrix4.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Math/Vector4.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_stdlib.h>

namespace ui = ImGui;

namespace Urho3D
{

class Texture2D;

/// Convert math types from/to ImGUI
/// @{
inline ImVec2 ToImGui(const Vector2& value) { return {value.x_, value.y_}; };
inline ImVec2 ToImGui(const IntVector2& value) { return {static_cast<float>(value.x_), static_cast<float>(value.y_)}; };
inline ImVec4 ToImGui(const Vector4& value) { return {value.x_, value.y_, value.z_, value.w_}; }
inline ImVec4 ToImGui(const Color& value) { return {value.r_, value.g_, value.b_, value.a_}; }
inline ImRect ToImGui(const IntRect& rect) { return { ToImGui(rect.Min()), ToImGui(rect.Max()) }; }

inline Vector2 ToVector2(const ImVec2& value) { return {value.x, value.y}; }
inline Vector4 ToVector4(const ImVec4& value) { return {value.x, value.y, value.z, value.w}; }
inline Color ToColor(const ImVec4& value) { return {value.x, value.y, value.z, value.w}; }
inline IntVector2 ToIntVector2(const ImVec2& value) { return {RoundToInt(value.x), RoundToInt(value.y)}; }
inline IntRect ToIntRect(const ImRect& value) { return {ToIntVector2(value.Min), ToIntVector2(value.Max)}; }
/// @}

/// State guards for paired calls.
/// @{
class IdScopeGuard
{
public:
    template <class T>
    explicit IdScopeGuard(const T& id)
    {
        ui::PushID(id);
    }

    ~IdScopeGuard()
    {
        ui::PopID();
    }
};

class ColorScopeGuard
{
public:
    ColorScopeGuard(ImGuiCol id, const ImVec4& color, bool enabled = true)
        : numColors_(enabled ? 1 : 0)
    {
        if (enabled)
            ui::PushStyleColor(id, color);
    }

    explicit ColorScopeGuard(std::initializer_list<ea::pair<ImGuiCol, ImVec4>> colors, bool enabled = true)
        : numColors_(enabled ? static_cast<unsigned>(colors.size()) : 0)
    {
        if (enabled)
        {
            for (const auto& [id, color] : colors)
                ui::PushStyleColor(id, color);
        }
    }

    ColorScopeGuard(ImGuiCol id, ImU32 color, bool enabled = true)
        : numColors_(enabled ? 1 : 0)
    {
        if (enabled)
            ui::PushStyleColor(id, color);
    }

    explicit ColorScopeGuard(std::initializer_list<ea::pair<ImGuiCol, ImU32>> colors, bool enabled = true)
        : numColors_(enabled ? static_cast<unsigned>(colors.size()) : 0)
    {
        if (enabled)
        {
            for (const auto& [id, color] : colors)
                ui::PushStyleColor(id, color);
        }
    }

    ColorScopeGuard(ImGuiCol id, const Color& color, bool enabled = true)
        : numColors_(enabled ? 1 : 0)
    {
        if (enabled)
            ui::PushStyleColor(id, ToImGui(color));
    }

    explicit ColorScopeGuard(std::initializer_list<ea::pair<ImGuiCol, Color>> colors, bool enabled = true)
        : numColors_(enabled ? static_cast<unsigned>(colors.size()) : 0)
    {
        if (enabled)
        {
            for (const auto& [id, color] : colors)
                ui::PushStyleColor(id, ToImGui(color));
        }
    }

    ~ColorScopeGuard()
    {
        ui::PopStyleColor(numColors_);
    }

private:
    const unsigned numColors_{};
};
/// @}

/// Convert engine texture to ImTextureID.
ImTextureID ToImTextureID(Texture2D* texture);

}

namespace ImGui
{

/// ImGui wrappers for Urho3D types.
/// @{
URHO3D_API bool IsMouseDown(Urho3D::MouseButton buttons);
URHO3D_API bool IsMouseDoubleClicked(Urho3D::MouseButton buttons);
URHO3D_API bool IsMouseDragPastThreshold(Urho3D::MouseButton buttons, float lockThreshold = -1.0f);
URHO3D_API bool IsMouseReleased(Urho3D::MouseButton buttons);
URHO3D_API bool IsMouseClicked(Urho3D::MouseButton buttons, bool repeat = false);
URHO3D_API bool IsItemClicked(Urho3D::MouseButton buttons);

URHO3D_API ImVec2 GetMouseDragDelta(Urho3D::MouseButton button, float lockThreshold = -1.0f);
URHO3D_API void ResetMouseDragDelta(Urho3D::MouseButton button);

URHO3D_API bool IsKeyDown(Urho3D::Key key);
URHO3D_API bool IsKeyPressed(Urho3D::Key key, bool repeat = true);
URHO3D_API bool IsKeyReleased(Urho3D::Key key);
URHO3D_API int GetKeyPressedAmount(Urho3D::Key key, float repeatDelay, float rate);
URHO3D_API float GetMouseWheel();
/// @}

}

/// ImGui math helpers.
/// @{
inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator!=(const ImVec2& lhs, const ImVec2& rhs) { return !(lhs == rhs); }
inline bool operator==(const ImRect& lhs, const ImRect& rhs) { return lhs.Min == rhs.Min && lhs.Max == rhs.Max; }
inline bool operator!=(const ImRect& lhs, const ImRect& rhs) { return !(lhs == rhs); }

inline ImRect operator+(const ImRect& lhs, const ImRect& rhs) { return ImRect(lhs.Min + rhs.Min, lhs.Max + rhs.Max); }
inline ImRect& operator+=(ImRect& lhs, const ImRect& rhs) { lhs.Min += rhs.Min; lhs.Max += rhs.Max; return lhs; }
inline ImRect operator/(const ImRect& lhs, float rhs) { return ImRect(lhs.Min / rhs, lhs.Max / rhs); }
inline ImRect& operator/=(ImRect& lhs, float rhs) { lhs.Min /= rhs; lhs.Max /= rhs; return lhs; }
inline ImRect& operator*=(ImRect& lhs, float rhs) { lhs.Min *= rhs; lhs.Max *= rhs; return lhs; }

inline ImRect ImRound(const ImRect& r) { return ImRect({Urho3D::Round(r.Min.x), Urho3D::Round(r.Min.y)}, {Urho3D::Round(r.Max.x), Urho3D::Round(r.Max.y)}); };
/// @}
