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
class RmlTextureComponent : public LogicComponent
{
    URHO3D_OBJECT(RmlTextureComponent, LogicComponent);
public:
    /// Construct.
    explicit RmlTextureComponent(Context* context);
    /// Destruct.
    ~RmlTextureComponent() override = default;

    /// Registers object with the engine.
    static void RegisterObject(Context* context);

    /// Set size of texture UI will be rendered into.
    void SetTextureSize(IntVector2 size);
    /// Get size of texture UI will be rendered into.
    IntVector2 GetTextureSize(IntVector2) const;
    /// Sets a name of virtual texture resource. Virtual texture gets created if/when component is added to the node.
    void SetVirtualResourceName(const ea::string& name);
    /// Returns a name of virtual texture resource.
    const ea::string& GetVirtualResourceName() const { return virtualResourceName_; }

    /// Return off-screen RmlUI instance.
    RmlUI* GetUI() const { return offScreenUI_; }
    /// Return texture where UI is rendered into.
    Texture2D* GetTexture() const { return texture_; }

protected:
    /// Handle component being added to Node or removed from it.
    void OnNodeSet(Node* node) override;
    /// Handle component being enabled or disabled.
    void OnSetEnabled() override;
    /// Convert screen coordinates to context-local coordinates of RmlUI instance.
    virtual void TranslateMousePos(IntVector2& screenPos) { }

    virtual /// Adds a manual texture resource.
    void AddVirtualResource(Resource* resource);
    virtual /// Releases a manual texture resource.
    void RemoveVirtualResource(Resource* resource);
    /// Set texture color to transparent.
    void ClearTexture();
    /// Handle updates of virtual resource name.
    void OnVirtualResourceNameSet();
    /// Return virtual resource pointer. Subclasses may change it to provide a different virtual resource. For example RmlMaterialComponent returns a material.
    virtual Resource* GetVirtualResource() { return texture_; }

    /// Texture that UIElement will be rendered into.
    SharedPtr<Texture2D> texture_;
    /// Subsystem that handles UI rendering to the texture.
    SharedPtr<RmlUI> offScreenUI_;
    /// Name of virtual resource which this component will register as.
    ea::string virtualResourceName_;
};

}
