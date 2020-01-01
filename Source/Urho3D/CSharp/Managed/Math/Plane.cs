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
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    /// Surface in three-dimensional space.
	[StructLayout(LayoutKind.Sequential)]
	public struct Plane
    {
        /// Construct from 3 vertices.
        public Plane(in Vector3 v0, in Vector3 v1, in Vector3 v2)
        {
            Vector3 dist1 = v1 - v0;
            Vector3 dist2 = v2 - v0;
            Vector3 normal = dist1.CrossProduct(dist2);
            Normal = normal.Normalized;
            AbsNormal = Normal.Abs;
            D = -Normal.DotProduct(v0);
        }

        /// Construct from a normal vector and a point on the plane.
        public Plane(in Vector3 normal, in Vector3 point)
        {
            Normal = normal.Normalized;
            AbsNormal = Normal.Abs;
            D = -Normal.DotProduct(point);
        }

        /// Construct from a 4-dimensional vector, where the w coordinate is the plane parameter.
        public Plane(in Vector4 plane)
        {
            Normal = new Vector3(plane.X, plane.Y, plane.Z);
            AbsNormal = Normal.Abs;
            D = plane.W;
        }

        /// Define from 3 vertices.
        public void Define(in Vector3 v0, in Vector3 v1, in Vector3 v2)
        {
            Vector3 dist1 = v1 - v0;
            Vector3 dist2 = v2 - v0;

            Define(dist1.CrossProduct(dist2), v0);
        }

        /// Define from a normal vector and a point on the plane.
        public void Define(in Vector3 normal, in Vector3 point)
        {
            Normal = normal.Normalized;
            AbsNormal = Normal.Abs;
            D = -Normal.DotProduct(point);
        }

        /// Define from a 4-dimensional vector, where the w coordinate is the plane parameter.
        public void Define(in Vector4 plane)
        {
            Normal = new Vector3(plane.X, plane.Y, plane.Z);
            AbsNormal = Normal.Abs;
            D = plane.W;
        }

        /// Transform with a 3x3 matrix.
        public void Transform(in Matrix3 transform)
        {
            Define(new Matrix4(transform).Inverted.Transposed * Vector4);
        }
        /// Transform with a 3x4 matrix.
        public void Transform(in Matrix3x4 transform)
        {
            Define(transform.Matrix4.Inverted.Transposed * Vector4);
        }
        /// Transform with a 4x4 matrix.
        public void Transform(in Matrix4 transform)
        {
            Define(transform.Inverted.Transposed * Vector4);
        }

        /// Project a point on the plane.
        public Vector3 Project(in Vector3 point) { return point - Normal * (Normal.DotProduct(point) + D); }

        /// Return signed distance to a point.
        public float Distance(in Vector3 point) { return Normal.DotProduct(point) + D; }

        /// Reflect a normalized direction vector.
        public Vector3 Reflect(in Vector3 direction) { return direction - (2.0f * Normal.DotProduct(direction) * Normal); }

        /// Return a reflection matrix.
        public Matrix3x4 ReflectionMatrix()
        {
            return new Matrix3x4(
                -2.0f * Normal.X * Normal.X + 1.0f,
                -2.0f * Normal.X * Normal.Y,
                -2.0f * Normal.X * Normal.Z,
                -2.0f * Normal.X * D,
                -2.0f * Normal.Y * Normal.X,
                -2.0f * Normal.Y * Normal.Y + 1.0f,
                -2.0f * Normal.Y * Normal.Z,
                -2.0f * Normal.Y * D,
                -2.0f * Normal.Z * Normal.X,
                -2.0f * Normal.Z * Normal.Y,
                -2.0f * Normal.Z * Normal.Z + 1.0f,
                -2.0f * Normal.Z * D
            );
        }
        /// Return transformed by a 3x3 matrix.
        public Plane Transformed(in Matrix3 transform)
        {
            return new Plane(new Matrix4(transform).Inverted.Transposed * Vector4);
        }
        /// Return transformed by a 3x4 matrix.
        public Plane Transformed(in Matrix3x4 transform)
        {
            return new Plane(transform.Matrix4.Inverted.Transposed * Vector4);
        }
        /// Return transformed by a 4x4 matrix.
        public Plane Transformed(in Matrix4 transform)
        {
            return new Plane(transform.Inverted.Transposed * Vector4);
        }

        /// Return as a vector.
        public Vector4 Vector4 => new Vector4(Normal, D);

		/// Plane normal.
		public Vector3 Normal;

		/// Plane absolute normal.
		public Vector3 AbsNormal;

		/// Plane constant.
		public float D;

		/// A horizontal plane at origin point
		public static readonly Plane Up = new Plane(new Vector3(0, 1, 0), new Vector3(0, 0, 0));
    }
}
