//
// Copyright (c) 2008-2016 the Urho3D project.
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

namespace Urho3D
{

/// Three-dimensional vector.
template<typename T>
class Vector3_
{
public:
    /// Construct a zero vector.
    Vector3_() :
        x_(0),
        y_(0),
        z_(0)
    {
    }

    /// Copy-construct from another vector.
    Vector3_(const Vector3_& vector) :
        x_(vector.x_),
        y_(vector.y_),
        z_(vector.z_)
    {
    }

    /// Construct from a two-dimensional vector and the Z coordinate.
    Vector3_(const Vector2& vector, T z) :
        x_(vector.x_),
        y_(vector.y_),
        z_(z)
    {
    }

    /// Construct from a two-dimensional vector (for Urho2D).
    Vector3_(const Vector2& vector) :
        x_(vector.x_),
        y_(vector.y_),
        z_(0)
    {
    }

    /// Construct from coordinates.
    Vector3_(T x, T y, T z) :
        x_(x),
        y_(y),
        z_(z)
    {
    }

    /// Construct from two-dimensional coordinates (for Urho2D).
    Vector3_(T x, T y) :
        x_(x),
        y_(y),
        z_(0)
    {
    }

    /// Construct from a data array.
    explicit Vector3_(const T* data) :
        x_(data[0]),
        y_(data[1]),
        z_(data[2])
    {
    }

    /// Assign from another vector.
    Vector3_& operator =(const Vector3_& rhs)
    {
        x_ = rhs.x_;
        y_ = rhs.y_;
        z_ = rhs.z_;
        return *this;
    }

    /// Test for equality with another vector without epsilon.
    bool operator ==(const Vector3_& rhs) const { return x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_; }

    /// Test for inequality with another vector without epsilon.
    bool operator !=(const Vector3_& rhs) const { return x_ != rhs.x_ || y_ != rhs.y_ || z_ != rhs.z_; }

    /// Add a vector.
    Vector3_ operator +(const Vector3_& rhs) const { return Vector3_(x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_); }

    /// Return negation.
    Vector3_ operator -() const { return Vector3_(-x_, -y_, -z_); }

    /// Subtract a vector.
    Vector3_ operator -(const Vector3_& rhs) const { return Vector3_(x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_); }

    /// Multiply with a scalar.
    Vector3_ operator *(float rhs) const { return Vector3_(x_ * rhs, y_ * rhs, z_ * rhs); }
    Vector3_ operator *(double rhs) const { return Vector3_(x_ * rhs, y_ * rhs, z_ * rhs); }
    Vector3_ operator *(int rhs) const { return Vector3_(x_ * rhs, y_ * rhs, z_ * rhs); }
    Vector3_ operator *(long long rhs) const { return Vector3_(x_ * rhs, y_ * rhs, z_ * rhs); }

    /// Multiply with a vector.
    Vector3_ operator *(const Vector3_& rhs) const { return Vector3_(x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_); }

    /// Divide by a scalar.
    Vector3_ operator /(float rhs) const { return Vector3_(x_ / rhs, y_ / rhs, z_ / rhs); }

    /// Divide by a vector.
    Vector3_ operator /(const Vector3_& rhs) const { return Vector3_(x_ / rhs.x_, y_ / rhs.y_, z_ / rhs.z_); }

