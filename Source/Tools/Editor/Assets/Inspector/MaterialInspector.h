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
#include "ResourceInspector.h"


namespace Urho3D
{

/// Renders material preview in attribute inspector.
class MaterialInspector : public ResourceInspector
{
    URHO3D_OBJECT(MaterialInspector, ResourceInspector);
public:
    explicit MaterialInspector(Context* context, Material* material);

    virtual void Render();
    void ToggleModel();
    void SetGrab(bool enable);

protected:
    void CreateObjects();

    /// Material which is being previewed.
    SharedPtr<Material> material_;
    /// Preview scene.
    SceneView view_;
    /// Node holding figure to which material is applied.
    WeakPtr<Node> node_;
    ///
    AttributeInspector attributeInspector_;
    /// Flag indicating if this widget grabbed mouse for rotating material node.
    bool mouseGrabbed_ = false;
    /// Index of current figure displaying material.
    int figureIndex_ = 0;
    /// A list of figures between which material view can be toggled.
    PODVector<const char*> figures_{"Sphere", "Box", "Torus", "TeaPot"};
    /// Distance from camera to figure.
    float distance_ = 1.5f;
    /// Object keeping track of automatic width of first column.
    AutoColumn autoColumn_;

};

}
