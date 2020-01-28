//
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

#include "../Precompiled.h"

#include "../IO/ArchiveSerialization.h"
#include "../IO/Log.h"
#include "../Math/Plane.h"
#include "../Math/TetrahedralMesh.h"

#include <EASTL/numeric.h>
#include <EASTL/sort.h>

#include <cmath>

namespace Urho3D
{

namespace
{

/// Edge of tetrahedral mesh.
struct TetrahedralMeshEdge
{
    /// Indices. Always sorted.
    unsigned indices_[2]{};
    /// Degree of silver-ness. The more the worse.
    float silverScore_{};
    /// Tetrahedron.
    unsigned tetIndex_{ M_MAX_UNSIGNED };
    /// How many times the edge is present in the tetrahedron.
    unsigned cardinality_{ 1 };

    /// Construct default.
    TetrahedralMeshEdge() = default;

    /// Construct valid.
    TetrahedralMeshEdge(unsigned i0, unsigned i1, unsigned tetIndex, float score)
        : indices_{ i0, i1 }
        , tetIndex_(tetIndex)
        , silverScore_(score)
    {
        if (indices_[0] > indices_[1])
            ea::swap(indices_[0], indices_[1]);
    }

    /// Compare for sorting.
    bool operator < (const TetrahedralMeshEdge& rhs) const
    {
        if (indices_[0] != rhs.indices_[0])
            return indices_[0] < rhs.indices_[0];
        if (indices_[1] != rhs.indices_[1])
            return indices_[1] < rhs.indices_[1];
        return silverScore_ < rhs.silverScore_;
    }

