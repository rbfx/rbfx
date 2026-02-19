// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/VectorCommon.h"

#include <EASTL/string.h>
#include <EASTL/tuple.h>

#include <type_traits>

namespace Urho3D
{

/// Two-dimensional vector with integer values.
template <class T> class BaseIntegerVector2 : public Detail::VectorTraits<T, 2>
{
public:
    /// Construct a zero vector.
    constexpr BaseIntegerVector2() noexcept = default;

    /// Construct from coordinates.
    constexpr BaseIntegerVector2(T x, T y) noexcept
        : x_(x)
        , y_(y)
    {
    }

    /// Construct from an array.
    constexpr explicit BaseIntegerVector2(const T data[]) noexcept
        : x_(data[0])
        , y_(data[1])
    {
    }

    /// Convert to tuple.
    constexpr auto Tie() const { return ea::tie(x_, y_); }

    /// Test for equality with another vector.
    constexpr bool operator==(const BaseIntegerVector2& rhs) const { return Tie() == rhs.Tie(); }

    /// Test for inequality with another vector.
    constexpr bool operator!=(const BaseIntegerVector2& rhs) const { return Tie() != rhs.Tie(); }

    /// Lexicographic comparison for sorting.
    constexpr bool operator<(const BaseIntegerVector2& rhs) const { return Tie() < rhs.Tie(); }

    /// Add a vector.
    constexpr BaseIntegerVector2 operator+(const BaseIntegerVector2& rhs) const { return {x_ + rhs.x_, y_ + rhs.y_}; }

    /// Return negation.
    constexpr BaseIntegerVector2 operator-() const { return {-x_, -y_}; }

    /// Subtract a vector.
    constexpr BaseIntegerVector2 operator-(const BaseIntegerVector2& rhs) const { return {x_ - rhs.x_, y_ - rhs.y_}; }

    /// Multiply with a scalar.
    constexpr BaseIntegerVector2 operator*(T rhs) const { return {x_ * rhs, y_ * rhs}; }

    /// Multiply with a vector.
    constexpr BaseIntegerVector2 operator*(const BaseIntegerVector2& rhs) const { return {x_ * rhs.x_, y_ * rhs.y_}; }

    /// Divide by a scalar.
    constexpr BaseIntegerVector2 operator/(T rhs) const { return {x_ / rhs, y_ / rhs}; }

    /// Divide by a vector.
    constexpr BaseIntegerVector2 operator/(const BaseIntegerVector2& rhs) const { return {x_ / rhs.x_, y_ / rhs.y_}; }

    /// Add-assign a vector.
    constexpr BaseIntegerVector2& operator+=(const BaseIntegerVector2& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
    }

    /// Subtract-assign a vector.
    constexpr BaseIntegerVector2& operator-=(const BaseIntegerVector2& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        return *this;
    }

    /// Multiply-assign a scalar.
    constexpr BaseIntegerVector2& operator*=(T rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    constexpr BaseIntegerVector2& operator*=(const BaseIntegerVector2& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        return *this;
    }

    /// Divide-assign a scalar.
    constexpr BaseIntegerVector2& operator/=(T rhs)
    {
        x_ /= rhs;
        y_ /= rhs;
        return *this;
    }

    /// Divide-assign a vector.
    constexpr BaseIntegerVector2& operator/=(const BaseIntegerVector2& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
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
            return ea::string{ea::string::CtorSprintf{}, "%d %d", x_, y_};
        else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) <= 4)
            return ea::string{ea::string::CtorSprintf{}, "%u %u", x_, y_};
        else
            return ea::string{"? ? ?"};
    }

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return Detail::CalculateVectorHash(*this); }

    /// Cast vector to different type.
    template <class OtherVectorType>
    constexpr OtherVectorType Cast(
        typename OtherVectorType::ScalarType z = {}, typename OtherVectorType::ScalarType w = {}) const
    {
        return Detail::CreateVectorCast<OtherVectorType>(x_, y_, z, w);
    }

    /// Cast vector to different type. Y and Z components are shuffled.
    template <class OtherVectorType>
    constexpr OtherVectorType CastXZ(
        typename OtherVectorType::ScalarType z = {}, typename OtherVectorType::ScalarType w = {}) const
    {
        return Detail::CreateVectorCast<OtherVectorType>(x_, z, y_, w);
    }

    /// Return Vector2 vector.
    Vector2 ToVector2() const;

    /// Return IntVector3 vector.
    IntVector3 ToIntVector3(int z = 0) const;

    /// Return Vector3 vector.
    Vector3 ToVector3(float z = 0.0f) const;

    /// Return Vector4 vector.
    Vector4 ToVector4(float z = 0.0f, float w = 0.0f) const;

    /// Return length.
    T Length() const { return static_cast<T>(Sqrt(static_cast<double>(x_ * x_ + y_ * y_))); }

    /// X coordinate.
    T x_{};
    /// Y coordinate.
    T y_{};

    /// Zero vector.
    static const BaseIntegerVector2<T> ZERO;
    /// (-1,0) vector.
    static const BaseIntegerVector2<T> LEFT;
    /// (1,0) vector.
    static const BaseIntegerVector2<T> RIGHT;
    /// (0,1) vector.
    static const BaseIntegerVector2<T> UP;
    /// (0,-1) vector.
    static const BaseIntegerVector2<T> DOWN;
    /// (1,1) vector.
    static const BaseIntegerVector2<T> ONE;
};

