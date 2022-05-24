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

#include "../SystemUI/TransformGizmo.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <ImGuizmo/ImGuizmo.h>

namespace Urho3D
{

namespace
{

Rect GetMainViewportRect()
{
#ifdef IMGUI_HAS_VIEWPORT
    const ImVec2 pos = ImGui::GetMainViewport()->Pos;
    const ImVec2 size = ImGui::GetMainViewport()->Size;
#else
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 pos = ImVec2(0, 0);
    const ImVec2 size = io.DisplaySize;
#endif
    return Rect{static_cast<Vector2>(pos), static_cast<Vector2>(pos + size)};
}

ImGuizmo::OPERATION GetInternalOperation(TransformGizmoOperation op)
{
    switch (op)
    {
    case TransformGizmoOperation::Translate:
        return ImGuizmo::TRANSLATE;
    case TransformGizmoOperation::Rotate:
        return ImGuizmo::ROTATE;
    case TransformGizmoOperation::Scale:
        return ImGuizmo::SCALE;
    default:
        URHO3D_ASSERT(0);
        return ImGuizmo::TRANSLATE;
    }
}

}

Matrix4 TransformGizmo::internalTransformMatrix;

TransformGizmo::TransformGizmo(Camera* camera)
    : TransformGizmo(camera, true, GetMainViewportRect())
{
}

TransformGizmo::TransformGizmo(Camera* camera, const Rect& viewportRect)
    : TransformGizmo(camera, false, viewportRect)
{
}

TransformGizmo::TransformGizmo(Camera* camera, bool isMainViewport, const Rect& viewportRect)
    : camera_(camera)
    , internalViewMatrix_(camera_->GetView().ToMatrix4().Transpose())
    , internalProjMatrix_(camera_->GetProjection().Transpose())
    , isMainViewport_(isMainViewport)
    , viewportRect_(viewportRect)
{

}

ea::optional<Matrix4> TransformGizmo::ManipulateTransform(Matrix4& transform,
    TransformGizmoOperation op, bool local, float snap) const
{
    if (op == TransformGizmoOperation::None)
        return ea::nullopt;

    PrepareToManipulate();

    if (!ImGuizmo::IsUsing())
        internalTransformMatrix = transform.Transpose();

    const ImGuizmo::OPERATION operation = GetInternalOperation(op);
    const ImGuizmo::MODE mode = local ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    const Vector3 snapVector = snap * Vector3::ONE;

    Matrix4 delta;
    ImGuizmo::Manipulate(internalViewMatrix_.Data(), internalProjMatrix_.Data(), operation,
        mode, &internalTransformMatrix.m00_, &delta.m00_, snap != 0.0f ? snapVector.Data() : nullptr);
    transform = internalTransformMatrix.Transpose();

    if (!ImGuizmo::IsUsing())
        return ea::nullopt;

    return delta.Transpose();
}

ea::optional<Vector3> TransformGizmo::ManipulatePosition(const Matrix4& transform, bool local, float snap) const
{
    Matrix4 transformCopy = transform;
    const auto delta = ManipulateTransform(transformCopy, TransformGizmoOperation::Translate, local, snap);
    if (!delta)
        return ea::nullopt;

    return Matrix3x4(*delta).Translation();
}

ea::optional<Quaternion> TransformGizmo::ManipulateRotation(const Matrix4& transform, bool local, float snap) const
{
    Matrix4 transformCopy = transform;
    const auto delta = ManipulateTransform(transformCopy, TransformGizmoOperation::Rotate, local, snap);
    if (!delta)
        return ea::nullopt;

    return Matrix3x4(*delta).Rotation();
}

ea::optional<Vector3> TransformGizmo::ManipulateScale(const Matrix4& transform, bool local, float snap) const
{
    Matrix4 transformCopy = transform;
    const auto delta = ManipulateTransform(transformCopy, TransformGizmoOperation::Scale, local, snap);
    if (!delta)
        return ea::nullopt;

    return Matrix3x4(*delta).SignedScale(Matrix3::IDENTITY);
}

void TransformGizmo::PrepareToManipulate() const
{
    const Vector2 pos = viewportRect_.Min();
    const Vector2 size = viewportRect_.Size();
    ImGuizmo::SetRect(pos.x_, pos.y_, size.x_, size.y_);

    if (isMainViewport_)
        ImGuizmo::SetDrawlist(ui::GetBackgroundDrawList());
    else
        ImGuizmo::SetDrawlist();

    ImGuizmo::SetOrthographic(camera_->IsOrthographic());
}

bool TransformNodesGizmo::Manipulate(const TransformGizmo& gizmo,
    TransformGizmoOperation op, bool local, bool pivoted, float snap)
{
    switch (op)
    {
    case TransformGizmoOperation::Translate:
        return ManipulatePosition(gizmo, local, pivoted, snap);

    case TransformGizmoOperation::Rotate:
        return ManipulateRotation(gizmo, local, pivoted, snap);

    case TransformGizmoOperation::Scale:
        return ManipulateScale(gizmo, local, pivoted, snap);

    default:
        return false;
    }
}

Matrix4 TransformNodesGizmo::GetGizmoTransform(bool pivoted) const
{
    if (pivoted && activeNode_)
        return activeNode_->GetWorldTransform().ToMatrix4();
    else if (nodes_.size() == 1)
        return (**nodes_.begin()).GetWorldTransform().ToMatrix4();
    else
    {
        Vector3 centerPosition;
        for (Node* node : nodes_)
            centerPosition += node->GetWorldPosition();
        centerPosition /= nodes_.size();
        return Matrix3x4{centerPosition, Quaternion::IDENTITY, Vector3::ONE}.ToMatrix4();
    }
}

bool TransformNodesGizmo::ManipulatePosition(const TransformGizmo& gizmo, bool local, bool pivoted, float snap)
{
    const auto delta = gizmo.ManipulatePosition(GetGizmoTransform(pivoted), local, snap);
    if (!delta)
        return false;

    if (*delta == Vector3::ZERO)
        return true;

    for (Node* node : nodes_)
    {
        if (node)
        {
            const Transform& oldTransform = node->GetDecomposedTransform();
            node->Translate(*delta, TS_WORLD);
            OnNodeTransformChanged(this, node, oldTransform);
        }
    }

    return true;
}

bool TransformNodesGizmo::ManipulateRotation(const TransformGizmo& gizmo, bool local, bool pivoted, float snap)
{
    const Matrix4 anchorTransform = GetGizmoTransform(pivoted);
    const auto delta = gizmo.ManipulateRotation(anchorTransform, local, snap);
    if (!delta)
        return false;

    if (*delta == Quaternion::IDENTITY)
        return true;

    for (Node* node : nodes_)
    {
        if (node)
        {
            const Transform& oldTransform = node->GetDecomposedTransform();
            if (pivoted)
                node->Rotate(*delta, TS_WORLD);
            else
                node->RotateAround(anchorTransform.Translation(), *delta, TS_WORLD);
            OnNodeTransformChanged(this, node, oldTransform);
        }
    }

    return true;
}

bool TransformNodesGizmo::ManipulateScale(const TransformGizmo& gizmo, bool local, bool pivoted, float snap)
{
    const Matrix4 anchorTransform = GetGizmoTransform(pivoted);
    const auto delta = gizmo.ManipulateScale(anchorTransform, local, snap);
    if (!delta)
        return false;

    if (*delta == Vector3::ONE)
        return true;

    for (Node* node : nodes_)
    {
        if (node)
        {
            const Transform& oldTransform = node->GetDecomposedTransform();
            if (pivoted)
                node->Scale(*delta);
            else
                node->ScaleAround(anchorTransform.Translation(), *delta, TS_WORLD);
            OnNodeTransformChanged(this, node, oldTransform);
        }
    }

    return true;

}

}
