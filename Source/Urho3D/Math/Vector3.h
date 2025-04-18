//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Math/Vector2.h"
#include "../Math/MathDefs.h"

#include <EASTL/tuple.h>

namespace Urho3D
{

/// Three-dimensional vector with integer values.
class URHO3D_API IntVector3
{
public:
    /// Construct a zero vector.
    IntVector3() noexcept :
        x_(0),
        y_(0),
        z_(0)
    {
    }

    /// Construct from coordinates.
    IntVector3(int x, int y, int z) noexcept :
        x_(x),
        y_(y),
        z_(z)
    {
    }

    /// Construct from an int array.
    explicit IntVector3(const int data[]) noexcept :
        x_(data[0]),
        y_(data[1]),
        z_(data[2])
    {
    }

    /// Copy-construct from another vector.
    IntVector3(const IntVector3& rhs) noexcept = default;

    /// Assign from another vector.
    IntVector3& operator =(const IntVector3& rhs) noexcept = default;

    /// Test for equality with another vector.
    bool operator ==(const IntVector3& rhs) const { return x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_; }

    /// Test for inequality with another vector.
    bool operator !=(const IntVector3& rhs) const { return x_ != rhs.x_ || y_ != rhs.y_ || z_ != rhs.z_; }

    /// Lexicographic comparison for sorting.
    bool operator<(const IntVector3& rhs) const { return ea::tie(x_, y_, z_) < ea::tie(rhs.x_, rhs.y_, rhs.z_); }

    /// Add a vector.
    IntVector3 operator +(const IntVector3& rhs) const { return IntVector3(x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_); }

    /// Return negation.
    IntVector3 operator -() const { return IntVector3(-x_, -y_, -z_); }

    /// Subtract a vector.
    IntVector3 operator -(const IntVector3& rhs) const { return IntVector3(x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_); }

    /// Multiply with a scalar.
    IntVector3 operator *(int rhs) const { return IntVector3(x_ * rhs, y_ * rhs, z_ * rhs); }

    /// Multiply with a vector.
    IntVector3 operator *(const IntVector3& rhs) const { return IntVector3(x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_); }

    /// Divide by a scalar.
    IntVector3 operator /(int rhs) const { return IntVector3(x_ / rhs, y_ / rhs, z_ / rhs); }

    /// Divide by a vector.
    IntVector3 operator /(const IntVector3& rhs) const { return IntVector3(x_ / rhs.x_, y_ / rhs.y_, z_ / rhs.z_); }

