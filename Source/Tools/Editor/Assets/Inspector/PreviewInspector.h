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
#include "ResourceInspector.h"


namespace Urho3D
{

class Model;

/// Renders a model preview in attribute inspector.
class PreviewInspector : public ResourceInspector
{
    URHO3D_OBJECT(PreviewInspector, ResourceInspector);
public:
    /// Construct.
    explicit PreviewInspector(Context* context);

    /// Model preview view mouse grabbing.
    void SetGrab(bool enable);
    /// Copy effects from specified render path.
    void SetEffectSource(RenderPath* renderPath);
    /// Set preview model by passing a resource name.
    void SetModel(const String& resourceName);
    /// Set preview model by passing model resourcei nstance.
    void SetModel(Model* model);

protected:
    /// Initialize preview.
    void CreateObjects();
    /// Render model preview.
    virtual void RenderPreview();
    /// Handle input of preview viewport.
    virtual void HandleInput();

    /// Preview scene.
    SceneView view_;
    /// Node holding figure to which material is applied.
    WeakPtr<Node> node_;
    /// Flag indicating if this widget grabbed mouse for rotating material node.
    bool mouseGrabbed_ = false;
    /// Distance from camera to figure.
    float distance_ = 1.5f;
};

}
