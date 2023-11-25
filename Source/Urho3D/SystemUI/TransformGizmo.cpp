// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/SystemUI/TransformGizmo.h"

#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/SystemUI/SystemUI.h"

#include <ImGuizmo/ImGuizmo.h>

namespace Urho3D
{

namespace
{

Rect GetMainViewportRect()
{
#ifdef IMGUI_HAS_VIEWPORT
    const ImVec2 pos = ui::GetMainViewport()->Pos;
    const ImVec2 size = ui::GetMainViewport()->Size;
#else
    ImGuiIO& io = ui::GetIO();
    const ImVec2 pos = ImVec2(0, 0);
    const ImVec2 size = io.DisplaySize;
#endif
    return Rect{ToVector2(pos), ToVector2(pos + size)};
}

ImGuizmo::OPERATION GetInternalOperation(TransformGizmoOperation op, TransformGizmoAxes axes)
{
    int result = 0;
    switch (op)
    {
    case TransformGizmoOperation::Translate:
        if (axes & TransformGizmoAxis::X)
            result |= ImGuizmo::TRANSLATE_X;
        if (axes & TransformGizmoAxis::Y)
            result |= ImGuizmo::TRANSLATE_Y;
        if (axes & TransformGizmoAxis::Z)
            result |= ImGuizmo::TRANSLATE_Z;
        break;

    case TransformGizmoOperation::Rotate:
        if (axes & TransformGizmoAxis::X)
            result |= ImGuizmo::ROTATE_X;
        if (axes & TransformGizmoAxis::Y)
            result |= ImGuizmo::ROTATE_Y;
        if (axes & TransformGizmoAxis::Z)
            result |= ImGuizmo::ROTATE_Z;
        if (axes & TransformGizmoAxis::Screen)
            result |= ImGuizmo::ROTATE_SCREEN;
        break;

    case TransformGizmoOperation::Scale:
        if (axes & TransformGizmoAxis::Universal)
            result |= ImGuizmo::SCALE_XU | ImGuizmo::SCALE_YU | ImGuizmo::SCALE_ZU;
        else
        {
            if (axes & TransformGizmoAxis::X)
                result |= ImGuizmo::SCALE_X;
            if (axes & TransformGizmoAxis::Y)
                result |= ImGuizmo::SCALE_Y;
            if (axes & TransformGizmoAxis::Z)
                result |= ImGuizmo::SCALE_Z;
        }
        break;

    default:
        URHO3D_ASSERT(0);
        result = ImGuizmo::TRANSLATE;
        break;
    }

    if (!result)
    {
        URHO3D_ASSERT(0);
        result = ImGuizmo::TRANSLATE;
    }

    return static_cast<ImGuizmo::OPERATION>(result);
}

} // namespace

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

ea::optional<Matrix4> TransformGizmo::ManipulateTransform(
    Matrix4& transform, TransformGizmoOperation op, TransformGizmoAxes axes, bool local, const Vector3& snap) const
{
    if (op == TransformGizmoOperation::None)
        return ea::nullopt;

    PrepareToManipulate();

    if (!ImGuizmo::IsUsing())
        internalTransformMatrix = transform.Transpose();

    const ImGuizmo::OPERATION operation = GetInternalOperation(op, axes);
    const ImGuizmo::MODE mode = local ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    const Vector3 snapVector = snap * Vector3::ONE;

    Matrix4 delta;
    ImGuizmo::Manipulate(internalViewMatrix_.Data(), internalProjMatrix_.Data(), operation, mode,
        &internalTransformMatrix.m00_, &delta.m00_, snap != Vector3::ZERO ? snapVector.Data() : nullptr);
    transform = internalTransformMatrix.Transpose();

    if (!ImGuizmo::IsUsing())
        return ea::nullopt;

    return delta.Transpose();
}

ea::optional<Vector3> TransformGizmo::ManipulatePosition(
    const Matrix4& transform, TransformGizmoAxes axes, bool local, const Vector3& snap) const
{
    Matrix4 transformCopy = transform;
    const auto delta = ManipulateTransform(transformCopy, TransformGizmoOperation::Translate, axes, local, snap);
    if (!delta)
        return ea::nullopt;

    return Matrix3x4(*delta).Translation();
}

ea::optional<Quaternion> TransformGizmo::ManipulateRotation(
    const Matrix4& transform, TransformGizmoAxes axes, bool local, float snap) const
{
    Matrix4 transformCopy = transform;
    const Vector3 snapVector = snap * Vector3::ONE;
    const auto delta = ManipulateTransform(transformCopy, TransformGizmoOperation::Rotate, axes, local, snapVector);
    if (!delta)
        return ea::nullopt;

    return Matrix3x4(*delta).Rotation();
}

ea::optional<Vector3> TransformGizmo::ManipulateScale(
    const Matrix4& transform, TransformGizmoAxes axes, bool local, float snap) const
{
    Matrix4 transformCopy = transform;
    const Vector3 snapVector = snap * Vector3::ONE;
    const auto delta = ManipulateTransform(transformCopy, TransformGizmoOperation::Scale, axes, local, snapVector);
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

TransformNodesGizmo::TransformNodesGizmo(Node* activeNode)
    : activeNode_(activeNode)
    , nodes_{WeakPtr<Node>{activeNode}}
{
}

bool TransformNodesGizmo::Manipulate(const TransformGizmo& gizmo, TransformGizmoOperation op, TransformGizmoAxes axes,
    bool local, bool pivoted, const Vector3& snap)
{
    switch (op)
    {
    case TransformGizmoOperation::Translate: return ManipulatePosition(gizmo, axes, local, pivoted, snap);

    case TransformGizmoOperation::Rotate: return ManipulateRotation(gizmo, axes, local, pivoted, snap);

    case TransformGizmoOperation::Scale: return ManipulateScale(gizmo, axes, local, pivoted, snap);

    default: return false;
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

bool TransformNodesGizmo::ManipulatePosition(
    const TransformGizmo& gizmo, TransformGizmoAxes axes, bool local, bool pivoted, const Vector3& snap)
{
    const auto delta = gizmo.ManipulatePosition(GetGizmoTransform(pivoted), axes, local, snap);
    if (!delta)
        return false;

    if (*delta == Vector3::ZERO)
        return true;

    for (Node* node : nodes_)
    {
        if (node)
        {
            const Transform& oldTransform = node->GetTransform();
            node->Translate(*delta, TS_WORLD);
            OnNodeTransformChanged(this, node, oldTransform);
        }
    }

    return true;
}

bool TransformNodesGizmo::ManipulateRotation(
    const TransformGizmo& gizmo, TransformGizmoAxes axes, bool local, bool pivoted, const Vector3& snap)
{
    const Matrix4 anchorTransform = GetGizmoTransform(pivoted);
    const auto delta = gizmo.ManipulateRotation(anchorTransform, axes, local, snap.x_);
    if (!delta)
        return false;

    if (*delta == Quaternion::IDENTITY)
        return true;

    for (Node* node : nodes_)
    {
        if (node)
        {
            const Transform& oldTransform = node->GetTransform();
            if (pivoted)
                node->Rotate(*delta, TS_WORLD);
            else
                node->RotateAround(anchorTransform.Translation(), *delta, TS_WORLD);
            OnNodeTransformChanged(this, node, oldTransform);
        }
    }

    return true;
}

bool TransformNodesGizmo::ManipulateScale(
    const TransformGizmo& gizmo, TransformGizmoAxes axes, bool local, bool pivoted, const Vector3& snap)
{
    const Matrix4 anchorTransform = GetGizmoTransform(pivoted);
    const auto delta = gizmo.ManipulateScale(anchorTransform, axes, local, snap.x_);
    if (!delta)
        return false;

    if (*delta == Vector3::ONE)
        return true;

    for (Node* node : nodes_)
    {
        if (node)
        {
            const Transform& oldTransform = node->GetTransform();
            if (pivoted)
                node->Scale(*delta);
            else
                node->ScaleAround(anchorTransform.Translation(), *delta, TS_WORLD);
            OnNodeTransformChanged(this, node, oldTransform);
        }
    }

    return true;
}

} // namespace Urho3D