    /// Add-assign a vector.
    IntVector3& operator +=(const IntVector3& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

    /// Subtract-assign a vector.
    IntVector3& operator -=(const IntVector3& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

    /// Multiply-assign a scalar.
    IntVector3& operator *=(int rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    IntVector3& operator *=(const IntVector3& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        z_ *= rhs.z_;
        return *this;
    }

    /// Divide-assign a scalar.
    IntVector3& operator /=(int rhs)
    {
        x_ /= rhs;
        y_ /= rhs;
        z_ /= rhs;
        return *this;
    }

    /// Divide-assign a vector.
    IntVector3& operator /=(const IntVector3& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        z_ /= rhs.z_;
        return *this;
    }

    /// Return integer data.
    const int* Data() const { return &x_; }

    /// Return as string.
    ea::string ToString() const;

    /// Return IntVector2 vector (z component is ignored).
    IntVector2 ToIntVector2() const { return {x_, y_}; }

    /// Return Vector2 vector (z component is ignored).
    Vector2 ToVector2() const { return {static_cast<float>(x_), static_cast<float>(y_)}; }

    /// Return Vector3 vector.
    Vector3 ToVector3() const;

    /// Return Vector4 vector.
    Vector4 ToVector4(float w = 0.0f) const;

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return (unsigned)x_ * 31 * 31 + (unsigned)y_ * 31 + (unsigned)z_; }

    /// Return length.
    float Length() const { return sqrtf((float)(x_ * x_ + y_ * y_ + z_ * z_)); }

    /// X coordinate.
    int x_;
    /// Y coordinate.
    int y_;
    /// Z coordinate.
    int z_;

    /// Zero vector.
    static const IntVector3 ZERO;
    /// (-1,0,0) vector.
    static const IntVector3 LEFT;
    /// (1,0,0) vector.
    static const IntVector3 RIGHT;
    /// (0,1,0) vector.
    static const IntVector3 UP;
    /// (0,-1,0) vector.
    static const IntVector3 DOWN;
    /// (0,0,1) vector.
    static const IntVector3 FORWARD;
    /// (0,0,-1) vector.
    static const IntVector3 BACK;
    /// (1,1,1) vector.
    static const IntVector3 ONE;
};

/// Three-dimensional vector.
class URHO3D_API Vector3
{
public:
    /// Construct a zero vector.
    Vector3() noexcept :
        x_(0.0f),
        y_(0.0f),
        z_(0.0f)
    {
    }

    /// Copy-construct from another vector.
    Vector3(const Vector3& vector) noexcept = default;

    /// Construct from coordinates.
    Vector3(float x, float y, float z) noexcept :
        x_(x),
        y_(y),
        z_(z)
    {
    }

    /// Construct from two-dimensional coordinates (for Urho2D).
    Vector3(float x, float y) noexcept :
        x_(x),
        y_(y),
        z_(0.0f)
    {
    }

    /// Construct from a float array.
    explicit Vector3(const float data[]) noexcept :
        x_(data[0]),
        y_(data[1]),
        z_(data[2])
    {
    }

    /// Construct from 2D vector in X0Z plane.
    static Vector3 FromXZ(const Vector2& vector, float y = 0.0f) { return { vector.x_, y, vector.y_ }; }

    /// Assign from another vector.
    Vector3& operator =(const Vector3& rhs) noexcept = default;

    /// Test for equality with another vector without epsilon.
    bool operator ==(const Vector3& rhs) const { return x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_; }

    /// Test for inequality with another vector without epsilon.
    bool operator !=(const Vector3& rhs) const { return x_ != rhs.x_ || y_ != rhs.y_ || z_ != rhs.z_; }

    /// Add a vector.
    Vector3 operator +(const Vector3& rhs) const { return Vector3(x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_); }

    /// Return negation.
    Vector3 operator -() const { return Vector3(-x_, -y_, -z_); }

    /// Subtract a vector.
    Vector3 operator -(const Vector3& rhs) const { return Vector3(x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_); }

    /// Multiply with a scalar.
    Vector3 operator *(float rhs) const { return Vector3(x_ * rhs, y_ * rhs, z_ * rhs); }

    /// Multiply with a vector.
    Vector3 operator *(const Vector3& rhs) const { return Vector3(x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_); }

    /// Divide by a scalar.
    Vector3 operator /(float rhs) const { return Vector3(x_ / rhs, y_ / rhs, z_ / rhs); }

    /// Divide by a vector.
    Vector3 operator /(const Vector3& rhs) const { return Vector3(x_ / rhs.x_, y_ / rhs.y_, z_ / rhs.z_); }

    /// Add-assign a vector.
    Vector3& operator +=(const Vector3& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

    /// Subtract-assign a vector.
    Vector3& operator -=(const Vector3& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

    /// Multiply-assign a scalar.
    Vector3& operator *=(float rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    Vector3& operator *=(const Vector3& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        z_ *= rhs.z_;
        return *this;
    }

    /// Divide-assign a scalar.
    Vector3& operator /=(float rhs)
    {
        float invRhs = 1.0f / rhs;
        x_ *= invRhs;
        y_ *= invRhs;
        z_ *= invRhs;
        return *this;
    }

    /// Divide-assign a vector.
    Vector3& operator /=(const Vector3& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        z_ /= rhs.z_;
        return *this;
    }

    /// Normalize to unit length.
    void Normalize()
    {
        float lenSquared = LengthSquared();
        if (!Urho3D::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
        {
            float invLen = 1.0f / sqrtf(lenSquared);
            x_ *= invLen;
            y_ *= invLen;
            z_ *= invLen;
        }
    }

    /// Return length.
    /// @property
    float Length() const { return sqrtf(x_ * x_ + y_ * y_ + z_ * z_); }

    /// Return squared length.
    /// @property
    float LengthSquared() const { return x_ * x_ + y_ * y_ + z_ * z_; }

    /// Calculate dot product.
    float DotProduct(const Vector3& rhs) const { return x_ * rhs.x_ + y_ * rhs.y_ + z_ * rhs.z_; }

    /// Calculate absolute dot product.
    float AbsDotProduct(const Vector3& rhs) const
    {
        return Urho3D::Abs(x_ * rhs.x_) + Urho3D::Abs(y_ * rhs.y_) + Urho3D::Abs(z_ * rhs.z_);
    }

    /// Project direction vector onto axis.
    float ProjectOntoAxis(const Vector3& axis) const { return DotProduct(axis.Normalized()); }

    /// Project position vector onto plane with given origin and normal.
    Vector3 ProjectOntoPlane(const Vector3& origin, const Vector3& normal) const
    {
        const Vector3 delta = *this - origin;
        return *this - normal.Normalized() * delta.ProjectOntoAxis(normal);
    }

    /// Project position vector onto line segment. Returns interpolation factor between line points.
    float ProjectOntoLineScalar(const Vector3& from, const Vector3& to, bool clamped = false) const
    {
        const Vector3 direction = to - from;
        const float lengthSquared = direction.LengthSquared();
        const float factor = (*this - from).DotProduct(direction) / lengthSquared;
        return clamped ? Clamp(factor, 0.0f, 1.0f) : factor;
    }

    /// Project position vector onto line segment. Returns new position.
    Vector3 ProjectOntoLine(const Vector3& from, const Vector3& to, bool clamped = false) const
    {
        return from.Lerp(to, ProjectOntoLineScalar(from, to, clamped));
    }

    /// Calculate distance to another position vector.
    float DistanceToPoint(const Vector3& point) const { return (*this - point).Length(); }

    /// Calculate distance to the plane with given origin and normal.
    float DistanceToPlane(const Vector3& origin, const Vector3& normal) const { return (*this - origin).ProjectOntoAxis(normal); }

    /// Make vector orthogonal to the axis.
    Vector3 Orthogonalize(const Vector3& axis) const { return axis.CrossProduct(*this).CrossProduct(axis).Normalized(); }

    /// Calculate cross product.
    Vector3 CrossProduct(const Vector3& rhs) const
    {
        return Vector3(
            y_ * rhs.z_ - z_ * rhs.y_,
            z_ * rhs.x_ - x_ * rhs.z_,
            x_ * rhs.y_ - y_ * rhs.x_
        );
    }

    /// Return absolute vector.
    Vector3 Abs() const { return Vector3(Urho3D::Abs(x_), Urho3D::Abs(y_), Urho3D::Abs(z_)); }

    /// Linear interpolation with another vector.
    Vector3 Lerp(const Vector3& rhs, float t) const { return *this * (1.0f - t) + rhs * t; }

    /// Test for equality with another vector with epsilon.
    bool Equals(const Vector3& rhs, float eps = M_EPSILON) const
    {
        return Urho3D::Equals(x_, rhs.x_, eps) && Urho3D::Equals(y_, rhs.y_, eps) && Urho3D::Equals(z_, rhs.z_, eps);
    }

    /// Returns the angle between this vector and another vector in degrees, from 0 to 180.
    float Angle(const Vector3& rhs) const { return Urho3D::Acos(DotProduct(rhs) / (Length() * rhs.Length())); }

    /// Returns the signed angle between this vector and another vector in degrees, from -180 to 180.
    /// Axis is used to determine the sign of the angle.
    /// If axis is orthogonal to both vectors, it is guaranteed that quaternion rotation with this axis and angle
    /// will result in transition from first vector to the second one.
    float SignedAngle(const Vector3& rhs, const Vector3& axis) const
    {
        const float angle = Angle(rhs);
        const float sign = (CrossProduct(rhs).DotProduct(axis) < 0.0f) ? -1.0f : 1.0f;
        return angle * sign;
    }

    /// Return whether any component is NaN.
    bool IsNaN() const { return Urho3D::IsNaN(x_) || Urho3D::IsNaN(y_) || Urho3D::IsNaN(z_); }

    /// Return whether any component is Inf.
    bool IsInf() const { return Urho3D::IsInf(x_) || Urho3D::IsInf(y_) || Urho3D::IsInf(z_); }

    /// Return normalized to unit length.
    Vector3 Normalized() const
    {
        const float lenSquared = LengthSquared();
        if (!Urho3D::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
        {
            float invLen = 1.0f / sqrtf(lenSquared);
            return *this * invLen;
        }
        else
            return *this;
    }

    /// Return normalized to unit length or zero if length is too small.
    Vector3 NormalizedOrDefault(const Vector3& defaultValue = Vector3::ZERO, float eps = M_LARGE_EPSILON) const
    {
        const float lenSquared = LengthSquared();
        if (lenSquared < eps * eps)
            return defaultValue;
        return *this / sqrtf(lenSquared);
    }

    /// Return normalized vector with length in given range.
    Vector3 ReNormalized(float minLength, float maxLength, const Vector3& defaultValue = Vector3::ZERO, float eps = M_LARGE_EPSILON) const
    {
        const float lenSquared = LengthSquared();
        if (lenSquared < eps * eps)
            return defaultValue;

        const float len = sqrtf(lenSquared);
        const float newLen = Clamp(len, minLength, maxLength);
        return *this * (newLen / len);
    }

    /// Return float data.
    const float* Data() const { return &x_; }

    /// Return as string.
    ea::string ToString() const;

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const
    {
        unsigned hash = 37;
        hash = 37 * hash + FloatToRawIntBits(x_);
        hash = 37 * hash + FloatToRawIntBits(y_);
        hash = 37 * hash + FloatToRawIntBits(z_);

        return hash;
    }

    /// Return IntVector2 vector (z component is ignored).
    IntVector2 ToIntVector2() const { return {static_cast<int>(x_), static_cast<int>(y_)}; }

    /// Return Vector2 vector (z component is ignored).
    Vector2 ToVector2() const { return {x_, y_}; }

    /// Return IntVector3 vector.
    IntVector3 ToIntVector3() const { return {static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(z_)}; }

    /// Return Vector4 vector.
    Vector4 ToVector4(float w = 0.0f) const;

    /// Return x and z components as 2D vector (y component is ignored).
    Vector2 ToXZ() const { return { x_, z_ }; }

    /// X coordinate.
    float x_;
    /// Y coordinate.
    float y_;
    /// Z coordinate.
    float z_;

    /// Zero vector.
    static const Vector3 ZERO;
    /// (-1,0,0) vector.
    static const Vector3 LEFT;
    /// (1,0,0) vector.
    static const Vector3 RIGHT;
    /// (0,1,0) vector.
    static const Vector3 UP;
    /// (0,-1,0) vector.
    static const Vector3 DOWN;
    /// (0,0,1) vector.
    static const Vector3 FORWARD;
    /// (0,0,-1) vector.
    static const Vector3 BACK;
    /// (1,1,1) vector.
    static const Vector3 ONE;
};

/// Multiply Vector3 with a scalar.
inline Vector3 operator *(float lhs, const Vector3& rhs) { return rhs * lhs; }

/// Multiply IntVector3 with a scalar.
inline IntVector3 operator *(int lhs, const IntVector3& rhs) { return rhs * lhs; }

/// Per-component linear interpolation between two 3-vectors.
inline Vector3 VectorLerp(const Vector3& lhs, const Vector3& rhs, const Vector3& t) { return lhs + (rhs - lhs) * t; }

/// Per-component min of two 3-vectors.
inline Vector3 VectorMin(const Vector3& lhs, const Vector3& rhs) { return Vector3(Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_), Min(lhs.z_, rhs.z_)); }

/// Per-component max of two 3-vectors.
inline Vector3 VectorMax(const Vector3& lhs, const Vector3& rhs) { return Vector3(Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_), Max(lhs.z_, rhs.z_)); }

/// Per-component floor of 3-vector.
inline Vector3 VectorFloor(const Vector3& vec) { return Vector3(Floor(vec.x_), Floor(vec.y_), Floor(vec.z_)); }

/// Per-component round of 3-vector.
inline Vector3 VectorRound(const Vector3& vec) { return Vector3(Round(vec.x_), Round(vec.y_), Round(vec.z_)); }

/// Per-component ceil of 3-vector.
inline Vector3 VectorCeil(const Vector3& vec) { return Vector3(Ceil(vec.x_), Ceil(vec.y_), Ceil(vec.z_)); }

/// Per-component absolute value of 3-vector.
inline Vector3 VectorAbs(const Vector3& vec) { return Vector3(Abs(vec.x_), Abs(vec.y_), Abs(vec.z_)); }

/// Per-component square root of 3-vector.
inline Vector3 VectorSqrt(const Vector3& vec) { return Vector3(Sqrt(vec.x_), Sqrt(vec.y_), Sqrt(vec.z_)); }

/// Per-component floor of 3-vector. Returns IntVector3.
inline IntVector3 VectorFloorToInt(const Vector3& vec) { return IntVector3(FloorToInt(vec.x_), FloorToInt(vec.y_), FloorToInt(vec.z_)); }

/// Per-component round of 3-vector. Returns IntVector3.
inline IntVector3 VectorRoundToInt(const Vector3& vec) { return IntVector3(RoundToInt(vec.x_), RoundToInt(vec.y_), RoundToInt(vec.z_)); }

/// Per-component ceil of 3-vector. Returns IntVector3.
inline IntVector3 VectorCeilToInt(const Vector3& vec) { return IntVector3(CeilToInt(vec.x_), CeilToInt(vec.y_), CeilToInt(vec.z_)); }

/// Per-component min of two 3-vectors.
inline IntVector3 VectorMin(const IntVector3& lhs, const IntVector3& rhs) { return IntVector3(Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_), Min(lhs.z_, rhs.z_)); }

/// Per-component max of two 3-vectors.
inline IntVector3 VectorMax(const IntVector3& lhs, const IntVector3& rhs) { return IntVector3(Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_), Max(lhs.z_, rhs.z_)); }

/// Per-component absolute value of integer 3-vector.
inline IntVector3 VectorAbs(const IntVector3& vec) { return IntVector3(Abs(vec.x_), Abs(vec.y_), Abs(vec.z_)); }

/// Return a random value from [0, 1) from 3-vector seed.
inline float StableRandom(const Vector3& seed) { return StableRandom(Vector2(StableRandom(Vector2(seed.x_, seed.y_)), seed.z_)); }

/// Return IntVector3 vector.
inline IntVector3 IntVector2::ToIntVector3(int z) const { return { x_, y_, z }; }

/// Return IntVector3 vector.
inline IntVector3 Vector2::ToIntVector3(int z) const { return { static_cast<int>(x_), static_cast<int>(y_), z }; }

/// Return Vector3 vector.
inline Vector3 IntVector2::ToVector3(float z) const { return { static_cast<float>(x_), static_cast<float>(y_), z }; }

/// Return Vector3 vector.
inline Vector3 Vector2::ToVector3(float z) const { return { x_, y_, z }; }

/// Return Vector3 vector.
inline Vector3 IntVector3::ToVector3() const { return { static_cast<float>(x_), static_cast<float>(y_), static_cast<float>(z_) }; }

}
