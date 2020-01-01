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
    /// Three-dimensional vector with integer values.
    [StructLayout(LayoutKind.Sequential)]
    public struct IntVector3 : IEquatable<IntVector3>
    {
        /// Construct from coordinates.
        public IntVector3(int x = 0, int y = 0, int z = 0)
        {
            X = x;
            Y = y;
            Z = z;
        }

        /// Construct from an int array.
        public IntVector3(IReadOnlyList<int> data)
        {
            X = data[0];
            Y = data[1];
            Z = data[2];
        }

        /// Test for equality with another vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(in IntVector3 lhs, in IntVector3 rhs)
        {
            return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
        }

        /// Test for inequality with another vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(in IntVector3 lhs, in IntVector3 rhs)
        {
            return lhs.X != rhs.X || lhs.Y != rhs.Y || lhs.Z != rhs.Z;
        }

        /// Add a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 operator +(in IntVector3 rhs)
        {
            return new IntVector3(rhs.X + rhs.X, rhs.Y + rhs.Y, rhs.Z + rhs.Z);
        }

        /// Return negation.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 operator -(in IntVector3 lhs)
        {
            return new IntVector3(-lhs.X, -lhs.Y, -lhs.Z);
        }

        /// Subtract a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 operator -(in IntVector3 lhs, in IntVector3 rhs)
        {
            return new IntVector3(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);
        }

        /// Multiply with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 operator *(in IntVector3 lhs, int rhs)
        {
            return new IntVector3(lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs);
        }

        /// Multiply with a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 operator *(in IntVector3 lhs, in IntVector3 rhs)
        {
            return new IntVector3(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z);
        }

        /// Divide by a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 operator /(in IntVector3 lhs, int rhs)
        {
            return new IntVector3(lhs.X / rhs, lhs.Y / rhs, lhs.Z / rhs);
        }

        /// Divide by a vector.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 operator /(in IntVector3 lhs, in IntVector3 rhs)
        {
            return new IntVector3(lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z);
        }

        /// Multiply IntVector3 with a scalar.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 operator *(int lhs, in IntVector3 rhs) { return rhs * lhs; }

        /// Return integer data.
        public int[] Data => new int[] {X, Y, Z};

        /// Return as string.
        public override string ToString()
        {
            return $"{X} {Y} {Z}";
        }

        public override int GetHashCode()
        {
            return (int) ((uint) X * 31 * 31 + (uint) Y * 31 + (uint) Z);
        }

        /// Return length.
        public float Length => (float) Math.Sqrt(X * X + Y * Y + Z * Z);

        /// Per-component min of two 3-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 Min(in IntVector3 lhs, in IntVector3 rhs)
        {
            return new IntVector3(Math.Min(lhs.X, rhs.X), Math.Min(lhs.Y, rhs.Y), Math.Min(lhs.Z, rhs.Z));
        }

        /// Per-component max of two 3-vectors.
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static IntVector3 Max(in IntVector3 lhs, in IntVector3 rhs)
        {
            return new IntVector3(Math.Max(lhs.X, rhs.X), Math.Max(lhs.Y, rhs.Y), Math.Max(lhs.Z, rhs.Z));
        }

        /// X coordinate.
        public int X;

        /// Y coordinate.
        public int Y;

        /// Z coordinate.
        public int Z;

        /// Zero vector.
        static IntVector3 Zero = new IntVector3(0, 0, 0);

        /// (-1,0,0) vector.
        static IntVector3 Left = new IntVector3(-1, 0, 0);

        /// (1,0,0) vector.
        static IntVector3 Right = new IntVector3(1, 0, 0);

        /// (0,1,0) vector.
        static IntVector3 Up = new IntVector3(0, 1, 0);

        /// (0,-1,0) vector.
        static IntVector3 Down = new IntVector3(0, -1, 0);

        /// (0,0,1) vector.
        static IntVector3 Forward = new IntVector3(0, 0, 1);

        /// (0,0,-1) vector.
        static IntVector3 Back = new IntVector3(0, 0, -1);

        /// (1,1,1) vector.
        static IntVector3 One = new IntVector3(1, 1, 1);

        public bool Equals(IntVector3 other)
        {
            return X == other.X && Y == other.Y && Z == other.Z;
        }

        public override bool Equals(object obj)
        {
            return obj is IntVector3 other && Equals(other);
        }
    }
}
