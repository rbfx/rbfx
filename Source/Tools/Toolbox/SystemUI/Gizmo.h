//
// Copyright (c) 2017-2019 the rbfx project.
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

#include "ToolboxAPI.h"
#include <Urho3D/Scene/Node.h>
#include <ImGui/imgui.h>


namespace Urho3D
{

class Camera;
class Node;

enum GizmoOperation
{
    GIZMOOP_TRANSLATE,
    GIZMOOP_ROTATE,
    GIZMOOP_SCALE,
    GIZMOOP_MAX
};

class URHO3D_TOOLBOX_API Gizmo : public Object
{
    URHO3D_OBJECT(Gizmo, Object);
public:
    /// Construct.
    Gizmo(Context* context);
    /// Destruct.
    virtual ~Gizmo();
    /// Manipulate node. Should be called from within E_UPDATE event.
    /// \param camera which observes the node.
    /// \param node to be manipulated.
    /// \returns true if node was manipulated on current frame.
    bool Manipulate(const Camera* camera, Node* node);
    /// Manipulate multiple nodes. Should be called from within E_UPDATE event.
    /// \param camera which observes the node.
    /// \param nodes to be manipulated. Specifying more than one node manipulates them in world space.
    /// \returns true if node was manipulated on current frame.
    bool Manipulate(const Camera* camera, const ea::vector<WeakPtr<Node>>& nodes);
    /// Manipulate current node selection. Should be called from within E_UPDATE event.
    /// \param camera which observes the node.
    /// \returns true if node(s) were manipulated on current frame.
    bool ManipulateSelection(const Camera* camera);
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
    /// Add a node to selection.
    bool Select(Node* node);
    /// Add a node to selection.
    bool Select(ea::vector<Node*> nodes);
    /// Remove a node from selection.
    bool Unselect(Node* node);
    /// Select if node was not selected or unselect if node was selected.
    void ToggleSelection(Node* node);
    /// Unselect all nodes.
    bool UnselectAll();
    /// Return true if node is selected by gizmo.
    bool IsSelected(Node* node) const;
    /// Return list of selected nodes.
    const ea::vector<WeakPtr<Node>>& GetSelection() const { return nodeSelection_; }
    /// Set screen rect to which gizmo rendering will be limited. Use when putting gizmo in a window.
    void SetScreenRect(const IntVector2& pos, const IntVector2& size);
    /// Set screen rect to which gizmo rendering will be limited. Use when putting gizmo in a window.
    void SetScreenRect(const IntRect& rect);

protected:

    /// Current gizmo operation. Translation, rotation or scaling.
    GizmoOperation operation_ = GIZMOOP_TRANSLATE;
    /// Current coordinate space to operate in. World or local.
    TransformSpace transformSpace_ = TS_WORLD;
    /// Saved node scale on operation start.
    ea::unordered_map<Node*, Vector3> nodeScaleStart_;
    /// Current operation origin. This is center point between all nodes that are being manipulated.
    Matrix4 currentOrigin_;
    /// Current node selection. Nodes removed from the scene are automatically unselected.
    ea::vector<WeakPtr<Node> > nodeSelection_;
    /// Camera which is used for automatic node selection in the scene camera belongs to.
    WeakPtr<Camera> autoModeCamera_;
    /// Position of display area gizmo is rendered in.
    ImVec2 displayPos_{};
    /// Size of display area gizmo is rendered in.
    ImVec2 displaySize_{};
    /// Flag indicating that gizmo was active on the last frame.
    bool wasActive_ = false;
    /// A map of initial transforms.
    ea::unordered_map<Node*, Matrix3x4> initialTransforms_;
};

}