    /// Compare edges.
    bool IsSameEdge(const TetrahedralMeshEdge& rhs) const
    {
        return indices_[0] == rhs.indices_[0] && indices_[1] == rhs.indices_[1];
    }
};

}

bool TetrahedralMeshSurface::CalculateAdjacency()
{
    // Collect all the edges
    edges_.clear();
    for (unsigned faceIndex = 0; faceIndex < faces_.size(); ++faceIndex)
    {
        const TetrahedralMeshSurfaceTriangle& face = faces_[faceIndex];
        for (unsigned edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
        {
            const unsigned i0 = face.indices_[(edgeIndex + 1) % 3];
            const unsigned i1 = face.indices_[(edgeIndex + 2) % 3];
            edges_.emplace_back(i0, i1, faceIndex, edgeIndex);
        }
    }

    // Match all the edges
    ea::sort(edges_.begin(), edges_.end());
    if (edges_.size() % 2 != 0)
        return false;

    for (unsigned pairIndex = 0; pairIndex < edges_.size() / 2; ++pairIndex)
    {
        const TetrahedralMeshSurfaceEdge& firstEdge = edges_[pairIndex * 2 + 0];
        const TetrahedralMeshSurfaceEdge& secondEdge = edges_[pairIndex * 2 + 1];

        // Edges should be equivalent, i.e. same edge
        if (firstEdge < secondEdge || secondEdge < firstEdge)
            return false;

        TetrahedralMeshSurfaceTriangle& firstFace = faces_[firstEdge.faceIndex_];
        TetrahedralMeshSurfaceTriangle& secondFace = faces_[secondEdge.faceIndex_];

        // Link should not be initialized before
        if (firstFace.neighbors_[firstEdge.edgeIndex_] != M_MAX_UNSIGNED
            || secondFace.neighbors_[secondEdge.edgeIndex_] != M_MAX_UNSIGNED)
        {
            return false;
        }

        firstFace.neighbors_[firstEdge.edgeIndex_] = secondEdge.faceIndex_;
        secondFace.neighbors_[secondEdge.edgeIndex_] = firstEdge.faceIndex_;
    }

    return true;
}

bool TetrahedralMeshSurface::IsClosedSurface() const
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

void TetrahedralMesh::Define(ea::span<const Vector3> positions)
{
    BoundingBox boundingBox(positions.data(), positions.size());
    const Vector3 size = boundingBox.Size();
    const float maxSize = ea::max({ 1.0f, size.x_, size.y_, size.z_ });
    boundingBox.min_ -= Vector3::ONE;
    boundingBox.max_ += Vector3::ONE;
    InitializeSuperMesh(boundingBox);
    BuildTetrahedrons(positions);
}

void TetrahedralMesh::CollectEdges(ea::vector<ea::pair<unsigned, unsigned>>& edges)
{
    edges.clear();

    // Collect everything
    for (unsigned tetIndex = 0; tetIndex < numInnerTetrahedrons_; ++tetIndex)
    {
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned i = 0; i < 4; ++i)
        {
            for (unsigned j = 0; j < 4; ++j)
            {
                const unsigned startIndex = tetrahedron.indices_[i];
                const unsigned endIndex = tetrahedron.indices_[j];
                edges.emplace_back(startIndex, endIndex);
            }
        }
    }

    // Sort edges and remove duplicates
    for (auto& edge : edges)
    {
        if (edge.first > edge.second)
            ea::swap(edge.first, edge.second);
    }

    ea::sort(edges.begin(), edges.end());
    edges.erase(ea::unique(edges.begin(), edges.end()), edges.end());
}

HighPrecisionSphere TetrahedralMesh::GetTetrahedronCircumsphere(unsigned tetIndex) const
{
    const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
    const HighPrecisionVector3 p0{ vertices_[tetrahedron.indices_[0]] };
    const HighPrecisionVector3 p1{ vertices_[tetrahedron.indices_[1]] };
    const HighPrecisionVector3 p2{ vertices_[tetrahedron.indices_[2]] };
    const HighPrecisionVector3 p3{ vertices_[tetrahedron.indices_[3]] };
    const HighPrecisionVector3 u1 = p1 - p0;
    const HighPrecisionVector3 u2 = p2 - p0;
    const HighPrecisionVector3 u3 = p3 - p0;

    const double d01 = u1.LengthSquared();
    const double d02 = u2.LengthSquared();
    const double d03 = u3.LengthSquared();

    const HighPrecisionVector3 u2u3 = u2.CrossProduct(u3);
    const HighPrecisionVector3 u3u1 = u3.CrossProduct(u1);
    const HighPrecisionVector3 u1u2 = u1.CrossProduct(u2);

    const HighPrecisionVector3 radiusNum = u2u3 * d01 + u3u1 * d02 + u1u2 * d03;
    const double radiusDen = 2 * u1.DotProduct(u2u3);

    if (Abs(radiusDen) < M_EPSILON * M_EPSILON)
    {
        URHO3D_LOGWARNING("Degenerate tetrahedron in tetrahedral mesh due to error in tetrahedral mesh generation");
        return HighPrecisionSphere{ {}, M_LARGE_VALUE * M_LARGE_VALUE };
    }

    const HighPrecisionVector3 center = p0 + radiusNum * (1.0 / radiusDen);

    // Radius is the minimum distance
    const double squaredDistances[4] = {
        (p0 - center).LengthSquared(),
        (p1 - center).LengthSquared(),
        (p2 - center).LengthSquared(),
        (p3 - center).LengthSquared()
    };

    const double radiusSquared = *ea::min_element(ea::begin(squaredDistances), ea::end(squaredDistances));
    return { center, Sqrt(radiusSquared) };
}

Vector4 TetrahedralMesh::GetInnerBarycentricCoords(unsigned tetIndex, const Vector3& position) const
{
    const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
    const Vector3& basePosition = vertices_[tetrahedron.indices_[0]];
    const Vector3 coords = tetrahedron.matrix_ * (position - basePosition);
    return { 1.0f - coords.x_ - coords.y_ - coords.z_, coords.x_, coords.y_, coords.z_ };
}

Vector4 TetrahedralMesh::GetOuterBarycentricCoords(unsigned tetIndex, const Vector3& position) const
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

Vector4 TetrahedralMesh::GetBarycentricCoords(unsigned tetIndex, const Vector3& position) const
{
    return tetIndex < numInnerTetrahedrons_
        ? GetInnerBarycentricCoords(tetIndex, position)
        : GetOuterBarycentricCoords(tetIndex, position);
}

Vector4 TetrahedralMesh::GetInterpolationFactors(const Vector3& position, unsigned& tetIndexHint) const
{
    if (tetrahedrons_.empty())
        return Vector4::ZERO;

    const unsigned maxIters = tetrahedrons_.size();
    if (tetIndexHint >= maxIters)
        tetIndexHint = 0;

    for (unsigned i = 0; i < maxIters; ++i)
    {
        const Vector4 weights = GetBarycentricCoords(tetIndexHint, position);
        if (weights.x_ >= 0.0f && weights.y_ >= 0.0f && weights.z_ >= 0.0f && weights.w_ >= 0.0f)
            return weights;

        if (weights.x_ < weights.y_ && weights.x_ < weights.z_ && weights.x_ < weights.w_)
            tetIndexHint = tetrahedrons_[tetIndexHint].neighbors_[0];
        else if (weights.y_ < weights.z_ && weights.y_ < weights.w_)
            tetIndexHint = tetrahedrons_[tetIndexHint].neighbors_[1];
        else if (weights.z_ < weights.w_)
            tetIndexHint = tetrahedrons_[tetIndexHint].neighbors_[2];
        else
            tetIndexHint = tetrahedrons_[tetIndexHint].neighbors_[3];
    }
    return GetBarycentricCoords(tetIndexHint, position);
}

int TetrahedralMesh::SolveCubicEquation(double result[], double a, double b, double c, double eps)
{
    // Performance-critical code, don't use degree-based functions here
    double a2 = a * a;
    double q = (a2 - 3 * b) / 9;
    double r = (a * (2 * a2 - 9 * b) + 27 * c) / 54;
    double r2 = r * r;
    double q3 = q * q * q;
    if (r2 <= (q3 + eps))
    {
        double t = r / std::sqrt(q3);
        if (t < -1)
            t = -1;
        if (t > 1)
            t = 1;
        t = std::acos(t);
        a /= 3;
        q = -2 * std::sqrt(q);
        result[0] = q * std::cos(t / 3) - a;
        result[1] = q * std::cos((t + M_PI * 2) / 3) - a;
        result[2] = q * std::cos((t - M_PI * 2) / 3) - a;
        return 3;
    }
    else
    {
        double A = -std::cbrt(std::abs(r) + std::sqrt(r2 - q3));
        if (r < 0)
            A = -A;
        const double B = A == 0 ? 0 : q / A;

        a /= 3;
        result[0] = (A + B) - a;
        result[1] = -0.5 * (A + B) - a;
        result[2] = 0.5 * std::sqrt(3.0) * (A - B);
        if (std::abs(result[2]) < eps)
        {
            result[2] = result[1];
            return 2;
        }
        return 1;
    }
}

float TetrahedralMesh::SolveCubic(const Vector3& abc)
{
    double results[3];
    const int numRoots = SolveCubicEquation(results, abc.x_, abc.y_, abc.z_, M_EPSILON);
    return static_cast<float>(GetSmallestPositiveRoot(results, numRoots));
}

float TetrahedralMesh::SolveQuadratic(const Vector3& abc)
{
    const double a = abc.x_;
    const double b = abc.y_;
    const double c = abc.z_;
    if (std::abs(a) < M_EPSILON)
        return -c / b;

    const double D = ea::max(0.0, b * b - 4 * a * c);
    const double invA = 1 / (2 * a);
    const double roots[2] = {
        (-b + std::sqrt(D)) * invA,
        (-b - std::sqrt(D)) * invA
    };

    return GetSmallestPositiveRoot(roots, 2);
}

double TetrahedralMesh::GetSmallestPositiveRoot(const double roots[], int count)
{
    double root = M_LARGE_VALUE;
    for (int i = 0; i < count; ++i)
    {
        if (roots[i] > -M_LARGE_EPSILON && roots[i] < root)
            root = roots[i];
    }
    return ea::max(0.0, root);
}

Vector3 TetrahedralMesh::GetTriangleBarycentricCoords(const Vector3& position,
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

unsigned TetrahedralMesh::FindTetrahedron(const Vector3& position, ea::vector<bool>& removed) const
{
    auto firstNotRemovedIter = ea::find(removed.begin(), removed.end(), false);
    if (firstNotRemovedIter == removed.end())
        return M_MAX_UNSIGNED;

    const unsigned maxIters = tetrahedrons_.size();
    unsigned tetIndex = firstNotRemovedIter - removed.begin();
    for (unsigned i = 0; i < maxIters; ++i)
    {
        // Found one
        const Vector4 weights = GetInnerBarycentricCoords(tetIndex, position);
        if (weights.x_ >= 0.0f && weights.y_ >= 0.0f && weights.z_ >= 0.0f && weights.w_ >= 0.0f)
            break;

        if (weights.x_ < weights.y_ && weights.x_ < weights.z_ && weights.x_ < weights.w_)
            tetIndex = tetrahedrons_[tetIndex].neighbors_[0];
        else if (weights.y_ < weights.z_ && weights.y_ < weights.w_)
            tetIndex = tetrahedrons_[tetIndex].neighbors_[1];
        else if (weights.z_ < weights.w_)
            tetIndex = tetrahedrons_[tetIndex].neighbors_[2];
        else
            tetIndex = tetrahedrons_[tetIndex].neighbors_[3];

        // Failed to find one
        if (tetIndex == M_MAX_UNSIGNED)
            break;
    }
    return tetIndex;
}

void TetrahedralMesh::InitializeSuperMesh(const BoundingBox& boundingBox)
{
    static const Vector3 offsets[NumSuperMeshVertices] =
    {
        { 0.0f, 0.0f, 0.0f }, // 0: 1st corner tetrahedron
        { 1.0f, 0.0f, 0.0f }, // 1:
        { 0.0f, 1.0f, 0.0f }, // 2:
        { 1.0f, 1.0f, 0.0f }, // 3: 2nd corner tetrahedron
        { 0.0f, 0.0f, 1.0f }, // 4:
        { 1.0f, 0.0f, 1.0f }, // 5: 3rh corner tetrahedron
        { 0.0f, 1.0f, 1.0f }, // 6: 4th corner tetrahedron
        { 1.0f, 1.0f, 1.0f }  // 7:
    };

    static const unsigned numTetrahedrons = 5;
    static const unsigned indices[numTetrahedrons][4] =
    {
        { 0, 1, 2, 4 }, // 1st corner tetrahedron
        { 3, 1, 2, 7 }, // 2nd corner tetrahedron
        { 5, 1, 4, 7 }, // 3rd corner tetrahedron
        { 6, 2, 4, 7 }, // 4th corner tetrahedron
        { 1, 2, 4, 7 }  // Central tetrahedron
    };

    static const unsigned neighbors[numTetrahedrons][4] =
    {
        { 4, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED },
        { 4, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED },
        { 4, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED },
        { 4, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED },
        { 3, 2, 1, 0 }, // Tetrahedrons with corners at (6, 5, 3, 0)
    };

    vertices_.resize(NumSuperMeshVertices);
    for (unsigned i = 0; i < NumSuperMeshVertices; ++i)
        vertices_[i] = boundingBox.min_ + boundingBox.Size() * offsets[i];

    tetrahedrons_.resize(numTetrahedrons);
    for (unsigned i = 0; i < numTetrahedrons; ++i)
    {
        Tetrahedron tetrahedron;
        for (unsigned j = 0; j < 4; ++j)
        {
            tetrahedron.indices_[j] = indices[i][j];
            tetrahedron.neighbors_[j] = neighbors[i][j];
        }
        tetrahedron.CalculateInnerMatrix(vertices_);
        tetrahedrons_[i] = tetrahedron;
    }
}

void TetrahedralMesh::BuildTetrahedrons(ea::span<const Vector3> positions)
{
    // Initialize context
    DelaunayContext ctx;
    ctx.circumspheres_.resize(tetrahedrons_.size());
    ctx.removed_.resize(tetrahedrons_.size());
    for (unsigned i = 0; i < tetrahedrons_.size(); ++i)
        ctx.circumspheres_[i] = GetTetrahedronCircumsphere(i);

    // Append vertices and initialize queue
    const unsigned startVertex = vertices_.size();
    vertices_.insert(vertices_.end(), positions.begin(), positions.end());

    ea::vector<unsigned> verticesQueue;
    for (unsigned newVertexIndex = startVertex; newVertexIndex < vertices_.size(); ++newVertexIndex)
        verticesQueue.push_back(newVertexIndex);

    // Triangulate
    TetrahedralMeshSurface holeSurface;
    ea::vector<unsigned> removedTetrahedrons;
    ea::vector<unsigned> postponedVertices;
    while (!verticesQueue.empty())
    {
        // Process current bunch of vertices
        postponedVertices.clear();
        for (unsigned newVertexIndex : verticesQueue)
        {
            // Carve hole in the mesh
            if (!FindAndRemoveIntersected(ctx, vertices_[newVertexIndex], holeSurface, removedTetrahedrons))
            {
                postponedVertices.push_back(newVertexIndex);
                continue;
            }

            // Disconnect carved out tetrahedrons
            DisconnectRemovedTetrahedrons(removedTetrahedrons);

            // Allocate space for new tetrahedrons
            while (removedTetrahedrons.size() < holeSurface.Size())
            {
                removedTetrahedrons.push_back(tetrahedrons_.size());
                tetrahedrons_.push_back();
                ctx.circumspheres_.push_back();
                ctx.removed_.push_back(true);
            }

            // Fill hole with tetrahedrons
            FillStarShapedHole(ctx, removedTetrahedrons, holeSurface, newVertexIndex);
        }

        // Re-enqueue postponed vertices
        ea::swap(postponedVertices, verticesQueue);

        // If all the vertices are postponed again, ignore them
        if (postponedVertices.size() == verticesQueue.size())
        {
            URHO3D_LOGWARNING("{} vertices are excluded from triangulation due to mathematical fluctuations",
                postponedVertices.size());
            break;
        }
    }

    // Dump failed attempts for debugging
    debugHighlightEdges_.clear();
    for (unsigned ignoredVertex : verticesQueue)
        FindAndRemoveIntersected(ctx, vertices_[ignoredVertex], holeSurface, removedTetrahedrons, true);

    // Finalize triangulation
    DisconnectSuperMeshTetrahedrons(ctx.removed_);
    FilterMeshSurface(ctx.removed_);
    EnsureMeshConnectivity(ctx.removed_);
    RemoveMarkedTetrahedrons(ctx.removed_);
    RemoveSuperMeshVertices();
    UpdateIgnoredVertices();

    numInnerTetrahedrons_ = tetrahedrons_.size();
    assert(IsAdjacencyValid(false));

    if (ignoredVertices_.size() > verticesQueue.size())
    {
        URHO3D_LOGWARNING("Triangulation is incomplete because vertices are too sparse, {} vertices are ignored",
            verticesQueue.size());
    }

    // Build the outer space and precompute matrices
    TetrahedralMeshSurface hullSurface;
    BuildHullSurface(hullSurface);

    CalculateHullNormals(hullSurface);
    BuildOuterTetrahedrons(hullSurface);
    CalculateOuterMatrices();
}

bool TetrahedralMesh::IsAdjacencyValid(bool fullyConnected) const
{
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborIndex = tetrahedrons_[tetIndex].neighbors_[faceIndex];
            if (neighborIndex != M_MAX_UNSIGNED)
            {
                const Tetrahedron& neighborTetrahedron = tetrahedrons_[neighborIndex];
                if (!neighborTetrahedron.HasNeighbor(tetIndex))
                    return false;
            }
            else if (fullyConnected)
                return false;
        }
    }
    return true;
}

