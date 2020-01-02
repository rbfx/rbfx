//
// Copyright (c) 2008-2019 the Urho3D project.
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

/// \file

#pragma once

#include "../Math/BoundingBox.h"
#include "../Math/Matrix3x4.h"
#include "../Math/Sphere.h"
#include "../Math/Vector3.h"

#include <EASTL/span.h>

#include <cmath>

namespace Urho3D
{

/// Surface triangle of tetrahedral mesh with adjacency information.
struct TetrahedralMeshSurfaceTriangle
{
    /// Indices of triangle vertices.
    unsigned indices_[3]{};
    /// Index of the 4th vertex of underlying tetrahedron. Unspecified if there's no underlying tetrahedron.
    unsigned unusedIndex_{ M_MAX_UNSIGNED };
    /// Indices of neighbor triangles.
    unsigned neighbors_[3]{ M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED};
    /// Index of underlying tetrahedron. M_MAX_UNSIGNED if empty.
    unsigned tetIndex_{ M_MAX_UNSIGNED };
    /// Face of underlying tetrahedron, from 0 to 3.
    unsigned tetFace_{};

    /// Return edge, from 0 to 2. Returned indices are sorted.
    ea::pair<unsigned, unsigned> GetEdge(unsigned edgeIndex) const
    {
        unsigned begin = indices_[edgeIndex];
        unsigned end = indices_[(edgeIndex + 1) % 3];
        if (begin > end)
            ea::swap(begin, end);
        return { begin, end };
    }

    /// Return whether the triangle has given neighbour.
    bool HasNeighbor(unsigned neighborIndex) const
    {
        return ea::count(ea::begin(neighbors_), ea::end(neighbors_), neighborIndex) != 0;
    }

    /// Normalize triangle indices so (p2 - p1) x (p3 - p1) is the normal.
    void Normalize(const ea::vector<Vector3>& vertices)
    {
        const Vector3 p0 = vertices[unusedIndex_];
        const Vector3 p1 = vertices[indices_[0]];
        const Vector3 p2 = vertices[indices_[1]];
        const Vector3 p3 = vertices[indices_[2]];
        const Vector3 outsideDirection = p1 - p0;
        const Vector3 actualNormal = (p2 - p1).CrossProduct(p3 - p1);
        if (outsideDirection.DotProduct(actualNormal) < 0.0f)
        {
            ea::swap(indices_[0], indices_[1]);
            ea::swap(neighbors_[0], neighbors_[1]);
        }
    }
};

/// Surface of tetrahedral mesh. Vertices are shared with tetrahedral mesh and are not stored.
struct TetrahedralMeshSurface
{
    /// Faces.
    ea::vector<TetrahedralMeshSurfaceTriangle> faces_;

    /// Clear.
    void Clear() { faces_.clear(); }

    /// Return size.
    unsigned Size() const { return faces_.size(); }

    /// Add face and update adjacency information.
    void AddFace(TetrahedralMeshSurfaceTriangle newFace)
    {
        // Find adjacent triangles
        const unsigned newFaceIndex = faces_.size();
        for (unsigned oldFaceIndex = 0; oldFaceIndex < newFaceIndex; ++oldFaceIndex)
        {
            TetrahedralMeshSurfaceTriangle& oldFace = faces_[oldFaceIndex];
            for (unsigned oldEdgeIndex = 0; oldEdgeIndex < 3; ++oldEdgeIndex)
            {
                const auto oldEdge = oldFace.GetEdge(oldEdgeIndex);
                for (unsigned newEdgeIndex = 0; newEdgeIndex < 3; ++newEdgeIndex)
                {
                    const auto newEdge = newFace.GetEdge(newEdgeIndex);
                    if (oldEdge == newEdge)
                    {
                        // +0 and +1 vertices belong to the edge, therefore the neighbor is stored at +2
                        oldFace.neighbors_[(oldEdgeIndex + 2) % 3] = newFaceIndex;
                        newFace.neighbors_[(newEdgeIndex + 2) % 3] = oldFaceIndex;
                    }
                }
            }
        }

        faces_.push_back(newFace);
    }

    /// Return whether the mesh is a closed surface.
    bool IsClosedSurface() const
    {
        for (unsigned faceIndex = 0; faceIndex < faces_.size(); ++faceIndex)
        {
            for (unsigned i = 0; i < 3; ++i)
            {
                const unsigned neighborFaceIndex = faces_[faceIndex].neighbors_[i];
                if (neighborFaceIndex == M_MAX_UNSIGNED)
                    return false;

                assert(faces_[neighborFaceIndex].HasNeighbor(faceIndex));
            }
        }
        return true;
    }
};

/// Tetrahedron with adjacency information.
struct Tetrahedron
{
    /// Special index for infinite vertex of outer tetrahedron, cubic equation.
    static const unsigned Infinity3 = M_MAX_UNSIGNED;
    /// Special index for infinite vertex of outer tetrahedron, square equation.
    static const unsigned Infinity2 = M_MAX_UNSIGNED - 1;

