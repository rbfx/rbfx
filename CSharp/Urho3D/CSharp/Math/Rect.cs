using System;
using System.Runtime.InteropServices;

namespace Urho3D
{
	[StructLayout(LayoutKind.Sequential)]
	public struct Rect
	{
        /// Minimum vector.
        Vector2 min_;
        /// Maximum vector.
        Vector2 max_;

		public Rect(float minX, float minY, float maxX, float maxY)
		{
			min_ = new Vector2(minX, minY);
			max_ = new Vector2(maxX, maxY);
		}

		public static readonly Rect Full = new Rect(-1.0f, -1.0f, 1.0f, 1.0f);
		public static readonly Rect Positive = new Rect(0.0f, 0.0f, 1.0f, 1.0f);
		public static readonly Rect Zero = new Rect(0.0f, 0.0f, 0.0f, 0.0f);
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct IntRect
	{
        /// Left coordinate.
        int left_;
        /// Top coordinate.
        int top_;
        /// Right coordinate.
        int right_;
        /// Bottom coordinate.
        int bottom_;

		public IntRect(int left, int top, int right, int bottom)
		{
			left_ = left;
			top_ = top;
			right_ = right;
			bottom_ = bottom;
		}

		public static readonly IntRect Zero = new IntRect(0, 0, 0, 0);
	}
}
