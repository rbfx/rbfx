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

/// 4x4 matrix for arbitrary linear transforms including projection.
[StructLayout(LayoutKind.Sequential)]
public struct Matrix4 : IEquatable<Matrix4>
{
    public override bool Equals(object obj)
    {
        return obj is Matrix4 other && Equals(other);
    }

    /// Copy-construct from a 3x3 matrix and set the extra elements to identity.
    public Matrix4(in Matrix3 matrix)
    {
        M00 = matrix.M00;
        M01 = matrix.M01;
        M02 = matrix.M02;
        M03 = 0.0f;
        M10 = matrix.M10;
        M11 = matrix.M11;
        M12 = matrix.M12;
        M13 = 0.0f;
        M20 = matrix.M20;
        M21 = matrix.M21;
        M22 = matrix.M22;
        M23 = 0.0f;
        M30 = 0.0f;
        M31 = 0.0f;
        M32 = 0.0f;
        M33 = 1.0f;
    }

    /// Construct from values or identity matrix by default.
    public Matrix4(float v00 = 1, float v01 = 0, float v02 = 0, float v03 = 0,
                   float v10 = 0, float v11 = 1, float v12 = 0, float v13 = 0,
                   float v20 = 0, float v21 = 0, float v22 = 1, float v23 = 0,
                   float v30 = 0, float v31 = 0, float v32 = 0, float v33 = 1)
    {
        M00 = v00;
        M01 = v01;
        M02 = v02;
        M03 = v03;
        M10 = v10;
        M11 = v11;
        M12 = v12;
        M13 = v13;
        M20 = v20;
        M21 = v21;
        M22 = v22;
        M23 = v23;
        M30 = v30;
        M31 = v31;
        M32 = v32;
        M33 = v33;
    }

    /// Construct from a float array.
    public Matrix4(IReadOnlyList<float> data)
    {
        M00 = data[0];
        M01 = data[1];
        M02 = data[2];
        M03 = data[3];
        M10 = data[4];
        M11 = data[5];
        M12 = data[6];
        M13 = data[7];
        M20 = data[8];
        M21 = data[9];
        M22 = data[10];
        M23 = data[11];
        M30 = data[12];
        M31 = data[13];
        M32 = data[14];
        M33 = data[15];
    }

    /// Test for equality with another matrix without epsilon.
    public static bool operator ==(in Matrix4 lhs, in Matrix4 rhs)
    {
        return lhs.M00 == rhs.M00 &&
               lhs.M01 == rhs.M01 &&
               lhs.M02 == rhs.M02 &&
               lhs.M03 == rhs.M03 &&
               lhs.M10 == rhs.M10 &&
               lhs.M11 == rhs.M11 &&
               lhs.M12 == rhs.M12 &&
               lhs.M13 == rhs.M13 &&
               lhs.M20 == rhs.M20 &&
               lhs.M21 == rhs.M21 &&
               lhs.M22 == rhs.M22 &&
               lhs.M23 == rhs.M23 &&
               lhs.M30 == rhs.M30 &&
               lhs.M31 == rhs.M31 &&
               lhs.M32 == rhs.M32 &&
               lhs.M33 == rhs.M33;
    }

    /// Test for inequality with another matrix without epsilon.
    public static bool operator !=(in Matrix4 lhs, in Matrix4 rhs) { return !(lhs == rhs); }

    /// Multiply a Vector3 which is assumed to represent position.
    public static Vector3 operator *(in Matrix4 lhs, in Vector3 rhs)
    {
        float invW = 1.0f / (lhs.M30 * rhs.X + lhs.M31 * rhs.Y + lhs.M32 * rhs.Z + lhs.M33);

        return new Vector3(
            (lhs.M00 * rhs.X + lhs.M01 * rhs.Y + lhs.M02 * rhs.Z + lhs.M03) * invW,
            (lhs.M10 * rhs.X + lhs.M11 * rhs.Y + lhs.M12 * rhs.Z + lhs.M13) * invW,
            (lhs.M20 * rhs.X + lhs.M21 * rhs.Y + lhs.M22 * rhs.Z + lhs.M23) * invW
        );
    }