    /// Indices of tetrahedron vertices.
    unsigned indices_[4]{};
    /// Indices of neighbor tetrahedrons. M_MAX_UNSIGNED if empty.
    unsigned neighbors_[4]{ M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED };
    /// Pre-computed matrix for calculating barycentric coordinates.
    Matrix3x4 matrix_;

    /// Return indices of specified triangle face of the tetrahedron.
    void GetTriangleFaceIndices(unsigned faceIndex, unsigned (&indices)[3]) const
    {
        unsigned j = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            if (i != faceIndex)
                indices[j++] = indices_[i];
        }
    }

    /// Return triangle face of the tetrahedron. Adjacency information is left uninitialized.
    TetrahedralMeshSurfaceTriangle GetTriangleFace(unsigned faceIndex, unsigned tetIndex, unsigned tetFace) const
    {
        TetrahedralMeshSurfaceTriangle face;
        GetTriangleFaceIndices(faceIndex, face.indices_);
        face.unusedIndex_ = indices_[faceIndex];
        face.tetIndex_ = tetIndex;
        face.tetFace_ = tetFace;
        return face;
    }

    /// Return face index corresponding to given neighbor. Return 4 if not found.
    unsigned GetNeighborFaceIndex(unsigned neighborTetIndex) const
    {
        return ea::find(ea::begin(neighbors_), ea::end(neighbors_), neighborTetIndex) - ea::begin(neighbors_);
    }

    /// Return whether the tetrahedron has given neighbour.
    bool HasNeighbor(unsigned neighborTetIndex) const
    {
        return GetNeighborFaceIndex(neighborTetIndex) < 4;
    }
};

/// Tetrahedral mesh.
class URHO3D_API TetrahedralMesh
{
public:
    /// Define mesh from vertices.
    void Define(ea::span<const Vector3> positions, float padding = 1.0f);

    /// Calculate circumsphere of given tetrahedron.
    Sphere GetTetrahedronCircumsphere(unsigned tetIndex) const;

    /// Calculate barycentric coordinates for inner tetrahedron.
    Vector4 GetInnerBarycentricCoords(unsigned tetIndex, const Vector3& position) const
    {
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        const Vector3& basePosition = vertices_[tetrahedron.indices_[0]];
        const Vector3 coords = tetrahedron.matrix_ * (position - basePosition);
        return { 1.0f - coords.x_ - coords.y_ - coords.z_, coords.x_, coords.y_, coords.z_ };
    }

    /// Calculate barycentric coordinates for outer tetrahedron.
    Vector4 GetOuterBarycentricCoords(unsigned tetIndex, const Vector3& position) const
    {
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        const Vector3 p1 = vertices_[tetrahedron.indices_[0]];
        const Vector3 p2 = vertices_[tetrahedron.indices_[1]];
        const Vector3 p3 = vertices_[tetrahedron.indices_[2]];
        const Vector3 normal = (p2 - p1).CrossProduct(p3 - p1);

        // Position is in the inner cell, return fake barycentric
        if (normal.DotProduct(position - p1) < 0.0f)
            return { 0.0f, 0.0f, 0.0f, -1.0f };

        const Vector3 poly = tetrahedron.matrix_ * position;
        const float t = tetrahedron.indices_[3] == Tetrahedron::Infinity3 ? SolveCubic(poly) : SolveQuadratic(poly);

        const Vector3 t1 = p1 + t * hullNormals_[tetrahedron.indices_[0]];
        const Vector3 t2 = p2 + t * hullNormals_[tetrahedron.indices_[1]];
        const Vector3 t3 = p3 + t * hullNormals_[tetrahedron.indices_[2]];
        const Vector3 coords = GetTriangleBarycentricCoords(position, t1, t2, t3);
        return { coords, 0.0f };
    }

    /// Calculate barycentric coordinates for tetrahedron.
    Vector4 GetBarycentricCoords(unsigned tetIndex, const Vector3& position) const
    {
        return tetIndex < numInnerTetrahedrons_
            ? GetInnerBarycentricCoords(tetIndex, position)
            : GetOuterBarycentricCoords(tetIndex, position);
    }

private:
    /// Solve cubic equation x^3 + a*x^2 + b*x + c = 0.
    static int SolveCubicEquation(double *x, double a, double b, double c, double eps)
    {
        const double TwoPi = M_PI * 2;
        double a2 = a * a;
        double q = (a2 - 3 * b) / 9;
        double r = (a * (2 * a2 - 9 * b) + 27 * c) / 54;
        double r2 = r * r;
        double q3 = q * q * q;
        double A, B;
        if (r2 <= (q3 + eps))
        {
            double t = r / sqrt(q3);
            if (t < -1)
                t = -1;
            if (t > 1)
                t = 1;
            t = acos(t);
            a /= 3;
            q = -2 * sqrt(q);
            x[0] = q * cos(t / 3) - a;
            x[1] = q * cos((t + TwoPi) / 3) - a;
            x[2] = q * cos((t - TwoPi) / 3) - a;
            return (3);
        }
        else
        {
            A = -std::cbrt(fabs(r) + sqrt(r2 - q3));
            if (r < 0)
                A = -A;
            B = (A == 0 ? 0 : B = q / A);

            a /= 3;
            x[0] = (A + B) - a;
            x[1] = -0.5 * (A + B) - a;
            x[2] = 0.5 * sqrt(3.) * (A - B);
            if (fabs(x[2]) < eps)
            {
                x[2] = x[1];
                return (2);
            }
            return (1);
        }
    }

