//
// Copyright (c) 2018 Rokas Kupstys
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


#include <Toolbox/Graphics/SceneView.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include "PreviewInspector.h"


namespace Urho3D
{

namespace Inspectable
{

/// A serializable proxy for Urho3D::Material for enabling inspection in attribute inspector.
class Material : public Serializable
{
URHO3D_OBJECT(Material, Serializable);
public:
    /// Construct.
    explicit Material(Urho3D::Material* material);
    /// Returns attached material.
    Urho3D::Material* GetMaterial() const { return material_.Get(); }
    /// Returns attached material.
    Urho3D::Material* GetMaterial() { return material_.Get(); }

    /// Registers object with the engine.
    static void RegisterObject(Context* context);

protected:
    /// Attached material.
    SharedPtr<Urho3D::Material> material_;
};

}

/// Renders material preview in attribute inspector.
class MaterialInspector : public PreviewInspector
{
    URHO3D_OBJECT(MaterialInspector, PreviewInspector);
public:
    explicit MaterialInspector(Context* context, Material* material);

    /// Render inspector window.
    void RenderInspector(const char* filter) override;
    /// Change material preview model to next one in the list.
    void ToggleModel();

protected:
    /// Save material resource to disk.
    void Save();
    ///
    void RenderPreview() override;
    ///
    void RenderCustomWidgets(VariantMap& args);

    /// Material which is being previewed.
    SharedPtr<Inspectable::Material> inspectable_;
    /// Material attribute inspector namespace.
    AttributeInspector attributeInspector_;
    /// Index of current figure displaying material.
    unsigned figureIndex_ = 0;
    /// A list of figures between which material view can be toggled.
    PODVector<const char*> figures_{"Sphere", "Box", "Torus", "TeaPot"};
};

}