void TetrahedralMesh::DisconnectTetrahedron(unsigned tetIndex)
{
    Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
    for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
    {
        const unsigned neighborIndex = tetrahedron.neighbors_[faceIndex];
        if (neighborIndex != M_MAX_UNSIGNED)
        {
            Tetrahedron& neighborTetrahedron = tetrahedrons_[neighborIndex];
            const unsigned neighborFaceIndex = neighborTetrahedron.GetNeighborFaceIndex(tetIndex);
            assert(neighborFaceIndex < 4);
            neighborTetrahedron.neighbors_[neighborFaceIndex] = M_MAX_UNSIGNED;
        }
    }
}

bool TetrahedralMesh::FindAndRemoveIntersected(TetrahedralMesh::DelaunayContext& ctx, const Vector3& position,
    TetrahedralMeshSurface& holeSurface, ea::vector<unsigned>& removedTetrahedrons, bool dumpErrors) const
{
    // Reset output
    holeSurface.Clear();
    removedTetrahedrons.clear();

    // Find first tetrahedron to remove
    const unsigned firstTetIndex = FindTetrahedron(position, ctx.removed_);
    if (firstTetIndex == M_MAX_UNSIGNED || !ctx.IsInsideCircumsphere(firstTetIndex, position))
    {
        URHO3D_LOGERROR("Cannot find tetrahedron to insert vertex at {}", position.ToString());
        assert(0);
        return false;
    }

    removedTetrahedrons.push_back(firstTetIndex);
    ctx.removed_[firstTetIndex] = true;

    // Do breadth search to collect all bad tetrahedrons
    // Note: size may change during the loop
    for (unsigned i = 0; i < removedTetrahedrons.size(); ++i)
    {
        const unsigned tetIndex = removedTetrahedrons[i];
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];

        // Process neighbors
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborTetIndex = tetrahedron.neighbors_[faceIndex];

            // Outer surface is reached
            if (neighborTetIndex == M_MAX_UNSIGNED)
                continue;

            // Ignore already removed tetrahedrons
            if (ctx.removed_[neighborTetIndex])
                continue;

            // If circumsphere of neighbor tetrahedron contains the point, remove this neighbor and queue it.
            if (ctx.IsInsideCircumsphere(neighborTetIndex, position))
            {
                removedTetrahedrons.push_back(neighborTetIndex);
                ctx.removed_[neighborTetIndex] = true;
            }
        }
    }

    // Collect triangles of the hole surface
    for (unsigned tetIndex : removedTetrahedrons)
    {
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborTetIndex = tetrahedron.neighbors_[faceIndex];

            // Outer surface is reached
            if (neighborTetIndex == M_MAX_UNSIGNED)
            {
                // Face of outer surface doesn't have underlying tetrahedron
                const TetrahedralMeshSurfaceTriangle holeTriangle = tetrahedron.GetTriangleFace(
                    faceIndex, M_MAX_UNSIGNED, M_MAX_UNSIGNED);
                holeSurface.faces_.push_back(holeTriangle);
                continue;
            }

            // If neighbor is not removed, add face
            if (!ctx.removed_[neighborTetIndex])
            {
                const Tetrahedron& neighborTetrahedron = tetrahedrons_[neighborTetIndex];
                const unsigned neighborFaceIndex = neighborTetrahedron.GetNeighborFaceIndex(tetIndex);

                const TetrahedralMeshSurfaceTriangle holeTriangle = neighborTetrahedron.GetTriangleFace(
                    neighborFaceIndex, neighborTetIndex, neighborFaceIndex);
                holeSurface.faces_.push_back(holeTriangle);
            }
        }
    }

    // Verify that all hole triangles are faced in right direction
    bool valid = true;
    const HighPrecisionVector3 p0{ position };
    for (TetrahedralMeshSurfaceTriangle& triangle : holeSurface.faces_)
    {
        // Outer triangles are always oriented right
        if (triangle.tetIndex_ == M_MAX_UNSIGNED)
            continue;

        // Normalize triangle orientation
        triangle.Normalize(vertices_);

        const HighPrecisionVector3 p1{ vertices_[triangle.indices_[0]] };
        const HighPrecisionVector3 p2{ vertices_[triangle.indices_[1]] };
        const HighPrecisionVector3 p3{ vertices_[triangle.indices_[2]] };
        const HighPrecisionVector3 normal = (p2 - p1).CrossProduct(p3 - p1);
        const double distance = (p0 - p1).DotProduct(normal);

        // If coplanar or worse, cannot add new vertex
        if (distance < M_LARGE_EPSILON)
        {
            valid = false;
            break;
        }
    }

    // Try to initialize adjacency for surface
    if (valid)
    {
        if (!holeSurface.CalculateAdjacency())
            valid = false;
    }

    // Revert all changes if invalid or if dump error mode is on
    if (!valid || dumpErrors)
    {
        if (dumpErrors)
        {
            assert(!valid);
            for (const TetrahedralMeshSurfaceTriangle& triangle : holeSurface.faces_)
            {
                const unsigned i0 = triangle.indices_[0];
                const unsigned i1 = triangle.indices_[1];
                const unsigned i2 = triangle.indices_[2];
                debugHighlightEdges_.emplace_back(i0, i1);
                debugHighlightEdges_.emplace_back(i1, i2);
                debugHighlightEdges_.emplace_back(i2, i0);
            }
        }

        for (unsigned tetIndex : removedTetrahedrons)
            ctx.removed_[tetIndex] = false;

        removedTetrahedrons.clear();
        holeSurface.Clear();
        return false;
    }

    if (!holeSurface.IsClosedSurface())
    {
        URHO3D_LOGERROR("Surface of the carved hole in tetrahedral mesh is incomplete for vertex at {}",
            position.ToString());
        assert(0);
        holeSurface.Clear();
        return false;
    }

    return true;
}

