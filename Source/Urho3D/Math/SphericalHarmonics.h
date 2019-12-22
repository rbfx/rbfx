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

        SphericalHarmonics9 sh{ dir };
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

}
