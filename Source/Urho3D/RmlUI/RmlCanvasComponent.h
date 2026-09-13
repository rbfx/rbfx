// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    void SetClearColor(const Color& clearColor) { clearColor_ = clearColor; }
    const Color& GetClearColor() const { return clearColor_; }

    /// Return off-screen RmlUI instance.
    RmlUI* GetUI() const { return offScreenUI_; }

protected:
    /// Set texture (for attribute).
    void SetTextureRef(const ResourceRef& texture);
    /// Get texture (for attribute).
    ResourceRef GetTextureRef() const;
    /// Handle component being added to Node or removed from it.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
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
    /// Color to clear texture to
    Color clearColor_{Color::TRANSPARENT_BLACK};
};

}
