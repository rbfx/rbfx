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

#include "ToolboxAPI.h"
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/SystemUIEvents.h>
#include <ImGuizmo/ImGuizmo.h>

namespace Urho3D
{

enum GizmoOperation
{
    GIZMOOP_TRANSLATE = ImGuizmo::TRANSLATE,
    GIZMOOP_ROTATE = ImGuizmo::ROTATE,
    GIZMOOP_SCALE = ImGuizmo::SCALE,
    GIZMOOP_MAX
};

class URHO3D_TOOLBOX_API Gizmo : public Object
{
    URHO3D_OBJECT(Gizmo, Object);
public:
    /// Construct.
    explicit Gizmo(Context* context);
    /// Destruct.
    ~Gizmo() override;
    /// Manipulate node. Should be called from within E_UPDATE event.
    /// \param camera which observes the node.
    /// \param node to be manipulated.
    /// \returns true if node was manipulated on current frame.
    bool ManipulateNode(const Camera* camera, Node* node);
    /// Manipulate multiple nodes. Should be called from within E_UPDATE event.
    /// \param camera which observes the node.
    /// \param nodes to be manipulated. Specifying more than one node manipulates them in world space.
    /// \returns true if node was manipulated on current frame.
    bool Manipulate(const Camera* camera, Node** begin, Node** end);
    template<typename Container>
    bool Manipulate(const Camera* camera, const Container& container)
    {
        manipulatedNodes_.clear();
        for (Node* node : container)
        {
            if (node)
                manipulatedNodes_.push_back(node);
        }
        return Manipulate(camera, manipulatedNodes_.begin(), manipulatedNodes_.end());
    }
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
    /// Get the center of selected nodes.
    /// \param nodes The nodes to be calculated.
    /// \param outCenter If returns true, it gets the center, else it will be set to ZERO vector
    /// \returns Returns the number of selected nodes.
    static int GetSelectionCenter(Vector3& outCenter, Node** begin, Node** end);
    template<typename Container>
    int GetSelectionCenter(Vector3& outCenter, const Container& container)
    {
        manipulatedNodes_.clear();
        for (Node* node : container)
        {
            if (node)
                manipulatedNodes_.push_back(node);
        }
        return GetSelectionCenter(outCenter, manipulatedNodes_.begin(), manipulatedNodes_.end());
    }

protected:
    /// Current gizmo operation. Translation, rotation or scaling.
    GizmoOperation operation_ = GIZMOOP_TRANSLATE;
    /// Current coordinate space to operate in. World or local.
    TransformSpace transformSpace_ = TS_WORLD;
    /// Saved node scale on operation start.
    ea::unordered_map<Node*, Vector3> nodeScaleStart_;
    /// Flag indicating that gizmo was active on the last frame.
    bool wasActive_ = false;
    /// A map of initial transforms.
    ea::unordered_map<Node*, Matrix3x4> initialTransforms_;
    ///
    ea::vector<Node*> manipulatedNodes_;
};

}
