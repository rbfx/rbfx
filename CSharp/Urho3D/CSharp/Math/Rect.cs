using System;
using System.Runtime.InteropServices;

namespace Urho3D
{
	[StructLayout(LayoutKind.Sequential)]
	public struct Rect
	{
        /// Minimum vector.
        public Vector2 Min;
        /// Maximum vector.
        public Vector2 Max;

		public Rect(float minX, float minY, float maxX, float maxY)
		{
			Min = new Vector2(minX, minY);
			Max = new Vector2(maxX, maxY);
		}

		public static readonly Rect Full = new Rect(-1.0f, -1.0f, 1.0f, 1.0f);
		public static readonly Rect Positive = new Rect(0.0f, 0.0f, 1.0f, 1.0f);
		public static readonly Rect Zero = new Rect(0.0f, 0.0f, 0.0f, 0.0f);
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct IntRect
	{
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
	}
}
