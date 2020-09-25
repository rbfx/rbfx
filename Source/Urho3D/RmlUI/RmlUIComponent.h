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

#include "../Scene/Component.h"
#include "../RmlUI/RmlUI.h"

namespace Urho3D
{

class Material;
class Texture2D;
class StaticModel;
class Viewport;
class UIElement;
class UIBatch;
class VertexBuffer;
class UIElement3D;

class RmlUIComponent : public Component
{
    URHO3D_OBJECT(RmlUIComponent, Component);
public:
    /// Construct.
    explicit RmlUIComponent(Context* context);
    /// Destruct.
    ~RmlUIComponent() override;

    /// Return UIElement.
    RmlUI* GetUI() const { return offScreenUI_; }
    /// Return material which will be used for rendering UI texture.
    Material* GetMaterial() const { return material_; }
    /// Return texture which will be used for rendering UI to.
    Texture2D* GetTexture() const { return texture_; }
    /// Set size of texture UI will be rendered into.
    void SetSize(IntVector2 size);

protected:
    /// Handle component being added to Node or removed from it.
    void OnNodeSet(Node* node) override;
    /// Convert screen coordinates to context-local coordinates of RmlUI instance.
    void ScreenToUI(IntVector2& screenPos);

    /// Material that is set to the model.
    SharedPtr<Material> material_;
    /// Texture that UIElement will be rendered into.
    SharedPtr<Texture2D> texture_;
    /// Model created by this component. If node already has StaticModel then this will be null.
    SharedPtr<StaticModel> model_;
    /// Subsystem that handles UI rendering to the texture.
    SharedPtr<RmlUI> offScreenUI_;
};

}
