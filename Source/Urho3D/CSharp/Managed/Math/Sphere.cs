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
    /// %Sphere in three-dimensional space.
    [StructLayout(LayoutKind.Sequential)]
    public struct Sphere : IEquatable<Sphere>
    {
        /// Construct from center and radius.
        public Sphere(in Vector3? center=null, float radius=float.NegativeInfinity)
        {
            Center = center.GetValueOrDefault(Vector3.Zero);
            Radius = radius;
        }
    
        /// Construct from an array of vertices.
        public Sphere(Vector3[] vertices)
        {
            Center = Vector3.Zero;
            Radius = 0;
            Define(vertices);
        }

        /// Construct from a bounding box.
        public Sphere(in BoundingBox box)
        {
            Center = Vector3.Zero;
            Radius = 0;
            Define(box);
        }

//        /// Construct from a frustum.
//        public Sphere(in Frustum frustum)
//        {
//            Center = Vector3.Zero;
//            Radius = 0;
//            Define(frustum);
//        }

//        /// Construct from a polyhedron.
//        public Sphere(in Polyhedron poly)
//        {
//            Center = Vector3.Zero;
//            Radius = 0;
//            Define(poly);
//        }

        /// Test for equality with another sphere.
        public static bool operator ==(in Sphere lhs, in Sphere rhs) { return lhs.Equals(rhs); }

        /// Test for inequality with another sphere.
        public static bool operator !=(in Sphere lhs, in Sphere rhs) { return !lhs.Equals(rhs); }

        /// Define from another sphere.
        public void Define(in Sphere sphere)
        {
            Define(sphere.Center, sphere.Radius);
        }

        /// Define from center and radius.
        public void Define(in Vector3 center, float radius)
        {
            Center = center;
            Radius = radius;
        }

        /// Define from an array of vertices.
        public void Define(Vector3[] vertices)
        {
            if (vertices.Length < 1)
                return;

            Clear();
            Merge(vertices);
        }

        /// Define from a bounding box.
        public void Define(in BoundingBox box)
        {
            Vector3 min = box.Min;
            Vector3 max = box.Max;

            Clear();
            Merge(min);
            Merge(new Vector3(max.X, min.Y, min.Z));
            Merge(new Vector3(min.X, max.Y, min.Z));
            Merge(new Vector3(max.X, max.Y, min.Z));
            Merge(new Vector3(min.X, min.Y, max.Z));
            Merge(new Vector3(max.X, min.Y, max.Z));
            Merge(new Vector3(min.X, max.Y, max.Z));
            Merge(max);
        }
        /// Define from a frustum.
//        public void Define(in Frustum frustum)
//        {
//            Define(frustum.Vertices);
//        }
//        /// Define from a polyhedron.
//        public void Define(in Polyhedron poly)
//        {
//            Clear();
//            Merge(poly);
//        }

        /// Merge a point.
        public void Merge(in Vector3 point)
        {
            if (Radius < 0.0f)
            {
                Center = point;
                Radius = 0.0f;
                return;
            }

            Vector3 offset = point - Center;
            float dist = offset.Length;

            if (dist > Radius)
            {
                float half = (dist - Radius) * 0.5f;
                Radius += half;
                Center += (half / dist) * offset;
            }
        }

        /// Merge an array of vertices.
        public void Merge(Vector3[] vertices)
        {
            foreach (var vert in vertices)
                Merge(vert);
        }
        /// Merge a bounding box.
        public void Merge(in BoundingBox box)
        {
            Vector3 min = box.Min;
            Vector3 max = box.Max;

            Merge(min);
            Merge(new Vector3(max.X, min.Y, min.Z));
            Merge(new Vector3(min.X, max.Y, min.Z));
            Merge(new Vector3(max.X, max.Y, min.Z));
            Merge(new Vector3(min.X, min.Y, max.Z));
            Merge(new Vector3(max.X, min.Y, max.Z));
            Merge(new Vector3(min.X, max.Y, max.Z));
            Merge(max);
        }
        /// Merge a frustum.
//        public void Merge(in Frustum frustum)
//        {
//            Merge(frustum.Vertices);
//        }
//        /// Merge a polyhedron.
//        public void Merge(in Polyhedron poly)
//        {
//            for (unsigned i = 0; i < poly.faces_.Size(); ++i)
//            {
//                const ea::vector<Vector3>& face = poly.faces_[i];
//                if (!face.Empty())
//                    Merge(&face[0], face.Size());
//            }
//        }
        /// Merge a sphere.
        public void Merge(in Sphere sphere)
        {
            if (Radius < 0.0f)
            {
                Center = sphere.Center;
                Radius = sphere.Radius;
                return;
            }

            Vector3 offset = sphere.Center - Center;
            float dist = offset.Length;

            // If sphere fits inside, do nothing
            if (dist + sphere.Radius < Radius)
                return;

            // If we fit inside the other sphere, become it
            if (dist + Radius < sphere.Radius)
            {
                Center = sphere.Center;
                Radius = sphere.Radius;
            }
            else
            {
                Vector3 NormalizedOffset = offset / dist;

                Vector3 min = Center - Radius * NormalizedOffset;
                Vector3 max = sphere.Center + sphere.Radius * NormalizedOffset;
                Center = (min + max) * 0.5f;
                Radius = (max - Center).Length;
            }
        }


        /// Clear to undefined state.
        public void Clear()
        {
            Center = Vector3.Zero;
            Radius = float.NegativeInfinity;
        }

        /// Return true if this sphere is defined via a previous call to Define() or Merge().
        public bool Defined()
        {
            return Radius >= 0.0f;
        }

        /// Test if a point is inside.
        public Intersection IsInside(in Vector3 point)
        {
            float distSquared = (point - Center).LengthSquared;
            if (distSquared < Radius * Radius)
                return Intersection.Inside;
            else
                return Intersection.Outside;
        }

        /// Test if another sphere is inside, outside or intersects.
        public Intersection IsInside(in Sphere sphere)
        {
            float dist = (sphere.Center - Center).Length;
            if (dist >= sphere.Radius + Radius)
                return Intersection.Outside;
            else if (dist + sphere.Radius < Radius)
                return Intersection.Inside;
            else
                return Intersection.Intersects;
        }

        /// Test if another sphere is (partially) inside or outside.
        public Intersection IsInsideFast(in Sphere sphere)
        {
            float distSquared = (sphere.Center - Center).LengthSquared;
            float combined = sphere.Radius + Radius;

            if (distSquared >= combined * combined)
                return Intersection.Outside;
            else
                return Intersection.Inside;
        }

        /// Test if a bounding box is inside, outside or intersects.
        public Intersection IsInside(in BoundingBox box)
        {
            float radiusSquared = Radius * Radius;
            float distSquared = 0;
            float temp;
            Vector3 min = box.Min;
            Vector3 max = box.Max;

            if (Center.X < min.X)
            {
                temp = Center.X - min.X;
                distSquared += temp * temp;
            }
            else if (Center.X > max.X)
            {
                temp = Center.X - max.X;
                distSquared += temp * temp;
            }
            if (Center.Y < min.Y)
            {
                temp = Center.Y - min.Y;
                distSquared += temp * temp;
            }
            else if (Center.Y > max.Y)
            {
                temp = Center.Y - max.Y;
                distSquared += temp * temp;
            }
            if (Center.Z < min.Z)
            {
                temp = Center.Z - min.Z;
                distSquared += temp * temp;
            }
            else if (Center.Z > max.Z)
            {
                temp = Center.Z - max.Z;
                distSquared += temp * temp;
            }

            if (distSquared >= radiusSquared)
                return Intersection.Outside;

            min -= Center;
            max -= Center;

            Vector3 tempVec = min; // - - -
            if (tempVec.LengthSquared >= radiusSquared)
                return Intersection.Intersects;
            tempVec.X = max.X; // + - -
            if (tempVec.LengthSquared >= radiusSquared)
                return Intersection.Intersects;
            tempVec.Y = max.Y; // + + -
            if (tempVec.LengthSquared >= radiusSquared)
                return Intersection.Intersects;
            tempVec.X = min.X; // - + -
            if (tempVec.LengthSquared >= radiusSquared)
                return Intersection.Intersects;
            tempVec.Z = max.Z; // - + +
            if (tempVec.LengthSquared >= radiusSquared)
                return Intersection.Intersects;
            tempVec.Y = min.Y; // - - +
            if (tempVec.LengthSquared >= radiusSquared)
                return Intersection.Intersects;
            tempVec.X = max.X; // + - +
            if (tempVec.LengthSquared >= radiusSquared)
                return Intersection.Intersects;
            tempVec.Y = max.Y; // + + +
            if (tempVec.LengthSquared >= radiusSquared)
                return Intersection.Intersects;

            return Intersection.Inside;
        }
        /// Test if a bounding box is (partially) inside or outside.
        public Intersection IsInsideFast(in BoundingBox box)
        {
            float radiusSquared = Radius * Radius;
            float distSquared = 0;
            float temp;
            Vector3 min = box.Min;
            Vector3 max = box.Max;

            if (Center.X < min.X)
            {
                temp = Center.X - min.X;
                distSquared += temp * temp;
            }
            else if (Center.X > max.X)
            {
                temp = Center.X - max.X;
                distSquared += temp * temp;
            }
            if (Center.Y < min.Y)
            {
                temp = Center.Y - min.Y;
                distSquared += temp * temp;
            }
            else if (Center.Y > max.Y)
            {
                temp = Center.Y - max.Y;
                distSquared += temp * temp;
            }
            if (Center.Z < min.Z)
            {
                temp = Center.Z - min.Z;
                distSquared += temp * temp;
            }
            else if (Center.Z > max.Z)
            {
                temp = Center.Z - max.Z;
                distSquared += temp * temp;
            }

            if (distSquared >= radiusSquared)
                return Intersection.Outside;
            else
                return Intersection.Inside;
        }
        /// Return distance of a point to the surface, or 0 if inside.
        public float Distance(in Vector3 point) { return Math.Max((point - Center).Length - Radius, 0.0f); }
        /// Return point on the sphere relative to sphere position.
        public Vector3 GetLocalPoint(float theta, float phi)
        {
            return new Vector3(
                (float) (Radius * Math.Sin(theta) * Math.Sin(phi)),
                (float) (Radius * Math.Cos(phi)),
                (float) (Radius * Math.Cos(theta) * Math.Sin(phi))
            );
        }
        /// Return point on the sphere.
        public Vector3 GetPoint(float theta, float phi) { return Center + GetLocalPoint(theta, phi); }
    
        /// Sphere center.
        public Vector3 Center;
        /// Sphere radius.
        public float Radius;

        public bool Equals(Sphere other)
        {
            return Center.Equals(other.Center) && Radius.Equals(other.Radius);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            return obj is Sphere other && Equals(other);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (Center.GetHashCode() * 397) ^ Radius.GetHashCode();
            }
        }
    }
}
