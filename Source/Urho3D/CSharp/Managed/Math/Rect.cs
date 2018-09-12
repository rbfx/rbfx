using System;
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
        public unsafe Rect(float* data)
        {
            Min.X = data[0];
            Min.Y = data[1];
            Max.X = data[2];
            Max.Y = data[3];
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
        Intersection IsInside(in Vector2 point)
        {
            if (point.X < Min.X || point.Y < Min.Y || point.X > Max.X || point.Y > Max.Y)
                return Intersection.Outside;
            else
                return Intersection.Inside;
        }

        /// Test if another rect is inside, outside or intersects.
        Intersection IsInside(in Rect rect)
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

    [StructLayout(LayoutKind.Sequential)]
    public struct IntRect : IEquatable<IntRect>
    {
        public bool Equals(IntRect other)
        {
            return Left == other.Left && Top == other.Top && Right == other.Right && Bottom == other.Bottom;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            return obj is IntRect && Equals((IntRect) obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = Left;
                hashCode = (hashCode * 397) ^ Top;
                hashCode = (hashCode * 397) ^ Right;
                hashCode = (hashCode * 397) ^ Bottom;
                return hashCode;
            }
        }

        /// Left coordinate.
        public int Left;
        /// Top coordinate.
        public int Top;
        /// Right coordinate.
        public int Right;
        /// Bottom coordinate.
        public int Bottom;

        public IntRect(int left, int top, int right, int bottom)
        {
            Left = left;
            Top = top;
            Right = right;
            Bottom = bottom;
        }

        public static readonly IntRect Zero = new IntRect(0, 0, 0, 0);

        public IntRect(in IntVector2 min, in IntVector2 max)
        {
            Left = min.X;
            Top = min.Y;
            Right = max.X;
            Bottom = max.Y;
        }

        public int Width => Right - Left;
        public int Height => Bottom - Top;
        public IntVector2 Size => new IntVector2(Width, Height);

        public static bool operator !=(in IntRect lhs, in IntRect rhs)
        {
            return lhs.Top != rhs.Top || lhs.Right != rhs.Right || lhs.Bottom != rhs.Bottom || lhs.Left != rhs.Left;
        }

        public static bool operator ==(in IntRect lhs, in IntRect rhs)
        {
            return !(lhs != rhs);
        }
    }
}