void TetrahedralMesh::DisconnectRemovedTetrahedrons(const ea::vector<unsigned>& removedTetrahedrons)
{
    // Disconnect removed tetrahedrons
    for (unsigned tetIndex : removedTetrahedrons)
    {
        Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborTetIndex = tetrahedron.neighbors_[faceIndex];
            if (neighborTetIndex != M_MAX_UNSIGNED)
            {
                Tetrahedron& neighborTetrahedron = tetrahedrons_[neighborTetIndex];
                const unsigned neighborFaceIndex = neighborTetrahedron.GetNeighborFaceIndex(tetIndex);

                tetrahedron.neighbors_[faceIndex] = M_MAX_UNSIGNED;
                neighborTetrahedron.neighbors_[neighborFaceIndex] = M_MAX_UNSIGNED;
            }
        }
    }
}

void TetrahedralMesh::FillStarShapedHole(TetrahedralMesh::DelaunayContext& ctx,
    const ea::vector<unsigned>& outputTetrahedrons, const TetrahedralMeshSurface& holeSurface, unsigned centerIndex)
{
    for (unsigned i = 0; i < holeSurface.Size(); ++i)
    {
        const unsigned newTetIndex = outputTetrahedrons[i];
        Tetrahedron& tetrahedron = tetrahedrons_[newTetIndex];
        const TetrahedralMeshSurfaceTriangle& holeTriangle = holeSurface.faces_[i];

        // Connect to newly added (or to be added) adjacent tetrahedrons filling the hole
        for (unsigned j = 0; j < 3; ++j)
        {
            tetrahedron.indices_[j] = holeTriangle.indices_[j];
            tetrahedron.neighbors_[j] = outputTetrahedrons[holeTriangle.neighbors_[j]];
        }

        // Connect to tetrahedron outside the hole
        tetrahedron.indices_[3] = centerIndex;
        tetrahedron.neighbors_[3] = holeTriangle.tetIndex_;
        if (holeTriangle.tetIndex_ != M_MAX_UNSIGNED)
        {
            Tetrahedron& neighborTetrahedron = tetrahedrons_[holeTriangle.tetIndex_];
            assert(neighborTetrahedron.neighbors_[holeTriangle.tetFace_] == M_MAX_UNSIGNED);
            neighborTetrahedron.neighbors_[holeTriangle.tetFace_] = newTetIndex;
        }

        tetrahedron.CalculateInnerMatrix(vertices_);

        ctx.removed_[newTetIndex] = false;
        ctx.circumspheres_[newTetIndex] = GetTetrahedronCircumsphere(newTetIndex);
    }
}