    /// Multiply a Vector4.
    public static Vector4 operator *(in Matrix4 lhs, in Vector4 rhs)
    {
        return new Vector4(
            lhs.M00 * rhs.X + lhs.M01 * rhs.Y + lhs.M02 * rhs.Z + lhs.M03 * rhs.W,
            lhs.M10 * rhs.X + lhs.M11 * rhs.Y + lhs.M12 * rhs.Z + lhs.M13 * rhs.W,
            lhs.M20 * rhs.X + lhs.M21 * rhs.Y + lhs.M22 * rhs.Z + lhs.M23 * rhs.W,
            lhs.M30 * rhs.X + lhs.M31 * rhs.Y + lhs.M32 * rhs.Z + lhs.M33 * rhs.W
        );
    }

    /// Add a matrix.
    public static Matrix4 operator +(in Matrix4 lhs, in Matrix4 rhs)
    {
        return new Matrix4(
            lhs.M00 + rhs.M00,
            lhs.M01 + rhs.M01,
            lhs.M02 + rhs.M02,
            lhs.M03 + rhs.M03,
            lhs.M10 + rhs.M10,
            lhs.M11 + rhs.M11,
            lhs.M12 + rhs.M12,
            lhs.M13 + rhs.M13,
            lhs.M20 + rhs.M20,
            lhs.M21 + rhs.M21,
            lhs.M22 + rhs.M22,
            lhs.M23 + rhs.M23,
            lhs.M30 + rhs.M30,
            lhs.M31 + rhs.M31,
            lhs.M32 + rhs.M32,
            lhs.M33 + rhs.M33
        );
    }

    /// Subtract a matrix.
    public static Matrix4 operator -(in Matrix4 lhs, in Matrix4 rhs)
    {
        return new Matrix4(
            lhs.M00 - rhs.M00,
            lhs.M01 - rhs.M01,
            lhs.M02 - rhs.M02,
            lhs.M03 - rhs.M03,
            lhs.M10 - rhs.M10,
            lhs.M11 - rhs.M11,
            lhs.M12 - rhs.M12,
            lhs.M13 - rhs.M13,
            lhs.M20 - rhs.M20,
            lhs.M21 - rhs.M21,
            lhs.M22 - rhs.M22,
            lhs.M23 - rhs.M23,
            lhs.M30 - rhs.M30,
            lhs.M31 - rhs.M31,
            lhs.M32 - rhs.M32,
            lhs.M33 - rhs.M33
        );
    }

    /// Multiply with a scalar.
    public static Matrix4 operator *(in Matrix4 lhs, float rhs)
    {
        return new Matrix4(
            lhs.M00 * rhs,
            lhs.M01 * rhs,
            lhs.M02 * rhs,
            lhs.M03 * rhs,
            lhs.M10 * rhs,
            lhs.M11 * rhs,
            lhs.M12 * rhs,
            lhs.M13 * rhs,
            lhs.M20 * rhs,
            lhs.M21 * rhs,
            lhs.M22 * rhs,
            lhs.M23 * rhs,
            lhs.M30 * rhs,
            lhs.M31 * rhs,
            lhs.M32 * rhs,
            lhs.M33 * rhs
        );
    }

    /// Multiply a matrix.
    public static Matrix4 operator *(in Matrix4 lhs, in Matrix4 rhs)
    {
        return new Matrix4(
            lhs.M00 * rhs.M00 + lhs.M01 * rhs.M10 + lhs.M02 * rhs.M20 + lhs.M03 * rhs.M30,
            lhs.M00 * rhs.M01 + lhs.M01 * rhs.M11 + lhs.M02 * rhs.M21 + lhs.M03 * rhs.M31,
            lhs.M00 * rhs.M02 + lhs.M01 * rhs.M12 + lhs.M02 * rhs.M22 + lhs.M03 * rhs.M32,
            lhs.M00 * rhs.M03 + lhs.M01 * rhs.M13 + lhs.M02 * rhs.M23 + lhs.M03 * rhs.M33,
            lhs.M10 * rhs.M00 + lhs.M11 * rhs.M10 + lhs.M12 * rhs.M20 + lhs.M13 * rhs.M30,
            lhs.M10 * rhs.M01 + lhs.M11 * rhs.M11 + lhs.M12 * rhs.M21 + lhs.M13 * rhs.M31,
            lhs.M10 * rhs.M02 + lhs.M11 * rhs.M12 + lhs.M12 * rhs.M22 + lhs.M13 * rhs.M32,
            lhs.M10 * rhs.M03 + lhs.M11 * rhs.M13 + lhs.M12 * rhs.M23 + lhs.M13 * rhs.M33,
            lhs.M20 * rhs.M00 + lhs.M21 * rhs.M10 + lhs.M22 * rhs.M20 + lhs.M23 * rhs.M30,
            lhs.M20 * rhs.M01 + lhs.M21 * rhs.M11 + lhs.M22 * rhs.M21 + lhs.M23 * rhs.M31,
            lhs.M20 * rhs.M02 + lhs.M21 * rhs.M12 + lhs.M22 * rhs.M22 + lhs.M23 * rhs.M32,
            lhs.M20 * rhs.M03 + lhs.M21 * rhs.M13 + lhs.M22 * rhs.M23 + lhs.M23 * rhs.M33,
            lhs.M30 * rhs.M00 + lhs.M31 * rhs.M10 + lhs.M32 * rhs.M20 + lhs.M33 * rhs.M30,
            lhs.M30 * rhs.M01 + lhs.M31 * rhs.M11 + lhs.M32 * rhs.M21 + lhs.M33 * rhs.M31,
            lhs.M30 * rhs.M02 + lhs.M31 * rhs.M12 + lhs.M32 * rhs.M22 + lhs.M33 * rhs.M32,
            lhs.M30 * rhs.M03 + lhs.M31 * rhs.M13 + lhs.M32 * rhs.M23 + lhs.M33 * rhs.M33
        );
    }

