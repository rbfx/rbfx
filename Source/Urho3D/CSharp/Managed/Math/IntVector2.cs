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
    /// Two-dimensional vector with integer values.
    [StructLayout(LayoutKind.Sequential)]
    public struct IntVector2 : IEquatable<IntVector2>
    {
        /// Construct from coordinates.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public IntVector2(int x = 0, int y = 0)
        {
            X = x;
            Y = y;
        }

        /// Construct from an int array.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public IntVector2(IReadOnlyList<int> data)
        {
            X = data[0];
            Y = data[1];
        }

        /// Construct from an float array.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public IntVector2(IReadOnlyList<float> data)
        {
            X = (int) data[0];
            Y = (int) data[1];
        }

        /// Test for equality with another vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(in IntVector2 lhs, in IntVector2 rhs)
        {
            return lhs.X == rhs.X && lhs.Y == rhs.Y;
        }

        /// Test for inequality with another vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(in IntVector2 lhs, in IntVector2 rhs)
        {
            return lhs.X != rhs.X || lhs.Y != rhs.Y;
        }

        /// Add a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 operator +(in IntVector2 lhs, in IntVector2 rhs)
        {
            return new IntVector2(lhs.X + rhs.X, lhs.Y + rhs.Y);
        }

        /// Return negation.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 operator -(in IntVector2 v)
        {
            return new IntVector2(-v.X, -v.Y);
        }

        /// Subtract a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 operator -(in IntVector2 lhs, in IntVector2 rhs)
        {
            return new IntVector2(lhs.X - rhs.X, lhs.Y - rhs.Y);
        }

        /// Multiply with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 operator *(in IntVector2 lhs, int rhs)
        {
            return new IntVector2(lhs.X * rhs, lhs.Y * rhs);
        }

        /// Multiply with a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 operator *(in IntVector2 lhs, in IntVector2 rhs)
        {
            return new IntVector2(lhs.X * rhs.X, lhs.Y * rhs.Y);
        }

        /// Divide by a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 operator /(in IntVector2 lhs, int rhs)
        {
            return new IntVector2(lhs.X / rhs, lhs.Y / rhs);
        }

        /// Divide by a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 operator /(in IntVector2 lhs, in IntVector2 rhs)
        {
            return new IntVector2(lhs.X / rhs.X, lhs.Y / rhs.Y);
        }

        /// Multiply IntVector2 with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 operator *(int lhs, in IntVector2 rhs)
        {
            return rhs * lhs;
        }

        /// Return value by index.
        public int this[int index]
        {
            get
            {
                if (index < 0 || index > 1)
                    throw new IndexOutOfRangeException();
                unsafe
                {
                    fixed (int* p = &X)
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
                    fixed (int* p = &X)
                    {
                        p[index] = value;
                    }
                }
            }
        }

        /// Return integer data.
        public int[] Data => new int[] {X, Y};

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
                return (int) ((uint) X * 31 + Y);
            }
        }

        /// Return length.
        public float Length => (float) Math.Sqrt(X * X + Y * Y);

        /// Per-component min of two 2-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 Min(in IntVector2 lhs, in IntVector2 rhs)
        {
            return new IntVector2(Math.Min(lhs.X, rhs.X), Math.Min(lhs.Y, rhs.Y));
        }

        /// Per-component max of two 2-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector2 Max(in IntVector2 lhs, in IntVector2 rhs)
        {
            return new IntVector2(Math.Max(lhs.X, rhs.X), Math.Max(lhs.Y, rhs.Y));
        }

        /// X coordinate.
        public int X;

        /// Y coordinate.
        public int Y;

        /// Zero vector.
        public static readonly IntVector2 Zero;

        /// (-1,0) vector.
        public static readonly IntVector2 Left = new IntVector2(-1, 0);

        /// (1,0) vector.
        public static readonly IntVector2 Right = new IntVector2(1, 0);

        /// (0,1) vector.
        public static readonly IntVector2 Up = new IntVector2(0, 1);

        /// (0,-1) vector.
        public static readonly IntVector2 Down = new IntVector2(0, -1);

        /// (1,1) vector.
        public static readonly IntVector2 One = new IntVector2(1, 1);

        public bool Equals(IntVector2 other)
        {
            return X == other.X && Y == other.Y;
        }

        public override bool Equals(object obj)
        {
            return obj is IntVector2 other && Equals(other);
        }
    }
}
