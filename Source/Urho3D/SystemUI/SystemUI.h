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

#include "../Core/Object.h"
#include "../Graphics/Texture2D.h"
#include "../Input/InputEvents.h"
#include "../Math/Matrix4.h"
#include "../Math/StringHash.h"
#include "../Math/Vector2.h"
#include "../Math/Vector4.h"
#include "../SystemUI/ImGui.h"
#include "../SystemUI/SystemUIEvents.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_map.h>

namespace Urho3D
{

class ImGuiDiligentRendererEx;

const float SYSTEMUI_DEFAULT_FONT_SIZE = 14;

class URHO3D_API SystemUI : public Object
{
    URHO3D_OBJECT(SystemUI, Object);

public:
    /// Construct.
    explicit SystemUI(Context* context, ImGuiConfigFlags flags=0);
    /// Destruct.
    ~SystemUI() override;

    /// Enable or disable relative mouse movement.
    /// Should be called withing ImGUI window.
    /// Relative mouse movement is automatically disabled when all mouse buttons are released.
    void SetRelativeMouseMove(bool enabled, bool revertMousePositionOnDisable);
    /// Return relative mouse movement value.
    const Vector2 GetRelativeMouseMove() const;

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

    /// When set to true, SystemUI will not consume SDL events and they will be passed to to Input and other subsystems.
    void SetPassThroughEvents(bool enabled) { passThroughEvents_ = enabled; }
    /// Return true if SystemUI is allowing events through even when SystemUI is handling them.
    bool GetPassThroughEvents() const { return passThroughEvents_; }

protected:
    ea::vector<SharedPtr<Texture2D>> fontTextures_;
    ea::vector<float> fontSizes_;
    ImGuiContext* imContext_{};
    ea::vector<SharedPtr<Texture2D>> referencedTextures_;
    /// When set to true, SystemUI will not consume SDL events and they will be passed to to Input and other subsystems.
    bool passThroughEvents_ = false;

    void PlatformInitialize();
    void PlatformShutdown();
    void ReallocateFontTexture();
    void ClearPerScreenFonts();
    SharedPtr<Texture2D> AllocateFontTexture(ImFontAtlas* atlas) const;
    void OnRawEvent(VariantMap& args);
    void OnScreenMode(VariantMap& args);
    void OnInputBegin();
    void OnInputEnd();
    void OnRenderEnd();
    void OnMouseVisibilityChanged(StringHash, VariantMap& args);

    bool enableRelativeMouseMove_{};
    Vector2 relativeMouseMove_;
    bool revertMousePositionOnDisable_{};
    ImVec2 revertMousePosition_;

    ea::unique_ptr<ImGuiDiligentRendererEx> impl_;
};

}
