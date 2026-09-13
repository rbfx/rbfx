// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include <Urho3D/Math/Plane.h>
#include <Urho3D/Math/Ray.h>
#include <Urho3D/IO/Log.h>

#include "../DebugNew.h"

namespace Urho3D
{

// Static initialization order can not be relied on, so do not use Vector3 constants
const Plane Plane::UP(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f));

void Plane::Transform(const Matrix3& transform)
{
    Define(Matrix4(transform).Inverse().Transpose() * ToVector4());
}

void Plane::Transform(const Matrix3x4& transform)
{
    Define(transform.ToMatrix4().Inverse().Transpose() * ToVector4());
}

void Plane::Transform(const Matrix4& transform)
{
    Define(transform.Inverse().Transpose() * ToVector4());
}

Matrix3x4 Plane::ReflectionMatrix() const
{
    return Matrix3x4(
        -2.0f * normal_.x_ * normal_.x_ + 1.0f,
        -2.0f * normal_.x_ * normal_.y_,
        -2.0f * normal_.x_ * normal_.z_,
        -2.0f * normal_.x_ * d_,
        -2.0f * normal_.y_ * normal_.x_,
        -2.0f * normal_.y_ * normal_.y_ + 1.0f,
        -2.0f * normal_.y_ * normal_.z_,
        -2.0f * normal_.y_ * d_,
        -2.0f * normal_.z_ * normal_.x_,
        -2.0f * normal_.z_ * normal_.y_,
        -2.0f * normal_.z_ * normal_.z_ + 1.0f,
        -2.0f * normal_.z_ * d_
    );
}

Plane Plane::Transformed(const Matrix3& transform) const
{
    return Plane(Matrix4(transform).Inverse().Transpose() * ToVector4());
}

Plane Plane::Transformed(const Matrix3x4& transform) const
{
    return Plane(transform.ToMatrix4().Inverse().Transpose() * ToVector4());
}

Plane Plane::Transformed(const Matrix4& transform) const
{
    return Plane(transform.Inverse().Transpose() * ToVector4());
}

Ray Plane::Intersect(const Plane& other) const
{
    // Calculate the direction of the line
    const Vector3 direction{normal_.CrossProduct(other.normal_)};

    // If the direction is zero, then the planes are parallel and do not intersect
    if (direction.Equals(Vector3::ZERO, 1e-6f))
    {
        return Ray{GetPoint(), Vector3::ZERO};
    }

    // Calculate the determinant of the matrix using cross product
    const float det = normal_.DotProduct(other.normal_.CrossProduct(direction));

    // If the determinant is zero, then the planes do not all intersect at one point
    if (Urho3D::Abs(det) < 1e-6f)
    {
        return Ray{GetPoint(), direction};
    }

    const Vector3 point{d_ * other.normal_.CrossProduct(direction) + other.d_ * direction.CrossProduct(normal_)};

    return Ray{-point / det, direction};
}

Vector3 Plane::Intersect(const Plane& planeB, const Plane& planeC) const
{
    // Calculate the determinant of the matrix using cross product
    const float det = normal_.DotProduct(planeB.normal_.CrossProduct(planeC.normal_));

    // If the determinant is zero, then the planes do not all intersect at one point
    if (Urho3D::Abs(det) < 1e-6f)
    {
        return GetPoint();
    }

    // Calculate the intersection point using Cramer's rule
    const Vector3 result =
        (-d_ * planeB.normal_.CrossProduct(planeC.normal_) - planeB.d_ * planeC.normal_.CrossProduct(normal_)
            - planeC.d_ * normal_.CrossProduct(planeB.normal_))
        / det;

    return result;
}

}
