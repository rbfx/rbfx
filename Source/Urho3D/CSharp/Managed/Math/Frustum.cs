using System;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    /// Frustum planes.
    public enum FrustumPlane
    {
        Near = 0,
        Left,
        Right,
        Up,
        Down,
        Far
    }

    /// Convex constructed of 6 planes.
    [StructLayout(LayoutKind.Sequential)]
    public struct Frustum
    {
        public const int NumPlanes = 6;
        public const int NumVertices = 8;

        /// Define with projection parameters and a transform matrix.
        public void Define(float fov, float aspectRatio, float zoom, float nearZ, float farZ, in Matrix3x4? transform = null)
        {
            planes_ = new Plane[NumPlanes];
            vertices_ = new Vector3[NumVertices];

            nearZ = Math.Max(nearZ, 0.0f);
            farZ = Math.Max(farZ, nearZ);
            float halfViewSize = (float) (Math.Tan(MathDefs.DegreesToRadians2(fov)) / zoom);
            Vector3 near, far;

            near.Z = nearZ;
            near.Y = near.Z * halfViewSize;
            near.X = near.Y * aspectRatio;
            far.Z = farZ;
            far.Y = far.Z * halfViewSize;
            far.X = far.Y * aspectRatio;

            Define(near, far, transform.GetValueOrDefault(Matrix3x4.Identity));
        }
        /// Define with near and far dimension vectors and a transform matrix.
        public void Define(in Vector3 near, in Vector3 far, in Matrix3x4? transformation = null)
        {
            planes_ = new Plane[NumPlanes];
            vertices_ = new Vector3[NumVertices];

            var transform = transformation.GetValueOrDefault(Matrix3x4.Identity);

            vertices_[0] = transform * near;
            vertices_[1] = transform * new Vector3(near.X, -near.Y, near.Z);
            vertices_[2] = transform * new Vector3(-near.X, -near.Y, near.Z);
            vertices_[3] = transform * new Vector3(-near.X, near.Y, near.Z);
            vertices_[4] = transform * far;
            vertices_[5] = transform * new Vector3(far.X, -far.Y, far.Z);
            vertices_[6] = transform * new Vector3(-far.X, -far.Y, far.Z);
            vertices_[7] = transform * new Vector3(-far.X, far.Y, far.Z);

            UpdatePlanes();
        }
        /// Define with a bounding box and a transform matrix.
        public void Define(in BoundingBox box, in Matrix3x4? transformation = null)
        {
            planes_ = new Plane[NumPlanes];
            vertices_ = new Vector3[NumVertices];

            var transform = transformation.GetValueOrDefault(Matrix3x4.Identity);
            
            vertices_[0] = transform * new Vector3(box.Max.X, box.Max.Y, box.Min.Z);
            vertices_[1] = transform * new Vector3(box.Max.X, box.Min.Y, box.Min.Z);
            vertices_[2] = transform * new Vector3(box.Min.X, box.Min.Y, box.Min.Z);
            vertices_[3] = transform * new Vector3(box.Min.X, box.Max.Y, box.Min.Z);
            vertices_[4] = transform * new Vector3(box.Max.X, box.Max.Y, box.Max.Z);
            vertices_[5] = transform * new Vector3(box.Max.X, box.Min.Y, box.Max.Z);
            vertices_[6] = transform * new Vector3(box.Min.X, box.Min.Y, box.Max.Z);
            vertices_[7] = transform * new Vector3(box.Min.X, box.Max.Y, box.Max.Z);

            UpdatePlanes();
        }
        /// Define from a projection or view-projection matrix.
        public void Define(in Matrix4 projection)
        {
            planes_ = new Plane[NumPlanes];
            vertices_ = new Vector3[NumVertices];

            Matrix4 projInverse = projection.Inverted();

            vertices_[0] = projInverse * new Vector3(1.0f, 1.0f, 0.0f);
            vertices_[1] = projInverse * new Vector3(1.0f, -1.0f, 0.0f);
            vertices_[2] = projInverse * new Vector3(-1.0f, -1.0f, 0.0f);
            vertices_[3] = projInverse * new Vector3(-1.0f, 1.0f, 0.0f);
            vertices_[4] = projInverse * new Vector3(1.0f, 1.0f, 1.0f);
            vertices_[5] = projInverse * new Vector3(1.0f, -1.0f, 1.0f);
            vertices_[6] = projInverse * new Vector3(-1.0f, -1.0f, 1.0f);
            vertices_[7] = projInverse * new Vector3(-1.0f, 1.0f, 1.0f);

            UpdatePlanes();
        }
        /// Define with orthographic projection parameters and a transform matrix.
        public void DefineOrtho(float orthoSize, float aspectRatio, float zoom, float nearZ, float farZ, in Matrix3x4? transformation = null)
        {
            planes_ = new Plane[NumPlanes];
            vertices_ = new Vector3[NumVertices];

            var transform = transformation.GetValueOrDefault(Matrix3x4.Identity);

            nearZ = Math.Max(nearZ, 0.0f);
            farZ = Math.Max(farZ, nearZ);
            float halfViewSize = orthoSize * 0.5f / zoom;
            Vector3 near, far;

            near.Z = nearZ;
            far.Z = farZ;
            far.Y = near.Y = halfViewSize;
            far.X = near.X = near.Y * aspectRatio;

            Define(near, far, transform);
        }
        /// Define a split (limited) frustum from a projection matrix, with near & far distances specified.
        public void DefineSplit(in Matrix4 projection, float near, float far)
        {
            planes_ = new Plane[NumPlanes];
            vertices_ = new Vector3[NumVertices];

            Matrix4 projInverse = projection.Inverted();

            // Figure out depth values for near & far
            Vector4 nearTemp = projection * new Vector4(0.0f, 0.0f, near, 1.0f);
            Vector4 farTemp = projection * new Vector4(0.0f, 0.0f, far, 1.0f);
            float nearZ = nearTemp.Z / nearTemp.W;
            float farZ = farTemp.Z / farTemp.W;

            vertices_[0] = projInverse * new Vector3(1.0f, 1.0f, nearZ);
            vertices_[1] = projInverse * new Vector3(1.0f, -1.0f, nearZ);
            vertices_[2] = projInverse * new Vector3(-1.0f, -1.0f, nearZ);
            vertices_[3] = projInverse * new Vector3(-1.0f, 1.0f, nearZ);
            vertices_[4] = projInverse * new Vector3(1.0f, 1.0f, farZ);
            vertices_[5] = projInverse * new Vector3(1.0f, -1.0f, farZ);
            vertices_[6] = projInverse * new Vector3(-1.0f, -1.0f, farZ);
            vertices_[7] = projInverse * new Vector3(-1.0f, 1.0f, farZ);

            UpdatePlanes();
        }
        /// Transform by a 3x3 matrix.
        public void Transform(in Matrix3 transform)
        {
            for (var i = 0; i < vertices_.Length; i++)
                vertices_[i] = transform * vertices_[i];

            UpdatePlanes();
        }
        /// Transform by a 3x4 matrix.
        public void Transform(in Matrix3x4 transform)
        {
            for (var i = 0; i < vertices_.Length; i++)
                vertices_[i] = transform * vertices_[i];

            UpdatePlanes();
        }

        /// Test if a point is inside or outside.
        public Intersection IsInside(in Vector3 point)
        {
            foreach (var plane in planes_)
            {
                if (plane.Distance(point) < 0.0f)
                    return Intersection.Outside;
            }

            return Intersection.Inside;
        }

        /// Test if a sphere is inside, outside or intersects.
        public Intersection IsInside(in Sphere sphere)
        {
            bool allInside = true;
            foreach (var plane in planes_)
            {
                float dist = plane.Distance(sphere.Center);
                if (dist < -sphere.Radius)
                    return Intersection.Outside;
                else if (dist < sphere.Radius)
                    allInside = false;
            }

            return allInside ? Intersection.Inside : Intersection.Intersects;
        }

        /// Test if a sphere if (partially) inside or outside.
        public Intersection IsInsideFast(in Sphere sphere)
        {
            foreach (var plane in planes_)
            {
                if (plane.Distance(sphere.Center) < -sphere.Radius)
                    return Intersection.Outside;
            }

            return Intersection.Inside;
        }

        /// Test if a bounding box is inside, outside or intersects.
        public Intersection IsInside(in BoundingBox box)
        {
            Vector3 center = box.Center;
            Vector3 edge = center - box.Min;
            bool allInside = true;

            foreach (var plane in planes_)
            {
                float dist = plane.Normal.DotProduct(center) + plane.D;
                float absDist = plane.AbsNormal.DotProduct(edge);

                if (dist < -absDist)
                    return Intersection.Outside;
                else if (dist < absDist)
                    allInside = false;
            }

            return allInside ? Intersection.Inside : Intersection.Intersects;
        }

        /// Test if a bounding box is (partially) inside or outside.
        public Intersection IsInsideFast(in BoundingBox box)
        {
            Vector3 center = box.Center;
            Vector3 edge = center - box.Min;

            foreach (var plane in planes_)
            {
                float dist = plane.Normal.DotProduct(center) + plane.D;
                float absDist = plane.AbsNormal.DotProduct(edge);

                if (dist < -absDist)
                    return Intersection.Outside;
            }

            return Intersection.Inside;
        }

        /// Return distance of a point to the frustum, or 0 if inside.
        public float Distance(in Vector3 point)
        {
            float distance = 0.0f;
            foreach (var plane in planes_)
                distance = Math.Max(-plane.Distance(point), distance);

            return distance;
        }

        /// Return transformed by a 3x3 matrix.
        public Frustum Transformed(in Matrix3 transform)
        {
            Frustum transformed = new Frustum();
            for (var i = 0; i < NumVertices; ++i)
                transformed.vertices_[i] = transform * vertices_[i];

            transformed.UpdatePlanes();
            return transformed;
        }
        /// Return transformed by a 3x4 matrix.
        public Frustum Transformed(in Matrix3x4 transform)
        {
            Frustum transformed = new Frustum();
            for (var i = 0; i < NumVertices; ++i)
                transformed.vertices_[i] = transform * vertices_[i];

            transformed.UpdatePlanes();
            return transformed;
        }
        
        private static Vector3 ClipEdgeZ(in Vector3 v0, in Vector3 v1, float clipZ)
        {
            return new Vector3(
                v1.X + (v0.X - v1.X) * ((clipZ - v1.Z) / (v0.Z - v1.Z)),
                v1.Y + (v0.Y - v1.Y) * ((clipZ - v1.Z) / (v0.Z - v1.Z)),
                clipZ
            );
        }

        private static void ProjectAndMergeEdge(Vector3 v0, Vector3 v1, ref Rect rect, in Matrix4 projection)
        {
            // Check if both vertices behind near plane
            if (v0.Z < MathDefs.MinNearClip && v1.Z < MathDefs.MinNearClip)
                return;

            // Check if need to clip one of the vertices
            if (v1.Z < MathDefs.MinNearClip)
                v1 = ClipEdgeZ(v1, v0, MathDefs.MinNearClip);
            else if (v0.Z < MathDefs.MinNearClip)
                v0 = ClipEdgeZ(v0, v1, MathDefs.MinNearClip);

            // Project, perspective divide and merge
            Vector3 tV0 = projection * v0;
            Vector3 tV1 = projection * v1;
            rect.Merge(new Vector2(tV0.X, tV0.Y));
            rect.Merge(new Vector2(tV1.X, tV1.Y));
        }

        /// Return projected by a 4x4 projection matrix.
        public Rect Projected(in Matrix4 projection)
        {
            Rect rect = new Rect();

            ProjectAndMergeEdge(vertices_[0], vertices_[4], ref rect, projection);
            ProjectAndMergeEdge(vertices_[1], vertices_[5], ref rect, projection);
            ProjectAndMergeEdge(vertices_[2], vertices_[6], ref rect, projection);
            ProjectAndMergeEdge(vertices_[3], vertices_[7], ref rect, projection);
            ProjectAndMergeEdge(vertices_[4], vertices_[5], ref rect, projection);
            ProjectAndMergeEdge(vertices_[5], vertices_[6], ref rect, projection);
            ProjectAndMergeEdge(vertices_[6], vertices_[7], ref rect, projection);
            ProjectAndMergeEdge(vertices_[7], vertices_[4], ref rect, projection);

            return rect;
        }
        /// Update the planes. Called internally.
        public void UpdatePlanes()
        {
            planes_[(int) FrustumPlane.Near].Define(vertices_[2], vertices_[1], vertices_[0]);
            planes_[(int) FrustumPlane.Left].Define(vertices_[3], vertices_[7], vertices_[6]);
            planes_[(int) FrustumPlane.Right].Define(vertices_[1], vertices_[5], vertices_[4]);
            planes_[(int) FrustumPlane.Up].Define(vertices_[0], vertices_[4], vertices_[7]);
            planes_[(int) FrustumPlane.Down].Define(vertices_[6], vertices_[5], vertices_[1]);
            planes_[(int) FrustumPlane.Far].Define(vertices_[5], vertices_[6], vertices_[7]);

            // Check if we ended up with inverted planes (reflected transform) and flip in that case
            if (planes_[(int) FrustumPlane.Near].Distance(vertices_[5]) < 0.0f)
            {
                for (int i = 0; i < planes_.Length; i++)
                {
                    var plane = planes_[i];
                    plane.Normal = -plane.Normal;
                    plane.D = -plane.D;
                    planes_[i] = plane;
                }
            }
        }

        /// Frustum planes.
        [MarshalAs(UnmanagedType.LPArray, SizeConst = NumPlanes)]
        public Plane[] planes_;
        /// Frustum vertices.
        [MarshalAs(UnmanagedType.LPArray, SizeConst = NumVertices)]
        public Vector3[] vertices_;
    }
}
