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
    /// Three-dimensional vector.
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3 : IEquatable<Vector3>
    {
        ///ruct from a two-dimensional vector and the Z coordinate.
        public Vector3(in Vector2 vector, float z = 0)
        {
            X = vector.X;
            Y = vector.Y;
            Z = z;
        }

        ///ruct from an IntVector3.
        public Vector3(in IntVector3 vector)
        {
            X = vector.X;
            Y = vector.Y;
            Z = vector.Z;
        }

        ///ruct from coordinates.
        public Vector3(float x = 0, float y = 0, float z = 0)
        {
            X = x;
            Y = y;
            Z = z;
        }

        ///ruct from a float array.
        public Vector3(IReadOnlyList<float> data)
        {
            X = data[0];
            Y = data[1];
            Z = data[2];
        }

        /// Test for equality with another vector without epsilon.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(in Vector3 lhs, in Vector3 rhs)
        {
            return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
        }

        /// Test for inequality with another vector without epsilon.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(in Vector3 lhs, in Vector3 rhs)
        {
            return lhs.X != rhs.X || lhs.Y != rhs.Y || lhs.Z != rhs.Z;
        }

        /// Add a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator +(in Vector3 lhs, in Vector3 rhs)
        {
            return new Vector3(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);
        }

        /// Return negation.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator -(in Vector3 rhs)
        {
            return new Vector3(-rhs.X, -rhs.Y, -rhs.Z);
        }

        /// Subtract a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator -(in Vector3 lhs, in Vector3 rhs)
        {
            return new Vector3(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);
        }

        /// Multiply with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator *(in Vector3 lhs, float rhs)
        {
            return new Vector3(lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs);
        }

        /// Multiply with a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator *(in Vector3 lhs, in Vector3 rhs)
        {
            return new Vector3(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z);
        }

        /// Divide by a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator /(in Vector3 lhs, float rhs)
        {
            return new Vector3(lhs.X / rhs, lhs.Y / rhs, lhs.Z / rhs);
        }

        /// Divide by a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator /(in Vector3 lhs, in Vector3 rhs)
        {
            return new Vector3(lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z);
        }

        /// Multiply Vector3 with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 operator *(float lhs, in Vector3 rhs)
        {
            return rhs * lhs;
        }

        /// Normalize to unit length.
        public void Normalize()
        {
            float lenSquared = LengthSquared;
            if (!MathDefs.Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
            {
                float invLen = 1.0f / (float) Math.Sqrt(lenSquared);
                X *= invLen;
                Y *= invLen;
                Z *= invLen;
            }
        }

        /// Return length.
        public float Length => (float) Math.Sqrt(LengthSquared);

        /// Return squared length.
        public float LengthSquared => X * X + Y * Y + Z * Z;

        /// Calculate dot product.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float DotProduct(in Vector3 rhs)
        {
            return X * rhs.X + Y * rhs.Y + Z * rhs.Z;
        }

        /// Calculate absolute dot product.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float AbsDotProduct(in Vector3 rhs)
        {
            return Math.Abs(X * rhs.X) + Math.Abs(Y * rhs.Y) + Math.Abs(Z * rhs.Z);
        }

        /// Project direction vector onto axis.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float ProjectOntoAxis(in Vector3 axis)
        {
            return DotProduct(axis.Normalized);
        }

        /// Project position vector onto plane with given origin and normal.
        public Vector3 ProjectOntoPlane(in Vector3 origin, in Vector3 normal)
        {
            Vector3 delta = this - origin;
            return this - normal.Normalized * delta.ProjectOntoAxis(normal);
        }

        /// Project position vector onto line segment.
        public Vector3 ProjectOntoLine(in Vector3 from, in Vector3 to, bool clamped = false)
        {
            Vector3 direction = to - from;
            float lengthSquared = direction.LengthSquared;
            float factor = (this - from).DotProduct(direction) / lengthSquared;

            if (clamped)
                factor = MathDefs.Clamp(factor, 0.0f, 1.0f);

            return from + direction * factor;
        }

        /// Calculate distance to another position vector.
        public float DistanceToPoint(in Vector3 point)
        {
            return (this - point).Length;
        }

        /// Calculate distance to the plane with given origin and normal.
        public float DistanceToPlane(in Vector3 origin, in Vector3 normal)
        {
            return (this - origin).ProjectOntoAxis(normal);
        }

        /// Make vector orthogonal to the axis.
        public Vector3 Orthogonalize(in Vector3 axis)
        {
            return axis.CrossProduct(this).CrossProduct(axis).Normalized;
        }

        /// Calculate cross product.
        public Vector3 CrossProduct(in Vector3 rhs)
        {
            return new Vector3(
                Y * rhs.Z - Z * rhs.Y,
                Z * rhs.X - X * rhs.Z,
                X * rhs.Y - Y * rhs.X
            );
        }

        /// Return absolute vector.
        public Vector3 Abs => new Vector3(Math.Abs(X), Math.Abs(Y), Math.Abs(Z));

        /// Linear interpolation with another vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector3 Lerp(in Vector3 rhs, float t)
        {
            return this * (1.0f - t) + rhs * t;
        }

        /// Test for equality with another vector with epsilon.
        public bool Equals(Vector3 rhs)
        {
            return MathDefs.Equals(X, rhs.X) && MathDefs.Equals(Y, rhs.Y) && MathDefs.Equals(Z, rhs.Z);
        }

        /// Test for equality with another vector with epsilon.
        public override bool Equals(object obj)
        {
            return obj is Vector3 other && Equals(other);
        }

        /// Returns the angle between this vector and another vector in degrees.
        public float Angle(in Vector3 rhs)
        {
            return (float) Math.Acos(DotProduct(rhs) / (Length * rhs.Length));
        }

        /// Return whether is NaN.
        public bool IsNaN => float.IsNaN(X) || float.IsNaN(Y) || float.IsNaN(Z);

        /// Return normalized to unit length.
        public Vector3 Normalized
        {
            get
            {
                float lenSquared = LengthSquared;
                if (!MathDefs.Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
                {
                    float invLen = 1.0f / (float) Math.Sqrt(lenSquared);
                    return this * invLen;
                }
                else
                    return this;
            }
        }

        /// Return float data.
        public float[] Data => new[] {X, Y, Z};

        /// Return as string.
        public override string ToString()
        {
            return $"{X} {Y} {Z}";
        }

        /// Return hash value for HashSet & HashMap.
        public override int GetHashCode()
        {
            uint hash = 37;
            hash = 37 * hash + MathDefs.FloatToRawIntBits(X);
            hash = 37 * hash + MathDefs.FloatToRawIntBits(Y);
            hash = 37 * hash + MathDefs.FloatToRawIntBits(Z);

            return (int) hash;
        }

        /// Per-component linear interpolation between two 3-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 Lerp(in Vector3 lhs, in Vector3 rhs, in Vector3 t)
        {
            return lhs + (rhs - lhs) * t;
        }

        /// Per-component min of two 3-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 Min(in Vector3 lhs, in Vector3 rhs)
        {
            return new Vector3(Math.Min(lhs.X, rhs.X), Math.Min(lhs.Y, rhs.Y), Math.Min(lhs.Z, rhs.Z));
        }

        /// Per-component max of two 3-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 Max(in Vector3 lhs, in Vector3 rhs)
        {
            return new Vector3(Math.Max(lhs.X, rhs.X), Math.Max(lhs.Y, rhs.Y), Math.Max(lhs.Z, rhs.Z));
        }

        /// Per-component floor of 3-vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 Floor(in Vector3 vec)
        {
            return new Vector3((float)Math.Floor(vec.X), (float)Math.Floor(vec.Y), (float)Math.Floor(vec.Z));
        }

        /// Per-component round of 3-vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 Round(in Vector3 vec)
        {
            return new Vector3((float)Math.Round(vec.X), (float)Math.Round(vec.Y), (float)Math.Round(vec.Z));
        }

        /// Per-component ceil of 3-vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3 Ceil(in Vector3 vec)
        {
            return new Vector3((float)Math.Ceiling(vec.X), (float)Math.Ceiling(vec.Y), (float)Math.Ceiling(vec.Z));
        }

        /// Per-component floor of 3-vector. Returns IntVector3.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 FloorToInt(in Vector3 vec)
        {
            return new IntVector3((int)Math.Floor(vec.X), (int)Math.Floor(vec.Y), (int)Math.Floor(vec.Z));
        }

        /// Per-component round of 3-vector. Returns IntVector3.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 RoundToInt(in Vector3 vec)
        {
            return new IntVector3((int)Math.Round(vec.X), (int)Math.Round(vec.Y), (int)Math.Round(vec.Z));
        }

        /// Per-component ceil of 3-vector. Returns IntVector3.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 CeilToInt(in Vector3 vec)
        {
            return new IntVector3((int)Math.Ceiling(vec.X), (int)Math.Ceiling(vec.Y), (int)Math.Ceiling(vec.Z));
        }

        /// Return a random value from [0, 1) from 3-vector seed.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float StableRandom(in Vector3 seed)
        {
            return Vector2.StableRandom(new Vector2(Vector2.StableRandom(new Vector2(seed.X, seed.Y)), seed.Z));
        }

        /// X coordinate.
        public float X;

        /// Y coordinate.
        public float Y;

        /// Z coordinate.
        public float Z;

        /// Zero vector.
        public static readonly Vector3 Zero;

        /// (-1,0,0) vector.
        public static readonly Vector3 Left = new Vector3(-1, 0, 0);

        /// (1,0,0) vector.
        public static readonly Vector3 Right = new Vector3(1, 0, 0);

        /// (0,1,0) vector.
        public static readonly Vector3 Up = new Vector3(0, 1, 0);

        /// (0,-1,0) vector.
        public static readonly Vector3 Down = new Vector3(0, -1, 0);

        /// (0,0,1) vector.
        public static readonly Vector3 Forward = new Vector3(0, 0, 1);

        /// (0,0,-1) vector.
        public static readonly Vector3 Back = new Vector3(0, 0, -1);

        /// (1,1,1) vector.
        public static readonly Vector3 One = new Vector3(1, 1, 1);
    };
}
