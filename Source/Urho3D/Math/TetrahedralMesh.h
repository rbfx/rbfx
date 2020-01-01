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
#include "../Math/Matrix3.h"
#include "../Math/Sphere.h"
#include "../Math/Vector3.h"

#include <EASTL/span.h>

namespace Urho3D
{

/// Surface triangle of tetrahedral mesh with adjacency information.
struct TetrahedralMeshSurfaceTriangle
{
    /// Indices of triangle vertices.
    unsigned indices_[3]{};
    /// Indices of neighbor triangles.
    unsigned neighbors_[3]{ M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED};
    /// Index of underlying tetrahedron. M_MAX_UNSIGNED if empty.
    unsigned tetIndex_{};
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
    /// Indices of tetrahedron vertices.
    unsigned indices_[4]{};
    /// Indices of neighbor tetrahedrons. M_MAX_UNSIGNED if empty.
    unsigned neighbors_[4]{ M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED };
    /// Pre-computed matrix for calculating barycentric coordinates.
    Matrix3 matrix_;

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
        face.tetIndex_ = tetIndex;
        face.tetFace_ = tetFace;
        return face;
    }

    /// Return whether the tetrahedron has given neighbour.
    bool HasNeighbor(unsigned neighborIndex) const
    {
        return ea::count(ea::begin(neighbors_), ea::end(neighbors_), neighborIndex) != 0;
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
        const Tetrahedron& cell = tetrahedrons_[tetIndex];
        const Vector3& basePosition = vertices_[cell.indices_[0]];
        const Vector3 coords = cell.matrix_ * (position - basePosition);
        return { 1.0f - coords.x_ - coords.y_ - coords.z_, coords.x_, coords.y_, coords.z_ };
    }

private:
    /// Create super-mesh for Delaunay triangulation.
    void InitializeSuperMesh(const BoundingBox& boundingBox);
    /// Build tetrahedrons for given positions.
    void BuildTetrahedrons(ea::span<const Vector3> positions);
    /// Return whether the adjacency is valid.
    bool IsAdjacencyValid() const;

    /// Data used for Delaunay triangulation.
    struct DelaunayContext
    {
        /// Circumspheres of mesh tetrahedrons.
        ea::vector<Sphere> circumspheres_;
        /// Whether the tetrahedron is removed.
        ea::vector<bool> removed_;

        /// List of tetrahedrons removed on current iteration.
        ea::vector<unsigned> badTetrahedrons_;
        /// Hole surface for current iteration.
        TetrahedralMeshSurface holeSurface_;
        /// Queue for breadth search of bad tetrahedrons.
        ea::vector<unsigned> searchQueue_;
    };

public:
    /// Vertices.
    ea::vector<Vector3> vertices_;
    /// Tetrahedrons.
    ea::vector<Tetrahedron> tetrahedrons_;
    /// Hull normals.
    ea::vector<Vector3> hullNormals_;
    /// Number of inner cells.
    unsigned numInnerCells_{};
};

}
