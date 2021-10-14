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


#include "../Container/ValueCache.h"
#include "../Core/Object.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/VertexBuffer.h"
#include "../Input/InputEvents.h"
#include "../Math/StringHash.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector4.h"
#include "../SystemUI/SystemUIEvents.h"

#define IM_VEC2_CLASS_EXTRA                                                                                            \
    operator Urho3D::Vector2() { return {x, y}; }                                                                      \
    ImVec2(const Urho3D::Vector2& vec) { x = vec.x_; y = vec.y_; }

#define IM_VEC4_CLASS_EXTRA                                                                                            \
    operator Urho3D::Vector4() { return {x, y, z, w}; }                                                                \
    ImVec4(const Urho3D::Vector4& vec) { x = vec.x_; y = vec.y_; z = vec.z_; w = vec.w_; }

#include <EASTL/unordered_map.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_stdlib.h>


namespace Urho3D
{

const float SYSTEMUI_DEFAULT_FONT_SIZE = 14;

class URHO3D_API SystemUI : public Object
{
URHO3D_OBJECT(SystemUI, Object);
public:
    /// Construct.
    explicit SystemUI(Context* context, ImGuiConfigFlags flags=0);
    /// Destruct.
    ~SystemUI() override;

    /// Enable or disable mouse wrapping for current display and window.
    /// Should be called withing ImGUI window.
    /// Mouse wrapping is automatically disabled when all mouse buttons are released.
    void SetMouseWrapping(bool enabled, bool revertMousePositionOnDisable);

    /// Add font to imgui subsystem.
    /// \param fontPath a string pointing to TTF font resource.
    /// \param size a font size. If 0 then size of last font is used.
    /// \param name is any string, stored internally as font name.
    /// \param ranges optional ranges of font that should be used. Parameter is ImWchar[] of {start1, stop1, ..., startN, stopN, 0}.
    /// \param merge set to true if new font should be merged to last active font.
    /// \return ImFont instance that may be used for setting current font when drawing GUI.
    ImFont* AddFont(const ea::string& fontPath, const ImWchar* ranges = nullptr, float size = 0, bool merge = false);
    ImFont* AddFont(const void* data, unsigned dsize, const char* name, const ImWchar* ranges = nullptr, float size = 0, bool merge = false);
    ImFont* AddFontCompressed(const void* data, unsigned dsize, const char* name, const ImWchar* ranges = nullptr, float size = 0, bool merge = false);
    /// Apply built-in system ui style.
    /// \param darkStyle enables dark style, otherwise it is a light style.
    /// \param alpha value between 0.0f - 1.0f
    void ApplyStyleDefault(bool darkStyle, float alpha);
    /// Hold a reference to this texture until end of frame.
    void ReferenceTexture(Texture2D* texture) { referencedTextures_.push_back(SharedPtr(texture)); }
#ifndef SWIG    // Due to some quirk SWIG fails to ignore this API.
    /// Return value cache for storing temporary UI state that expires when unused.
    ValueCache& GetValueCache() { return cache_; }
#endif
    /// When set to true, SystemUI will not consume SDL events and they will be passed to to Input and other subsystems.
    void SetPassThroughEvents(bool enabled) { passThroughEvents_ = enabled; }
    /// Return true if SystemUI is allowing events through even when SystemUI is handling them.
    bool GetPassThroughEvents() const { return passThroughEvents_; }

protected:
    VertexBuffer vertexBuffer_;
    IndexBuffer indexBuffer_;
    ea::vector<SharedPtr<Texture2D>> fontTextures_;
    ea::vector<float> fontSizes_;
    ImGuiContext* imContext_;
    ea::vector<SharedPtr<Texture2D>> referencedTextures_;
    ValueCache cache_{context_};
    /// When set to true, SystemUI will not consume SDL events and they will be passed to to Input and other subsystems.
    bool passThroughEvents_ = false;

