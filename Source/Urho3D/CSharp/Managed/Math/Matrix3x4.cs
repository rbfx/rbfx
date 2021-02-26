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

namespace Urho3DNet
{

/// 3x4 matrix for scene node transform calculations.
public struct Matrix3x4
{

    /// Copy-construct from a 3x3 matrix and set the extra elements to identity.
    public Matrix3x4(in Matrix3 matrix)
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
    }

    /// Copy-construct from a 4x4 matrix which is assumed to contain no projection.
    public Matrix3x4(in Matrix4 matrix)
    {
        M00 = matrix.M00;
        M01 = matrix.M01;
        M02 = matrix.M02;
        M03 = matrix.M03;
        M10 = matrix.M10;
        M11 = matrix.M11;
        M12 = matrix.M12;
        M13 = matrix.M13;
        M20 = matrix.M20;
        M21 = matrix.M21;
        M22 = matrix.M22;
        M23 = matrix.M23;
    }

    /// Construct from values or identity matrix by default.
    public Matrix3x4(
        float v00 = 1, float v01 = 0, float v02 = 0, float v03 = 0,
        float v10 = 0, float v11 = 1, float v12 = 0, float v13 = 0,
        float v20 = 0, float v21 = 0, float v22 = 1, float v23 = 0)
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
    }

    /// Construct from a float array.
    public Matrix3x4(IReadOnlyList<float> data)
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
    }

    /// Construct from translation, rotation and uniform scale.
    public Matrix3x4(in Vector3 translation, in Quaternion rotation, float scale)
    {
        M00 = M01 = M02 = M03 = M10 = M11 = M12 = M13 = M20 = M21 = M22 = M23 = 0;
        SetRotation(rotation.RotationMatrix * scale);
        SetTranslation(translation);
    }

    /// Construct from translation, rotation and nonuniform scale.
    public Matrix3x4(in Vector3 translation, in Quaternion rotation, in Vector3 scale)
    {
        M00 = M01 = M02 = M03 = M10 = M11 = M12 = M13 = M20 = M21 = M22 = M23 = 0;
        SetRotation(rotation.RotationMatrix.Scaled(scale));
        SetTranslation(translation);
    }

    /// Test for equality with another matrix without epsilon.
    public static bool operator ==(in Matrix3x4 lhs, in Matrix3x4 rhs)
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
               lhs.M23 == rhs.M23;
    }

    /// Test for inequality with another matrix without epsilon.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool operator !=(in Matrix3x4 lhs, in Matrix3x4 rhs) { return !(lhs == rhs); }

    /// Multiply a Vector3 which is assumed to represent position.
    public static Vector3 operator *(in Matrix3x4 lhs, in Vector3 rhs)
    {
        return new Vector3(
            (lhs.M00 * rhs.X + lhs.M01 * rhs.Y + lhs.M02 * rhs.Z + lhs.M03),
            (lhs.M10 * rhs.X + lhs.M11 * rhs.Y + lhs.M12 * rhs.Z + lhs.M13),
            (lhs.M20 * rhs.X + lhs.M21 * rhs.Y + lhs.M22 * rhs.Z + lhs.M23)
        );
    }

    /// Multiply a Vector4.
    public static Vector3 operator *(in Matrix3x4 lhs, in Vector4 rhs)
    {
        return new Vector3(
            (lhs.M00 * rhs.X + lhs.M01 * rhs.Y + lhs.M02 * rhs.Z + lhs.M03 * rhs.W),
            (lhs.M10 * rhs.X + lhs.M11 * rhs.Y + lhs.M12 * rhs.Z + lhs.M13 * rhs.W),
            (lhs.M20 * rhs.X + lhs.M21 * rhs.Y + lhs.M22 * rhs.Z + lhs.M23 * rhs.W)
        );
    }

    /// Add a matrix.
    public static Matrix3x4 operator +(in Matrix3x4 lhs, in Matrix3x4 rhs)
    {
        return new Matrix3x4(
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
            lhs.M23 + rhs.M23
        );
    }

    /// Subtract a matrix.
    public static Matrix3x4 operator -(in Matrix3x4 lhs, in Matrix3x4 rhs)
    {
        return new Matrix3x4(
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
            lhs.M23 - rhs.M23
        );
    }

    /// Multiply with a scalar.
    public static Matrix3x4 operator *(in Matrix3x4 lhs, float rhs)
    {
        return new Matrix3x4(
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
            lhs.M23 * rhs
        );
    }

    /// Multiply a matrix.
    public static Matrix3x4 operator *(in Matrix3x4 lhs, in Matrix3x4 rhs)
    {
        return new Matrix3x4(
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
            lhs.M20 * rhs.M03 + lhs.M21 * rhs.M13 + lhs.M22 * rhs.M23 + lhs.M23
        );
    }

    /// Multiply a 4x4 matrix.
    public static Matrix4 operator *(in Matrix3x4 lhs, in Matrix4 rhs)
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
            rhs.M30,
            rhs.M31,
            rhs.M32,
            rhs.M33
        );
    }

    /// Multiply a 3x4 matrix with a scalar.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Matrix3x4 operator *(float lhs, in Matrix3x4 rhs) { return rhs * lhs; }

    /// Set translation elements.
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
public void SetTranslation(in Vector3 translation)
    {
        M03 = translation.X;
        M13 = translation.Y;
        M23 = translation.Z;
    }

    /// Set rotation elements from a 3x3 matrix.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void SetRotation(in Matrix3 rotation)
    {
        M00 = rotation.M00;
        M01 = rotation.M01;
        M02 = rotation.M02;
        M10 = rotation.M10;
        M11 = rotation.M11;
        M12 = rotation.M12;
        M20 = rotation.M20;
        M21 = rotation.M21;
        M22 = rotation.M22;
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

    /// Return the combined rotation and scaling matrix.
    public Matrix3 Matrix3 => new Matrix3(M00, M01,M02,M10,M11,M12,M20,M21,M22);

    /// Convert to a 4x4 matrix by filling in an identity last row.
    public Matrix4 Matrix4 => new Matrix4(M00,M01,M02,M03,M10,M11,M12,M13,M20,M21,M22,M23,0.0f,0.0f,0.0f,1.0f);

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
    }

    /// Return the translation part.
    public Vector3 Translation => new Vector3(M03,M13,M23);

    /// Return the rotation part.
    public Quaternion Rotation => new Quaternion(RotationMatrix);

    /// Return the scaling part.
    public Vector3 Scale=>new Vector3(
            (float)Math.Sqrt(M00 * M00 + M10 * M10 + M20 * M20),
            (float)Math.Sqrt(M01 * M01 + M11 * M11 + M21 * M21),
            (float)Math.Sqrt(M02 * M02 + M12 * M12 + M22 * M22)
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

    /// Test for equality with another matrix with epsilon.
    public bool Equals(Matrix3x4 rhs)
    {
        return M00 == rhs.M00 &&
               M01 == rhs.M01 &&
               M02 == rhs.M02 &&
               M03 == rhs.M03 &&
               M10 == rhs.M10 &&
               M11 == rhs.M11 &&
               M12 == rhs.M12 &&
               M13 == rhs.M13 &&
               M20 == rhs.M20 &&
               M21 == rhs.M21 &&
               M22 == rhs.M22 &&
               M23 == rhs.M23;
    }

    /// Return decomposition to translation, rotation and scale.
   public  void Decompose(out Vector3 translation, out Quaternion rotation, out Vector3 scale)
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
    public Matrix3x4 Inverse()
    {
        float det = M00 * M11 * M22 +
                    M10 * M21 * M02 +
                    M20 * M01 * M12 -
                    M20 * M11 * M02 -
                    M10 * M01 * M22 -
                    M00 * M21 * M12;

        float invDet = 1.0f / det;
        Matrix3x4 ret;

        ret.M00 = (M11 * M22 - M21 * M12) * invDet;
        ret.M01 = -(M01 * M22 - M21 * M02) * invDet;
        ret.M02 = (M01 * M12 - M11 * M02) * invDet;
        ret.M03 = -(M03 * ret.M00 + M13 * ret.M01 + M23 * ret.M02);
        ret.M10 = -(M10 * M22 - M20 * M12) * invDet;
        ret.M11 = (M00 * M22 - M20 * M02) * invDet;
        ret.M12 = -(M00 * M12 - M10 * M02) * invDet;
        ret.M13 = -(M03 * ret.M10 + M13 * ret.M11 + M23 * ret.M12);
        ret.M20 = (M10 * M21 - M20 * M11) * invDet;
        ret.M21 = -(M00 * M21 - M20 * M01) * invDet;
        ret.M22 = (M00 * M11 - M10 * M01) * invDet;
        ret.M23 = -(M03 * ret.M20 + M13 * ret.M21 + M23 * ret.M22);

        return ret;
    }

    /// Return float data.
    public float[] Data => new []{M00, M01, M02, M03, M10, M11, M12, M13, M20, M21, M22, M23};

    /// Return matrix element.
    public float this[int i, int j]
    {
        get
        {
            if (i < 0 || i > 2 || j < 0 || j > 3)
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
            if (i < 0 || i > 2 || j < 0 || j > 3)
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
    public Vector3 Column(int j) { return new Vector3(this[0, j], this[1, j], this[2, j]); }

    /// Return as string.
    public override string ToString()
    {
        return $"{M00} {M01} {M02} {M03} {M10} {M11} {M12} {M13} {M20} {M21} {M22} {M23}";
    }

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

    /// Zero matrix.
    public static readonly Matrix3x4 Zero = new Matrix3x4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    /// Identity matrix.
    public static readonly Matrix3x4 Identity = new Matrix3x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0);
};

}