/// Two-dimensional vector with real-number values.
template <class T> class BaseVector2 : public Detail::VectorTraits<T, 2>
{
public:
    /// Construct a zero vector.
    constexpr BaseVector2() noexcept = default;

    /// Construct from coordinates.
    constexpr BaseVector2(T x, T y) noexcept
        : x_(x)
        , y_(y)
    {
    }

    /// Construct from a scalar array.
    constexpr explicit BaseVector2(const T data[]) noexcept
        : x_(data[0])
        , y_(data[1])
    {
    }

    /// Convert to tuple.
    constexpr auto Tie() const { return ea::tie(x_, y_); }

    /// Test for equality with another vector without epsilon.
    constexpr bool operator==(const BaseVector2& rhs) const { return Tie() == rhs.Tie(); }

    /// Test for inequality with another vector without epsilon.
    constexpr bool operator!=(const BaseVector2& rhs) const { return Tie() != rhs.Tie(); }

    /// Add a vector.
    constexpr BaseVector2 operator+(const BaseVector2& rhs) const { return {x_ + rhs.x_, y_ + rhs.y_}; }

    /// Return negation.
    constexpr BaseVector2 operator-() const { return {-x_, -y_}; }

    /// Subtract a vector.
    constexpr BaseVector2 operator-(const BaseVector2& rhs) const { return {x_ - rhs.x_, y_ - rhs.y_}; }

    /// Multiply with a scalar.
    constexpr BaseVector2 operator*(T rhs) const { return {x_ * rhs, y_ * rhs}; }

    /// Multiply with a vector.
    constexpr BaseVector2 operator*(const BaseVector2& rhs) const { return {x_ * rhs.x_, y_ * rhs.y_}; }

    /// Divide by a scalar.
    constexpr BaseVector2 operator/(T rhs) const { return {x_ / rhs, y_ / rhs}; }

    /// Divide by a vector.
    constexpr BaseVector2 operator/(const BaseVector2& rhs) const { return {x_ / rhs.x_, y_ / rhs.y_}; }

    /// Add-assign a vector.
    constexpr BaseVector2& operator+=(const BaseVector2& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
    }

    /// Subtract-assign a vector.
    constexpr BaseVector2& operator-=(const BaseVector2& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        return *this;
    }

    /// Multiply-assign a scalar.
    constexpr BaseVector2& operator*=(T rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    constexpr BaseVector2& operator*=(const BaseVector2& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        return *this;
    }

    /// Divide-assign a scalar.
    constexpr BaseVector2& operator/=(T rhs)
    {
        const T invRhs = T{1} / rhs;
        x_ *= invRhs;
        y_ *= invRhs;
        return *this;
    }

    /// Divide-assign a vector.
    constexpr BaseVector2& operator/=(const BaseVector2& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
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
        }
    }

    /// Return length.
    /// @property
    T Length() const { return Sqrt(x_ * x_ + y_ * y_); }

    /// Return squared length.
    /// @property
    constexpr T LengthSquared() const { return x_ * x_ + y_ * y_; }

    /// Calculate dot product.
    constexpr T DotProduct(const BaseVector2& rhs) const { return x_ * rhs.x_ + y_ * rhs.y_; }

    /// Calculate absolute dot product.
    T AbsDotProduct(const BaseVector2& rhs) const { return Abs(x_ * rhs.x_) + Abs(y_ * rhs.y_); }

    /// Calculate "cross product" aka z component of cross product of (x1, y1, 0) and (x2, y2, 0).
    constexpr T CrossProduct(const BaseVector2& rhs) const { return x_ * rhs.y_ - y_ * rhs.x_; }

    /// Project vector onto axis.
    T ProjectOntoAxis(const BaseVector2& axis) const { return DotProduct(axis.Normalized()); }

    /// Project position vector onto line segment. Returns interpolation factor between line points.
    T ProjectOntoLineScalar(const BaseVector2& from, const BaseVector2& to, bool clamped = false) const
    {
        const BaseVector2 direction = to - from;
        const T lengthSquared = direction.LengthSquared();
        const T factor = (*this - from).DotProduct(direction) / lengthSquared;
        return clamped ? Clamp(factor, T{0}, T{1}) : factor;
    }

    /// Project position vector onto line segment. Returns new position.
    BaseVector2 ProjectOntoLine(const BaseVector2& from, const BaseVector2& to, bool clamped = false) const
    {
        return from.Lerp(to, ProjectOntoLineScalar(from, to, clamped));
    }

    /// Calculate distance to another position vector.
    T DistanceToPoint(const BaseVector2& point) const { return (*this - point).Length(); }

    /// Return scalar cross product of 2D vectors.
    constexpr T ScalarCrossProduct(const BaseVector2& rhs) const { return y_ * rhs.x_ - x_ * rhs.y_; }

    /// Returns the angle between this vector and another vector in degrees.
    T Angle(const BaseVector2& rhs) const { return Urho3D::Acos(DotProduct(rhs) / (Length() * rhs.Length())); }

    /// Returns signed angle between this vector and another vector in degrees. Clockwise direction is positive.
    T SignedAngle(const BaseVector2& rhs) const { return Angle(rhs) * (ScalarCrossProduct(rhs) >= T{0} ? T{1} : T{-1}); }

    /// Return absolute vector.
    BaseVector2 Abs() const { return BaseVector2(Urho3D::Abs(x_), Urho3D::Abs(y_)); }

    /// Linear interpolation with another vector.
    BaseVector2 Lerp(const BaseVector2& rhs, T t) const { return *this * (T{1} - t) + rhs * t; }

    /// Test for equality with another vector with epsilon.
    bool Equals(const BaseVector2& rhs, T eps = static_cast<T>(M_EPSILON)) const
    {
        return Urho3D::Equals(x_, rhs.x_, eps) && Urho3D::Equals(y_, rhs.y_, eps);
    }

    /// Return whether any component is NaN.
    bool IsNaN() const { return Urho3D::IsNaN(x_) || Urho3D::IsNaN(y_); }

    /// Return whether any component is Inf.
    bool IsInf() const { return Urho3D::IsInf(x_) || Urho3D::IsInf(y_); }

    /// Return normalized to unit length.
    BaseVector2 Normalized() const
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
    BaseVector2 NormalizedOrDefault(
        const BaseVector2& defaultValue = BaseVector2::ZERO, T eps = static_cast<T>(M_LARGE_EPSILON)) const
    {
        const T lenSquared = LengthSquared();
        if (lenSquared < eps * eps)
            return defaultValue;
        return *this / Sqrt(lenSquared);
    }

    /// Return normalized vector with length in given range.
    BaseVector2 ReNormalized(T minLength, T maxLength, const BaseVector2& defaultValue = BaseVector2::ZERO,
        T eps = static_cast<T>(M_LARGE_EPSILON)) const
    {
        const T lenSquared = LengthSquared();
        if (lenSquared < eps * eps)
            return defaultValue;
        const T len = Sqrt(lenSquared);
        const T newLen = Clamp(len, minLength, maxLength);
        return *this * (newLen / len);
    }

    /// Return orthogonal vector (clockwise).
    constexpr BaseVector2 GetOrthogonalClockwise() const { return {y_, -x_}; }

    /// Return orthogonal vector (counter-clockwise).
    constexpr BaseVector2 GetOrthogonalCounterClockwise() const { return {-y_, x_}; }

    /// Return data.
    constexpr const T* Data() const { return &x_; }

    /// Return mutable data.
    constexpr T* MutableData() { return &x_; }

    /// Return as string.
    ea::string ToString() const
    {
        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
            return ea::string{ea::string::CtorSprintf{}, "%g %g", x_, y_};
        else
            return ea::string{"? ? ?"};
    }

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return Detail::CalculateVectorHash(*this); }

    /// Cast vector to different type.
    template <class OtherVectorType>
    constexpr OtherVectorType Cast(
        typename OtherVectorType::ScalarType z = {}, typename OtherVectorType::ScalarType w = {}) const
    {
        return Detail::CreateVectorCast<OtherVectorType>(x_, y_, z, w);
    }

    /// Cast vector to different type. Y and Z components are shuffled.
    template <class OtherVectorType>
    constexpr OtherVectorType CastXZ(
        typename OtherVectorType::ScalarType z = {}, typename OtherVectorType::ScalarType w = {}) const
    {
        return Detail::CreateVectorCast<OtherVectorType>(x_, z, y_, w);
    }

    /// Return IntVector2 vector.
    IntVector2 ToIntVector2() const { return Cast<IntVector2>(); }

    /// Return IntVector3 vector.
    IntVector3 ToIntVector3(int z = 0) const;

    /// Return Vector3 vector.
    Vector3 ToVector3(float z = 0.0f) const;

    /// Return Vector4 vector.
    Vector4 ToVector4(float z = 0.0f, float w = 0.0f) const;

    /// X coordinate.
    T x_{};
    /// Y coordinate.
    T y_{};

    /// Zero vector.
    static const BaseVector2<T> ZERO;
    /// (-1,0) vector.
    static const BaseVector2<T> LEFT;
    /// (1,0) vector.
    static const BaseVector2<T> RIGHT;
    /// (0,1) vector.
    static const BaseVector2<T> UP;
    /// (0,-1) vector.
    static const BaseVector2<T> DOWN;
    /// (1,1) vector.
    static const BaseVector2<T> ONE;
};

