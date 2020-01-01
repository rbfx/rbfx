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
    /// Four-dimensional vector.
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector4 : IEquatable<Vector4>
    {
        /// Construct from a 3-dimensional vector and the W coordinate.
        public Vector4(in Vector3 vector, float w)
        {
            X = vector.X;
            Y = vector.Y;
            Z = vector.Z;
            W = w;
        }

        /// Construct from coordinates.
        public Vector4(float x = 0, float y = 0, float z = 0, float w = 0)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        /// Construct from a float array.
        public Vector4(IReadOnlyList<float> data)
        {
            X = data[0];
            Y = data[1];
            Z = data[2];
            W = data[3];
        }

        /// Test for equality with another vector without epsilon.
        public static bool operator ==(in Vector4 lhs, in Vector4 rhs)
        {
            return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z && lhs.W == rhs.W;
        }

        /// Test for inequality with another vector without epsilon.
        public static bool operator !=(in Vector4 lhs, in Vector4 rhs)
        {
            return lhs.X != rhs.X || lhs.Y != rhs.Y || lhs.Z != rhs.Z || lhs.W != rhs.W;
        }

        /// Add a vector.
        public static Vector4 operator +(in Vector4 lhs, in Vector4 rhs)
        {
            return new Vector4(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z, lhs.W + rhs.W);
        }

        /// Return negation.
        public static Vector4 operator -(in Vector4 rhs)
        {
            return new Vector4(-rhs.X, -rhs.Y, -rhs.Z, -rhs.W);
        }

        /// Subtract a vector.
        public static Vector4 operator -(in Vector4 lhs, in Vector4 rhs)
        {
            return new Vector4(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z, lhs.W - rhs.W);
        }

        /// Multiply with a scalar.
        public static Vector4 operator *(in Vector4 lhs, float rhs)
        {
            return new Vector4(lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs, lhs.W * rhs);
        }

        /// Multiply with a vector.
        public static Vector4 operator *(in Vector4 lhs, in Vector4 rhs)
        {
            return new Vector4(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z, lhs.W * rhs.W);
        }

        /// Divide by a scalar.
        public static Vector4 operator /(in Vector4 lhs, float rhs)
        {
            return new Vector4(lhs.X / rhs, lhs.Y / rhs, lhs.Z / rhs, lhs.W / rhs);
        }

        /// Divide by a vector.
        public static Vector4 operator /(in Vector4 lhs, in Vector4 rhs)
        {
            return new Vector4(lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z, lhs.W / rhs.W);
        }

        /// Return value by index.
        public float this[int index]
        {
            get
            {
                if (index < 0 || index > 3)
                    throw new IndexOutOfRangeException();
                unsafe
                {
                    fixed (float* p = &X)
                    {
                        return p[index];
                    }
                }
            }
            set
            {
                if (index < 0 || index > 3)
                    throw new IndexOutOfRangeException();
                unsafe
                {
                    fixed (float* p = &X)
                    {
                        p[index] = value;
                    }
                }
            }
        }

        /// Calculate dot product.
        float DotProduct(in Vector4 rhs)
        {
            return X * rhs.X + Y * rhs.Y + Z * rhs.Z + W * rhs.W;
        }

        /// Calculate absolute dot product.
        float AbsDotProduct(in Vector4 rhs)
        {
            return Math.Abs(X * rhs.X) + Math.Abs(Y * rhs.Y) + Math.Abs(Z * rhs.Z) + Math.Abs(W * rhs.W);
        }

        /// Project vector onto axis.
        float ProjectOntoAxis(in Vector3 axis)
        {
            return DotProduct(new Vector4(axis.Normalized, 0.0f));
        }

        /// Return absolute vector.
        Vector4 Abs => new Vector4(Math.Abs(X), Math.Abs(Y), Math.Abs(Z), Math.Abs(W));

        /// Linear interpolation with another vector.
        Vector4 Lerp(in Vector4 rhs, float t)
        {
            return this * (1.0f - t) + rhs * t;
        }

        /// Test for equality with another vector with epsilon.
        public bool Equals(Vector4 rhs)
        {
            return MathDefs.Equals(X, rhs.X) && MathDefs.Equals(Y, rhs.Y) && MathDefs.Equals(Z, rhs.Z) &&
                   MathDefs.Equals(W, rhs.W);
        }

        public override bool Equals(object obj)
        {
            return obj is Vector4 other && Equals(other);
        }

        /// Return whether is NaN.
        bool IsNaN => float.IsNaN(X) || float.IsNaN(Y) || float.IsNaN(Z) || float.IsNaN(W);

        /// Return float data.
        float[] Data => new float[] {X, Y, Z, W};

        /// Return as string.
        public override string ToString()
        {
            return $"{X} {Y} {Z} {W}";
        }

        /// Return hash value for HashSet & HashMap.
        public override int GetHashCode()
        {
            uint hash = 37;
            hash = 37 * hash + MathDefs.FloatToRawIntBits(X);
            hash = 37 * hash + MathDefs.FloatToRawIntBits(Y);
            hash = 37 * hash + MathDefs.FloatToRawIntBits(Z);
            hash = 37 * hash + MathDefs.FloatToRawIntBits(W);

            return (int) hash;
        }

        /// Multiply Vector4 with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4 operator *(float lhs, in Vector4 rhs)
        {
            return rhs * lhs;
        }

        /// Per-component linear interpolation between two 4-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4 Lerp(in Vector4 lhs, in Vector4 rhs, in Vector4 t)
        {
            return lhs + (rhs - lhs) * t;
        }

        /// Per-component min of two 4-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4 Min(in Vector4 lhs, in Vector4 rhs)
        {
            return new Vector4(Math.Min(lhs.X, rhs.X), Math.Min(lhs.Y, rhs.Y), Math.Min(lhs.Z, rhs.Z),
                Math.Min(lhs.W, rhs.W));
        }

        /// Per-component max of two 4-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4 Max(in Vector4 lhs, in Vector4 rhs)
        {
            return new Vector4(Math.Max(lhs.X, rhs.X), Math.Max(lhs.Y, rhs.Y), Math.Max(lhs.Z, rhs.Z),
                Math.Max(lhs.W, rhs.W));
        }

        /// Per-component floor of 4-vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4 Floor(in Vector4 vec)
        {
            return new Vector4((float) Math.Floor(vec.X), (float) Math.Floor(vec.Y), (float) Math.Floor(vec.Z),
                (float) Math.Floor(vec.W));
        }

        /// Per-component round of 4-vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4 Round(in Vector4 vec)
        {
            return new Vector4((float) Math.Round(vec.X), (float) Math.Round(vec.Y), (float) Math.Round(vec.Z),
                (float) Math.Round(vec.W));
        }

        /// Per-component ceil of 4-vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4 Ceil(in Vector4 vec)
        {
            return new Vector4((float) Math.Ceiling(vec.X), (float) Math.Ceiling(vec.Y), (float) Math.Ceiling(vec.Z),
                (float) Math.Ceiling(vec.W));
        }

        public Vector2 Xy => new Vector2(X, Y);
        public Vector2 Zw => new Vector2(Z, W);

        /// X coordinate.
        public float X;

        /// Y coordinate.
        public float Y;

        /// Z coordinate.
        public float Z;

        /// W coordinate.
        public float W;

        /// Zero vector.
        static Vector4 Zero;

        /// (1,1,1,1) vector.
        static Vector4 One = new Vector4(1, 1, 1, 1);
    };
}
