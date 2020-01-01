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
    /// Two-dimensional vector.
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2 : IEquatable<Vector2>
    {
        /// Construct from an IntVector2.
        public Vector2(in IntVector2 vector)
        {
            X = vector.X;
            Y = vector.Y;
        }

        /// Construct from coordinates.
        public Vector2(float x, float y)
        {
            X = x;
            Y = y;
        }

        /// Construct from a float array.
        public Vector2(IReadOnlyList<float> data)
        {
            X = data[0];
            Y = data[1];
        }

        /// Test for equality with another vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(in Vector2 lhs, in Vector2 rhs)
        {
            return lhs.X == rhs.X && lhs.Y == rhs.Y;
        }

        /// Test for inequality with another vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(in Vector2 lhs, in Vector2 rhs)
        {
            return lhs.X != rhs.X || lhs.Y != rhs.Y;
        }

        /// Add a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator +(in Vector2 lhs, in Vector2 rhs)
        {
            return new Vector2(lhs.X + rhs.X, lhs.Y + rhs.Y);
        }

        /// Return negation.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator -(in Vector2 v)
        {
            return new Vector2(-v.X, -v.Y);
        }

        /// Subtract a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator -(in Vector2 lhs, in Vector2 rhs)
        {
            return new Vector2(lhs.X - rhs.X, lhs.Y - rhs.Y);
        }

        /// Multiply with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator *(in Vector2 lhs, float rhs)
        {
            return new Vector2(lhs.X * rhs, lhs.Y * rhs);
        }

        /// Multiply with a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator *(in Vector2 lhs, in Vector2 rhs)
        {
            return new Vector2(lhs.X * rhs.X, lhs.Y * rhs.Y);
        }

        /// Divide by a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator /(in Vector2 lhs, float rhs)
        {
            return new Vector2(lhs.X / rhs, lhs.Y / rhs);
        }

        /// Divide by a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator /(in Vector2 lhs, in Vector2 rhs)
        {
            return new Vector2(lhs.X / rhs.X, lhs.Y / rhs.Y);
        }

        /// Multiply Vector2 with a scalar
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 operator *(float lhs, in Vector2 rhs)
        {
            return rhs * lhs;
        }

        /// Return value by index.
        public float this[int index]
        {
            get
            {
                if (index < 0 || index > 1)
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
                if (index < 0 || index > 1)
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

        /// Normalize to unit length.
        public void Normalize()
        {
            float lenSquared = LengthSquared;
            if (!MathDefs.Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
            {
                float invLen = 1.0f / (float) Math.Sqrt(lenSquared);
                X *= invLen;
                Y *= invLen;
            }
        }

        /// Return length.
        public float Length => (float) Math.Sqrt(X * X + Y * Y);

        /// Return squared length.
        public float LengthSquared => X * X + Y * Y;

        /// Calculate dot product.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float DotProduct(in Vector2 rhs)
        {
            return X * rhs.X + Y * rhs.Y;
        }

        /// Calculate absolute dot product.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float AbsDotProduct(in Vector2 rhs)
        {
            return Math.Abs(X * rhs.X) + Math.Abs(Y * rhs.Y);
        }

        /// Project vector onto axis.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float ProjectOntoAxis(in Vector2 axis)
        {
            return DotProduct(axis.Normalized);
        }

        /// Returns the angle between this vector and another vector in degrees.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float Angle(in Vector2 rhs)
        {
            return (float) Math.Acos(DotProduct(rhs) / (Length * rhs.Length));
        }

        /// Return absolute vector.
        public Vector2 Abs => new Vector2(Math.Abs(X), Math.Abs(Y));

        /// Linear interpolation with another vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector2 Lerp(in Vector2 rhs, float t)
        {
            return this * (1.0f - t) + rhs * t;
        }

        /// Test for equality with another vector with epsilon.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Vector2 rhs)
        {
            return MathDefs.Equals(X, rhs.X) && MathDefs.Equals(Y, rhs.Y);
        }

        /// Return whether is NaN.
        public bool IsNaN => float.IsNaN(X) || float.IsNaN(Y);

        /// Return normalized to unit length.
        public Vector2 Normalized
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
        public float[] Data => new float[] {X, Y};

        /// Return as string.
        public override string ToString()
        {
            return $"{X} {Y}";
        }

        /// Return hash value for HashSet & HashMap.
        public override int GetHashCode()
        {
            unchecked
            {
                return (X.GetHashCode() * 31) ^ Y.GetHashCode();
            }
        }

        /// Per-component linear interpolation between two 2-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 Lerp(in Vector2 lhs, in Vector2 rhs, in Vector2 t)
        {
            return lhs + (rhs - lhs) * t;
        }

        /// Per-component min of two 2-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 Min(in Vector2 lhs, in Vector2 rhs)
        {
            return new Vector2(Math.Min(lhs.X, rhs.X), Math.Min(lhs.Y, rhs.Y));
        }

        /// Per-component max of two 2-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 Max(in Vector2 lhs, in Vector2 rhs)
        {
            return new Vector2(Math.Max(lhs.X, rhs.X), Math.Max(lhs.Y, rhs.Y));
        }

        /// Per-component floor of 2-vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 Floor(in Vector2 vec)
        {
            return new Vector2((float) Math.Floor(vec.X), (float) Math.Floor(vec.Y));
        }

        /// Per-component round of 2-vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 Round(in Vector2 vec)
        {
            return new Vector2((float) Math.Round(vec.X), (float) Math.Round(vec.Y));
        }

        /// Per-component ceil of 2-vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2 Ceil(in Vector2 vec)
        {
            return new Vector2((float) Math.Ceiling(vec.X), (float) Math.Ceiling(vec.Y));
        }

        /// Per-component floor of 2-vector. Returns IntVector2.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 FloorToInt(in Vector2 vec)
        {
            return new IntVector2((int) Math.Floor(vec.X), (int) Math.Floor(vec.Y));
        }

        /// Per-component round of 2-vector. Returns IntVector2.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 RoundToInt(in Vector2 vec)
        {
            return new IntVector2((int) Math.Round(vec.X), (int) Math.Round(vec.Y));
        }

        /// Per-component ceil of 2-vector. Returns IntVector2.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 CeilToInt(in Vector2 vec)
        {
            return new IntVector2((int) Math.Ceiling(vec.X), (int) Math.Ceiling(vec.Y));
        }

        /// Return a random value from [0, 1) from 2-vector seed.
        /// http://stackoverflow.com/questions/12964279/whats-the-origin-of-this-glsl-rand-one-liner
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float StableRandom(in Vector2 seed)
        {
            return (float) MathDefs.Fract(
                Math.Sin(MathDefs.RadiansToDegrees(seed.DotProduct(new Vector2(12.9898f, 78.233f)))) * 43758.5453f);
        }

        /// Return a random value from [0, 1) from scalar seed.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float StableRandom(float seed)
        {
            return StableRandom(new Vector2(seed, seed));
        }

        /// X coordinate.
        public float X;

        /// Y coordinate.
        public float Y;

        /// Zero vector.
        public static readonly Vector2 ZERO;

        /// (-1,0) vector.
        public static readonly Vector2 LEFT = new Vector2(-1, 0);

        /// (1,0) vector.
        public static readonly Vector2 RIGHT = new Vector2(1, 0);

        /// (0,1) vector.
        public static readonly Vector2 UP = new Vector2(0, 1);

        /// (0,-1) vector.
        public static readonly Vector2 DOWN = new Vector2(0, -1);

        /// (1,1) vector.
        public static readonly Vector2 ONE = new Vector2(1, 1);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override bool Equals(object obj)
        {
            return obj is Vector2 other && Equals(other);
        }
    }
}
