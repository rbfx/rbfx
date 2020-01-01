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

#include "../Precompiled.h"

#include "../IO/Log.h"
#include "../Math/TetrahedralMesh.h"

#include <EASTL/numeric.h>
#include <EASTL/sort.h>

namespace Urho3D
{

void TetrahedralMesh::Define(ea::span<const Vector3> positions, float padding)
{
    BoundingBox boundingBox(positions.data(), positions.size());
    boundingBox.min_ -= Vector3::ONE * padding;
    boundingBox.max_ += Vector3::ONE * padding;
    InitializeSuperMesh(boundingBox);
    BuildTetrahedrons(positions);
}

Sphere TetrahedralMesh::GetTetrahedronCircumsphere(unsigned tetIndex) const
{
    const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
    const Vector3 p0 = vertices_[tetrahedron.indices_[0]];
    const Vector3 p1 = vertices_[tetrahedron.indices_[1]];
    const Vector3 p2 = vertices_[tetrahedron.indices_[2]];
    const Vector3 p3 = vertices_[tetrahedron.indices_[3]];
    const Vector3 u1 = p1 - p0;
    const Vector3 u2 = p2 - p0;
    const Vector3 u3 = p3 - p0;
    const float d01 = u1.LengthSquared();
    const float d02 = u2.LengthSquared();
    const float d03 = u3.LengthSquared();
    const Vector3 num = d01 * u2.CrossProduct(u3) + d02 * u3.CrossProduct(u1) + d03 * u1.CrossProduct(u2);
    const float den = 2 * u1.DotProduct(u2.CrossProduct(u3));

    if (Abs(den) < M_EPSILON)
    {
        URHO3D_LOGERROR("Degenerate tetrahedron in tetrahedral mesh due to error in tetrahedral mesh generation");
        assert(0);
        return Sphere(Vector3::ZERO, M_LARGE_VALUE);
    }

    const Vector3 r0 = num / den;
    const Vector3 center = p0 + r0;

    const float eps = M_LARGE_EPSILON;
    const float radius = ea::max({ r0.Length(), (p1 - center).Length(), (p2 - center).Length(), (p3 - center).Length()});

    return { center, radius + eps };
}

void TetrahedralMesh::InitializeSuperMesh(const BoundingBox& boundingBox)
{
    static const unsigned numVertices = 8;
    static const Vector3 offsets[numVertices] =
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

    // TODO(glow): Use Tetrahedron here
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

    vertices_.resize(numVertices);
    for (unsigned i = 0; i < numVertices; ++i)
        vertices_[i] = boundingBox.min_ + boundingBox.Size() * offsets[i];

    tetrahedrons_.resize(numTetrahedrons);
    for (unsigned i = 0; i < numTetrahedrons; ++i)
    {
        Tetrahedron cell;
        for (unsigned j = 0; j < 4; ++j)
        {
            cell.indices_[j] = indices[i][j];
            cell.neighbors_[j] = neighbors[i][j];
        }
        tetrahedrons_[i] = cell;
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

    // Append vertices
    const unsigned startVertex = vertices_.size();
    vertices_.insert(vertices_.end(), positions.begin(), positions.end());

    // Triangulate
    TetrahedralMeshSurface holeSurface;
    ea::vector<unsigned> removedTetrahedrons;

    for (unsigned newVertexIndex = startVertex; newVertexIndex < vertices_.size(); ++newVertexIndex)
    {
        // Carve hole in the mesh
        FindAndRemoveIntersected(ctx, vertices_[newVertexIndex], holeSurface, removedTetrahedrons);
        if (!holeSurface.IsClosedSurface())
        {
            URHO3D_LOGERROR("Hole surface is invalid");
            assert(0);
            return;
        }

        // Allocate space for new tetrahedrons
        while (removedTetrahedrons.size() < holeSurface.Size())
        {
            removedTetrahedrons.push_back(tetrahedrons_.size());
            tetrahedrons_.push_back();
            ctx.circumspheres_.push_back();
            ctx.removed_.push_back(true);
        }

        for (unsigned i = 0; i < holeSurface.Size(); ++i)
        {
            const unsigned newCellIndex = removedTetrahedrons[i];
            Tetrahedron& tetrahedron = tetrahedrons_[newCellIndex];
            const TetrahedralMeshSurfaceTriangle& face = holeSurface.faces_[i];

            for (unsigned j = 0; j < 3; ++j)
            {
                tetrahedron.indices_[j] = face.indices_[j];
                tetrahedron.neighbors_[j] = removedTetrahedrons[face.neighbors_[j]];
            }
            tetrahedron.indices_[3] = newVertexIndex;
            tetrahedron.neighbors_[3] = face.tetIndex_;
            if (face.tetIndex_ != M_MAX_UNSIGNED)
                tetrahedrons_[face.tetIndex_].neighbors_[face.tetFace_] = newCellIndex;
            ctx.removed_[newCellIndex] = false;
            ctx.circumspheres_[newCellIndex] = GetTetrahedronCircumsphere(newCellIndex);
        }
    }

    for (unsigned i = 0; i < tetrahedrons_.size(); ++i)
    {
        for (unsigned j = 0; j < 4; ++j)
        {
            if (tetrahedrons_[i].indices_[j] < 8)
            {
                ctx.removed_[i] = true;
                break;
            }
        }
        if (ctx.removed_[i])
        {
            for (unsigned j = 0; j < 4; ++j)
            {
                const unsigned neighborIndex = tetrahedrons_[i].neighbors_[j];
                if (neighborIndex != M_MAX_UNSIGNED)
                {
                    ea::replace(ea::begin(tetrahedrons_[neighborIndex].neighbors_),
                        ea::end(tetrahedrons_[neighborIndex].neighbors_), i, M_MAX_UNSIGNED);
                }
            }
        }
    }

    // Copy output
    ea::vector<Tetrahedron> cells = ea::move(tetrahedrons_);
    ea::vector<unsigned> newIndices(cells.size());
    tetrahedrons_.clear();
    for (unsigned i = 0; i < cells.size(); ++i)
    {
        if (ctx.removed_[i])
        {
            newIndices[i] = M_MAX_UNSIGNED;
            continue;
        }

        newIndices[i] = tetrahedrons_.size();
        tetrahedrons_.push_back(cells[i]);
    }
    for (Tetrahedron& cell : tetrahedrons_)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            if (cell.neighbors_[i] == M_MAX_UNSIGNED)
                continue;
            const unsigned newIndex = newIndices[cell.neighbors_[i]];
            assert(newIndex != M_MAX_UNSIGNED);
            cell.neighbors_[i] = newIndex;
        }
    }