/// Multiply BaseVector2 with a scalar.
template <class T> inline BaseVector2<T> operator *(T lhs, const BaseVector2<T>& rhs) { return rhs * lhs; }

/// Multiply BaseIntegerVector2 with a scalar.
template <class T> inline BaseIntegerVector2<T> operator *(T lhs, const BaseIntegerVector2<T>& rhs) { return rhs * lhs; }

/// Per-component linear interpolation between two 2-vectors.
template <class T> inline BaseVector2<T> VectorLerp(const BaseVector2<T>& lhs, const BaseVector2<T>& rhs, const BaseVector2<T>& t) { return lhs + (rhs - lhs) * t; }

/// Per-component min of two 2-vectors.
template <class T> inline BaseVector2<T> VectorMin(const BaseVector2<T>& lhs, const BaseVector2<T>& rhs) { return BaseVector2<T>(Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_)); }

/// Per-component max of two 2-vectors.
template <class T> inline BaseVector2<T> VectorMax(const BaseVector2<T>& lhs, const BaseVector2<T>& rhs) { return BaseVector2<T>(Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_)); }

/// Per-component floor of 2-vector.
template <class T> inline BaseVector2<T> VectorFloor(const BaseVector2<T>& vec) { return BaseVector2<T>(Floor(vec.x_), Floor(vec.y_)); }

