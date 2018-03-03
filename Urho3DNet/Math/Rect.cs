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
	}
}
