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

#include "../Core/Signal.h"
#include "../Scene/Node.h"

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

/// Utility class for gizmo manipulation. It's okay to recreate this class on every frame.
class URHO3D_API TransformGizmo
{
public:
    /// Setup gizmo for main viewport.
    explicit TransformGizmo(Camera* camera);
    /// Setup gizmo for current window rectangle.
    TransformGizmo(Camera* camera, const Rect& viewportRect);

    /// Manipulate transform. Returns delta matrix in world space.
    ea::optional<Matrix4> ManipulateTransform(Matrix4& transform, TransformGizmoOperation op, bool local, const Vector3& snap) const;

    /// Manipulate position. Returns delta position in world space.
    ea::optional<Vector3> ManipulatePosition(const Matrix4& transform, bool local, const Vector3& snap) const;
    /// Manipulate rotation. Returns delta rotation in world space.
    ea::optional<Quaternion> ManipulateRotation(const Matrix4& transform, bool local, float snap) const;
    /// Manipulate scale. Returns multiplicative delta scale in local space.
    ea::optional<Vector3> ManipulateScale(const Matrix4& transform, bool local, float snap) const;

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
    {}

    /// Manipulate nodes.
    bool Manipulate(const TransformGizmo& gizmo, TransformGizmoOperation op, bool local, bool pivoted, const Vector3& snap);

private:
    Matrix4 GetGizmoTransform(bool pivoted) const;
    bool ManipulatePosition(const TransformGizmo& gizmo, bool local, bool pivoted, const Vector3& snap);
    bool ManipulateRotation(const TransformGizmo& gizmo, bool local, bool pivoted, const Vector3& snap);
    bool ManipulateScale(const TransformGizmo& gizmo, bool local, bool pivoted, const Vector3& snap);

    WeakPtr<const Node> activeNode_;
    ea::vector<WeakPtr<Node>> nodes_;
};

}
