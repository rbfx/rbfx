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
    /// Infinite straight line in three-dimensional space.
    [StructLayout(LayoutKind.Sequential)]
    public struct Ray : IEquatable<Ray>
    {
        /// Construct from origin and direction. The direction will be normalized.
        public Ray(in Vector3 origin, in Vector3 direction)
        {
            Origin = origin;
            Direction = direction.Normalized;
        }

        /// Check for equality with another ray.
        public static bool operator ==(in Ray lhs, in Ray rhs) { return lhs.Equals(rhs); }

        /// Check for inequality with another ray.
        public static bool operator !=(in Ray lhs, in Ray rhs) { return !lhs.Equals(rhs); }

        /// Define from origin and direction. The direction will be normalized.
        public void Define(in Vector3 origin, in Vector3 direction)
        {
            Origin = origin;
            Direction = direction.Normalized;
        }

        /// Project a point on the ray.
        Vector3 Project(in Vector3 point)
        {
            Vector3 offset = point - Origin;
            return Origin + offset.DotProduct(Direction) * Direction;
        }

        /// Return distance of a point from the ray.
        float Distance(in Vector3 point)
        {
            Vector3 projected = Project(point);
            return (point - projected).Length;
        }

        /// Return closest point to another ray.
        Vector3 ClosestPoint(in Ray ray)
        {
            // Algorithm based on http://paulbourke.net/geometry/lineline3d/
            Vector3 p13 = Origin - ray.Origin;
            Vector3 p43 = ray.Direction;
            Vector3 p21 = Direction;

            float d1343 = p13.DotProduct(p43);
            float d4321 = p43.DotProduct(p21);
            float d1321 = p13.DotProduct(p21);
            float d4343 = p43.DotProduct(p43);
            float d2121 = p21.DotProduct(p21);

            float d = d2121 * d4343 - d4321 * d4321;
            if (Math.Abs(d) < MathDefs.Epsilon)
                return Origin;
            float n = d1343 * d4321 - d1321 * d4343;
            float a = n / d;

            return Origin + a * Direction;
        }
        /// Return hit distance to a plane, or infinity if no hit.
        float HitDistance(in Plane plane)
        {
            float d = plane.Normal.DotProduct(Direction);
            if (Math.Abs(d) >= MathDefs.Epsilon)
            {
                float t = -(plane.Normal.DotProduct(Origin) + plane.D) / d;
                if (t >= 0.0f)
                    return t;
                else
                    return float.PositiveInfinity;
            }
            else
                return float.PositiveInfinity;
        }
        /// Return hit distance to a bounding box, or infinity if no hit.
        float HitDistance(in BoundingBox box)
        {
            // If undefined, no hit (infinite distance)
            if (!box.Defined)
                return float.PositiveInfinity;

            // Check for ray origin being inside the box
            if (box.IsInside(Origin) != Intersection.Outside)
                return 0.0f;

            float dist = float.PositiveInfinity;

            // Check for intersecting in the X-direction
            if (Origin.X < box.Min.X && Direction.X > 0.0f)
            {
                float x = (box.Min.X - Origin.X) / Direction.X;
                if (x < dist)
                {
                    Vector3 point = Origin + x * Direction;
                    if (point.Y >= box.Min.Y && point.Y <= box.Max.Y && point.Z >= box.Min.Z && point.Z <= box.Max.Z)
                        dist = x;
                }
            }
            if (Origin.X > box.Max.X && Direction.X < 0.0f)
            {
                float x = (box.Max.X - Origin.X) / Direction.X;
                if (x < dist)
                {
                    Vector3 point = Origin + x * Direction;
                    if (point.Y >= box.Min.Y && point.Y <= box.Max.Y && point.Z >= box.Min.Z && point.Z <= box.Max.Z)
                        dist = x;
                }
            }
            // Check for intersecting in the Y-direction
            if (Origin.Y < box.Min.Y && Direction.Y > 0.0f)
            {
                float x = (box.Min.Y - Origin.Y) / Direction.Y;
                if (x < dist)
                {
                    Vector3 point = Origin + x * Direction;
                    if (point.X >= box.Min.X && point.X <= box.Max.X && point.Z >= box.Min.Z && point.Z <= box.Max.Z)
                        dist = x;
                }
            }
            if (Origin.Y > box.Max.Y && Direction.Y < 0.0f)
            {
                float x = (box.Max.Y - Origin.Y) / Direction.Y;
                if (x < dist)
                {
                    Vector3 point = Origin + x * Direction;
                    if (point.X >= box.Min.X && point.X <= box.Max.X && point.Z >= box.Min.Z && point.Z <= box.Max.Z)
                        dist = x;
                }
            }
            // Check for intersecting in the Z-direction
            if (Origin.Z < box.Min.Z && Direction.Z > 0.0f)
            {
                float x = (box.Min.Z - Origin.Z) / Direction.Z;
                if (x < dist)
                {
                    Vector3 point = Origin + x * Direction;
                    if (point.X >= box.Min.X && point.X <= box.Max.X && point.Y >= box.Min.Y && point.Y <= box.Max.Y)
                        dist = x;
                }
            }
            if (Origin.Z > box.Max.Z && Direction.Z < 0.0f)
            {
                float x = (box.Max.Z - Origin.Z) / Direction.Z;
                if (x < dist)
                {
                    Vector3 point = Origin + x * Direction;
                    if (point.X >= box.Min.X && point.X <= box.Max.X && point.Y >= box.Min.Y && point.Y <= box.Max.Y)
                        dist = x;
                }
            }

            return dist;
        }
        /// Return hit distance to a frustum, or infinity if no hit. If solidInside parameter is true (default) rays originating from inside return zero distance, otherwise the distance to the closest plane.
//        float HitDistance(in Frustum frustum, bool solidInside = true)
//        {
//            float maxOutside = 0.0f;
//            float minInside = float.PositiveInfinity;
//            bool allInside = true;
//
//            foreach (var plane in frustum.Planes)
//            {
//                float distance = HitDistance(plane);
//
//                if (plane.Distance(Origin) < 0.0f)
//                {
//                    maxOutside = Math.Max(maxOutside, distance);
//                    allInside = false;
//                }
//                else
//                    minInside = Math.Min(minInside, distance);
//            }
//
//            if (allInside)
//                return solidInside ? 0.0f : minInside;
//            else if (maxOutside <= minInside)
//                return maxOutside;
//            else
//                return float.PositiveInfinity;
//        }
        /// Return hit distance to a sphere, or infinity if no hit.
        float HitDistance(in Sphere sphere)
        {
            Vector3 centeredOrigin = Origin - sphere.Center;
            float squaredRadius = sphere.Radius * sphere.Radius;

            // Check if ray originates inside the sphere
            if (centeredOrigin.LengthSquared <= squaredRadius)
                return 0.0f;

            // Calculate intersection by quadratic equation
            float a = Direction.DotProduct(Direction);
            float b = 2.0f * centeredOrigin.DotProduct(Direction);
            float c = centeredOrigin.DotProduct(centeredOrigin) - squaredRadius;
            float d = b * b - 4.0f * a * c;

            // No solution
            if (d < 0.0f)
                return float.PositiveInfinity;

            // Get the nearer solution
            float dSqrt = (float) Math.Sqrt(d);
            float dist = (-b - dSqrt) / (2.0f * a);
            if (dist >= 0.0f)
                return dist;
            else
                return (-b + dSqrt) / (2.0f * a);
        }
        /// Return hit distance to a triangle, or infinity if no hit. Optionally return hit normal and hit barycentric coordinate at intersect point.
        float HitDistance(in Vector3 v0, in Vector3 v1, in Vector3 v2, out Vector3 outNormal, out Vector3 outBary)
        {
            // Based on Fast, Minimum Storage Ray/Triangle Intersection by Möller & Trumbore
            // http://www.graphics.cornell.edu/pubs/1997/MT97.pdf
            // Calculate edge vectors
            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;

            // Calculate determinant & check backfacing
            Vector3 p = Direction.CrossProduct(edge2);
            float det = edge1.DotProduct(p);
            if (det >= MathDefs.Epsilon)
            {
                // Calculate u & v parameters and test
                Vector3 t = Origin - v0;
                float u = t.DotProduct(p);
                if (u >= 0.0f && u <= det)
                {
                    Vector3 q = t.CrossProduct(edge1);
                    float v = Direction.DotProduct(q);
                    if (v >= 0.0f && u + v <= det)
                    {
                        float distance = edge2.DotProduct(q) / det;
                        // Discard hits behind the ray
                        if (distance >= 0.0f)
                        {
                            // There is an intersection, so calculate distance & optional normal
                            outNormal = edge1.CrossProduct(edge2);
                            outBary = new Vector3(1 - (u / det) - (v / det), u / det, v / det);

                            return distance;
                        }
                    }
                }
            }

            outNormal = Vector3.Zero;
            outBary = Vector3.Zero;

            return float.PositiveInfinity;
        }
//        /// Return hit distance to non-indexed geometry data, or infinity if no hit. Optionally return hit normal and hit uv coordinates at intersect point.
//        float HitDistance(const void* vertexData, unsigned vertexStride, unsigned vertexStart, unsigned vertexCount,
//            Vector3* outNormal = nullptr, Vector2* outUV = nullptr, unsigned uvOffset = 0)
//        {
//            float nearest = float.PositiveInfinity;
//            const unsigned char* vertices = ((const unsigned char*)vertexData) + vertexStart * vertexStride;
//            unsigned index = 0, nearestIdx = M_MAX_UNSIGNED;
//            Vector3 barycentric;
//            Vector3* outBary = outUV ? &barycentric : nullptr;
//
//            while (index + 2 < vertexCount)
//            {
//                const Vector3& v0 = *((in Vector3*)(vertices[index * vertexStride]));
//                const Vector3& v1 = *((in Vector3*)(vertices[(index + 1) * vertexStride]));
//                const Vector3& v2 = *((in Vector3*)(vertices[(index + 2) * vertexStride]));
//                float distance = HitDistance(v0, v1, v2, outNormal, outBary);
//                if (distance < nearest)
//                {
//                    nearestIdx = index;
//                    nearest = distance;
//                }
//                index += 3;
//            }
//
//            if (outUV)
//            {
//                if (nearestIdx == M_MAX_UNSIGNED)
//                    *outUV = Vector2::ZERO;
//                else
//                {
//                    // Interpolate the UV coordinate using barycentric coordinate
//                    const Vector2& uv0 = *((in Vector2*)(vertices[uvOffset + nearestIdx * vertexStride]));
//                    const Vector2& uv1 = *((in Vector2*)(vertices[uvOffset + (nearestIdx + 1) * vertexStride]));
//                    const Vector2& uv2 = *((in Vector2*)(vertices[uvOffset + (nearestIdx + 2) * vertexStride]));
//                    *outUV = Vector2(uv0.X * barycentric.X + uv1.X * barycentric.Y + uv2.X * barycentric.Z,
//                        uv0.Y * barycentric.X + uv1.Y * barycentric.Y + uv2.Y * barycentric.Z);
//                }
//            }
//
//            return nearest;
//        }
//        /// Return hit distance to indexed geometry data, or infinity if no hit. Optionally return hit normal and hit uv coordinates at intersect point.
//        float HitDistance(const void* vertexData, unsigned vertexStride, const void* indexData, unsigned indexSize, unsigned indexStart,
//            unsigned indexCount, Vector3* outNormal = nullptr, Vector2* outUV = nullptr, unsigned uvOffset = 0)
//        {
//            float nearest = float.PositiveInfinity;
//            const auto* vertices = (const unsigned char*)vertexData;
//            Vector3 barycentric;
//            Vector3* outBary = outUV ? &barycentric : nullptr;
//
//            // 16-bit indices
//            if (indexSize == sizeof(unsigned short))
//            {
//                const unsigned short* indices = ((const unsigned short*)indexData) + indexStart;
//                const unsigned short* indicesEnd = indices + indexCount;
//                const unsigned short* nearestIndices = nullptr;
//
//                while (indices < indicesEnd)
//                {
//                    const Vector3& v0 = *((in Vector3*)(vertices[indices[0] * vertexStride]));
//                    const Vector3& v1 = *((in Vector3*)(vertices[indices[1] * vertexStride]));
//                    const Vector3& v2 = *((in Vector3*)(vertices[indices[2] * vertexStride]));
//                    float distance = HitDistance(v0, v1, v2, outNormal, outBary);
//                    if (distance < nearest)
//                    {
//                        nearestIndices = indices;
//                        nearest = distance;
//                    }
//                    indices += 3;
//                }
//
//                if (outUV)
//                {
//                    if (nearestIndices == nullptr)
//                        *outUV = Vector2::ZERO;
//                    else
//                    {
//                        // Interpolate the UV coordinate using barycentric coordinate
//                        const Vector2& uv0 = *((in Vector2*)(vertices[uvOffset + nearestIndices[0] * vertexStride]));
//                        const Vector2& uv1 = *((in Vector2*)(vertices[uvOffset + nearestIndices[1] * vertexStride]));
//                        const Vector2& uv2 = *((in Vector2*)(vertices[uvOffset + nearestIndices[2] * vertexStride]));
//                        *outUV = Vector2(uv0.X * barycentric.X + uv1.X * barycentric.Y + uv2.X * barycentric.Z,
//                            uv0.Y * barycentric.X + uv1.Y * barycentric.Y + uv2.Y * barycentric.Z);
//                    }
//                }
//            }
//            // 32-bit indices
//            else
//            {
//                const unsigned* indices = ((const unsigned*)indexData) + indexStart;
//                const unsigned* indicesEnd = indices + indexCount;
//                const unsigned* nearestIndices = nullptr;
//
//                while (indices < indicesEnd)
//                {
//                    const Vector3& v0 = *((in Vector3*)(vertices[indices[0] * vertexStride]));
//                    const Vector3& v1 = *((in Vector3*)(vertices[indices[1] * vertexStride]));
//                    const Vector3& v2 = *((in Vector3*)(vertices[indices[2] * vertexStride]));
//                    float distance = HitDistance(v0, v1, v2, outNormal, outBary);
//                    if (distance < nearest)
//                    {
//                        nearestIndices = indices;
//                        nearest = distance;
//                    }
//                    indices += 3;
//                }
//
//                if (outUV)
//                {
//                    if (nearestIndices == nullptr)
//                        *outUV = Vector2::ZERO;
//                    else
//                    {
//                        // Interpolate the UV coordinate using barycentric coordinate
//                        const Vector2& uv0 = *((in Vector2*)(vertices[uvOffset + nearestIndices[0] * vertexStride]));
//                        const Vector2& uv1 = *((in Vector2*)(vertices[uvOffset + nearestIndices[1] * vertexStride]));
//                        const Vector2& uv2 = *((in Vector2*)(vertices[uvOffset + nearestIndices[2] * vertexStride]));
//                        *outUV = Vector2(uv0.X * barycentric.X + uv1.X * barycentric.Y + uv2.X * barycentric.Z,
//                            uv0.Y * barycentric.X + uv1.Y * barycentric.Y + uv2.Y * barycentric.Z);
//                    }
//                }
//            }
//
//            return nearest;
//        }
//        /// Return whether ray is inside non-indexed geometry.
//        bool InsideGeometry(const void* vertexData, unsigned vertexSize, unsigned vertexStart, unsigned vertexCount)
//        {
//            float currentFrontFace = float.PositiveInfinity;
//            float currentBackFace = float.PositiveInfinity;
//            const unsigned char* vertices = ((const unsigned char*)vertexData) + vertexStart * vertexSize;
//            unsigned index = 0;
//
//            while (index + 2 < vertexCount)
//            {
//                const Vector3& v0 = *((in Vector3*)(vertices[index * vertexSize]));
//                const Vector3& v1 = *((in Vector3*)(vertices[(index + 1) * vertexSize]));
//                const Vector3& v2 = *((in Vector3*)(vertices[(index + 2) * vertexSize]));
//                float frontFaceDistance = HitDistance(v0, v1, v2);
//                float backFaceDistance = HitDistance(v2, v1, v0);
//                currentFrontFace = Min(frontFaceDistance > 0.0f ? frontFaceDistance : float.PositiveInfinity, currentFrontFace);
//                // A backwards face is just a regular one, with the vertices in the opposite order. This essentially checks backfaces by
//                // checking reversed frontfaces
//                currentBackFace = Min(backFaceDistance > 0.0f ? backFaceDistance : float.PositiveInfinity, currentBackFace);
//                index += 3;
//            }
//
//            // If the closest face is a backface, that means that the ray originates from the inside of the geometry
//            // NOTE: there may be cases where both are equal, as in, no collision to either. This is prevented in the most likely case
//            // (ray doesn't hit either) by this conditional
//            if (currentFrontFace != float.PositiveInfinity || currentBackFace != float.PositiveInfinity)
//                return currentBackFace < currentFrontFace;
//
//            // It is still possible for two triangles to be equally distant from the triangle, however, this is extremely unlikely.
//            // As such, it is safe to assume they are not
//            return false;
//        }
//        /// Return whether ray is inside indexed geometry.
//        bool InsideGeometry(const void* vertexData, unsigned vertexSize, const void* indexData, unsigned indexSize, unsigned indexStart,
//            unsigned indexCount)
//        {
//            float currentFrontFace = float.PositiveInfinity;
//            float currentBackFace = float.PositiveInfinity;
//            const auto* vertices = (const unsigned char*)vertexData;
//
//            // 16-bit indices
//            if (indexSize == sizeof(unsigned short))
//            {
//                const unsigned short* indices = ((const unsigned short*)indexData) + indexStart;
//                const unsigned short* indicesEnd = indices + indexCount;
//
//                while (indices < indicesEnd)
//                {
//                    const Vector3& v0 = *((in Vector3*)(vertices[indices[0] * vertexSize]));
//                    const Vector3& v1 = *((in Vector3*)(vertices[indices[1] * vertexSize]));
//                    const Vector3& v2 = *((in Vector3*)(vertices[indices[2] * vertexSize]));
//                    float frontFaceDistance = HitDistance(v0, v1, v2);
//                    float backFaceDistance = HitDistance(v2, v1, v0);
//                    currentFrontFace = Min(frontFaceDistance > 0.0f ? frontFaceDistance : float.PositiveInfinity, currentFrontFace);
//                    // A backwards face is just a regular one, with the vertices in the opposite order. This essentially checks backfaces by
//                    // checking reversed frontfaces
//                    currentBackFace = Min(backFaceDistance > 0.0f ? backFaceDistance : float.PositiveInfinity, currentBackFace);
//                    indices += 3;
//                }
//            }
//            // 32-bit indices
//            else
//            {
//                const unsigned* indices = ((const unsigned*)indexData) + indexStart;
//                const unsigned* indicesEnd = indices + indexCount;
//
//                while (indices < indicesEnd)
//                {
//                    const Vector3& v0 = *((in Vector3*)(vertices[indices[0] * vertexSize]));
//                    const Vector3& v1 = *((in Vector3*)(vertices[indices[1] * vertexSize]));
//                    const Vector3& v2 = *((in Vector3*)(vertices[indices[2] * vertexSize]));
//                    float frontFaceDistance = HitDistance(v0, v1, v2);
//                    float backFaceDistance = HitDistance(v2, v1, v0);
//                    currentFrontFace = Min(frontFaceDistance > 0.0f ? frontFaceDistance : float.PositiveInfinity, currentFrontFace);
//                    // A backwards face is just a regular one, with the vertices in the opposite order. This essentially checks backfaces by
//                    // checking reversed frontfaces
//                    currentBackFace = Min(backFaceDistance > 0.0f ? backFaceDistance : float.PositiveInfinity, currentBackFace);
//                    indices += 3;
//                }
//            }
//
//            // If the closest face is a backface, that means that the ray originates from the inside of the geometry
//            // NOTE: there may be cases where both are equal, as in, no collision to either. This is prevented in the most likely case
//            // (ray doesn't hit either) by this conditional
//            if (currentFrontFace != float.PositiveInfinity || currentBackFace != float.PositiveInfinity)
//                return currentBackFace < currentFrontFace;
//
//            // It is still possible for two triangles to be equally distant from the triangle, however, this is extremely unlikely.
//            // As such, it is safe to assume they are not
//            return false;
//        }
        /// Return transformed by a 3x4 matrix. This may result in a non-normalized direction.
        Ray Transformed(in Matrix3x4 transform)
        {
            Ray ret;
            ret.Origin = transform * Origin;
            ret.Direction = transform * new Vector4(Direction, 0.0f);
            return ret;
        }

        /// Ray origin.
        public Vector3 Origin;
        /// Ray direction.
        public Vector3 Direction;

        public bool Equals(Ray other)
        {
            return Origin.Equals(other.Origin) && Direction.Equals(other.Direction);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            return obj is Ray other && Equals(other);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (Origin.GetHashCode() * 397) ^ Direction.GetHashCode();
            }
        }
    }
}