/// Per-component round of 2-vector.
template <class T> inline BaseVector2<T> VectorRound(const BaseVector2<T>& vec) { return BaseVector2<T>(Round(vec.x_), Round(vec.y_)); }

/// Per-component ceil of 2-vector.
template <class T> inline BaseVector2<T> VectorCeil(const BaseVector2<T>& vec) { return BaseVector2<T>(Ceil(vec.x_), Ceil(vec.y_)); }

/// Per-component absolute value of 2-vector.
template <class T> inline BaseVector2<T> VectorAbs(const BaseVector2<T>& vec) { return BaseVector2<T>(Abs(vec.x_), Abs(vec.y_)); }

/// Per-component square root of 2-vector.
template <class T> inline BaseVector2<T> VectorSqrt(const BaseVector2<T>& vec) { return BaseVector2<T>(Sqrt(vec.x_), Sqrt(vec.y_)); }

/// Per-component floor of 2-vector. Returns BaseIntegerVector2.
template <class T> inline IntVector2 VectorFloorToInt(const BaseVector2<T>& vec) { return IntVector2(FloorToInt(vec.x_), FloorToInt(vec.y_)); }

/// Per-component round of 2-vector. Returns BaseIntegerVector2.
template <class T> inline IntVector2 VectorRoundToInt(const BaseVector2<T>& vec) { return IntVector2(RoundToInt(vec.x_), RoundToInt(vec.y_)); }

