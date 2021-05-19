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
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect : IEquatable<Rect>
    {
        /// Return left-top corner position.
        public Vector2 Min;
        /// Return right-bottom corner position.
        public Vector2 Max;
        /// Return left coordinate.
        public float Left => Min.X;
        /// Return top coordinate.
        public float Top => Min.Y;
        /// Return right coordinate.
        public float Right => Max.X;
        /// Return bottom coordinate.
        public float Bottom => Max.Y;

        /// Construct from coordinates.
        public Rect(float minX=float.PositiveInfinity, float minY=float.PositiveInfinity,
            float maxX=float.NegativeInfinity, float maxY=float.NegativeInfinity)
        {
            Min = new Vector2(minX, minY);
            Max = new Vector2(maxX, maxY);
        }

        /// Construct from minimum and maximum vectors.
        public Rect(in Vector2 min, in Vector2 max)
        {
            Min = min;
            Max = max;
        }

        /// Construct from a Vector4.
        public Rect(in Vector4 vector)
        {
            Min = vector.Xy;
            Max = vector.Zw;
        }

        /// Construct from a float array.
        public Rect(IReadOnlyList<float> data)
        {
            Min = new Vector2(data[0], data[1]);
            Max = new Vector2(data[2], data[3]);
        }

        /// Add two rects together.
        public static Rect operator +(in Rect lhs, in Rect rhs)
        {
            return new Rect(lhs.Min + rhs.Min, lhs.Max + rhs.Max);
        }

        /// Subtract rhs from lhs.
        public static Rect operator -(in Rect lhs, in Rect rhs)
        {
            return new Rect(lhs.Min - rhs.Min, lhs.Max - rhs.Max);
        }

        /// Divide lhs from rhs.
        public static Rect operator /(in Rect lhs, in Rect rhs)
        {
            return new Rect(lhs.Min.X / rhs.Min.X, lhs.Min.Y / rhs.Min.Y, lhs.Max.X / rhs.Max.X, lhs.Max.Y / rhs.Max.Y);
        }

        /// Multiply lhs by rhs.
        public static Rect operator *(in Rect lhs, in Rect rhs)
        {
            return new Rect(lhs.Min.X * rhs.Min.X, lhs.Min.Y * rhs.Min.Y, lhs.Max.X * rhs.Max.X, lhs.Max.Y * rhs.Max.Y);
        }

        /// Test for equality with another rect.
        public static bool operator ==(in Rect lhs, in Rect rhs)
        {
            return lhs.Equals(rhs);
        }

        /// Test for inequality with another rect.
        public static bool operator !=(in Rect lhs, in Rect rhs)
        {
            return !lhs.Equals(rhs);
        }

        /// Merge a point.
        public void Merge(in Vector2 point)
        {
            if (point.X < Min.X)
                Min.X = point.X;
            if (point.X > Max.X)
                Max.X = point.X;
            if (point.Y < Min.Y)
                Min.Y = point.Y;
            if (point.Y > Max.Y)
                Max.Y = point.Y;
        }

        /// Merge a rect.
        public void Merge(in Rect rect)
        {
            if (rect.Min.X < Min.X)
                Min.X = rect.Min.X;
            if (rect.Min.Y < Min.Y)
                Min.Y = rect.Min.Y;
            if (rect.Max.X > Max.X)
                Max.X = rect.Max.X;
            if (rect.Max.Y > Max.Y)
                Max.Y = rect.Max.Y;
        }

        /// Clear to undefined state.
        public void Clear()
        {
            Min = new Vector2(float.PositiveInfinity, float.PositiveInfinity);
            Max = new Vector2(float.NegativeInfinity, float.NegativeInfinity);
        }

        /// Clip with another rect.
        public void Clip(in Rect rect)
        {
            if (rect.Min.X > Min.X)
                Min.X = rect.Min.X;
            if (rect.Max.X < Max.X)
                Max.X = rect.Max.X;
            if (rect.Min.Y > Min.Y)
                Min.Y = rect.Min.Y;
            if (rect.Max.Y < Max.Y)
                Max.Y = rect.Max.Y;

            if (Min.X > Max.X || Min.Y > Max.Y)
            {
                Min = new Vector2(float.PositiveInfinity, float.PositiveInfinity);
                Max = new Vector2(float.NegativeInfinity, float.NegativeInfinity);
            }
        }

        /// Return true if this rect is defined via a previous call to Define() or Merge().
        public bool Defined => !float.IsPositiveInfinity(Min.X);

        /// Return center.
        public Vector2 Center => (Max + Min) * 0.5f;

        /// Return size.
        public Vector2 Size => Max - Min;

        /// Return half-size.
        public Vector2 HalfSize => (Max - Min) * 0.5f;

        /// Test whether a point is inside.
        public Intersection IsInside(in Vector2 point)
        {
            if (point.X < Min.X || point.Y < Min.Y || point.X > Max.X || point.Y > Max.Y)
                return Intersection.Outside;
            else
                return Intersection.Inside;
        }

        /// Test if another rect is inside, outside or intersects.
        public Intersection IsInside(in Rect rect)
        {
            if (rect.Max.X < Min.X || rect.Min.X > Max.X || rect.Max.Y < Min.Y || rect.Min.Y > Max.Y)
                return Intersection.Outside;
            else if (rect.Min.X < Min.X || rect.Max.X > Max.X || rect.Min.Y < Min.Y || rect.Max.Y > Max.Y)
                return Intersection.Intersects;
            else
                return Intersection.Inside;
        }

        /// Rect in the range (-1, -1) - (1, 1)
        public static readonly Rect Full = new Rect(-1.0f, -1.0f, 1.0f, 1.0f);
        /// Rect in the range (0, 0) - (1, 1)
        public static readonly Rect Positive = new Rect(0.0f, 0.0f, 1.0f, 1.0f);
        /// Zero-sized rect.
        public static readonly Rect Zero = new Rect(0.0f, 0.0f, 0.0f, 0.0f);

        /// Test for equality with another rect with epsilon.
        public bool Equals(Rect other)
        {
            return Min.Equals(other.Min) && Max.Equals(other.Max);
        }

        /// Test for equality with another rect with epsilon.
        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            return obj is Rect other && Equals(other);
        }

        /// Returns hashcode.
        public override int GetHashCode()
        {
            unchecked
            {
                return (Min.GetHashCode() * 397) ^ Max.GetHashCode();
            }
        }

        /// Return as string.
        public override string ToString()
        {
            return $"{Min.X} {Min.Y} {Max.X} {Max.Y}";
        }
    }
}
