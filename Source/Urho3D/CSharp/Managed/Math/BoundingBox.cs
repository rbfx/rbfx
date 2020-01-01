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
//    enum Intersection
//    {
//        Outside,
//        Intersects,
//        Inside
//    }

    [StructLayout(LayoutKind.Sequential)]
    public struct BoundingBox
    {
        /// Minimum vector.
        public Vector3 Min;
        private float _dummyMin; // This is never used, but exists to pad the Min value to four floats.
        /// Maximum vector.
        public Vector3 Max;
        private float _dummyMax; // This is never used, but exists to pad the Max value to four floats.

        /// Copy-construct from another bounding box.
        public BoundingBox(in BoundingBox box)
        {
            Min = box.Min;
            Max = box.Max;
            _dummyMin = 0;
            _dummyMax = 0;
        }

        ///ruct from a rect, with the Z dimension left zero.
        public BoundingBox(in Rect rect)
        {
            Min = new Vector3(rect.Min, 0.0f);
            Max = new Vector3(rect.Max, 0.0f);
            _dummyMin = 0;
            _dummyMax = 0;
        }

        ///ruct from minimum and maximum vectors.
        public BoundingBox(in Vector3 min, in Vector3 max)
        {
            Min = min;
            Max = max;
            _dummyMin = 0;
            _dummyMax = 0;
        }

        ///ruct from minimum and maximum floats (all dimensions same.)
        public BoundingBox(float min, float max)
        {
            Min = new Vector3(min, min, min);
            Max = new Vector3(max, max, max);
            _dummyMin = 0;
            _dummyMax = 0;
        }

        /// Construct from an array of vertices.
        public BoundingBox(Vector3[] vertices)
        {
            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
            _dummyMin = 0;
            _dummyMax = 0;
            Define(vertices);
        }

        /// Construct from a frustum.
//        public BoundingBox(in Frustum frustum)
//        {
//            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
//            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
//            _dummyMin = 0;
//            _dummyMax = 0;
//            Define(frustum);
//        }

//        /// Construct from a polyhedron.
//        public BoundingBox(Polyhedron poly)
//        {
//            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
//            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
//            _dummyMin = 0;
//            _dummyMax = 0;
//            Define(poly);
//        }

        /// Construct from a sphere.
        public BoundingBox(in Sphere sphere)
        {
            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
            _dummyMin = 0;
            _dummyMax = 0;
            Define(sphere);
        }

        /// Define from another bounding box.
        public void Define(in BoundingBox box)
        {
            Define(box.Min, box.Max);
        }

        /// Define from a Rect.
        public void Define(in Rect rect)
        {
            Define(new Vector3(rect.Min, 0.0f), new Vector3(rect.Max, 0.0f));
        }

        /// Define from minimum and maximum vectors.
        public void Define(in Vector3 min, in Vector3 max)
        {
            Min = min;
            Max = max;
        }

        /// Define from minimum and maximum floats (all dimensions same.)
        public void Define(float min, float max)
        {
            Min = new Vector3(min, min, min);
            Max = new Vector3(max, max, max);
        }

        /// Define from a point.
        public void Define(in Vector3 point)
        {
            Min = Max = point;
        }

        /// Merge a point.
        public void Merge(in Vector3 point)
        {
            if (point.X < Min.X)
                Min.X = point.X;
            if (point.Y < Min.Y)
                Min.Y = point.Y;
            if (point.Z < Min.Z)
                Min.Z = point.Z;
            if (point.X > Max.X)
                Max.X = point.X;
            if (point.Y > Max.Y)
                Max.Y = point.Y;
            if (point.Z > Max.Z)
                Max.Z = point.Z;
        }

        /// Merge another bounding box.
        public void Merge(in BoundingBox box)
        {
            if (box.Min.X < Min.X)
                Min.X = box.Min.X;
            if (box.Min.Y < Min.Y)
                Min.Y = box.Min.Y;
            if (box.Min.Z < Min.Z)
                Min.Z = box.Min.Z;
            if (box.Max.X > Max.X)
                Max.X = box.Max.X;
            if (box.Max.Y > Max.Y)
                Max.Y = box.Max.Y;
            if (box.Max.Z > Max.Z)
                Max.Z = box.Max.Z;
        }

        /// Define from an array of vertices.
        public void Define(Vector3[] vertices)
        {
            Clear();
            Merge(vertices);
        }

        /// Define from a frustum.
//        public void Define(in Frustum frustum)
//        {
//            Clear();
//            Define(frustum.Vertices);
//        }

//        /// Define from a polyhedron.
//        void Define(Polyhedron poly)
//        {
//            Clear();
//            Merge(poly);
//        }

        /// Define from a sphere.
        public void Define(in Sphere sphere)
        {
            Vector3 center = sphere.Center;
            float radius = sphere.Radius;

            Min = center + new Vector3(-radius, -radius, -radius);
            Max = center + new Vector3(radius, radius, radius);
        }

        /// Merge an array of vertices.
        public void Merge(Vector3[] vertices)
        {
            foreach (var point in vertices)
            {
                Merge(point);
            }
        }
        /// Merge a frustum.
//        public void Merge(Frustum frustum)
//        {
//            Merge(frustum.Vertices);
//        }

//        /// Merge a polyhedron.
//        void Merge(Polyhedron poly)
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
            Vector3 center = sphere.Center;
            float radius = sphere.Radius;

            Merge(center + new Vector3(radius, radius, radius));
            Merge(center + new Vector3(-radius, -radius, -radius));
        }

        /// Clip with another bounding box. The box can become degenerate (undefined) as a result.
        public void Clip(in BoundingBox box)
        {
            if (box.Min.X > Min.X)
                Min.X = box.Min.X;
            if (box.Max.X < Max.X)
                Max.X = box.Max.X;
            if (box.Min.Y > Min.Y)
                Min.Y = box.Min.Y;
            if (box.Max.Y < Max.Y)
                Max.Y = box.Max.Y;
            if (box.Min.Z > Min.Z)
                Min.Z = box.Min.Z;
            if (box.Max.Z < Max.Z)
                Max.Z = box.Max.Z;

            if (Min.X > Max.X || Min.Y > Max.Y || Min.Z > Max.Z)
            {
                Clear();
            }
        }

        /// Transform with a 3x3 matrix.
        public void Transform(in Matrix3 transform)
        {
            this = Transformed(new Matrix3x4(transform));
        }

        /// Transform with a 3x4 matrix.
        public void Transform(in Matrix3x4 transform)
        {
            this = Transformed(transform);
        }

        /// Clear to undefined state.
        public void Clear()
        {
            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
        }

        public bool Defined => float.IsPositiveInfinity(Min.X);

        /// Return center.
        public Vector3 Center => (Max + Min) * 0.5f;

        /// Return size.
        public Vector3 Size => Max - Min;

        /// Return half-size.
        public Vector3 HalfSize => (Max - Min) * 0.5f;

        /// Return transformed by a 3x3 matrix.
        public BoundingBox Transformed(in Matrix3 transform)
        {
            return Transformed(new Matrix3x4(transform));
        }

        /// Return transformed by a 3x4 matrix.
        public BoundingBox Transformed(in Matrix3x4 transform)
        {
            Vector3 newCenter = transform * Center;
            Vector3 oldEdge = Size * 0.5f;
            Vector3 newEdge = new Vector3(
                Math.Abs(transform.M00) * oldEdge.X + Math.Abs(transform.M01) * oldEdge.Y + Math.Abs(transform.M02) * oldEdge.Z,
                Math.Abs(transform.M10) * oldEdge.X + Math.Abs(transform.M11) * oldEdge.Y + Math.Abs(transform.M12) * oldEdge.Z,
                Math.Abs(transform.M20) * oldEdge.X + Math.Abs(transform.M21) * oldEdge.Y + Math.Abs(transform.M22) * oldEdge.Z
            );

            return new BoundingBox(newCenter - newEdge, newCenter + newEdge);
        }

        /// Return projected by a 4x4 projection matrix.
        public Rect Projected(in Matrix4 projection)
        {
            Vector3 projMin = Min;
            Vector3 projMax = Max;
            if (projMin.Z < MathDefs.MinNearClip)
                projMin.Z = MathDefs.MinNearClip;
            if (projMax.Z < MathDefs.MinNearClip)
                projMax.Z = MathDefs.MinNearClip;

            var vertices = new Vector3[8];
            vertices[0] = projMin;
            vertices[1] = new Vector3(projMax.X, projMin.Y, projMin.Z);
            vertices[2] = new Vector3(projMin.X, projMax.Y, projMin.Z);
            vertices[3] = new Vector3(projMax.X, projMax.Y, projMin.Z);
            vertices[4] = new Vector3(projMin.X, projMin.Y, projMax.Z);
            vertices[5] = new Vector3(projMax.X, projMin.Y, projMax.Z);
            vertices[6] = new Vector3(projMin.X, projMax.Y, projMax.Z);
            vertices[7] = projMax;

            Rect rect = new Rect();
            foreach (var vertice in vertices)
            {
                Vector3 projected = projection * vertice;
                rect.Merge(new Vector2(projected.X, projected.Y));
            }

            return rect;
        }

        /// Return distance to point.
        public float DistanceToPoint(in Vector3 point)
        {
            Vector3 offset = Center - point;
            Vector3 absOffset = new Vector3(Math.Abs(offset.X), Math.Abs(offset.Y), Math.Abs(offset.Z));
            return Vector3.Max(Vector3.Zero, absOffset - HalfSize).Length;
        }

        /// Test if a point is inside.
        public Intersection IsInside(in Vector3 point)
        {
            if (point.X < Min.X || point.X > Max.X || point.Y < Min.Y || point.Y > Max.Y ||
                point.Z < Min.Z || point.Z > Max.Z)
                return Intersection.Outside;
            else
                return Intersection.Inside;
        }

        /// Test if another bounding box is inside, outside or intersects.
        public Intersection IsInside(in BoundingBox box)
        {
            if (box.Max.X < Min.X || box.Min.X > Max.X || box.Max.Y < Min.Y || box.Min.Y > Max.Y ||
                box.Max.Z < Min.Z || box.Min.Z > Max.Z)
                return Intersection.Outside;
            else if (box.Min.X < Min.X || box.Max.X > Max.X || box.Min.Y < Min.Y || box.Max.Y > Max.Y ||
                     box.Min.Z < Min.Z || box.Max.Z > Max.Z)
                return Intersection.Intersects;
            else
                return Intersection.Inside;
        }

        /// Test if another bounding box is (partially) inside or outside.
        public Intersection IsInsideFast(in BoundingBox box)
        {
            if (box.Max.X < Min.X || box.Min.X > Max.X || box.Max.Y < Min.Y || box.Min.Y > Max.Y ||
                box.Max.Z < Min.Z || box.Min.Z > Max.Z)
                return Intersection.Outside;
            else
                return Intersection.Inside;
        }

        /// Test if a sphere is inside, outside or intersects.
        public Intersection IsInside(in Sphere sphere)
        {
            float distSquared = 0;
            float temp;
            Vector3 center = sphere.Center;

            if (center.X < Min.X)
            {
                temp = center.X - Min.X;
                distSquared += temp * temp;
            }
            else if (center.X > Max.X)
            {
                temp = center.X - Max.X;
                distSquared += temp * temp;
            }
            if (center.Y < Min.Y)
            {
                temp = center.Y - Min.Y;
                distSquared += temp * temp;
            }
            else if (center.Y > Max.Y)
            {
                temp = center.Y - Max.Y;
                distSquared += temp * temp;
            }
            if (center.Z < Min.Z)
            {
                temp = center.Z - Min.Z;
                distSquared += temp * temp;
            }
            else if (center.Z > Max.Z)
            {
                temp = center.Z - Max.Z;
                distSquared += temp * temp;
            }

            float radius = sphere.Radius;
            if (distSquared >= radius * radius)
                return Intersection.Outside;
            else if (center.X - radius < Min.X || center.X + radius > Max.X || center.Y - radius < Min.Y ||
                     center.Y + radius > Max.Y || center.Z - radius < Min.Z || center.Z + radius > Max.Z)
                return Intersection.Intersects;
            else
                return Intersection.Inside;
        }

        /// Test if a sphere is (partially) inside or outside.
        public Intersection IsInsideFast(in Sphere sphere)
        {
            float distSquared = 0;
            float temp;
            Vector3 center = sphere.Center;

            if (center.X < Min.X)
            {
                temp = center.X - Min.X;
                distSquared += temp * temp;
            }
            else if (center.X > Max.X)
            {
                temp = center.X - Max.X;
                distSquared += temp * temp;
            }
            if (center.Y < Min.Y)
            {
                temp = center.Y - Min.Y;
                distSquared += temp * temp;
            }
            else if (center.Y > Max.Y)
            {
                temp = center.Y - Max.Y;
                distSquared += temp * temp;
            }
            if (center.Z < Min.Z)
            {
                temp = center.Z - Min.Z;
                distSquared += temp * temp;
            }
            else if (center.Z > Max.Z)
            {
                temp = center.Z - Max.Z;
                distSquared += temp * temp;
            }

            float radius = sphere.Radius;
            if (distSquared >= radius * radius)
                return Intersection.Outside;
            else
                return Intersection.Inside;
        }

        /// Return as string.
        public override string ToString()
        {
            return Min.ToString() + " - " + Max.ToString();
        }

    }
}
