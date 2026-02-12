// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/Vector2.h"
#include "Urho3D/Math/VectorCommon.h"

#include <EASTL/string.h>
#include <EASTL/tuple.h>

#include <type_traits>

namespace Urho3D
{

/// Three-dimensional vector with integer values.
template <class T> class BaseIntegerVector3 : public Detail::VectorTraits<T, 3>
{
public:
    /// Construct a zero vector.
    constexpr BaseIntegerVector3() noexcept = default;

    /// Construct from coordinates.
    constexpr BaseIntegerVector3(T x, T y, T z) noexcept
        : x_(x)
        , y_(y)
        , z_(z)
    {
    }

    /// Construct from an array.
    constexpr explicit BaseIntegerVector3(const T data[]) noexcept
        : x_(data[0])
        , y_(data[1])
        , z_(data[2])
    {
    }

    /// Convert to tuple.
    constexpr auto Tie() const { return ea::tie(x_, y_, z_); }

    /// Test for equality with another vector.
    constexpr bool operator==(const BaseIntegerVector3& rhs) const { return Tie() == rhs.Tie(); }

    /// Test for inequality with another vector.
    constexpr bool operator!=(const BaseIntegerVector3& rhs) const { return Tie() != rhs.Tie(); }

    /// Lexicographic comparison for sorting.
    constexpr bool operator<(const BaseIntegerVector3& rhs) const { return Tie() < rhs.Tie(); }

    /// Add a vector.
    constexpr BaseIntegerVector3 operator+(const BaseIntegerVector3& rhs) const { return {x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_}; }

    /// Return negation.
    constexpr BaseIntegerVector3 operator-() const { return {-x_, -y_, -z_}; }

    /// Subtract a vector.
    constexpr BaseIntegerVector3 operator-(const BaseIntegerVector3& rhs) const { return {x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_}; }

    /// Multiply with a scalar.
    constexpr BaseIntegerVector3 operator*(T rhs) const { return {x_ * rhs, y_ * rhs, z_ * rhs}; }

    /// Multiply with a vector.
    constexpr BaseIntegerVector3 operator*(const BaseIntegerVector3& rhs) const { return {x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_}; }

    /// Divide by a scalar.
    constexpr BaseIntegerVector3 operator/(T rhs) const { return {x_ / rhs, y_ / rhs, z_ / rhs}; }

    /// Divide by a vector.
    constexpr BaseIntegerVector3 operator/(const BaseIntegerVector3& rhs) const { return {x_ / rhs.x_, y_ / rhs.y_, z_ / rhs.z_}; }

    /// Add-assign a vector.
    constexpr BaseIntegerVector3& operator+=(const BaseIntegerVector3& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

    /// Subtract-assign a vector.
    constexpr BaseIntegerVector3& operator-=(const BaseIntegerVector3& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

    /// Multiply-assign a scalar.
    constexpr BaseIntegerVector3& operator*=(T rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    constexpr BaseIntegerVector3& operator*=(const BaseIntegerVector3& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        z_ *= rhs.z_;
        return *this;
    }

    /// Divide-assign a scalar.
    constexpr BaseIntegerVector3& operator/=(T rhs)
    {
        x_ /= rhs;
        y_ /= rhs;
        z_ /= rhs;
        return *this;
    }

    /// Divide-assign a vector.
    constexpr BaseIntegerVector3& operator/=(const BaseIntegerVector3& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        z_ /= rhs.z_;
        return *this;
    }

    /// Return integer data.
    constexpr const T* Data() const { return &x_; }

    /// Return integer data.
    constexpr T* MutableData() { return &x_; }

    /// Return as string.
    ea::string ToString() const
    {
        if constexpr (std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= 4)
            return ea::string{ea::string::CtorSprintf{}, "%d %d %d", x_, y_, z_};
        else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) <= 4)
            return ea::string{ea::string::CtorSprintf{}, "%u %u %u", x_, y_, z_};
        else
            return ea::string{"? ? ?"};
    }

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return Detail::CalculateVectorHash(*this); }

    /// Cast vector to different type.
    template <class OtherVectorType> constexpr OtherVectorType Cast(typename OtherVectorType::ScalarType w = {}) const
    {
        return Detail::CreateVectorCast<OtherVectorType>(x_, y_, z_, w);
    }

    /// Cast vector to different type. Y and Z components are shuffled.
    template <class OtherVectorType> constexpr OtherVectorType CastXZ(typename OtherVectorType::ScalarType w = {}) const
    {
        return Detail::CreateVectorCast<OtherVectorType>(x_, z_, y_, w);
    }

    /// Return IntVector2 vector (z component is ignored).
    IntVector2 ToIntVector2() const { return Cast<IntVector2>(); }

    /// Return Vector2 vector (z component is ignored).
    Vector2 ToVector2() const { return Cast<Vector2>(); }

    /// Return Vector3 vector.
    Vector3 ToVector3() const;

    /// Return Vector4 vector.
    Vector4 ToVector4(float w = 0.0f) const;

    /// Return length.
    T Length() const { return static_cast<T>(Sqrt(static_cast<double>(x_ * x_ + y_ * y_ + z_ * z_))); }

    /// X coordinate.
    T x_{};
    /// Y coordinate.
    T y_{};
    /// Z coordinate.
    T z_{};

    /// Zero vector.
    static const BaseIntegerVector3<T> ZERO;
    /// (-1,0,0) vector.
    static const BaseIntegerVector3<T> LEFT;
    /// (1,0,0) vector.
    static const BaseIntegerVector3<T> RIGHT;
    /// (0,1,0) vector.
    static const BaseIntegerVector3<T> UP;
    /// (0,-1,0) vector.
    static const BaseIntegerVector3<T> DOWN;
    /// (0,0,1) vector.
    static const BaseIntegerVector3<T> FORWARD;
    /// (0,0,-1) vector.
    static const BaseIntegerVector3<T> BACK;
    /// (1,1,1) vector.
    static const BaseIntegerVector3<T> ONE;
};

/// Three-dimensional vector with real-number values.
template <class T> class BaseVector3 : public Detail::VectorTraits<T, 3>
{
public:
    /// Construct a zero vector.
    constexpr BaseVector3() noexcept = default;

    /// Construct from coordinates.
    constexpr BaseVector3(T x, T y, T z = T{}) noexcept
        : x_(x)
        , y_(y)
        , z_(z)
    {
    }

    /// Construct from a scalar array.
    constexpr explicit BaseVector3(const T data[]) noexcept
        : x_(data[0])
        , y_(data[1])
        , z_(data[2])
    {
    }

    /// Construct from 2D vector in X0Z plane.
    static constexpr BaseVector3 FromXZ(const Vector2& vector, T y = T{0}) { return {vector.x_, y, vector.y_}; }

    /// Convert to tuple.
    constexpr auto Tie() const { return ea::tie(x_, y_, z_); }

    /// Test for equality with another vector without epsilon.
    constexpr bool operator==(const BaseVector3& rhs) const { return Tie() == rhs.Tie(); }

    /// Test for inequality with another vector without epsilon.
    constexpr bool operator!=(const BaseVector3& rhs) const { return Tie() != rhs.Tie(); }

    /// Add a vector.
    constexpr BaseVector3 operator+(const BaseVector3& rhs) const { return {x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_}; }

    /// Return negation.
    constexpr BaseVector3 operator-() const { return {-x_, -y_, -z_}; }

    /// Subtract a vector.
    constexpr BaseVector3 operator-(const BaseVector3& rhs) const { return {x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_}; }

    /// Multiply with a scalar.
    constexpr BaseVector3 operator*(T rhs) const { return {x_ * rhs, y_ * rhs, z_ * rhs}; }

    /// Multiply with a vector.
    constexpr BaseVector3 operator*(const BaseVector3& rhs) const { return {x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_}; }

    /// Divide by a scalar.
    constexpr BaseVector3 operator/(T rhs) const { return {x_ / rhs, y_ / rhs, z_ / rhs}; }

    /// Divide by a vector.
    constexpr BaseVector3 operator/(const BaseVector3& rhs) const { return {x_ / rhs.x_, y_ / rhs.y_, z_ / rhs.z_}; }

    /// Add-assign a vector.
    constexpr BaseVector3& operator+=(const BaseVector3& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

    /// Subtract-assign a vector.
    constexpr BaseVector3& operator-=(const BaseVector3& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

    /// Multiply-assign a scalar.
    constexpr BaseVector3& operator*=(T rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    constexpr BaseVector3& operator*=(const BaseVector3& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        z_ *= rhs.z_;
        return *this;
    }

    /// Divide-assign a scalar.
    constexpr BaseVector3& operator/=(T rhs)
    {
        const T invRhs = T{1} / rhs;
        x_ *= invRhs;
        y_ *= invRhs;
        z_ *= invRhs;
        return *this;
    }

    /// Divide-assign a vector.
    constexpr BaseVector3& operator/=(const BaseVector3& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        z_ /= rhs.z_;
        return *this;
    }

    /// Normalize to unit length.
    void Normalize()
    {
        const T lenSquared = LengthSquared();
        if (!Urho3D::Equals(lenSquared, T{1}) && lenSquared > T{0})
        {
            const T invLen = T{1} / Sqrt(lenSquared);
            x_ *= invLen;
            y_ *= invLen;
            z_ *= invLen;
        }
    }

    /// Return length.
    /// @property
    T Length() const { return Sqrt(x_ * x_ + y_ * y_ + z_ * z_); }

    /// Return squared length.
    /// @property
    constexpr T LengthSquared() const { return x_ * x_ + y_ * y_ + z_ * z_; }

    /// Calculate dot product.
    constexpr T DotProduct(const BaseVector3& rhs) const { return x_ * rhs.x_ + y_ * rhs.y_ + z_ * rhs.z_; }

    /// Calculate absolute dot product.
    T AbsDotProduct(const BaseVector3& rhs) const { return Abs(x_ * rhs.x_) + Abs(y_ * rhs.y_) + Abs(z_ * rhs.z_); }

    /// Project direction vector onto axis.
    T ProjectOntoAxis(const BaseVector3& axis) const { return DotProduct(axis.Normalized()); }

    /// Project position vector onto plane with given origin and normal.
    BaseVector3 ProjectOntoPlane(const BaseVector3& origin, const BaseVector3& normal) const
    {
        const BaseVector3 delta = *this - origin;
        return *this - normal.Normalized() * delta.ProjectOntoAxis(normal);
    }

    /// Project position vector onto line segment. Returns interpolation factor between line points.
    T ProjectOntoLineScalar(const BaseVector3& from, const BaseVector3& to, bool clamped = false) const
    {
        const BaseVector3 direction = to - from;
        const T lengthSquared = direction.LengthSquared();
        const T factor = (*this - from).DotProduct(direction) / lengthSquared;
        return clamped ? Clamp(factor, T{0}, T{1}) : factor;
    }

    /// Project position vector onto line segment. Returns new position.
    BaseVector3 ProjectOntoLine(const BaseVector3& from, const BaseVector3& to, bool clamped = false) const
    {
        return from.Lerp(to, ProjectOntoLineScalar(from, to, clamped));
    }

    /// Calculate distance to another position vector.
    T DistanceToPoint(const BaseVector3& point) const { return (*this - point).Length(); }

    /// Calculate distance to the plane with given origin and normal.
    T DistanceToPlane(const BaseVector3& origin, const BaseVector3& normal) const
    {
        return (*this - origin).ProjectOntoAxis(normal);
    }

    /// Make vector orthogonal to the axis.
    BaseVector3 Orthogonalize(const BaseVector3& axis) const
    {
        return axis.CrossProduct(*this).CrossProduct(axis).Normalized();
    }

    /// Calculate cross product.
    constexpr BaseVector3 CrossProduct(const BaseVector3& rhs) const
    {
        return {
            y_ * rhs.z_ - z_ * rhs.y_,
            z_ * rhs.x_ - x_ * rhs.z_,
            x_ * rhs.y_ - y_ * rhs.x_,
        };
    }

    /// Return absolute vector.
    BaseVector3 Abs() const { return BaseVector3(Urho3D::Abs(x_), Urho3D::Abs(y_), Urho3D::Abs(z_)); }

    /// Linear interpolation with another vector.
    BaseVector3 Lerp(const BaseVector3& rhs, T t) const { return *this * (T{1} - t) + rhs * t; }

    /// Test for equality with another vector with epsilon.
    bool Equals(const BaseVector3& rhs, T eps = static_cast<T>(M_EPSILON)) const
    {
        return Urho3D::Equals(x_, rhs.x_, eps) && Urho3D::Equals(y_, rhs.y_, eps) && Urho3D::Equals(z_, rhs.z_, eps);
    }

    /// Returns the angle between this vector and another vector in degrees, from 0 to 180.
    T Angle(const BaseVector3& rhs) const { return Urho3D::Acos(DotProduct(rhs) / (Length() * rhs.Length())); }

    /// Returns the signed angle between this vector and another vector in degrees, from -180 to 180.
    /// Axis is used to determine the sign of the angle.
    /// If axis is orthogonal to both vectors, it is guaranteed that quaternion rotation with this axis and angle
    /// will result in transition from first vector to the second one.
    T SignedAngle(const BaseVector3& rhs, const BaseVector3& axis) const
    {
        const T angle = Angle(rhs);
        const T sign = (CrossProduct(rhs).DotProduct(axis) < T{0}) ? T{-1} : T{1};
        return angle * sign;
    }

    /// Return whether any component is NaN.
    bool IsNaN() const { return Urho3D::IsNaN(x_) || Urho3D::IsNaN(y_) || Urho3D::IsNaN(z_); }

    /// Return whether any component is Inf.
    bool IsInf() const { return Urho3D::IsInf(x_) || Urho3D::IsInf(y_) || Urho3D::IsInf(z_); }

    /// Return normalized to unit length.
    BaseVector3 Normalized() const
    {
        const T lenSquared = LengthSquared();
        if (!Urho3D::Equals(lenSquared, T{1}) && lenSquared > T{0})
        {
            const T invLen = T{1} / Sqrt(lenSquared);
            return *this * invLen;
        }
        else
            return *this;
    }

    /// Return normalized to unit length or zero if length is too small.
    BaseVector3 NormalizedOrDefault(
        const BaseVector3& defaultValue = BaseVector3::ZERO, T eps = static_cast<T>(M_LARGE_EPSILON)) const
    {
        const T lenSquared = LengthSquared();
        if (lenSquared < eps * eps)
            return defaultValue;
        return *this / Sqrt(lenSquared);
    }

    /// Return normalized vector with length in given range.
    BaseVector3 ReNormalized(T minLength, T maxLength, const BaseVector3& defaultValue = BaseVector3::ZERO,
        T eps = static_cast<T>(M_LARGE_EPSILON)) const
    {
        const T lenSquared = LengthSquared();
        if (lenSquared < eps * eps)
            return defaultValue;
        const T len = Sqrt(lenSquared);
        const T newLen = Clamp(len, minLength, maxLength);
        return *this * (newLen / len);
    }

    /// Return data.
    constexpr const T* Data() const { return &x_; }

    /// Return mutable data.
    constexpr T* MutableData() { return &x_; }

    /// Return as string.
    ea::string ToString() const
    {
        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
            return ea::string{ea::string::CtorSprintf{}, "%g %g %g", x_, y_, z_};
        else
            return ea::string{"? ? ?"};
    }

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return Detail::CalculateVectorHash(*this); }

    /// Cast vector to different type.
    template <class OtherVectorType> constexpr OtherVectorType Cast(typename OtherVectorType::ScalarType w = {}) const
    {
        return Detail::CreateVectorCast<OtherVectorType>(x_, y_, z_, w);
    }

    /// Cast vector to different type. Y and Z components are shuffled.
    template <class OtherVectorType> constexpr OtherVectorType CastXZ(typename OtherVectorType::ScalarType w = {}) const
    {
        return Detail::CreateVectorCast<OtherVectorType>(x_, z_, y_, w);
    }

    /// Return IntVector2 vector (z component is ignored).
    IntVector2 ToIntVector2() const { return Cast<IntVector2>(); }

    /// Return Vector2 vector (z component is ignored).
    Vector2 ToVector2() const { return Cast<Vector2>(); }

    /// Return IntVector3 vector.
    IntVector3 ToIntVector3() const { return Cast<IntVector3>(); }

    /// Return Vector4 vector.
    Vector4 ToVector4(float w = 0.0f) const;

    /// Return x and z components as 2D vector (y component is ignored).
    Vector2 ToXZ() const { return CastXZ<Vector2>(); }

    /// X coordinate.
    T x_{};
    /// Y coordinate.
    T y_{};
    /// Z coordinate.
    T z_{};

    /// Zero vector.
    static const BaseVector3<T> ZERO;
    /// (-1,0,0) vector.
    static const BaseVector3<T> LEFT;
    /// (1,0,0) vector.
    static const BaseVector3<T> RIGHT;
    /// (0,1,0) vector.
    static const BaseVector3<T> UP;
    /// (0,-1,0) vector.
    static const BaseVector3<T> DOWN;
    /// (0,0,1) vector.
    static const BaseVector3<T> FORWARD;
    /// (0,0,-1) vector.
    static const BaseVector3<T> BACK;
    /// (1,1,1) vector.
    static const BaseVector3<T> ONE;
};

/// Multiply BaseVector3 with a scalar.
template <class T> inline BaseVector3<T> operator *(T lhs, const BaseVector3<T>& rhs) { return rhs * lhs; }

/// Multiply BaseIntegerVector3 with a scalar.
template <class T> inline BaseIntegerVector3<T> operator *(T lhs, const BaseIntegerVector3<T>& rhs) { return rhs * lhs; }

/// Per-component linear interpolation between two 3-vectors.
template <class T> inline BaseVector3<T> VectorLerp(const BaseVector3<T>& lhs, const BaseVector3<T>& rhs, const BaseVector3<T>& t) { return lhs + (rhs - lhs) * t; }

/// Per-component min of two 3-vectors.
template <class T> inline BaseVector3<T> VectorMin(const BaseVector3<T>& lhs, const BaseVector3<T>& rhs) { return BaseVector3<T>(Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_), Min(lhs.z_, rhs.z_)); }

/// Per-component max of two 3-vectors.
template <class T> inline BaseVector3<T> VectorMax(const BaseVector3<T>& lhs, const BaseVector3<T>& rhs) { return BaseVector3<T>(Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_), Max(lhs.z_, rhs.z_)); }

/// Per-component floor of 3-vector.
template <class T> inline BaseVector3<T> VectorFloor(const BaseVector3<T>& vec) { return BaseVector3<T>(Floor(vec.x_), Floor(vec.y_), Floor(vec.z_)); }

/// Per-component round of 3-vector.
template <class T> inline BaseVector3<T> VectorRound(const BaseVector3<T>& vec) { return BaseVector3<T>(Round(vec.x_), Round(vec.y_), Round(vec.z_)); }

/// Per-component ceil of 3-vector.
template <class T> inline BaseVector3<T> VectorCeil(const BaseVector3<T>& vec) { return BaseVector3<T>(Ceil(vec.x_), Ceil(vec.y_), Ceil(vec.z_)); }

/// Per-component absolute value of 3-vector.
template <class T> inline BaseVector3<T> VectorAbs(const BaseVector3<T>& vec) { return BaseVector3<T>(Abs(vec.x_), Abs(vec.y_), Abs(vec.z_)); }

/// Per-component square root of 3-vector.
template <class T> inline BaseVector3<T> VectorSqrt(const BaseVector3<T>& vec) { return BaseVector3<T>(Sqrt(vec.x_), Sqrt(vec.y_), Sqrt(vec.z_)); }

/// Per-component floor of 3-vector. Returns BaseIntegerVector3.
template <class T> inline IntVector3 VectorFloorToInt(const BaseVector3<T>& vec) { return IntVector3(FloorToInt(vec.x_), FloorToInt(vec.y_), FloorToInt(vec.z_)); }

/// Per-component round of 3-vector. Returns BaseIntegerVector3.
template <class T> inline IntVector3 VectorRoundToInt(const BaseVector3<T>& vec) { return IntVector3(RoundToInt(vec.x_), RoundToInt(vec.y_), RoundToInt(vec.z_)); }

/// Per-component ceil of 3-vector. Returns BaseIntegerVector3.
template <class T> inline IntVector3 VectorCeilToInt(const BaseVector3<T>& vec) { return IntVector3(CeilToInt(vec.x_), CeilToInt(vec.y_), CeilToInt(vec.z_)); }

/// Per-component min of two integer 3-vectors.
template <class T> inline BaseIntegerVector3<T> VectorMin(const BaseIntegerVector3<T>& lhs, const BaseIntegerVector3<T>& rhs) { return BaseIntegerVector3<T>(Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_), Min(lhs.z_, rhs.z_)); }