void TetrahedralMesh::DisconnectSuperMeshTetrahedrons(ea::vector<bool>& removed)
{
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        // Any tetrahedron containing super-vertex is to be removed
        Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned j = 0; j < 4; ++j)
        {
            if (tetrahedron.indices_[j] < NumSuperMeshVertices)
            {
                removed[tetIndex] = true;
                break;
            }
        }

        if (!removed[tetIndex])
            continue;

        DisconnectTetrahedron(tetIndex);
    }
}

void TetrahedralMesh::EnsureMeshConnectivity(ea::vector<bool>& removed)
{
    // Find first tetrahedron
    unsigned firstTetIndex{};
    for (firstTetIndex = 0; firstTetIndex < tetrahedrons_.size(); ++firstTetIndex)
    {
        if (!removed[firstTetIndex])
            break;
    }

    // Mesh is empty
    if (firstTetIndex >= tetrahedrons_.size())
        return;

    // Do breadth search to collect all tetrahedrons
    // Note: size may change during the loop
    ea::vector<unsigned> queue;
    ea::vector<bool> visited(tetrahedrons_.size(), false);

    queue.push_back(firstTetIndex);
    visited[firstTetIndex] = true;

    for (unsigned i = 0; i < queue.size(); ++i)
    {
        const unsigned tetIndex = queue[i];
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];

        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborTetIndex = tetrahedron.neighbors_[faceIndex];
            if (neighborTetIndex == M_MAX_UNSIGNED)
                continue;

            if (removed[neighborTetIndex] || visited[neighborTetIndex])
                continue;

            queue.push_back(neighborTetIndex);
            visited[neighborTetIndex] = true;
        }
    }

    // Remove all non-visited tetrahedrons, no need to disconnect them
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        if (!visited[tetIndex])
            removed[tetIndex] = true;
    }
}

