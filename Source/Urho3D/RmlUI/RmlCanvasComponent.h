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
#pragma once

#include "../Scene/LogicComponent.h"

namespace Urho3D
{

class RmlUI;
class Texture2D;

/// Renders off-screen UI into a texture.
class URHO3D_API RmlCanvasComponent : public LogicComponent
{
    URHO3D_OBJECT(RmlCanvasComponent, LogicComponent);
public:
    /// Construct.
    explicit RmlCanvasComponent(Context* context);
    /// Destruct.
    ~RmlCanvasComponent() override;
    /// Registers object with the engine.
    static void RegisterObject(Context* context);

    /// Set size of texture UI will be rendered into.
    void SetUISize(IntVector2 size);
    /// Set texture canvas will render into.
    void SetTexture(Texture2D* texture);
    /// Return texture where UI is rendered into.
    Texture2D* GetTexture() const { return texture_; }
    /// Enable input remapping (use if canvas renders on 3D objects).
    void SetRemapMousePos(bool remap) { remapMousePos_ = remap; }
    /// Return whether input remapping is enabled.
    bool GetRemapMousePos() const { return remapMousePos_; }
    /// Return off-screen RmlUI instance.
    RmlUI* GetUI() const { return offScreenUI_; }

protected:
    /// Set texture (for attribute).
    void SetTextureRef(const ResourceRef& texture);
    /// Get texture (for attribute).
    ResourceRef GetTextureRef() const;
    /// Handle component being added to Node or removed from it.
    void OnNodeSet(Node* node) override;
    /// Handle component being enabled or disabled.
    void OnSetEnabled() override;
    /// Set texture color to transparent.
    void ClearTexture();
    /// Convert screen coordinates to context-local coordinates of RmlUI instance.
    void RemapMousePos(IntVector2& screenPos);

    /// Texture that UIElement will be rendered into.
    SharedPtr<Texture2D> texture_;
    /// Subsystem that handles UI rendering to the texture.
    SharedPtr<RmlUI> offScreenUI_;
    /// Flag indicating that this component remaps mouse position on to a sibling StaticModel if present.
    bool remapMousePos_ = true;
};

}