    /// Multiply with a 3x4 matrix.
    public static Matrix4 operator *(in Matrix4 lhs, in Matrix3x4 rhs)
    {
        return new Matrix4(
            lhs.M00 * rhs.M00 + lhs.M01 * rhs.M10 + lhs.M02 * rhs.M20,
            lhs.M00 * rhs.M01 + lhs.M01 * rhs.M11 + lhs.M02 * rhs.M21,
            lhs.M00 * rhs.M02 + lhs.M01 * rhs.M12 + lhs.M02 * rhs.M22,
            lhs.M00 * rhs.M03 + lhs.M01 * rhs.M13 + lhs.M02 * rhs.M23 + lhs.M03,
            lhs.M10 * rhs.M00 + lhs.M11 * rhs.M10 + lhs.M12 * rhs.M20,
            lhs.M10 * rhs.M01 + lhs.M11 * rhs.M11 + lhs.M12 * rhs.M21,
            lhs.M10 * rhs.M02 + lhs.M11 * rhs.M12 + lhs.M12 * rhs.M22,
            lhs.M10 * rhs.M03 + lhs.M11 * rhs.M13 + lhs.M12 * rhs.M23 + lhs.M13,
            lhs.M20 * rhs.M00 + lhs.M21 * rhs.M10 + lhs.M22 * rhs.M20,
            lhs.M20 * rhs.M01 + lhs.M21 * rhs.M11 + lhs.M22 * rhs.M21,
            lhs.M20 * rhs.M02 + lhs.M21 * rhs.M12 + lhs.M22 * rhs.M22,
            lhs.M20 * rhs.M03 + lhs.M21 * rhs.M13 + lhs.M22 * rhs.M23 + lhs.M23,
            lhs.M30 * rhs.M00 + lhs.M31 * rhs.M10 + lhs.M32 * rhs.M20,
            lhs.M30 * rhs.M01 + lhs.M31 * rhs.M11 + lhs.M32 * rhs.M21,
            lhs.M30 * rhs.M02 + lhs.M31 * rhs.M12 + lhs.M32 * rhs.M22,
            lhs.M30 * rhs.M03 + lhs.M31 * rhs.M13 + lhs.M32 * rhs.M23 + lhs.M33
        );
    }

    /// Multiply a 4x4 matrix with a scalar.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Matrix4 operator *(float lhs, in Matrix4 rhs) { return rhs * lhs; }

    /// Get or set translation elements.
    public Vector3 Translation
    {
        get => new Vector3(M03, M13, M23);
        set
        {
            M03 = value.X;
            M13 = value.Y;
            M23 = value.Z;
        }
    }

    /// Set rotation elements from a 3x3 matrix.
    /// Return the rotation matrix with scaling removed.
    public Matrix3 RotationMatrix
    {
        get
        {
            var invScale = new Vector3(
                1.0f / (float)Math.Sqrt(M00 * M00 + M10 * M10 + M20 * M20),
                1.0f / (float)Math.Sqrt(M01 * M01 + M11 * M11 + M21 * M21),
                1.0f / (float)Math.Sqrt(M02 * M02 + M12 * M12 + M22 * M22)
            );
            return Matrix3.Scaled(invScale);
        }
        set
        {
            M00 = value.M00;
            M01 = value.M01;
            M02 = value.M02;
            M10 = value.M10;
            M11 = value.M11;
            M12 = value.M12;
            M20 = value.M20;
            M21 = value.M21;
            M22 = value.M22;
        }
    }