void TetrahedralMesh::FilterMeshSurface(ea::vector<bool>& removed)
{
    ea::vector<TetrahedralMeshEdge> surfaceEdges;

    // Append edge to vector or update cardinality and score if already present.
    const auto appendOrAccumulate = [&surfaceEdges](unsigned startIndex, const TetrahedralMeshEdge& edge)
    {
        // Try to update existing edges
        for (unsigned i = startIndex; i < surfaceEdges.size(); ++i)
        {
            TetrahedralMeshEdge& existingEdge = surfaceEdges[i];
            if (existingEdge.IsSameEdge(edge) && existingEdge.tetIndex_ == edge.tetIndex_)
            {
                ++existingEdge.cardinality_;
                existingEdge.silverScore_ = ea::max(existingEdge.silverScore_, edge.silverScore_);
                return;
            }
        }

        // Append new edge
        surfaceEdges.push_back(edge);
    };

    // Collect surface edges
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        if (removed[tetIndex])
            continue;

        const unsigned startIndex = surfaceEdges.size();
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            if (tetrahedron.neighbors_[faceIndex] != M_MAX_UNSIGNED)
                continue;

            const TetrahedralMeshSurfaceTriangle triangle = tetrahedron.GetTriangleFace(
                faceIndex, tetIndex, faceIndex);
            const float score = triangle.CalculateScore(vertices_);

            appendOrAccumulate(startIndex,
                TetrahedralMeshEdge(triangle.indices_[0], triangle.indices_[1], tetIndex, score));
            appendOrAccumulate(startIndex,
                TetrahedralMeshEdge(triangle.indices_[1], triangle.indices_[2], tetIndex, score));
            appendOrAccumulate(startIndex,
                TetrahedralMeshEdge(triangle.indices_[2], triangle.indices_[0], tetIndex, score));
        }
    }

    // Remove extra tetrahedrons
    ea::vector<unsigned> removedTetrahedrons;
    ea::sort(surfaceEdges.begin(), surfaceEdges.end());

    auto firstEdge = surfaceEdges.begin();
    while (firstEdge != surfaceEdges.end())
    {
        const auto isSameEdge = [&](const TetrahedralMeshEdge& rhs) { return firstEdge->IsSameEdge(rhs); };
        const auto lastEdge = ea::find_if_not(firstEdge, surfaceEdges.end(), isSameEdge);

        const auto accumulateCardinality = [&](unsigned value, const TetrahedralMeshEdge& rhs)
        {
            if (!removed[rhs.tetIndex_])
                return value + rhs.cardinality_;
            return value;
        };
        const unsigned totalCardinality = ea::accumulate(firstEdge, lastEdge, 0u, accumulateCardinality);

        // Need to remove some tetrahedrons, going from back to front
        static const unsigned MaxCardinality = 2;
        if (totalCardinality > MaxCardinality)
        {
            unsigned remainingCardinality = totalCardinality;
            for (auto iter = ea::prev(lastEdge); iter != firstEdge; --iter)
            {
                if (removed[iter->tetIndex_])
                    continue;

                removed[iter->tetIndex_] = true;
                removedTetrahedrons.push_back(iter->tetIndex_);

                remainingCardinality -= iter->cardinality_;
                if (remainingCardinality <= MaxCardinality)
                    break;
            }
        }

        firstEdge = lastEdge;
    }

    // Disconnect removed tetrahedrons
    for (unsigned tetIndex : removedTetrahedrons)
        DisconnectTetrahedron(tetIndex);
}