/// Per-component ceil of 2-vector. Returns BaseIntegerVector2.
template <class T> inline IntVector2 VectorCeilToInt(const BaseVector2<T>& vec) { return IntVector2(CeilToInt(vec.x_), CeilToInt(vec.y_)); }

/// Per-component min of two integer 2-vectors.
template <class T> inline BaseIntegerVector2<T> VectorMin(const BaseIntegerVector2<T>& lhs, const BaseIntegerVector2<T>& rhs) { return BaseIntegerVector2<T>(Min(lhs.x_, rhs.x_), Min(lhs.y_, rhs.y_)); }

/// Per-component max of two integer 2-vectors.
template <class T> inline BaseIntegerVector2<T> VectorMax(const BaseIntegerVector2<T>& lhs, const BaseIntegerVector2<T>& rhs) { return BaseIntegerVector2<T>(Max(lhs.x_, rhs.x_), Max(lhs.y_, rhs.y_)); }

/// Per-component absolute value of integer 2-vector.
template <class T> inline BaseIntegerVector2<T> VectorAbs(const BaseIntegerVector2<T>& vec) { return BaseIntegerVector2<T>(Abs(vec.x_), Abs(vec.y_)); }

/// Return a random value from [0, 1) from 2-vector seed.
/// http://stackoverflow.com/questions/12964279/whats-the-origin-of-this-glsl-rand-one-liner
template <class T> T StableRandom(const BaseVector2<T>& seed) { return Fract(Sin(seed.DotProduct(BaseVector2<T>(12.9898f, 78.233f)) * M_RADTODEG) * 43758.5453f); }

/// Return a random value from [0, 1) from scalar seed.
template <class T> T StableRandom(T seed) { return StableRandom(BaseVector2<T>(seed, seed)); }

/// Return Vector2 vector.
template <class T> Vector2 BaseIntegerVector2<T>::ToVector2() const { return Cast<Vector2>(); }

template <class T> inline constexpr BaseVector2<T> BaseVector2<T>::ZERO{T{0}, T{0}};
template <class T> inline constexpr BaseVector2<T> BaseVector2<T>::LEFT{T{-1}, T{0}};
template <class T> inline constexpr BaseVector2<T> BaseVector2<T>::RIGHT{T{1}, T{0}};
template <class T> inline constexpr BaseVector2<T> BaseVector2<T>::UP{T{0}, T{1}};
template <class T> inline constexpr BaseVector2<T> BaseVector2<T>::DOWN{T{0}, T{-1}};
template <class T> inline constexpr BaseVector2<T> BaseVector2<T>::ONE{T{1}, T{1}};

template <class T> inline constexpr BaseIntegerVector2<T> BaseIntegerVector2<T>::ZERO{T{0}, T{0}};
template <class T> inline constexpr BaseIntegerVector2<T> BaseIntegerVector2<T>::LEFT{T{-1}, T{0}};
template <class T> inline constexpr BaseIntegerVector2<T> BaseIntegerVector2<T>::RIGHT{T{1}, T{0}};
template <class T> inline constexpr BaseIntegerVector2<T> BaseIntegerVector2<T>::UP{T{0}, T{1}};
template <class T> inline constexpr BaseIntegerVector2<T> BaseIntegerVector2<T>::DOWN{T{0}, T{-1}};
template <class T> inline constexpr BaseIntegerVector2<T> BaseIntegerVector2<T>::ONE{T{1}, T{1}};

} // namespace Urho3D
