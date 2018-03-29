using System;
using System.Runtime.InteropServices;

namespace Urho3D
{
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
        public BoundingBox(BoundingBox box)
        {
            Min = box.Min;
            Max = box.Max;
            _dummyMin = 0;
            _dummyMax = 0;
        }

        ///ruct from a rect, with the Z dimension left zero.
        public BoundingBox(Rect rect)
        {
            Min = new Vector3(rect.Min, 0.0f);
            Max = new Vector3(rect.Max, 0.0f);
            _dummyMin = 0;
            _dummyMax = 0;
        }

        ///ruct from minimum and maximum vectors.
        public BoundingBox(Vector3 min, Vector3 max)
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

        ///ruct from an array of vertices.
        BoundingBox(Vector3[] vertices)
        {
            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
            _dummyMin = 0;
            _dummyMax = 0;
            Define(vertices);
        }

//        ///ruct from a frustum.
//        BoundingBox(Frustum frustum)
//        {
//            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
//            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
//            Define(frustum);
//        }
//
//        ///ruct from a polyhedron.
//        explicit BoundingBox(Polyhedron poly)
//        {
//            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
//            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
//            Define(poly);
//        }
//
//        ///ruct from a sphere.
//        explicit BoundingBox(Sphere sphere)
//        {
//            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
//            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
//            Define(sphere);
//        }

        /// Define from another bounding box.
        void Define(BoundingBox box)
        {
            Define(box.Min, box.Max);
        }

        /// Define from a Rect.
        void Define(Rect rect)
        {
            Define(new Vector3(rect.Min, 0.0f), new Vector3(rect.Max, 0.0f));
        }

        /// Define from minimum and maximum vectors.
        void Define(Vector3 min, Vector3 max)
        {
            Min = min;
            Max = max;
        }

        /// Define from minimum and maximum floats (all dimensions same.)
        void Define(float min, float max)
        {
            Min = new Vector3(min, min, min);
            Max = new Vector3(max, max, max);
        }

        /// Define from a point.
        void Define(Vector3 point)
        {
            Min = Max = point;
        }

        /// Merge a point.
        void Merge(Vector3 point)
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
        void Merge(BoundingBox box)
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
        void Define(Vector3[] vertices)
        {
            Clear();
            Merge(vertices);
        }

//        /// Define from a frustum.
//        void Define(Frustum frustum)
//        {
//            Clear();
//            Define(frustum.vertices_, NUM_FRUSTUM_VERTICES);
//        }
//
//        /// Define from a polyhedron.
//        void Define(Polyhedron poly)
//        {
//            Clear();
//            Merge(poly);
//        }
//
//        /// Define from a sphere.
//        void Define(Sphere sphere)
//        {
//            Vector3 center = sphere.center_;
//            float radius = sphere.radius_;
//
//            Min = center + new Vector3(-radius, -radius, -radius);
//            Max = center + new Vector3(radius, radius, radius);
//        }

        /// Merge an array of vertices.
        void Merge(Vector3[] vertices)
        {
            foreach (var point in vertices)
            {
                Merge(point);
            }
        }
//        /// Merge a frustum.
//        void Merge(Frustum frustum)
//        {
//            Merge(frustum.vertices_, NUM_FRUSTUM_VERTICES);
//        }
//
//        /// Merge a polyhedron.
//        void Merge(Polyhedron poly)
//        {
//            for (unsigned i = 0; i < poly.faces_.Size(); ++i)
//            {
//                const PODVector<Vector3>& face = poly.faces_[i];
//                if (!face.Empty())
//                    Merge(&face[0], face.Size());
//            }
//        }
//
//        /// Merge a sphere.
//        void Merge(Sphere sphere)
//        {
//            Vector3 center = sphere.center_;
//            float radius = sphere.radius_;
//
//            Merge(center + new Vector3(radius, radius, radius));
//            Merge(center + new Vector3(-radius, -radius, -radius));
//        }

        /// Clip with another bounding box. The box can become degenerate (undefined) as a result.
        void Clip(BoundingBox box)
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
        void Transform(Matrix3 transform)
        {
            this = Transformed(new Matrix3x4(transform));
        }

        /// Transform with a 3x4 matrix.
        void Transform(Matrix3x4 transform)
        {
            this = Transformed(transform);
        }