void TetrahedralMesh::RemoveMarkedTetrahedrons(const ea::vector<bool>& removed)
{
    // Prepare for reconstruction
    ea::vector<Tetrahedron> tetrahedronsCopy = ea::move(tetrahedrons_);
    ea::vector<unsigned> oldToNewIndexMap(tetrahedronsCopy.size());
    tetrahedrons_.clear();

    // Rebuild array and create index
    for (unsigned oldTetIndex = 0; oldTetIndex < tetrahedronsCopy.size(); ++oldTetIndex)
    {
        if (removed[oldTetIndex])
        {
            oldToNewIndexMap[oldTetIndex] = M_MAX_UNSIGNED;
            continue;
        }

        oldToNewIndexMap[oldTetIndex] = tetrahedrons_.size();
        tetrahedrons_.push_back(tetrahedronsCopy[oldTetIndex]);
    }

    // Adjust neighbor indices
    for (Tetrahedron& tetrahedron : tetrahedrons_)
    {
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned oldIndex = tetrahedron.neighbors_[faceIndex];
            if (oldIndex != M_MAX_UNSIGNED)
            {
                const unsigned newIndex = oldToNewIndexMap[oldIndex];
                assert(newIndex != M_MAX_UNSIGNED);
                tetrahedron.neighbors_[faceIndex] = newIndex;
            }
        }
    }
}

void TetrahedralMesh::RemoveSuperMeshVertices()
{
    ea::rotate(vertices_.begin(), vertices_.begin() + NumSuperMeshVertices, vertices_.end());

    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
            tetrahedrons_[tetIndex].indices_[faceIndex] -= NumSuperMeshVertices;
    }

    for (auto& edge : debugHighlightEdges_)
    {
        for (unsigned* index : { &edge.first, &edge.second })
        {
            if (*index < NumSuperMeshVertices)
                *index += vertices_.size() - NumSuperMeshVertices;
            else
                *index -= NumSuperMeshVertices;
        }
    }
}

void TetrahedralMesh::UpdateIgnoredVertices()
{
    ea::vector<bool> ignored_(vertices_.size(), true);
    for (const Tetrahedron& tetrahedron : tetrahedrons_)
    {
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
            ignored_[tetrahedron.indices_[faceIndex]] = false;
    }

    ignoredVertices_.clear();
    for (unsigned vertexIndex = 0; vertexIndex < vertices_.size() - NumSuperMeshVertices; ++vertexIndex)
    {
        if (ignored_[vertexIndex])
            ignoredVertices_.push_back(vertexIndex);
    }
}

void TetrahedralMesh::BuildHullSurface(TetrahedralMeshSurface& hullSurface)
{
    hullSurface.Clear();
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            if (tetrahedron.neighbors_[faceIndex] != M_MAX_UNSIGNED)
                continue;

            const TetrahedralMeshSurfaceTriangle& hullTriangle = tetrahedron.GetTriangleFace(
                faceIndex, tetIndex, faceIndex);
            hullSurface.faces_.push_back(hullTriangle);
        }
    }

    hullSurface.CalculateAdjacency();
    assert(hullSurface.IsClosedSurface());

    for (TetrahedralMeshSurfaceTriangle& hullTriangle : hullSurface.faces_)
        hullTriangle.Normalize(vertices_);
}

void TetrahedralMesh::CalculateHullNormals(const TetrahedralMeshSurface& hullSurface)
{
    hullNormals_.resize(vertices_.size());

    for (const TetrahedralMeshSurfaceTriangle& triangle : hullSurface.faces_)
    {
        const Vector3 p1 = vertices_[triangle.indices_[0]];
        const Vector3 p2 = vertices_[triangle.indices_[1]];
        const Vector3 p3 = vertices_[triangle.indices_[2]];
        const Vector3 normal = (p2 - p1).CrossProduct(p3 - p1);

        // Accumulate vertex normals
        for (unsigned j = 0; j < 3; ++j)
            hullNormals_[triangle.indices_[j]] += normal;
    }

    // Normalize outputs
    for (Vector3& normal : hullNormals_)
    {
        if (normal != Vector3::ZERO)
            normal.Normalize();
    }
}

void TetrahedralMesh::BuildOuterTetrahedrons(const TetrahedralMeshSurface& hullSurface)
{
    tetrahedrons_.resize(numInnerTetrahedrons_ + hullSurface.Size());

    for (unsigned tetIndex = numInnerTetrahedrons_; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        const TetrahedralMeshSurfaceTriangle& hullTrinagle = hullSurface.faces_[tetIndex - numInnerTetrahedrons_];

        for (unsigned faceIndex = 0; faceIndex < 3; ++faceIndex)
        {
            tetrahedron.indices_[faceIndex] = hullTrinagle.indices_[faceIndex];
            tetrahedron.neighbors_[faceIndex] = numInnerTetrahedrons_ + hullTrinagle.neighbors_[faceIndex];
        }

        tetrahedron.indices_[3] = Tetrahedron::Infinity3;
        tetrahedron.neighbors_[3] = hullTrinagle.tetIndex_;
        tetrahedrons_[hullTrinagle.tetIndex_].neighbors_[hullTrinagle.tetFace_] = tetIndex;
    }

    assert(IsAdjacencyValid(true));
}