    /// Calculate most positive root of cubic equation x^3 + a*x^2 + b*x + c = 0.
    static float SolveCubic(const Vector3& abc)
    {
        const double a = abc.x_;
        const double b = abc.y_;
        const double c = abc.z_;

        double ret[3];
        const int num = SolveCubicEquation(ret, a, b, c, M_EPSILON);
        if (num == 3)
            ret[0] = ret[0];
        return float(*ea::max_element(ret, ret + num));
    }
    /// Calculate most positive root of quadratic equation a*x^2 + b*x + c = 0.
    static float SolveQuadratic(const Vector3& abc)
    {
        const float a = abc.x_;
        const float b = abc.y_;
        const float c = abc.z_;
        if (std::abs(a) < M_EPSILON)
            return -c / b;

        assert(b * b - 4 * a * c > -M_EPSILON);
        const float D = ea::max(0.0f, b * b - 4 * a * c);

        return a > 0
            ? (-b + std::sqrt(D)) / (2 * a)
            : (-b - std::sqrt(D)) / (2 * a);
    }
    /// Calculate barycentric coordinates on triangle.
    static Vector3 GetTriangleBarycentricCoords(const Vector3& position,
        const Vector3& p1, const Vector3& p2, const Vector3& p3)
    {
        const Vector3 v12 = p2 - p1;
        const Vector3 v13 = p3 - p1;
        const Vector3 v0 = position - p1;
        const float d00 = v12.DotProduct(v12);
        const float d01 = v12.DotProduct(v13);
        const float d11 = v13.DotProduct(v13);
        const float d20 = v0.DotProduct(v12);
        const float d21 = v0.DotProduct(v13);
        const float denom = d00 * d11 - d01 * d01;
        const float v = (d11 * d20 - d01 * d21) / denom;
        const float w = (d00 * d21 - d01 * d20) / denom;
        const float u = 1.0f - v - w;
        return { u, v, w };
    }

    /// Number of initial super-mesh vertices.
    static const unsigned NumSuperMeshVertices = 8;
    /// Create super-mesh for Delaunay triangulation.
    void InitializeSuperMesh(const BoundingBox& boundingBox);
    /// Build tetrahedrons for given positions.
    void BuildTetrahedrons(ea::span<const Vector3> positions);
    /// Return whether the adjacency is valid.
    bool IsAdjacencyValid(bool fullyConnected) const;

    /// Data used for Delaunay triangulation.
    struct DelaunayContext
    {
        /// Circumspheres of mesh tetrahedrons.
        ea::vector<Sphere> circumspheres_;
        /// Whether the tetrahedron is removed.
        ea::vector<bool> removed_;

        /// Queue for breadth search of bad tetrahedrons. Used by FindAndRemoveIntersected only.
        ea::vector<unsigned> searchQueue_;
    };

    /// Find and remove (aka set removed flag) tetrahedrons whose circumspheres intersect given point. Returns hole surface.
    void FindAndRemoveIntersected(DelaunayContext& ctx, const Vector3& position,
        TetrahedralMeshSurface& holeSurface, ea::vector<unsigned>& removedTetrahedrons) const;
    /// Fill star-shaped hole with tetrahedrons connected to specified vertex.
    /// Output tetrahedrons should be allocated beforehand.
    void FillStarShapedHole(DelaunayContext& ctx, const ea::vector<unsigned>& outputTetrahedrons,
        const TetrahedralMeshSurface& holeSurface, unsigned centerIndex);
    /// Mark super-mesh tetrahedrons in the to-be-removed array and disconnect all related adjacency.
    void DisconnectSuperMeshTetrahedrons(ea::vector<bool>& removed);
    /// Remove marked tetrahedrons from array.
    void RemoveMarkedTetrahedrons(const ea::vector<bool>& removed);
    /// Remove super-mesh vertices.
    void RemoveSuperMeshVertices();
    /// Build hull surface.
    void BuildHullSurface();
    /// Calculate hull normals.
    void CalculateHullNormals();
    /// Build outer tetrahedrons.
    void BuildOuterTetrahedrons();
    /// Calculate matrices for inner tetrahedrons.
    void CalculateInnerMatrices();
    /// Calculate matrices for outer tetrahedrons.
    void CalculateOuterMatrices();

public:
    /// Vertices.
    ea::vector<Vector3> vertices_;
    /// Tetrahedrons.
    ea::vector<Tetrahedron> tetrahedrons_;
    /// Hull surface.
    TetrahedralMeshSurface hullSurface_;

    /// Hull normals.
    ea::vector<Vector3> hullNormals_;
    /// Number of inner tetrahedrons.
    unsigned numInnerTetrahedrons_{};
};

}
