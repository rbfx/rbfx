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
    /// 3x3 matrix for rotation and scaling.
    [StructLayout(LayoutKind.Sequential)]
    public struct Matrix3 : IEquatable<Matrix3>
    {
        /// Construct from values or identity matrix by default.
        public Matrix3(float v00 = 1, float v01 = 0, float v02 = 0,
            float v10 = 0, float v11 = 1, float v12 = 0,
            float v20 = 0, float v21 = 0, float v22 = 1)
        {
            M00 = v00;
            M01 = v01;
            M02 = v02;
            M10 = v10;
            M11 = v11;
            M12 = v12;
            M20 = v20;
            M21 = v21;
            M22 = v22;
        }

        /// Construct from a float array.
        public Matrix3(IReadOnlyList<float> data)
        {
            M00 = data[0];
            M01 = data[1];
            M02 = data[2];
            M10 = data[3];
            M11 = data[4];
            M12 = data[5];
            M20 = data[6];
            M21 = data[7];
            M22 = data[8];
        }

        /// Test for equality with another matrix without epsilon.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(in Matrix3 lhs, in Matrix3 rhs)
        {
            return lhs.M00 == rhs.M00 &&
                   lhs.M01 == rhs.M01 &&
                   lhs.M02 == rhs.M02 &&
                   lhs.M10 == rhs.M10 &&
                   lhs.M11 == rhs.M11 &&
                   lhs.M12 == rhs.M12 &&
                   lhs.M20 == rhs.M20 &&
                   lhs.M21 == rhs.M21 &&
                   lhs.M22 == rhs.M22;
        }

        /// Test for inequality with another matrix without epsilon.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(in Matrix3 lhs, in Matrix3 rhs)
        {
            return !(lhs == rhs);
        }

        /// Multiply a Vector3.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator *(in Matrix3 lhs, in Vector3 rhs)
        {
            return new Vector3(
                lhs.M00 * rhs.X + lhs.M01 * rhs.Y + lhs.M02 * rhs.Z,
                lhs.M10 * rhs.X + lhs.M11 * rhs.Y + lhs.M12 * rhs.Z,
                lhs.M20 * rhs.X + lhs.M21 * rhs.Y + lhs.M22 * rhs.Z
            );
        }

        /// Add a matrix.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix3 operator +(in Matrix3 lhs, in Matrix3 rhs)
        {
            return new Matrix3(
                lhs.M00 + rhs.M00,
                lhs.M01 + rhs.M01,
                lhs.M02 + rhs.M02,
                lhs.M10 + rhs.M10,
                lhs.M11 + rhs.M11,
                lhs.M12 + rhs.M12,
                lhs.M20 + rhs.M20,
                lhs.M21 + rhs.M21,
                lhs.M22 + rhs.M22
            );
        }

        /// Subtract a matrix.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix3 operator -(in Matrix3 lhs, in Matrix3 rhs)
        {
            return new Matrix3(
                lhs.M00 - rhs.M00,
                lhs.M01 - rhs.M01,
                lhs.M02 - rhs.M02,
                lhs.M10 - rhs.M10,
                lhs.M11 - rhs.M11,
                lhs.M12 - rhs.M12,
                lhs.M20 - rhs.M20,
                lhs.M21 - rhs.M21,
                lhs.M22 - rhs.M22
            );
        }

        /// Multiply with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix3 operator *(in Matrix3 lhs, float rhs)
        {
            return new Matrix3(
                lhs.M00 * rhs,
                lhs.M01 * rhs,
                lhs.M02 * rhs,
                lhs.M10 * rhs,
                lhs.M11 * rhs,
                lhs.M12 * rhs,
                lhs.M20 * rhs,
                lhs.M21 * rhs,
                lhs.M22 * rhs
            );
        }

        /// Multiply a matrix.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix3 operator *(in Matrix3 lhs, in Matrix3 rhs)
        {
            return new Matrix3(
                lhs.M00 * rhs.M00 + lhs.M01 * rhs.M10 + lhs.M02 * rhs.M20,
                lhs.M00 * rhs.M01 + lhs.M01 * rhs.M11 + lhs.M02 * rhs.M21,
                lhs.M00 * rhs.M02 + lhs.M01 * rhs.M12 + lhs.M02 * rhs.M22,
                lhs.M10 * rhs.M00 + lhs.M11 * rhs.M10 + lhs.M12 * rhs.M20,
                lhs.M10 * rhs.M01 + lhs.M11 * rhs.M11 + lhs.M12 * rhs.M21,
                lhs.M10 * rhs.M02 + lhs.M11 * rhs.M12 + lhs.M12 * rhs.M22,
                lhs.M20 * rhs.M00 + lhs.M21 * rhs.M10 + lhs.M22 * rhs.M20,
                lhs.M20 * rhs.M01 + lhs.M21 * rhs.M11 + lhs.M22 * rhs.M21,
                lhs.M20 * rhs.M02 + lhs.M21 * rhs.M12 + lhs.M22 * rhs.M22
            );
        }

        /// Multiply a 3x3 matrix with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix3 operator *(float lhs, in Matrix3 rhs)
        {
            return rhs * lhs;
        }

        /// Set scaling elements.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetScale(in Vector3 scale)
        {
            M00 = scale.X;
            M11 = scale.Y;
            M22 = scale.Z;
        }

        /// Set uniform scaling elements.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetScale(float scale)
        {
            M00 = scale;
            M11 = scale;
            M22 = scale;
        }

        /// Return the scaling part.
        public Vector3 Scale => new Vector3(
            (float) Math.Sqrt(M00 * M00 + M10 * M10 + M20 * M20),
            (float) Math.Sqrt(M01 * M01 + M11 * M11 + M21 * M21),
            (float) Math.Sqrt(M02 * M02 + M12 * M12 + M22 * M22)
        );

        /// Return the scaling part with the sign. Reference rotation matrix is required to avoid ambiguity.
        public Vector3 SignedScale(in Matrix3 rotation)
        {
            return new Vector3(
                rotation.M00 * M00 + rotation.M10 * M10 + rotation.M20 * M20,
                rotation.M01 * M01 + rotation.M11 * M11 + rotation.M21 * M21,
                rotation.M02 * M02 + rotation.M12 * M12 + rotation.M22 * M22
            );
        }

        /// Return transposed.
        public Matrix3 Transposed => new Matrix3(M00, M10, M20, M01, M11, M21, M02, M12, M22);

        /// Return scaled by a vector.
        public Matrix3 Scaled(in Vector3 scale)
        {
            return new Matrix3(
                M00 * scale.X,
                M01 * scale.Y,
                M02 * scale.Z,
                M10 * scale.X,
                M11 * scale.Y,
                M12 * scale.Z,
                M20 * scale.X,
                M21 * scale.Y,
                M22 * scale.Z
            );
        }

        /// Test for equality with another matrix with epsilon.
        public bool Equals(Matrix3 rhs)
        {
            return this == rhs;
        }

        public override bool Equals(object obj)
        {
            return obj is Matrix3 other && Equals(other);
        }

        /// Return inverse.
        public Matrix3 Inverse()
        {
            float det = M00 * M11 * M22 +
                        M10 * M21 * M02 +
                        M20 * M01 * M12 -
                        M20 * M11 * M02 -
                        M10 * M01 * M22 -
                        M00 * M21 * M12;

            float invDet = 1.0f / det;

            return new Matrix3(
                (M11 * M22 - M21 * M12) * invDet,
                -(M01 * M22 - M21 * M02) * invDet,
                (M01 * M12 - M11 * M02) * invDet,
                -(M10 * M22 - M20 * M12) * invDet,
                (M00 * M22 - M20 * M02) * invDet,
                -(M00 * M12 - M10 * M02) * invDet,
                (M10 * M21 - M20 * M11) * invDet,
                -(M00 * M21 - M20 * M01) * invDet,
                (M00 * M11 - M10 * M01) * invDet
            );
        }

        /// Return float data.
        public float[] Data()
            => new[] {M00, M01, M02, M10, M11, M12, M20, M21, M22};

        /// Return matrix element.
        public float this[int i, int j]
        {
            get
            {
                unsafe
                {
                    fixed (float* p = &M00)
                    {
                        return p[i * 3 + j];
                    }
                }
            }
            set
            {
                unsafe
                {
                    fixed (float* p = &M00)
                    {
                        p[i * 3 + j] = value;
                    }
                }
            }
        }

        /// Return matrix row.
        public Vector3 Row(int i)
        {
            return new Vector3(this[i, 0], this[i, 1], this[i, 2]);
        }

        /// Return matrix column.
        public Vector3 Column(int j)
        {
            return new Vector3(this[0, j], this[1, j], this[2, j]);
        }

        /// Return as string.
        public override string ToString()
        {
            return $"{M00} {M01} {M02} {M10} {M11} {M12} {M20} {M21} {M22}";
        }

        public float M00;
        public float M01;
        public float M02;
        public float M10;
        public float M11;
        public float M12;
        public float M20;
        public float M21;
        public float M22;

        /// Bulk transpose matrices.
        public static unsafe void BulkTranspose(float* dest, float* src, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                dest[0] = src[0];
                dest[1] = src[3];
                dest[2] = src[6];
                dest[3] = src[1];
                dest[4] = src[4];
                dest[5] = src[7];
                dest[6] = src[2];
                dest[7] = src[5];
                dest[8] = src[8];

                dest += 9;
                src += 9;
            }
        }

        /// Zero matrix.
        static Matrix3 Zero = new Matrix3(0, 0, 0, 0, 0, 0, 0, 0, 0);

        /// Identity matrix.
        static Matrix3 Identity = new Matrix3(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1);

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = M00.GetHashCode();
                hashCode = (hashCode * 397) ^ M01.GetHashCode();
                hashCode = (hashCode * 397) ^ M02.GetHashCode();
                hashCode = (hashCode * 397) ^ M10.GetHashCode();
                hashCode = (hashCode * 397) ^ M11.GetHashCode();
                hashCode = (hashCode * 397) ^ M12.GetHashCode();
                hashCode = (hashCode * 397) ^ M20.GetHashCode();
                hashCode = (hashCode * 397) ^ M21.GetHashCode();
                hashCode = (hashCode * 397) ^ M22.GetHashCode();
                return hashCode;
            }
        }
    };
}