    /// Add-assign a vector.
    Vector3_& operator +=(const Vector3_& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

    /// Subtract-assign a vector.
    Vector3_& operator -=(const Vector3_& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

    /// Multiply-assign a scalar.
    Vector3_& operator *=(float rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    Vector3_& operator *=(const Vector3_& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        z_ *= rhs.z_;
        return *this;
    }

    /// Divide-assign a scalar.
    Vector3_& operator /=(float rhs)
    {
        float invRhs = Reciprocal(rhs);
        x_ *= invRhs;
        y_ *= invRhs;
        z_ *= invRhs;
        return *this;
    }

    /// Divide-assign a vector.
    Vector3_& operator /=(const Vector3_& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        z_ /= rhs.z_;
        return *this;
    }

    /// Normalize to unit length.
    void Normalize()
    {
        T lenSquared = LengthSquared();
        if (!Urho3D::Equals(lenSquared, static_cast<T>(1)) && lenSquared > static_cast<T>(0))
        {
            T invLen = ReciprocalSqrt(lenSquared);
            x_ *= invLen;
            y_ *= invLen;
            z_ *= invLen;
        }
    }

    /// Return length.
    T Length() const { return Sqrt(x_ * x_ + y_ * y_ + z_ * z_); }

    /// Return squared length.
    T LengthSquared() const { return x_ * x_ + y_ * y_ + z_ * z_; }

    /// Calculate dot product.
    T DotProduct(const Vector3_& rhs) const { return x_ * rhs.x_ + y_ * rhs.y_ + z_ * rhs.z_; }

    /// Calculate absolute dot product.
    T AbsDotProduct(const Vector3_& rhs) const
    {
        return Urho3D::Abs(x_ * rhs.x_) + Urho3D::Abs(y_ * rhs.y_) + Urho3D::Abs(z_ * rhs.z_);
    }

    /// Calculate cross product.
    Vector3_ CrossProduct(const Vector3_& rhs) const
    {
        return Vector3_(
            y_ * rhs.z_ - z_ * rhs.y_,
            z_ * rhs.x_ - x_ * rhs.z_,
            x_ * rhs.y_ - y_ * rhs.x_
        );
    }

    /// Return absolute vector.
    Vector3_ Abs() const { return Vector3_(Urho3D::Abs(x_), Urho3D::Abs(y_), Urho3D::Abs(z_)); }

    /// Linear interpolation with another vector.
    Vector3_ Lerp(const Vector3_& rhs, float t) const { return *this * (static_cast<T>(1) - t) + rhs * t; }

    /// Test for equality with another vector with epsilon.
    bool Equals(const Vector3_& rhs) const
    {
        return Urho3D::Equals(x_, rhs.x_) && Urho3D::Equals(y_, rhs.y_) && Urho3D::Equals(z_, rhs.z_);
    }

    /// Returns the angle between this vector and another vector in degrees.
    float Angle(const Vector3_& rhs) const { return Urho3D::Acos(DotProduct(rhs) / (Length() * rhs.Length())); }

    /// Return whether is NaN.
    bool IsNaN() const { return Urho3D::IsNaN(x_) || Urho3D::IsNaN(y_) || Urho3D::IsNaN(z_); }

    /// Return normalized to unit length.
    Vector3_ Normalized() const
    {
        T lenSquared = LengthSquared();
        if (!Urho3D::Equals(lenSquared, static_cast<T>(1)) && lenSquared > static_cast<T>(0))
        {
            T invLen = ReciprocalSqrt(lenSquared);
            return *this * invLen;
        }
        else
            return *this;
    }

    /// Return float data.
    const T* Data() const { return &x_; }

    /// Return as string.
    String ToString() const;

    /// X coordinate.
    T x_;
    /// Y coordinate.
    T y_;
    /// Z coordinate.
    T z_;

    /// Zero vector.
    static const Vector3_ ZERO;
    /// (-1,0,0) vector.
    static const Vector3_ LEFT;
    /// (1,0,0) vector.
    static const Vector3_ RIGHT;
    /// (0,1,0) vector.
    static const Vector3_ UP;
    /// (0,-1,0) vector.
    static const Vector3_ DOWN;
    /// (0,0,1) vector.
    static const Vector3_ FORWARD;
    /// (0,0,-1) vector.
    static const Vector3_ BACK;
    /// (1,1,1) vector.
    static const Vector3_ ONE;
};

/// Multiply Vector3_ with a scalar.

typedef Vector3_<float> Vector3;
typedef Vector3_<double> DoubleVector3;
typedef Vector3_<int> IntVector3;
typedef Vector3_<long long> LongLongVector3;

template class URHO3D_API Vector3_<float>;
template class URHO3D_API Vector3_<double>;
template class URHO3D_API Vector3_<int>;
template class URHO3D_API Vector3_<long long>;

inline Vector3 operator *(float lhs, const Vector3& rhs) { return rhs * lhs; }
inline Vector3 operator *(double lhs, const Vector3& rhs) { return rhs * lhs; }
inline Vector3 operator *(int lhs, const Vector3& rhs) { return rhs * lhs; }
inline Vector3 operator *(long long lhs, const Vector3& rhs) { return rhs * lhs; }

inline DoubleVector3 operator *(float lhs, const DoubleVector3& rhs) { return rhs * lhs; }
inline DoubleVector3 operator *(double lhs, const DoubleVector3& rhs) { return rhs * lhs; }
inline DoubleVector3 operator *(int lhs, const DoubleVector3& rhs) { return rhs * lhs; }
inline DoubleVector3 operator *(long long lhs, const DoubleVector3& rhs) { return rhs * lhs; }

inline IntVector3 operator *(float lhs, const IntVector3& rhs) { return rhs * lhs; }
inline IntVector3 operator *(double lhs, const IntVector3& rhs) { return rhs * lhs; }
inline IntVector3 operator *(int lhs, const IntVector3& rhs) { return rhs * lhs; }
inline IntVector3 operator *(long long lhs, const IntVector3& rhs) { return rhs * lhs; }

inline LongLongVector3 operator *(float lhs, const LongLongVector3& rhs) { return rhs * lhs; }
inline LongLongVector3 operator *(double lhs, const LongLongVector3& rhs) { return rhs * lhs; }
inline LongLongVector3 operator *(int lhs, const LongLongVector3& rhs) { return rhs * lhs; }
inline LongLongVector3 operator *(long long lhs, const LongLongVector3& rhs) { return rhs * lhs; }

}