/// Per-component max of two integer 3-vectors.
template <class T> inline BaseIntegerVector3<T> VectorMax(const BaseIntegerVector3<T>& lhs, const BaseIntegerVector3<T>& rhs) { return BaseIntegerVector3<T>(Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_), Max(lhs.z_, rhs.z_)); }

/// Per-component absolute value of integer 3-vector.
template <class T> inline BaseIntegerVector3<T> VectorAbs(const BaseIntegerVector3<T>& vec) { return BaseIntegerVector3<T>(Abs(vec.x_), Abs(vec.y_), Abs(vec.z_)); }

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
template <class T> Vector3 BaseIntegerVector3<T>::ToVector3() const { return Cast<Vector3>(); }

template <class T> inline constexpr BaseVector3<T> BaseVector3<T>::ZERO{T{0}, T{0}, T{0}};
template <class T> inline constexpr BaseVector3<T> BaseVector3<T>::LEFT{T{-1}, T{0}, T{0}};
template <class T> inline constexpr BaseVector3<T> BaseVector3<T>::RIGHT{T{1}, T{0}, T{0}};
template <class T> inline constexpr BaseVector3<T> BaseVector3<T>::UP{T{0}, T{1}, T{0}};
template <class T> inline constexpr BaseVector3<T> BaseVector3<T>::DOWN{T{0}, T{-1}, T{0}};
template <class T> inline constexpr BaseVector3<T> BaseVector3<T>::FORWARD{T{0}, T{0}, T{1}};
template <class T> inline constexpr BaseVector3<T> BaseVector3<T>::BACK{T{0}, T{0}, T{-1}};
template <class T> inline constexpr BaseVector3<T> BaseVector3<T>::ONE{T{1}, T{1}, T{1}};

template <class T> inline constexpr BaseIntegerVector3<T> BaseIntegerVector3<T>::ZERO{T{0}, T{0}, T{0}};
template <class T> inline constexpr BaseIntegerVector3<T> BaseIntegerVector3<T>::LEFT{T{-1}, T{0}, T{0}};
template <class T> inline constexpr BaseIntegerVector3<T> BaseIntegerVector3<T>::RIGHT{T{1}, T{0}, T{0}};
template <class T> inline constexpr BaseIntegerVector3<T> BaseIntegerVector3<T>::UP{T{0}, T{1}, T{0}};
template <class T> inline constexpr BaseIntegerVector3<T> BaseIntegerVector3<T>::DOWN{T{0}, T{-1}, T{0}};
template <class T> inline constexpr BaseIntegerVector3<T> BaseIntegerVector3<T>::FORWARD{T{0}, T{0}, T{1}};
template <class T> inline constexpr BaseIntegerVector3<T> BaseIntegerVector3<T>::BACK{T{0}, T{0}, T{-1}};
template <class T> inline constexpr BaseIntegerVector3<T> BaseIntegerVector3<T>::ONE{T{1}, T{1}, T{1}};

} // namespace Urho3D