    void PlatformInitialize();
    void PlatformShutdown();
    void ReallocateFontTexture();
    void ClearPerScreenFonts();
    ImTextureID AllocateFontTexture(ImFontAtlas* atlas);
    void OnRawEvent(VariantMap& args);
    void OnScreenMode(VariantMap& args);
    void OnInputEnd(VariantMap& args);
    void OnRenderEnd();
    void OnMouseVisibilityChanged(StringHash, VariantMap& args);

    bool enableWrapping_{};
    ImVec2 minWrapBound_;
    ImVec2 maxWrapBound_;
    bool revertMousePositionOnDisable_{};
    ImVec2 revertMousePosition_;
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

enum ImGuiItemMouseActivation
{
    ImGuiItemMouseActivation_Click,
    ImGuiItemMouseActivation_Dragging,
};

namespace ImGui
{

URHO3D_API bool IsMouseDown(Urho3D::MouseButton button);
URHO3D_API bool IsMouseDoubleClicked(Urho3D::MouseButton button);
URHO3D_API bool IsMouseDragging(Urho3D::MouseButton button, float lock_threshold=-1.0f);
URHO3D_API bool IsMouseReleased(Urho3D::MouseButton button);
URHO3D_API bool IsMouseClicked(Urho3D::MouseButton button, bool repeat=false);
URHO3D_API bool IsItemClicked(Urho3D::MouseButton button);
URHO3D_API ImVec2 GetMouseDragDelta(Urho3D::MouseButton button, float lock_threshold = -1.0f);
URHO3D_API void ResetMouseDragDelta(Urho3D::MouseButton button);
URHO3D_API bool SetDragDropVariant(const ea::string& types, const Urho3D::Variant& variant, ImGuiCond cond = 0);
URHO3D_API const Urho3D::Variant& AcceptDragDropVariant(const ea::string& type, ImGuiDragDropFlags flags = 0);
URHO3D_API void Image(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
/// Render an image which is also an item that can be activated.
URHO3D_API void ImageItem(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
URHO3D_API bool ImageButton(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4 & bg_col = ImVec4(0, 0, 0, 0), const ImVec4 & tint_col = ImVec4(1, 1, 1, 1));
URHO3D_API bool IsKeyDown(Urho3D::Key key);
URHO3D_API bool IsKeyPressed(Urho3D::Key key, bool repeat = true);
URHO3D_API bool IsKeyReleased(Urho3D::Key key);
URHO3D_API int GetKeyPressedAmount(Urho3D::Key key, float repeat_delay, float rate);
/// Activate last item if specified mouse button is pressed and held over it, deactivate when released.
URHO3D_API bool ItemMouseActivation(Urho3D::MouseButton button, unsigned flags = ImGuiItemMouseActivation_Click);
URHO3D_API void HideCursorWhenActive(Urho3D::MouseButton button, bool on_drag = false);

}

static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
static inline bool operator!=(const ImVec2& lhs, const ImVec2& rhs) { return !(lhs == rhs); }
static inline bool operator==(const ImRect& lhs, const ImRect& rhs) { return lhs.Min == rhs.Min && lhs.Max == rhs.Max; }
static inline bool operator!=(const ImRect& lhs, const ImRect& rhs) { return !(lhs == rhs); }
static inline ImRect operator+(const ImRect& lhs, const ImRect& rhs) { return ImRect(lhs.Min + rhs.Min, lhs.Max + rhs.Max); }
static inline ImRect& operator+=(ImRect& lhs, const ImRect& rhs) { lhs.Min += rhs.Min; lhs.Max += rhs.Max; return lhs; }
static inline ImRect operator/(const ImRect& lhs, float rhs) { return ImRect(lhs.Min / rhs, lhs.Max / rhs); }
static inline ImRect& operator/=(ImRect& lhs, float rhs) { lhs.Min /= rhs; lhs.Max /= rhs; return lhs; }
static inline ImRect& operator*=(ImRect& lhs, float rhs) { lhs.Min *= rhs; lhs.Max *= rhs; return lhs; }
static inline ImRect ImRound(const ImRect& r) { return ImRect({Urho3D::Round(r.Min.x), Urho3D::Round(r.Min.y)}, {Urho3D::Round(r.Max.x), Urho3D::Round(r.Max.y)}); };

namespace ui = ImGui;
