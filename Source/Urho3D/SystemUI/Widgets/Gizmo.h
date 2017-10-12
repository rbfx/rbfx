//
// Copyright (c) 2008-2017 the Urho3D project.
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


#include "../../Core/Object.h"
#include "../../Graphics/Camera.h"
#include "../../Scene/Node.h"


namespace Urho3D
{

enum GizmoOperation
{
    GIZMOOP_TRANSLATE,
    GIZMOOP_ROTATE,
    GIZMOOP_SCALE,
    GIZMOOP_MAX
};

class URHO3D_API Gizmo : public Object
{
    URHO3D_OBJECT(Gizmo, Object);
public:
    /// Construct.
    Gizmo(Context* context);
    /// Manipulate node. Should be called from within E_UPDATE event.
    /// \param camera which observes the node.
    /// \param node to be manipulated.
    /// \returns true if node was manipulated on current frame.
    bool Manipulate(const Camera* camera, Node* node);
    /// Manipulate multiple nodes. Should be called from within E_UPDATE event.
    /// \param camera which observes the node.
    /// \param nodes to be manipulated. Specifying more than one node manipulates them in world space.
    /// \returns true if node was manipulated on current frame.
    bool Manipulate(const Camera* camera, const PODVector<Node*>& nodes);
    /// Set operation mode. Possible modes: rotation, translation and scaling.
    void SetOperation(GizmoOperation operation) { operation_ = operation; }
    /// Get current manipulation mode.
    GizmoOperation GetOperation() const { return operation_; };
    /// Set transform space in which gizmo should operate. Parent transform space is not supported.
    void SetTransformSpace(TransformSpace transformSpace) { transformSpace_ = transformSpace; }
    /// Get transform space in which gizmo is operating.
    TransformSpace GetTransformSpace() const { return transformSpace_; }
    /// Returns state of gizmo.
    /// \returns true if gizmo is active, i.e. mouse is held down.
    bool IsActive() const;
    /// Render gizmo ui. This needs to be called between ui::Begin() / ui::End().
    void RenderUI();

protected:
    /// Current gizmo operation. Translation, rotation or scaling.
    GizmoOperation operation_ = GIZMOOP_TRANSLATE;
    /// Current coordinate space to operate in. World or local.
    TransformSpace transformSpace_ = TS_WORLD;
    /// Saved node scale on operation start.
    HashMap<Node*, Vector3> nodeScaleStart_;
    /// Current operation origin. This is center point between all nodes that are being manipulated.
    Matrix4 currentOrigin_;
};

}
