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


#include "Urho3D/Core/Object.h"
#include "Urho3D/Math/StringHash.h"
#include "Urho3D/Container/HashMap.h"
#include "Urho3D/Graphics/VertexBuffer.h"
#include "Urho3D/Graphics/IndexBuffer.h"
#include "Urho3D/Math/Matrix4.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/Input/InputEvents.h"
#include "SystemUIEvents.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>


namespace ui = ImGui;


namespace Urho3D
{

const float SYSTEMUI_DEFAULT_FONT_SIZE = 14.f;

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
    /// \param scale is a vector of {hscale, vscale, dscale}. Passing no parameter detects scale as Graphics::GetDisplayDPI() / 96.f.
    void SetScale(Vector3 scale = Vector3::ZERO);
    /// Add font to imgui subsystem.
    /// \param fontPath a string pointing to TTF font resource.
    /// \param size a font size. If 0 then size of last font is used.
    /// \param ranges optional ranges of font that should be used. Parameter is array of {start1, stop1, ..., startN, stopN, 0}.
    /// \param merge set to true if new font should be merged to last active font.
    /// \return ImFont instance that may be used for setting current font when drawing GUI.
    ImFont* AddFont(const String& fontPath, const unsigned short* ranges, float size = 0, bool merge = false);
    /// Add font to imgui subsystem.
    /// \param fontPath a string pointing to TTF font resource.
    /// \param size a font size. If 0 then size of last font is used.
    /// \param ranges optional ranges of font that should be used. Parameter is std::initializer_list of {start1, stop1, ..., startN, stopN, 0}.
    /// \param merge set to true if new font should be merged to last active font.
    /// \return ImFont instance that may be used for setting current font when drawing GUI.
    ImFont* AddFont(const String& fontPath, const std::initializer_list<unsigned short>& ranges = {}, float size = 0,
        bool merge = false);
    /// Apply built-in system ui style.
    /// \param darkStyle enables dark style, otherwise it is a light style.
    /// \param alpha value between 0.0f - 1.0f
    void ApplyStyleDefault(bool darkStyle, float alpha);
    /// Return whether user is interacting with any ui element.
    bool IsAnyItemActive() const;
    /// Return whether mouse is hovering any system ui component.
    bool IsAnyItemHovered() const;
    /// Set data which is currently being dragged.
    void SetDragData(const Variant& dragData) { dragData_ = dragData; }
    /// Return data which is currently dragged.
    Variant GetDragData() const { return dragData_; }
    /// Return true if item is being dragged.
    bool HasDragData() const { return dragData_.GetType() != VAR_NONE; }
    /// Return font scale.
    float GetFontScale() const { return fontScale_; }

protected:
    float uiZoom_ = 1.f;
    float fontScale_ = 1.f;
    Matrix4 projection_;
    VertexBuffer vertexBuffer_;
    IndexBuffer indexBuffer_;
    SharedPtr<Texture2D> fontTexture_;
    Variant dragData_;
    PODVector<float> fontSizes_;

    void ReallocateFontTexture();
    void UpdateProjectionMatrix();
    void OnRenderDrawLists(ImDrawData* data);
    void OnRawEvent(VariantMap& args);
    void OnUpdate();
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

bool URHO3D_API IsMouseDown(Urho3D::MouseButton button);
bool URHO3D_API IsMouseDoubleClicked(Urho3D::MouseButton button);
bool URHO3D_API IsMouseDragging(Urho3D::MouseButton button, float lock_threshold=-1.f);
bool URHO3D_API IsMouseReleased(Urho3D::MouseButton button);
bool URHO3D_API IsMouseClicked(Urho3D::MouseButton button, bool repeat=false);
bool URHO3D_API IsItemClicked(Urho3D::MouseButton button);

}