void TetrahedralMesh::CalculateOuterMatrices()
{
    for (unsigned tetIndex = numInnerTetrahedrons_; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];

        Vector3 positions[3];
        Vector3 normals[3];
        for (unsigned i = 0; i < 3; ++i)
        {
            positions[i] = vertices_[tetrahedron.indices_[i]];
            normals[i] = hullNormals_[tetrahedron.indices_[i]];
        }

        const Vector3 A = positions[0] - positions[2];
        const Vector3 Ap = normals[0] - normals[2];
        const Vector3 B = positions[1] - positions[2];
        const Vector3 Bp = normals[1] - normals[2];
        const Vector3 P2 = positions[2];
        const Vector3 Cp = -normals[2];

        Matrix3x4& m = tetrahedron.matrix_;

        m.m00_ = // input.x *
            + Ap.y_ * Bp.z_
            - Ap.z_ * Bp.y_;
        m.m01_ = // input.y *
            - Ap.x_ * Bp.z_
            + Ap.z_ * Bp.x_;
        m.m02_ = // input.z *
            + Ap.x_ * Bp.y_
            - Ap.y_ * Bp.x_;
        m.m03_ = // 1 *
            + A.x_ * Bp.y_* Cp.z_
            - A.y_ * Bp.x_ * Cp.z_
            + Ap.x_ * B.y_ * Cp.z_
            - Ap.y_ * B.x_ * Cp.z_
            + A.z_ * Bp.x_ * Cp.y_
            - A.z_ * Bp.y_ * Cp.x_
            + Ap.z_ * B.x_ * Cp.y_
            - Ap.z_ * B.y_ * Cp.x_
            - A.x_ * Bp.z_ * Cp.y_
            + A.y_ * Bp.z_ * Cp.x_
            - Ap.x_ * B.z_ * Cp.y_
            + Ap.y_ * B.z_ * Cp.x_;
        m.m03_ -= P2.x_ * m.m00_ + P2.y_ * m.m01_ + P2.z_ * m.m02_;

        m.m10_ = // input.x *
            + Ap.y_ * B.z_
            + A.y_ * Bp.z_
            - Ap.z_ * B.y_
            - A.z_ * Bp.y_;
        m.m11_ = // input.y *
            - A.x_ * Bp.z_
            - Ap.x_ * B.z_
            + A.z_ * Bp.x_
            + Ap.z_ * B.x_;
        m.m12_ = // input.z *
            + A.x_ * Bp.y_
            - A.y_ * Bp.x_
            + Ap.x_ * B.y_
            - Ap.y_ * B.x_;
        m.m13_ = // 1 *
            + A.x_ * B.y_ * Cp.z_
            - A.y_ * B.x_ * Cp.z_
            - A.x_ * B.z_ * Cp.y_
            + A.y_ * B.z_ * Cp.x_
            + A.z_ * B.x_ * Cp.y_
            - A.z_ * B.y_ * Cp.x_;
        m.m13_ -= P2.x_ * m.m10_ + P2.y_ * m.m11_ + P2.z_ * m.m12_;

        m.m20_ = // input.x *
            - A.z_ * B.y_
            + A.y_ * B.z_;
        m.m21_ = // input.y *
            - A.x_ * B.z_
            + A.z_ * B.x_;
        m.m22_ = // input.z *
            + A.x_ * B.y_
            - A.y_ * B.x_;
        m.m23_ = 0.0f; // 1 *
        m.m23_ -= P2.x_ * m.m20_ + P2.y_ * m.m21_ + P2.z_ * m.m22_;

        const float a =
            + Ap.x_ * Bp.y_ * Cp.z_
            - Ap.y_ * Bp.x_ * Cp.z_
            + Ap.z_ * Bp.x_ * Cp.y_
            - Ap.z_ * Bp.y_ * Cp.x_
            + Ap.y_ * Bp.z_ * Cp.x_
            - Ap.x_ * Bp.z_ * Cp.y_;

        if (Abs(a) > M_LARGE_EPSILON)
        {
            // d is not zero, so the polynomial at^3 + bt^2 + ct + d = 0 is actually cubic
            // and we can simplify to the monic form t^3 + pt^2 + qt + r = 0
            m = m * (1.0f / a);
        }
        else
        {
            // It's actually a quadratic or even linear equation
            tetrahedron.indices_[3] = Tetrahedron::Infinity2;
        }
    }
}

bool SerializeValue(Archive& archive, const char* name, Tetrahedron& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        SerializeValue(archive, "Index0", value.indices_[0]);
        SerializeValue(archive, "Index1", value.indices_[1]);
        SerializeValue(archive, "Index2", value.indices_[2]);
        SerializeValue(archive, "Index3", value.indices_[3]);
        SerializeValue(archive, "Neighbor0", value.neighbors_[0]);
        SerializeValue(archive, "Neighbor1", value.neighbors_[1]);
        SerializeValue(archive, "Neighbor2", value.neighbors_[2]);
        SerializeValue(archive, "Neighbor3", value.neighbors_[3]);
        SerializeValue(archive, "Matrix", value.matrix_);
        return true;
    }
    return false;
}

bool SerializeValue(Archive& archive, const char* name, TetrahedralMesh& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        SerializeVector(archive, "Vertices", "Position", value.vertices_);
        SerializeVector(archive, "Tetrahedrons", "Tetrahedron", value.tetrahedrons_);
        SerializeVector(archive, "HullNormals", "Hulls", value.hullNormals_);
        SerializeValue(archive, "NumInnerTetrahedrons", value.numInnerTetrahedrons_);
        return true;
    }
    return false;
}

}