    /// Get or set scaling elements.
    public Vector3 Scale
    {
        get
        {
            return new Vector3(
                (float)Math.Sqrt(M00 * M00 + M10 * M10 + M20 * M20),
                (float)Math.Sqrt(M01 * M01 + M11 * M11 + M21 * M21),
                (float)Math.Sqrt(M02 * M02 + M12 * M12 + M22 * M22)
            );
        }
        set
        {
            M00 = value.X;
            M11 = value.Y;
            M22 = value.Z;
        }
    }

    /// Set uniform scaling elements.
    public void SetScale(float scale)
    {
        M00 = scale;
        M11 = scale;
        M22 = scale;
    }

    /// Return the combined rotation and scaling matrix.
    public Matrix3 Matrix3 => new Matrix3(M00,M01,M02,M10,M11,M12,M20,M21,M22);

    /// Return the rotation part.
    public Quaternion Rotation => new Quaternion(RotationMatrix);

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
    public Matrix4 Transposed => new Matrix4(M00,M10,M20,M30,M01,M11,M21,M31,M02,M12,M22,M32,M03,M13,M23,M33);

    /// Test for equality with another matrix with epsilon.
    public bool Equals(Matrix4 rhs)
    {
        return this == rhs;
    }

    public override int GetHashCode()
    {
        unchecked
        {
            var hashCode = M00.GetHashCode();
            hashCode = (hashCode * 397) ^ M01.GetHashCode();
            hashCode = (hashCode * 397) ^ M02.GetHashCode();
            hashCode = (hashCode * 397) ^ M03.GetHashCode();
            hashCode = (hashCode * 397) ^ M10.GetHashCode();
            hashCode = (hashCode * 397) ^ M11.GetHashCode();
            hashCode = (hashCode * 397) ^ M12.GetHashCode();
            hashCode = (hashCode * 397) ^ M13.GetHashCode();
            hashCode = (hashCode * 397) ^ M20.GetHashCode();
            hashCode = (hashCode * 397) ^ M21.GetHashCode();
            hashCode = (hashCode * 397) ^ M22.GetHashCode();
            hashCode = (hashCode * 397) ^ M23.GetHashCode();
            hashCode = (hashCode * 397) ^ M30.GetHashCode();
            hashCode = (hashCode * 397) ^ M31.GetHashCode();
            hashCode = (hashCode * 397) ^ M32.GetHashCode();
            hashCode = (hashCode * 397) ^ M33.GetHashCode();
            return hashCode;
        }
    }

    /// Return decomposition to translation, rotation and scale.
    public void Decompose(out Vector3 translation, out Quaternion rotation, out Vector3 scale)
    {
        translation.X = M03;
        translation.Y = M13;
        translation.Z = M23;

        scale.X = (float)Math.Sqrt(M00 * M00 + M10 * M10 + M20 * M20);
        scale.Y = (float)Math.Sqrt(M01 * M01 + M11 * M11 + M21 * M21);
        scale.Z = (float)Math.Sqrt(M02 * M02 + M12 * M12 + M22 * M22);

        var invScale = new Vector3(1.0f / scale.X, 1.0f / scale.Y, 1.0f / scale.Z);
        rotation = new Quaternion(Matrix3.Scaled(invScale));
    }