        /// Clear to undefined state.
        void Clear()
        {
            Min = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
            Max = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);
        }

        private bool Defined => float.IsPositiveInfinity(Min.X);

        /// Return center.
        Vector3 Center => (Max + Min) * 0.5f;

        /// Return size.
        Vector3 Size => Max - Min;

        /// Return half-size.
        Vector3 HalfSize => (Max - Min) * 0.5f;

        /// Return transformed by a 3x3 matrix.
        BoundingBox Transformed(Matrix3 transform)
        {
            return Transformed(new Matrix3x4(transform));
        }

        /// Return transformed by a 3x4 matrix.
        BoundingBox Transformed(Matrix3x4 transform)
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
        Rect Projected(Matrix4 projection)
        {
            Vector3 projMin = Min;
            Vector3 projMax = Max;
            if (projMin.Z < MathDefs.MinNearclip)
                projMin.Z = MathDefs.MinNearclip;
            if (projMax.Z < MathDefs.MinNearclip)
                projMax.Z = MathDefs.MinNearclip;

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
        float DistanceToPoint(Vector3 point)
        {
            Vector3 offset = Center - point;
            Vector3 absOffset = new Vector3(Math.Abs(offset.X), Math.Abs(offset.Y), Math.Abs(offset.Z));
            return Vector3.Max(Vector3.Zero, absOffset - HalfSize).Length;
        }

        /// Test if a point is inside.
        Intersection IsInside(Vector3 point)
        {
            if (point.X < Min.X || point.X > Max.X || point.Y < Min.Y || point.Y > Max.Y ||
                point.Z < Min.Z || point.Z > Max.Z)
                return Intersection.OUTSIDE;
            else
                return Intersection.INSIDE;
        }

        /// Test if another bounding box is inside, outside or intersects.
        Intersection IsInside(BoundingBox box)
        {
            if (box.Max.X < Min.X || box.Min.X > Max.X || box.Max.Y < Min.Y || box.Min.Y > Max.Y ||
                box.Max.Z < Min.Z || box.Min.Z > Max.Z)
                return Intersection.OUTSIDE;
            else if (box.Min.X < Min.X || box.Max.X > Max.X || box.Min.Y < Min.Y || box.Max.Y > Max.Y ||
                     box.Min.Z < Min.Z || box.Max.Z > Max.Z)
                return Intersection.INTERSECTS;
            else
                return Intersection.INSIDE;
        }

        /// Test if another bounding box is (partially) inside or outside.
        Intersection IsInsideFast(BoundingBox box)
        {
            if (box.Max.X < Min.X || box.Min.X > Max.X || box.Max.Y < Min.Y || box.Min.Y > Max.Y ||
                box.Max.Z < Min.Z || box.Min.Z > Max.Z)
                return Intersection.OUTSIDE;
            else
                return Intersection.INSIDE;
        }

//        /// Test if a sphere is inside, outside or intersects.
//        Intersection IsInside(Sphere sphere)
//        {
//            float distSquared = 0;
//            float temp;
//            const Vector3& center = sphere.center_;
//
//            if (center.x_ < min_.x_)
//            {
//                temp = center.x_ - min_.x_;
//                distSquared += temp * temp;
//            }
//            else if (center.x_ > max_.x_)
//            {
//                temp = center.x_ - max_.x_;
//                distSquared += temp * temp;
//            }
//            if (center.y_ < min_.y_)
//            {
//                temp = center.y_ - min_.y_;
//                distSquared += temp * temp;
//            }
//            else if (center.y_ > max_.y_)
//            {
//                temp = center.y_ - max_.y_;
//                distSquared += temp * temp;
//            }
//            if (center.z_ < min_.z_)
//            {
//                temp = center.z_ - min_.z_;
//                distSquared += temp * temp;
//            }
//            else if (center.z_ > max_.z_)
//            {
//                temp = center.z_ - max_.z_;
//                distSquared += temp * temp;
//            }
//
//            float radius = sphere.radius_;
//            if (distSquared >= radius * radius)
//                return OUTSIDE;
//            else if (center.x_ - radius < min_.x_ || center.x_ + radius > max_.x_ || center.y_ - radius < min_.y_ ||
//                     center.y_ + radius > max_.y_ || center.z_ - radius < min_.z_ || center.z_ + radius > max_.z_)
//                return INTERSECTS;
//            else
//                return INSIDE;
//        }
//
//        /// Test if a sphere is (partially) inside or outside.
//        Intersection IsInsideFast(const Sphere& sphere) const
//        {
//            float distSquared = 0;
//            float temp;
//            const Vector3& center = sphere.center_;
//
//            if (center.x_ < min_.x_)
//            {
//                temp = center.x_ - min_.x_;
//                distSquared += temp * temp;
//            }
//            else if (center.x_ > max_.x_)
//            {
//                temp = center.x_ - max_.x_;
//                distSquared += temp * temp;
//            }
//            if (center.y_ < min_.y_)
//            {
//                temp = center.y_ - min_.y_;
//                distSquared += temp * temp;
//            }
//            else if (center.y_ > max_.y_)
//            {
//                temp = center.y_ - max_.y_;
//                distSquared += temp * temp;
//            }
//            if (center.z_ < min_.z_)
//            {
//                temp = center.z_ - min_.z_;
//                distSquared += temp * temp;
//            }
//            else if (center.z_ > max_.z_)
//            {
//                temp = center.z_ - max_.z_;
//                distSquared += temp * temp;
//            }
//
//            float radius = sphere.radius_;
//            if (distSquared >= radius * radius)
//                return OUTSIDE;
//            else
//                return INSIDE;
//        }

        /// Return as string.
        public override string ToString()
        {
            return Min.ToString() + " - " + Max.ToString();
        }

    }
}
