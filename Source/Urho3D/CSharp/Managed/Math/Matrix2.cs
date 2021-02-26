//
// Copyright (c) 2008-2019 the Urho3D project.
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

using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    /// 2x2 matrix for rotation and scaling.
    [StructLayout(LayoutKind.Sequential)]
    public struct Matrix2 : IEquatable<Matrix2>
    {
        /// Construct from values.
        public Matrix2(float v00 = 1, float v01 = 0,
            float v10 = 0, float v11 = 1)
        {
            M00 = v00;
            M01 = v01;
            M10 = v10;
            M11 = v11;
        }

        /// Construct from a float array.
        public Matrix2(IReadOnlyList<float> data)
        {
            M00 = data[0];
            M01 = data[1];
            M10 = data[2];
            M11 = data[3];
        }

        /// Test for equality with another matrix without epsilon.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(in Matrix2 lhs, in Matrix2 rhs)
        {
            return !(lhs != rhs);
        }

        /// Test for inequality with another matrix without epsilon.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(in Matrix2 lhs, in Matrix2 rhs)
        {
            return lhs.M00 != rhs.M00 ||
                   lhs.M01 != rhs.M01 ||
                   lhs.M10 != rhs.M10 ||
                   lhs.M11 != rhs.M11;
        }

        /// Multiply a Vector2.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator *(in Matrix2 lhs, in Vector2 rhs)
        {
            return new Vector2(
                lhs.M00 * rhs.X + lhs.M01 * rhs.Y,
                lhs.M10 * rhs.X + lhs.M11 * rhs.Y
            );
        }

        /// Add a matrix.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix2 operator +(in Matrix2 lhs, in Matrix2 rhs)
        {
            return new Matrix2(
                lhs.M00 + rhs.M00,
                lhs.M01 + rhs.M01,
                lhs.M10 + rhs.M10,
                lhs.M11 + rhs.M11
            );
        }

        /// Subtract a matrix.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix2 operator -(in Matrix2 lhs, in Matrix2 rhs)
        {
            return new Matrix2(
                lhs.M00 - rhs.M00,
                lhs.M01 - rhs.M01,
                lhs.M10 - rhs.M10,
                lhs.M11 - rhs.M11
            );
        }

        /// Multiply with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix2 operator *(in Matrix2 lhs, float rhs)
        {
            return new Matrix2(
                lhs.M00 * rhs,
                lhs.M01 * rhs,
                lhs.M10 * rhs,
                lhs.M11 * rhs
            );
        }

        /// Multiply a matrix.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix2 operator *(in Matrix2 lhs, in Matrix2 rhs)
        {
            return new Matrix2(
                lhs.M00 * rhs.M00 + lhs.M01 * rhs.M10,
                lhs.M00 * rhs.M01 + lhs.M01 * rhs.M11,
                lhs.M10 * rhs.M00 + lhs.M11 * rhs.M10,
                lhs.M10 * rhs.M01 + lhs.M11 * rhs.M11
            );
        }

        /// Multiply a 2x2 matrix with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix2 operator *(float lhs, in Matrix2 rhs)
        {
            return rhs * lhs;
        }

        /// Set scaling elements.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetScale(in Vector2 scale)
        {
            M00 = scale.X;
            M11 = scale.Y;
        }

        /// Set uniform scaling elements.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetScale(float scale)
        {
            M00 = scale;
            M11 = scale;
        }

        /// Return the scaling part.
        public Vector2 Scale => new Vector2(
                (float) Math.Sqrt(M00 * M00 + M10 * M10),
                (float) Math.Sqrt(M01 * M01 + M11 * M11)
            );

        /// Return transpose.
        public Matrix2 Transposed => new Matrix2(M00, M10, M01, M11);

        /// Return scaled by a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Matrix2 Scaled(in Vector2 scale)
        {
            return new Matrix2(
                M00 * scale.X,
                M01 * scale.Y,
                M10 * scale.X,
                M11 * scale.Y
            );
        }

        /// Test for equality with another matrix with epsilon.
        public bool Equals(Matrix2 rhs)
        {
            return MathDefs.Equals(M00, rhs.M00) &&
                   MathDefs.Equals(M01, rhs.M01) &&
                   MathDefs.Equals(M10, rhs.M10) &&
                   MathDefs.Equals(M11, rhs.M11);
        }

        /// Return inverse.
        public Matrix2 Inverse
        {
            get
            {
                float det = M00 * M11 - M01 * M10;
                float invDet = 1.0f / det;
                return new Matrix2(M11, -M01, -M10, M00) * invDet;
            }
        }

        /// Return float data.
        public float[] Data => new[] {M00, M01, M10, M11};

        /// Return as string.
        public override string ToString()
        {
            return $"{M00} {M01} {M10} {M11}";
        }

        public float M00;
        public float M01;
        public float M10;
        public float M11;

        /// Bulk transpose matrices.
        public static unsafe void BulkTranspose(float* dest, float* src, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                dest[0] = src[0];
                dest[1] = src[2];
                dest[2] = src[1];
                dest[3] = src[3];

                dest += 4;
                src += 4;
            }
        }

        /// Zero matrix.
        public static readonly Matrix2 ZERO = new Matrix2(0, 0, 0, 0);

        /// Identity matrix.
        public static readonly Matrix2 IDENTITY = new Matrix2(
            1, 0,
            0, 1);

        public override bool Equals(object obj)
        {
            return obj is Matrix2 other && Equals(other);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = M00.GetHashCode();
                hashCode = (hashCode * 397) ^ M01.GetHashCode();
                hashCode = (hashCode * 397) ^ M10.GetHashCode();
                hashCode = (hashCode * 397) ^ M11.GetHashCode();
                return hashCode;
            }
        }
    };
}
