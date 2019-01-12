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
            Normal = normal.Normalized();
            AbsNormal = Normal.Abs();
            D = -Normal.DotProduct(v0);
        }

        /// Construct from a normal vector and a point on the plane.
        public Plane(in Vector3 normal, in Vector3 point)
        {
            Normal = normal.Normalized();
            AbsNormal = Normal.Abs();
            D = -Normal.DotProduct(point);
        }

        /// Construct from a 4-dimensional vector, where the w coordinate is the plane parameter.
        public Plane(in Vector4 plane)
        {
            Normal = new Vector3(plane.X, plane.Y, plane.Z);
            AbsNormal = Normal.Abs();
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
            Normal = normal.Normalized();
            AbsNormal = Normal.Abs();
            D = -Normal.DotProduct(point);
        }

        /// Define from a 4-dimensional vector, where the w coordinate is the plane parameter.
        public void Define(in Vector4 plane)
        {
            Normal = new Vector3(plane.X, plane.Y, plane.Z);
            AbsNormal = Normal.Abs();
            D = plane.W;
        }

        /// Transform with a 3x3 matrix.
        public void Transform(in Matrix3 transform)
        {
            Define(new Matrix4(transform).Inverted().Transposed() * ToVector4());
        }
        /// Transform with a 3x4 matrix.
        public void Transform(in Matrix3x4 transform)
        {
            Define(transform.ToMatrix4().Inverted().Transposed() * ToVector4());
        }
        /// Transform with a 4x4 matrix.
        public void Transform(in Matrix4 transform)
        {
            Define(transform.Inverted().Transposed() * ToVector4());
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
            return new Plane(new Matrix4(transform).Inverted().Transposed() * ToVector4());
        }
        /// Return transformed by a 3x4 matrix.
        public Plane Transformed(in Matrix3x4 transform)
        {
            return new Plane(transform.ToMatrix4().Inverted().Transposed() * ToVector4());
        }
        /// Return transformed by a 4x4 matrix.
        public Plane Transformed(in Matrix4 transform)
        {
            return new Plane(transform.Inverted().Transposed() * ToVector4());
        }

        /// Return as a vector.
        public Vector4 ToVector4() { return new Vector4(Normal, D); }

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