    /// Return inverse.
    public Matrix4 Inverted
    {
        get
        {
            float v0 = M20 * M31 - M21 * M30;
            float v1 = M20 * M32 - M22 * M30;
            float v2 = M20 * M33 - M23 * M30;
            float v3 = M21 * M32 - M22 * M31;
            float v4 = M21 * M33 - M23 * M31;
            float v5 = M22 * M33 - M23 * M32;

            float i00 = (v5 * M11 - v4 * M12 + v3 * M13);
            float i10 = -(v5 * M10 - v2 * M12 + v1 * M13);
            float i20 = (v4 * M10 - v2 * M11 + v0 * M13);
            float i30 = -(v3 * M10 - v1 * M11 + v0 * M12);

            float invDet = 1.0f / (i00 * M00 + i10 * M01 + i20 * M02 + i30 * M03);

            i00 *= invDet;
            i10 *= invDet;
            i20 *= invDet;
            i30 *= invDet;

            float i01 = -(v5 * M01 - v4 * M02 + v3 * M03) * invDet;
            float i11 = (v5 * M00 - v2 * M02 + v1 * M03) * invDet;
            float i21 = -(v4 * M00 - v2 * M01 + v0 * M03) * invDet;
            float i31 = (v3 * M00 - v1 * M01 + v0 * M02) * invDet;

            v0 = M10 * M31 - M11 * M30;
            v1 = M10 * M32 - M12 * M30;
            v2 = M10 * M33 - M13 * M30;
            v3 = M11 * M32 - M12 * M31;
            v4 = M11 * M33 - M13 * M31;
            v5 = M12 * M33 - M13 * M32;

            float i02 = (v5 * M01 - v4 * M02 + v3 * M03) * invDet;
            float i12 = -(v5 * M00 - v2 * M02 + v1 * M03) * invDet;
            float i22 = (v4 * M00 - v2 * M01 + v0 * M03) * invDet;
            float i32 = -(v3 * M00 - v1 * M01 + v0 * M02) * invDet;

            v0 = M21 * M10 - M20 * M11;
            v1 = M22 * M10 - M20 * M12;
            v2 = M23 * M10 - M20 * M13;
            v3 = M22 * M11 - M21 * M12;
            v4 = M23 * M11 - M21 * M13;
            v5 = M23 * M12 - M22 * M13;

            float i03 = -(v5 * M01 - v4 * M02 + v3 * M03) * invDet;
            float i13 = (v5 * M00 - v2 * M02 + v1 * M03) * invDet;
            float i23 = -(v4 * M00 - v2 * M01 + v0 * M03) * invDet;
            float i33 = (v3 * M00 - v1 * M01 + v0 * M02) * invDet;

            return new Matrix4(
                i00, i01, i02, i03,
                i10, i11, i12, i13,
                i20, i21, i22, i23,
                i30, i31, i32, i33);
        }
    }

    /// Return float data.
    private float[] Data => new[]
        {M00, M10, M20, M30, M01, M11, M21, M31, M02, M12, M22, M32, M03, M13, M23, M33};

    /// Return matrix element.
    public float this[int i, int j]
    {
        get
        {
            if (i < 0 || i > 3 || j < 0 || j > 3)
                throw new IndexOutOfRangeException();
            unsafe
            {
                fixed (float* p = &M00)
                {
                    return p[i * 4 + j];
                }
            }
        }
        set
        {
            if (i < 0 || i > 3 || j < 0 || j > 3)
                throw new IndexOutOfRangeException();
            unsafe
            {
                fixed (float* p = &M00)
                {
                    p[i * 4 + j] = value;
                }
            }
        }
    }

    /// Return matrix row.
    public Vector4 Row(int i) { return new Vector4(this[i, 0], this[i, 1], this[i, 2], this[i, 3]); }

    /// Return matrix column.
    public Vector4 Column(int j) { return new Vector4(this[0, j], this[1, j], this[2, j], this[3, j]); }

    /// Return as string.
    public override string ToString()
    {
        return
            $"{M00} {M10} {M20} {M30} {M01} {M11} {M21} {M31} {M02} {M12} {M22} {M32} {M03} {M13} {M23} {M33}"; }

    public float M00;
    public float M01;
    public float M02;
    public float M03;
    public float M10;
    public float M11;
    public float M12;
    public float M13;
    public float M20;
    public float M21;
    public float M22;
    public float M23;
    public float M30;
    public float M31;
    public float M32;
    public float M33;

    /// Bulk transpose matrices.
    public static unsafe void BulkTranspose(float* dest, float* src, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            dest[0] = src[0];
            dest[1] = src[4];
            dest[2] = src[8];
            dest[3] = src[12];
            dest[4] = src[1];
            dest[5] = src[5];
            dest[6] = src[9];
            dest[7] = src[13];
            dest[8] = src[2];
            dest[9] = src[6];
            dest[10] = src[10];
            dest[11] = src[14];
            dest[12] = src[3];
            dest[13] = src[7];
            dest[14] = src[11];
            dest[15] = src[15];
            dest += 16;
            src += 16;
        }
    }

    /// Zero matrix.
    public static readonly Matrix4 Zero = new Matrix4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    /// Identity matrix.
    public static readonly Matrix4 Identity = new Matrix4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    };

}
