//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include "../Math/Color.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"

namespace Urho3D
{

/// Spherical harmonics scalar coefficients, 3 bands.
struct SphericalHarmonics9
{
    /// Construct default.
    SphericalHarmonics9() = default;

    /// Construct SH9 coefficients from given *normalized* direction.
    explicit SphericalHarmonics9(const Vector3& dir)
    {
        values_[0] = 0.282095f;

        values_[1] = 0.488603f * dir.y_;
        values_[2] = 0.488603f * dir.z_;
        values_[3] = 0.488603f * dir.x_;

        values_[4] = 1.092548f * dir.x_ * dir.y_;
        values_[5] = 1.092548f * dir.y_ * dir.z_;
        values_[6] = 0.315392f * (3.0f * dir.z_ * dir.z_ - 1.0f);
        values_[7] = 1.092548f * dir.x_ * dir.z_;
        values_[8] = 0.546274f * (dir.x_ * dir.x_ - dir.y_ * dir.y_);
    }

    /// Coefficients.
    float values_[9]{};

    /// Zero harmonics.
    static const SphericalHarmonics9 ZERO;
};

/// Spherical harmonics color coefficients, 3 bands. Use Vector3 instead of Color to save memory.
struct SphericalHarmonicsColor9
{
    /// Construct default.
    SphericalHarmonicsColor9() = default;

    /// Construct SH9 coefficients from given *normalized* direction and color.
    SphericalHarmonicsColor9(const Vector3& dir, const Vector3& color)
    {
        const SphericalHarmonics9 sh9{ dir };
        for (unsigned i = 0; i < 9; ++i)
            values_[i] = color * sh9.values_[i];
    }

    /// Construct SH9 coefficients from given *normalized* direction and color.
    SphericalHarmonicsColor9(const Vector3& dir, const Color& color) : SphericalHarmonicsColor9(dir, color.ToVector3()) {}

    /// Evaluate at given direction.
    Vector3 Evaluate(const Vector3& dir) const
    {
        static const float c0 = 1.0f;
        static const float c1 = 2.0f / 3.0f;
        static const float c2 = 0.25f;

        SphericalHarmonics9 sh{ dir.Normalized() };
        sh.values_[0] *= c0;
        sh.values_[1] *= c1;
        sh.values_[2] *= c1;
        sh.values_[3] *= c1;
        sh.values_[4] *= c2;
        sh.values_[5] *= c2;
        sh.values_[6] *= c2;
        sh.values_[7] *= c2;
        sh.values_[8] *= c2;

        Vector3 result;
        for (unsigned i = 0; i < 9; ++i)
            result += sh.values_[i] * values_[i];

        return result;
    }

    /// Accumulate spherical harmonics (inplace).
    SphericalHarmonicsColor9& operator +=(const SphericalHarmonicsColor9& rhs)
    {
        for (unsigned i = 0; i < 9; ++i)
            values_[i] += rhs.values_[i];
        return *this;
    }

    /// Scale spherical harmonics (inplace).
    SphericalHarmonicsColor9& operator *=(float rhs)
    {
        for (unsigned i = 0; i < 9; ++i)
            values_[i] *= rhs;
        return *this;
    }

    /// Scale spherical harmonics.
    SphericalHarmonicsColor9 operator *(float rhs) const
    {
        SphericalHarmonicsColor9 result = *this;
        result *= rhs;
        return result;
    }

    /// Coefficients.
    Vector3 values_[9]{};

    /// Zero harmonics.
    static const SphericalHarmonicsColor9 ZERO;
};

/// Spherical harmonics, optimized for dot products.
struct SphericalHarmonicsDot9
{
    /// Construct default.
    SphericalHarmonicsDot9() = default;

    /// Construct from spherical harmonics.
    explicit SphericalHarmonicsDot9(SphericalHarmonicsColor9 sh)
    {
        // Apply weights
        static const float c0 = 1.0f;
        static const float c1 = 2.0f / 3.0f;
        static const float c2 = 0.25f;

        sh.values_[0] *= c0 * 0.282095f;
        sh.values_[1] *= c1 * 0.488603f; // y
        sh.values_[2] *= c1 * 0.488603f; // z
        sh.values_[3] *= c1 * 0.488603f; // x
        sh.values_[4] *= c2 * 1.092548f; // x * y
        sh.values_[5] *= c2 * 1.092548f; // y * z
        sh.values_[6] *= c2 * 0.315392f; // 3 * z * z - 1
        sh.values_[7] *= c2 * 1.092548f; // x * z
        sh.values_[8] *= c2 * 0.546274f; // x * x - y * y

        sh.values_[0] = sh.values_[0] - sh.values_[6];
        sh.values_[6] *= 3.0f;

        // (Nx, Ny, Nz, 1)
        Ar_ = Vector4(sh.values_[3].x_, sh.values_[1].x_, sh.values_[2].x_, sh.values_[0].x_);
        Ag_ = Vector4(sh.values_[3].y_, sh.values_[1].y_, sh.values_[2].y_, sh.values_[0].y_);
        Ab_ = Vector4(sh.values_[3].z_, sh.values_[1].z_, sh.values_[2].z_, sh.values_[0].z_);

        // (Nx*Ny, Ny*Nz, Nz*Nz, Nz*Nx)
        Br_ = Vector4(sh.values_[4].x_, sh.values_[5].x_, sh.values_[6].x_, sh.values_[7].x_);
        Bg_ = Vector4(sh.values_[4].y_, sh.values_[5].y_, sh.values_[6].y_, sh.values_[7].y_);
        Bb_ = Vector4(sh.values_[4].z_, sh.values_[5].z_, sh.values_[6].z_, sh.values_[7].z_);

        // Nx*Nx - Ny*Ny
        C_ = sh.values_[8];
    }

    /// Evaluate.
    Vector3 Evaluate(const Vector3& dir) const
    {
        const Vector4 l01{ dir, 1.0f };
        const Vector4 l2{ dir.x_ * dir.y_, dir.y_ * dir.z_, dir.z_ * dir.z_, dir.z_ * dir.x_ };
        const float l2xxyy = dir.x_ * dir.x_ - dir.y_ * dir.y_;

        Vector3 result;
        result.x_ = Ar_.DotProduct(l01);
        result.y_ = Ag_.DotProduct(l01);
        result.z_ = Ab_.DotProduct(l01);
        result.x_ += Br_.DotProduct(l2);
        result.y_ += Bg_.DotProduct(l2);
        result.z_ += Bb_.DotProduct(l2);
        result += C_ * l2xxyy;
        return result;
    }

    /// Dot product with (Nx, Ny, Nz, 1), red channel.
    Vector4 Ar_;
    /// Dot product with (Nx, Ny, Nz, 1), green channel.
    Vector4 Ag_;
    /// Dot product with (Nx, Ny, Nz, 1), blue channel.
    Vector4 Ab_;
    /// Dot product with (Nx*Ny, Ny*Nz, Nz*Nz, Nz*Nx), red channel.
    Vector4 Br_;
    /// Dot product with (Nx*Ny, Ny*Nz, Nz*Nz, Nz*Nx), green channel.
    Vector4 Bg_;
    /// Dot product with (Nx*Ny, Ny*Nz, Nz*Nz, Nz*Nx), blue channel.
    Vector4 Bb_;
    /// Color, multiplied by (Nx*Nx - Ny*Ny).
    Vector3 C_;
    /// Padding. Always 1.0f;
    float padding_{ 1.0f };
};

}