    vertices_.erase(vertices_.begin(), vertices_.begin() + 8);
    for (unsigned i = 0; i < tetrahedrons_.size(); ++i)
    {
        for (unsigned j = 0; j < 4; ++j)
            tetrahedrons_[i].indices_[j] -= 8;
    }

    for (unsigned i = 0; i < tetrahedrons_.size(); ++i)
    {
        Tetrahedron& cell = tetrahedrons_[i];
        const Vector3 p0 = vertices_[cell.indices_[0]];
        const Vector3 p1 = vertices_[cell.indices_[1]];
        const Vector3 p2 = vertices_[cell.indices_[2]];
        const Vector3 p3 = vertices_[cell.indices_[3]];
        const Vector3 u1 = p1 - p0;
        const Vector3 u2 = p2 - p0;
        const Vector3 u3 = p3 - p0;
        cell.matrix_ = Matrix3(u1.x_, u2.x_, u3.x_, u1.y_, u2.y_, u3.y_, u1.z_, u2.z_, u3.z_).Inverse();
        assert(GetInnerBarycentricCoords(i, p0).Equals(Vector4(1, 0, 0, 0)));
        assert(GetInnerBarycentricCoords(i, p1).Equals(Vector4(0, 1, 0, 0)));
        assert(GetInnerBarycentricCoords(i, p2).Equals(Vector4(0, 0, 1, 0)));
        assert(GetInnerBarycentricCoords(i, p3).Equals(Vector4(0, 0, 0, 1)));
    }

    assert(IsAdjacencyValid());
}

bool TetrahedralMesh::IsAdjacencyValid() const
{
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborIndex = tetrahedrons_[tetIndex].neighbors_[faceIndex];
            if (neighborIndex != M_MAX_UNSIGNED)
            {
                const Tetrahedron& neighborCell = tetrahedrons_[neighborIndex];
                if (!neighborCell.HasNeighbor(tetIndex))
                    return false;
            }
        }
    }
    return true;
}

void TetrahedralMesh::FindAndRemoveIntersected(TetrahedralMesh::DelaunayContext& ctx, const Vector3& position,
    TetrahedralMeshSurface& holeSurface, ea::vector<unsigned>& removedTetrahedrons) const
{
    ctx.searchQueue_.clear();

    // Reset output
    holeSurface.Clear();
    removedTetrahedrons.clear();

    // Find first tetrahedron to remove
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        if (!ctx.removed_[tetIndex] && ctx.circumspheres_[tetIndex].IsInside(position) != OUTSIDE)
        {
            removedTetrahedrons.push_back(tetIndex);
            ctx.searchQueue_.push_back(tetIndex);
            ctx.removed_[tetIndex] = true;
            break;
        }
    }

    if (removedTetrahedrons.empty())
    {
        URHO3D_LOGERROR("Cannot update tetrahedral mesh for vertex position");
        assert(0);
        return;
    }

    // Do breadth search to collect all bad cells and build hole mesh
    // Note: size may change during the loop
    for (unsigned i = 0; i < ctx.searchQueue_.size(); ++i)
    {
        const unsigned tetIndex = ctx.searchQueue_[i];
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];

        // Process neighbors
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborTetIndex = tetrahedron.neighbors_[faceIndex];

            // Outer surface is reached
            if (neighborTetIndex == M_MAX_UNSIGNED)
            {
                // Face of outer surface doesn't have underlying tetrahedron
                const TetrahedralMeshSurfaceTriangle holeFace = tetrahedron.GetTriangleFace(
                    faceIndex, M_MAX_UNSIGNED, M_MAX_UNSIGNED);
                holeSurface.AddFace(holeFace);
                continue;
            }

            // Ignore removed cells
            if (ctx.removed_[neighborTetIndex])
                continue;

            // If circumsphere of neighbor tetrahedron contains the point, remove this neighbor and queue it.
            if (ctx.circumspheres_[neighborTetIndex].IsInside(position) != OUTSIDE)
            {
                removedTetrahedrons.push_back(neighborTetIndex);
                ctx.searchQueue_.push_back(neighborTetIndex);
                ctx.removed_[neighborTetIndex] = true;
                continue;
            }

            // Boundary of the hole is found
            const Tetrahedron& neighborTetrahedron = tetrahedrons_[neighborTetIndex];
            const unsigned neighborFaceIndex = neighborTetrahedron.GetNeighborFaceIndex(tetIndex);
            const TetrahedralMeshSurfaceTriangle newFace = neighborTetrahedron.GetTriangleFace(
                neighborFaceIndex, neighborTetIndex, neighborFaceIndex);
            holeSurface.AddFace(newFace);
        }
    }
}

}
