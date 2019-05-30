//
// Copyright (c) 2008-2017 the Urho3D project.
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


#include <EASTL/unordered_map.h>

#include "Urho3D/Core/Object.h"
#include "Urho3D/Math/StringHash.h"
#include "Urho3D/Graphics/VertexBuffer.h"
#include "Urho3D/Graphics/IndexBuffer.h"
#include "Urho3D/Math/Matrix4.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/Input/InputEvents.h"
#include "SystemUIEvents.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>


namespace Urho3D
{

const float SYSTEMUI_DEFAULT_FONT_SIZE = 14;

class URHO3D_API SystemUI : public Object
{
URHO3D_OBJECT(SystemUI, Object);
public:
    /// Construct.
    explicit SystemUI(Context* context);
    /// Destruct.
    ~SystemUI() override;

    /// Get ui scale.
    /// \return scale of ui.
    float GetZoom() const { return uiZoom_; };
    /// Set ui scale.
    /// \param zoom of ui.
    void SetZoom(float zoom);
    /// Update DPI scale.
    /// \param scale is a vector of {hscale, vscale, dscale}. If `pixelPerfect` is `true` then scale will be rounded to nearest power of two.
    void SetScale(Vector3 scale, bool pixelPerfect=true);
    /// Add font to imgui subsystem.
    /// \param fontPath a string pointing to TTF font resource.
    /// \param size a font size. If 0 then size of last font is used.
    /// \param ranges optional ranges of font that should be used. Parameter is ImWchar[] of {start1, stop1, ..., startN, stopN, 0}.
    /// \param merge set to true if new font should be merged to last active font.
    /// \return ImFont instance that may be used for setting current font when drawing GUI.
    ImFont* AddFont(const ea::string& fontPath, const ImWchar* ranges = nullptr, float size = 0, bool merge = false);
    ImFont* AddFont(const void* data, unsigned dsize, const ImWchar* ranges = nullptr, float size = 0, bool merge = false);
    ImFont* AddFontCompressed(const void* data, unsigned dsize, const ImWchar* ranges = nullptr, float size = 0, bool merge = false);
    /// Apply built-in system ui style.
    /// \param darkStyle enables dark style, otherwise it is a light style.
    /// \param alpha value between 0.0f - 1.0f
    void ApplyStyleDefault(bool darkStyle, float alpha);
    /// Return whether user is interacting with any ui element.
    bool IsAnyItemActive() const;
    /// Return whether mouse is hovering any system ui component.
    bool IsAnyItemHovered() const;
    /// Return font scale.
    float GetFontScale() const { return fontScale_; }
    /// Prepares font textures, updates projection matrix and does other things that are required to start this subsystem.
    void Start();

protected:
    float uiZoom_ = 1.f;
    float fontScale_ = 1.f;
    Matrix4 projection_;
    VertexBuffer vertexBuffer_;
    IndexBuffer indexBuffer_;
    SharedPtr<Texture2D> fontTexture_;
    ea::vector<float> fontSizes_;
    ImGuiContext* imContext_;

    void ReallocateFontTexture();
    void UpdateProjectionMatrix();
    void OnRenderDrawLists(ImDrawData* data);
    void OnRawEvent(VariantMap& args);
};

/// Convert Color to ImVec4.
inline ImVec4 ToImGui(const Color& color) { return {color.r_, color.g_, color.b_, color.a_}; }
/// Convert IntVector2 to ImVec2.
inline ImVec2 ToImGui(IntVector2 vec) { return {(float)vec.x_, (float)vec.y_}; };
/// Convert Vector2 to ImVec2.
inline ImVec2 ToImGui(Vector2 vec) { return {vec.x_, vec.y_}; };
/// Convert IntRect to ImRect.
inline ImRect ToImGui(const IntRect& rect) { return { ToImGui(rect.Min()), ToImGui(rect.Max()) }; }
/// Convert ImVec2 to IntVector2.
inline IntVector2 ToIntVector2(const ImVec2& vec) { return {(int)Round(vec.x), (int)Round(vec.y)}; }
/// Convert ImRect to IntRect
inline IntRect ToIntRect(const ImRect& rect) { return {ToIntVector2(rect.Min), ToIntVector2(rect.Max)}; }

}

namespace ImGui
{

URHO3D_API bool IsMouseDown(Urho3D::MouseButton button);
URHO3D_API bool IsMouseDoubleClicked(Urho3D::MouseButton button);
URHO3D_API bool IsMouseDragging(Urho3D::MouseButton button, float lock_threshold=-1.0f);
URHO3D_API bool IsMouseReleased(Urho3D::MouseButton button);
URHO3D_API bool IsMouseClicked(Urho3D::MouseButton button, bool repeat=false);
URHO3D_API bool IsItemClicked(Urho3D::MouseButton button);
URHO3D_API bool SetDragDropVariant(const char* type, const Urho3D::Variant& variant, ImGuiCond cond = 0);
URHO3D_API const Urho3D::Variant& AcceptDragDropVariant(const char* type, ImGuiDragDropFlags flags = 0);

namespace litterals
{

/// Scale a literal value according to x axis DPI.
URHO3D_API float operator "" _dpx(long double x);
/// Scale a literal value according to x axis DPI.
URHO3D_API float operator "" _dpx(unsigned long long x);

/// Scale a literal value according to y axis DPI.
URHO3D_API float operator "" _dpy(long double y);
/// Scale a literal value according to y axis DPI.
URHO3D_API float operator "" _dpy(unsigned long long y);

/// Scale a literal value according to diagonal axis DPI.
URHO3D_API float operator "" _dp(long double z);
/// Scale a literal value according to diagonal axis DPI.
URHO3D_API float operator "" _dp(unsigned long long z);

/// Scale a literal value according to x axis DPI which was rounded to the nearest power of two.
URHO3D_API float operator "" _pdpx(long double x);
/// Scale a literal value according to x axis DPI which was rounded to the nearest power of two.
URHO3D_API float operator "" _pdpx(unsigned long long x);

/// Scale a literal value according to y axis DPI which was rounded to the nearest power of two.
URHO3D_API float operator "" _pdpy(long double y);
/// Scale a literal value according to y axis DPI which was rounded to the nearest power of two.
URHO3D_API float operator "" _pdpy(unsigned long long y);

/// Scale a literal value according to diagonal axis DPI which was rounded to the nearest power of two.
URHO3D_API float operator "" _pdp(long double z);
/// Scale a literal value according to diagonal axis DPI which was rounded to the nearest power of two.
URHO3D_API float operator "" _pdp(unsigned long long z);

}

/// Scale a value according to x axis DPI.
URHO3D_API float dpx(float x);
/// Scale a value according to y axis DPI.
URHO3D_API float dpy(float y);
/// Scale a value according to diagonal axis DPI.
URHO3D_API float dp(float z);

/// Scale a literal value according to x axis DPI which was rounded to the nearest power of two.
URHO3D_API float pdpx(float x);
/// Scale a literal value according to y axis DPI which was rounded to the nearest power of two.
URHO3D_API float pdpy(float y);
/// Scale a literal value according to diagonal axis DPI which was rounded to the nearest power of two.
URHO3D_API float pdp(float z);

}

namespace ui = ImGui;
