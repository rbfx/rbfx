//
// Copyright (c) 2017 the Urho3D project.
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
#include "SystemUIEvents.h"

#include <ImGui/imgui.h>
namespace ui = ImGui;


namespace Urho3D
{

class SystemUI : public Object
{
URHO3D_OBJECT(SystemUI, Object);
public:
    explicit SystemUI(Context* context);
    ~SystemUI() override;

    /// Get ui scale.
    /// \return scale of ui.
    float GetScale() const { return uiScale_; };
    /// Set ui scale.
    /// \param scale of ui.
    void SetScale(float scale);
    /// Add font to imgui subsystem.
    /// \param fontPath a string pointing to TTF font resource.
    /// \param size a font size. If 0 then size of last font is used.
    /// \param ranges optional ranges of font that should be used. Parameter is array of {start1, stop1, ..., startN, stopN, 0}.
    /// \param merge set to true if new font should be merged to last active font.
    /// \return ImFont instance that may be used for setting current font when drawing GUI.
    ImFont* AddFont(const String& fontPath, float size = 0, const unsigned short* ranges = nullptr, bool merge = false);
    /// Add font to imgui subsystem.
    /// \param fontPath a string pointing to TTF font resource.
    /// \param size a font size. If 0 then size of last font is used.
    /// \param ranges optional ranges of font that should be used. Parameter is std::initializer_list of {start1, stop1, ..., startN, stopN, 0}.
    /// \param merge set to true if new font should be merged to last active font.
    /// \return ImFont instance that may be used for setting current font when drawing GUI.
    ImFont* AddFont(const String& fontPath, float size = 0, const std::initializer_list<unsigned short>& ranges = {},
        bool merge = false);

    /// Apply built-in system ui style.
    /// \param darkStyle enables dark style, otherwise it is a light style.
    /// \param alpha value between 0.0f - 1.0f
    void ApplyStyleDefault(bool darkStyle, float alpha);

protected:
    float uiScale_ = 1.f;
    Matrix4 projection_;
    VertexBuffer vertexBuffer_;
    IndexBuffer indexBuffer_;
    SharedPtr<Texture2D> fontTexture_;

    void ReallocateFontTexture();
    void UpdateProjectionMatrix();
    void OnPostUpdate(VariantMap& args);
    void OnRenderDrawLists(ImDrawData* data);
    void OnRawEvent(VariantMap& args);
};

}
