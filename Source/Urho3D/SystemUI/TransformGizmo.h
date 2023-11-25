// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/FlagSet.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/Scene/Node.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class Camera;

/// Gizmo manipulation mode.
enum class TransformGizmoOperation
{
    None,
    Translate,
    Rotate,
    Scale
};

/// Gizmo manipulation axes.
enum class TransformGizmoAxis
{
    None = 0,
    X = 1 << 0,
    Y = 1 << 1,
    Z = 1 << 2,
    Screen = 1 << 3,
    Universal = 1 << 4,
};
URHO3D_FLAGSET(TransformGizmoAxis, TransformGizmoAxes);

/// Utility class for gizmo manipulation. It's okay to recreate this class on every frame.
class URHO3D_API TransformGizmo
{
public:
    /// Setup gizmo for main viewport.
    explicit TransformGizmo(Camera* camera);
    /// Setup gizmo for current window rectangle.
    TransformGizmo(Camera* camera, const Rect& viewportRect);

    /// Manipulate transform. Returns delta matrix in world space.
    ea::optional<Matrix4> ManipulateTransform(
        Matrix4& transform, TransformGizmoOperation op, TransformGizmoAxes axes, bool local, const Vector3& snap) const;

    /// Manipulate position. Returns delta position in world space.
    ea::optional<Vector3> ManipulatePosition(
        const Matrix4& transform, TransformGizmoAxes axes, bool local, const Vector3& snap) const;
    /// Manipulate rotation. Returns delta rotation in world space.
    ea::optional<Quaternion> ManipulateRotation(
        const Matrix4& transform, TransformGizmoAxes axes, bool local, float snap) const;
    /// Manipulate scale. Returns multiplicative delta scale in local space.
    ea::optional<Vector3> ManipulateScale(
        const Matrix4& transform, TransformGizmoAxes axes, bool local, float snap) const;

private:
    TransformGizmo(Camera* camera, bool isMainViewport, const Rect& viewportRect);
    void PrepareToManipulate() const;

    Camera* const camera_{};
    const Matrix4 internalViewMatrix_;
    const Matrix4 internalProjMatrix_;

    const bool isMainViewport_{};
    const Rect viewportRect_;

    static Matrix4 internalTransformMatrix;
};

/// Gizmo for manipulating a set of scene nodes.
class URHO3D_API TransformNodesGizmo
{
public:
    Signal<void(Node* node, const Transform& oldTransform), TransformNodesGizmo> OnNodeTransformChanged;

    TransformNodesGizmo() = default;
    template <class Iter>
    TransformNodesGizmo(const Node* activeNode, Iter begin, Iter end)
        : activeNode_(activeNode)
        , nodes_(begin, end)
    {
    }
    explicit TransformNodesGizmo(Node* activeNode);

    /// Manipulate nodes.
    bool Manipulate(const TransformGizmo& gizmo, TransformGizmoOperation op, TransformGizmoAxes axes, bool local,
        bool pivoted, const Vector3& snap);

private:
    Matrix4 GetGizmoTransform(bool pivoted) const;
    bool ManipulatePosition(
        const TransformGizmo& gizmo, TransformGizmoAxes axes, bool local, bool pivoted, const Vector3& snap);
    bool ManipulateRotation(
        const TransformGizmo& gizmo, TransformGizmoAxes axes, bool local, bool pivoted, const Vector3& snap);
    bool ManipulateScale(
        const TransformGizmo& gizmo, TransformGizmoAxes axes, bool local, bool pivoted, const Vector3& snap);

    WeakPtr<const Node> activeNode_;
    ea::vector<WeakPtr<Node>> nodes_;
};

} // namespace Urho3D
