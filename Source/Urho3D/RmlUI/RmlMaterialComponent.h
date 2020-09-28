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

#include "../RmlUI/RmlTextureComponent.h"

namespace Urho3D
{

class RmlUI;
class Texture2D;

/// Renders off-screen UI into a texture.
class RmlMaterialComponent : public RmlTextureComponent
{
    URHO3D_OBJECT(RmlMaterialComponent, RmlTextureComponent);
public:
    /// Construct.
    explicit RmlMaterialComponent(Context* context);
    /// Destruct.
    ~RmlMaterialComponent() override = default;

    /// Registers object with the engine.
    static void RegisterObject(Context* context);

    /// Return material which renders UI on to objects.
    Material* GetMaterial() { return material_; }
    /// Return true if mouse position is remapped on to UI rendered on to sibling StaticModel.
    bool GetRemapMousePositions() const { return remapMousePos_; }
    /// Enable or disable mouse position remapping on to sibling StaticModel.
    void SetRemapMousePositions(bool enable) { remapMousePos_ = enable; }

protected:
    /// Convert screen coordinates to context-local coordinates of RmlUI instance.
    void TranslateMousePos(IntVector2& screenPos) override;
    /// Return virtual resource pointer.
    Resource* GetVirtualResource() override;

    /// Material managed by this component.
    SharedPtr<Material> material_;
    /// Flag indicating that this component remaps mouse position on to a sibling StaticModel if present.
    bool remapMousePos_ = true;
};

}
