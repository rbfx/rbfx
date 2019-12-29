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

#include "../Graphics/GlobalIllumination.h"

#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Scene/Scene.h"

#include <EASTL/numeric.h>
#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

/// Calculate circumsphere of the tetrahedron.
Sphere CalculateTetrahedronCircumsphere(const AdjacentTetrahedron& cell, const ea::vector<Vector3>& vertices)
{
    const Vector3 p0 = vertices[cell.indices_[0]];
    const Vector3 p1 = vertices[cell.indices_[1]];
    const Vector3 p2 = vertices[cell.indices_[2]];
    const Vector3 p3 = vertices[cell.indices_[3]];
    const Vector3 u1 = p1 - p0;
    const Vector3 u2 = p2 - p0;
    const Vector3 u3 = p3 - p0;
    const float d01 = u1.LengthSquared();
    const float d02 = u2.LengthSquared();
    const float d03 = u3.LengthSquared();
    const Vector3 r0 = (d01 * u2.CrossProduct(u3) + d02 * u3.CrossProduct(u1) + d03 * u1.CrossProduct(u2))
        / (2 * u1.DotProduct(u2.CrossProduct(u3)));
    const Vector3 center = p0 + r0;

    assert(Equals((p1 - center).Length(), r0.Length(), M_LARGE_EPSILON));
    assert(Equals((p2 - center).Length(), r0.Length(), M_LARGE_EPSILON));
    assert(Equals((p3 - center).Length(), r0.Length(), M_LARGE_EPSILON));

    return { center, r0.Length() + M_LARGE_EPSILON };
}

/// Initialize tetrahedral mesh for given volume.
void InitializeTetrahedralMesh(TetrahedralMesh& mesh, const BoundingBox& volume)
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

    // Initialize vertices
    mesh.vertices_.resize(numVertices);
    for (unsigned i = 0; i < numVertices; ++i)
        mesh.vertices_[i] = volume.min_ + volume.Size() * offsets[i];

    // Initialize cells
    mesh.cells_.resize(numTetrahedrons);
    for (unsigned i = 0; i < numTetrahedrons; ++i)
    {
        AdjacentTetrahedron cell;
        for (unsigned j = 0; j < 4; ++j)
        {
            cell.indices_[j] = indices[i][j];
            cell.neighbors_[j] = neighbors[i][j];
        }
        mesh.cells_[i] = cell;
    }
};

/// Tetrahedron for Delaunay triangulation.
struct DelaunayTetrahedron
{
    /// Tetrahedron with adjacency information.
    AdjacentTetrahedron tet_;
    /// Cell circumsphere.
    Sphere circumsphere_;
    /// Whether the cell is cosidered bad during breadth search.
    bool bad_{};
};

/// Face of the hole in tetrahedral mesh.
struct TetrahedralMeshHoleFace
{
    /// Indices.
    unsigned indices_[3]{};
    /// Underlying cell.
    unsigned cell_{};
    /// Underlying cell face.
    unsigned face_{};
    /// Neighbor faces.
    unsigned neighbors_[3]{ M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED};
    /// Return edge, indexes are sorted.
    ea::pair<unsigned, unsigned> GetEdge(unsigned i) const
    {
        unsigned begin = indices_[i];
        unsigned end = indices_[(i + 1) % 3];
        if (begin > end)
            ea::swap(begin, end);
        return { begin, end };
    }
};

/// Return face of the tetrahedron.
TetrahedralMeshHoleFace GetTetrahedronHoleFace(const AdjacentTetrahedron& tet, unsigned faceIndex, unsigned cellIndex)
{
    TetrahedralMeshHoleFace face;
    face.cell_ = cellIndex;
    face.face_ = faceIndex;

    unsigned j = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        if (i != faceIndex)
            face.indices_[j++] = tet.indices_[i];
    }
    return face;
}

/// Add triangle to hole mesh.
void AddTriangleToHole(ea::vector<TetrahedralMeshHoleFace>& mesh, TetrahedralMeshHoleFace newFace)
{
    // Find adjacent triangles
    const unsigned newIndex = mesh.size();
    for (unsigned oldIndex = 0; oldIndex < newIndex; ++oldIndex)
    {
        TetrahedralMeshHoleFace& oldFace = mesh[oldIndex];
        for (unsigned i = 0; i < 3; ++i)
        {
            const auto oldEdge = oldFace.GetEdge(i);
            for (unsigned j = 0; j < 3; ++j)
            {
                const auto newEdge = newFace.GetEdge(j);
                if (oldEdge == newEdge)
                {
                    oldFace.neighbors_[(i + 2) % 3] = newIndex;
                    newFace.neighbors_[(j + 2) % 3] = oldIndex;
                }
            }
        }
    }

    mesh.push_back(newFace);
}

/// Return whether the hole mesh is fully connected.
bool IsHoleMeshConnected(ea::vector<TetrahedralMeshHoleFace>& mesh)
{
    for (unsigned faceIndex = 0; faceIndex < mesh.size(); ++faceIndex)
    {
        for (unsigned i = 0; i < 3; ++i)
        {
            const unsigned neighborIndex = mesh[faceIndex].neighbors_[i];
            if (neighborIndex == M_MAX_UNSIGNED)
                return false;

            const TetrahedralMeshHoleFace& neighborFace = mesh[neighborIndex];
            if (ea::count(ea::begin(neighborFace.neighbors_), ea::end(neighborFace.neighbors_), faceIndex) == 0)
                return false;
        }
    }
    return true;
}

/// Return whether the adjacency information of tetrahedral mesh is valid.
bool IsTetrahedralMeshAdjacencyValid(const TetrahedralMesh& mesh)
{
    for (unsigned cellIndex = 0; cellIndex < mesh.cells_.size(); ++cellIndex)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            const unsigned neighborIndex = mesh.cells_[cellIndex].neighbors_[i];
            if (neighborIndex != M_MAX_UNSIGNED)
            {
                const AdjacentTetrahedron& neighborCell = mesh.cells_[neighborIndex];
                if (ea::count(ea::begin(neighborCell.neighbors_), ea::end(neighborCell.neighbors_), cellIndex) == 0)
                    return false;
            }
        }
    }
    return true;
}

/// Add vertices to tetrahedral mesh.
void AddTetrahedralMeshVertices(TetrahedralMesh& mesh, ea::span<const Vector3> positions)
{
    // Copy cells and initialize all required data
    ea::vector<DelaunayTetrahedron> cells(mesh.cells_.size());
    for (unsigned i = 0; i < mesh.cells_.size(); ++i)
    {
        cells[i].tet_ = mesh.cells_[i];
        cells[i].circumsphere_ = CalculateTetrahedronCircumsphere(cells[i].tet_, mesh.vertices_);
    }

    unsigned tempCount = 0;

    // Triangulate
    ea::vector<unsigned> badCells;
    ea::vector<TetrahedralMeshHoleFace> holeMesh;
    ea::vector<unsigned> searchQueue;
    for (const Vector3& position : positions)
    {
        const unsigned newIndex = mesh.vertices_.size();
        mesh.vertices_.push_back(position);

        badCells.clear();
        searchQueue.clear();
        holeMesh.clear();

        // Find first bad cell
        for (unsigned cellIndex = 0; cellIndex < cells.size(); ++cellIndex)
        {
            DelaunayTetrahedron& cell = cells[cellIndex];
            if (!cell.bad_ && cell.circumsphere_.IsInside(position) != OUTSIDE)
            {
                badCells.push_back(cellIndex);
                searchQueue.push_back(cellIndex);
                cell.bad_ = true;
                break;
            }
        }

        if (badCells.empty())
        {
            assert(0);
            return;
        }

        // Do breadth search to collect all bad cells and build hole mesh
        unsigned firstCell = 0;
        while (firstCell < searchQueue.size())
        {
            const unsigned lastCell = searchQueue.size();
            for (unsigned i = firstCell; i < lastCell; ++i)
            {
                const DelaunayTetrahedron& cell = cells[searchQueue[i]];

                // Process neighbors
                for (unsigned j = 0; j < 4; ++j)
                {
                    const unsigned nextIndex = cell.tet_.neighbors_[j];
                    if (nextIndex == M_MAX_UNSIGNED)
                    {
                        // Missing neighbor closes hole
                        const TetrahedralMeshHoleFace newFace = GetTetrahedronHoleFace(cell.tet_, j, M_MAX_UNSIGNED);
                        AddTriangleToHole(holeMesh, newFace);
                        continue;
                    }

                    // Ignore bad cells, they are already processed
                    DelaunayTetrahedron& nextCell = cells[nextIndex];
                    if (nextCell.bad_)
                        continue;

                    if (cells[nextIndex].circumsphere_.IsInside(position) != OUTSIDE)
                    {
                        // If cell is bad too, add it to queue
                        badCells.push_back(nextIndex);
                        searchQueue.push_back(nextIndex);
                        nextCell.bad_ = true;
                    }
                    else
                    {
                        // Add new face to hole mesh
                        const unsigned nextFaceIndex = ea::find(
                            ea::begin(nextCell.tet_.neighbors_), ea::end(nextCell.tet_.neighbors_), searchQueue[i])
                            - ea::begin(nextCell.tet_.neighbors_);
                        const TetrahedralMeshHoleFace newFace = GetTetrahedronHoleFace(nextCell.tet_, nextFaceIndex, nextIndex);
                        AddTriangleToHole(holeMesh, newFace);
                    }
                }
            }
            firstCell = lastCell;
        }

        // Create new cells on top of bad cells
        if (!IsHoleMeshConnected(holeMesh))
        {
            assert(0);
            return;
        }
        while (holeMesh.size() > badCells.size())
        {
            DelaunayTetrahedron placeholder;
            placeholder.bad_ = true;
            badCells.push_back(cells.size());
            cells.push_back(placeholder);
        }

        for (unsigned i = 0; i < holeMesh.size(); ++i)
        {
            const unsigned newCellIndex = badCells[i];
            DelaunayTetrahedron& cell = cells[newCellIndex];
            const TetrahedralMeshHoleFace& face = holeMesh[i];

            for (unsigned j = 0; j < 3; ++j)
            {
                cell.tet_.indices_[j] = face.indices_[j];
                cell.tet_.neighbors_[j] = badCells[face.neighbors_[j]];
            }
            cell.tet_.indices_[3] = newIndex;
            cell.tet_.neighbors_[3] = face.cell_;
            if (face.cell_ != M_MAX_UNSIGNED)
                cells[face.cell_].tet_.neighbors_[face.face_] = newCellIndex;
            cell.bad_ = false;
            cell.circumsphere_ = CalculateTetrahedronCircumsphere(cell.tet_, mesh.vertices_);
        }
    }

    for (unsigned i = 0; i < cells.size(); ++i)
    {
        for (unsigned j = 0; j < 4; ++j)
        {
            if (cells[i].tet_.indices_[j] < 8)
            {
                cells[i].bad_ = true;
                break;
            }
        }
        if (cells[i].bad_)
        {
            for (unsigned j = 0; j < 4; ++j)
            {
                const unsigned neighborIndex = cells[i].tet_.neighbors_[j];
                if (neighborIndex != M_MAX_UNSIGNED)
                {
                    ea::replace(ea::begin(cells[neighborIndex].tet_.neighbors_),
                        ea::end(cells[neighborIndex].tet_.neighbors_), i, M_MAX_UNSIGNED);
                }
            }
        }
    }

    // Copy output
    ea::vector<unsigned> newIndices(cells.size());
    mesh.cells_.clear();
    for (unsigned i = 0; i < cells.size(); ++i)
    {
        if (cells[i].bad_)
        {
            newIndices[i] = M_MAX_UNSIGNED;
            continue;
        }

        newIndices[i] = mesh.cells_.size();
        mesh.cells_.push_back(cells[i].tet_);
    }
    for (AdjacentTetrahedron& cell : mesh.cells_)
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

    mesh.vertices_.erase(mesh.vertices_.begin(), mesh.vertices_.begin() + 8);
    for (unsigned i = 0; i < mesh.cells_.size(); ++i)
    {
        for (unsigned j = 0; j < 4; ++j)
            mesh.cells_[i].indices_[j] -= 8;
    }

    assert(IsTetrahedralMeshAdjacencyValid(mesh));
}

}

extern const char* SUBSYSTEM_CATEGORY;

GlobalIllumination::GlobalIllumination(Context* context) :
    Component(context)
{
}

GlobalIllumination::~GlobalIllumination() = default;

void GlobalIllumination::RegisterObject(Context* context)
{
    context->RegisterFactory<GlobalIllumination>(SUBSYSTEM_CATEGORY);
}

void GlobalIllumination::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    for (const AdjacentTetrahedron& cell : lightProbesMesh_.cells_)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            for (unsigned j = 0; j < 4; ++j)
            {
                const unsigned startIndex = cell.indices_[i];
                const unsigned endIndex = cell.indices_[j];
                const Vector3& startPos = lightProbesMesh_.vertices_[startIndex];
                const Vector3& endPos = lightProbesMesh_.vertices_[endIndex];
                const Vector3 midPos = startPos.Lerp(endPos, 0.5f);
                const Color startColor{ lightProbes_[startIndex].bakedLight_.EvaluateAverage() };
                const Color endColor{ lightProbes_[startIndex].bakedLight_.EvaluateAverage() };
                debug->AddLine(startPos, midPos, startColor);
                debug->AddLine(midPos, midPos, endColor);
            }
        }
    }
}

void GlobalIllumination::ResetLightProbes()
{
    lightProbes_.clear();
    lightProbesMesh_ = {};
}

void GlobalIllumination::CompileLightProbes()
{
    ResetLightProbes();

    // Collect light probes
    ea::vector<LightProbeGroup*> groups;
    GetScene()->GetComponents<LightProbeGroup>(groups, true);

    ea::vector<Vector3> positions;
    for (LightProbeGroup* group : groups)
    {
        Node* node = group->GetNode();
        for (const LightProbe& probe : group->GetLightProbes())
        {
            const Vector3 worldPosition = node->LocalToWorld(probe.position_);
            positions.push_back(worldPosition);
            lightProbes_.push_back(LightProbe{ worldPosition, probe.bakedLight_ });
        }
    }

    // Build mesh
    if (lightProbes_.empty())
        return;

    // Add padding to avoid vertex collision
    BoundingBox boundingBox(positions.data(), positions.size());
    boundingBox.min_ -= Vector3::ONE;
    boundingBox.max_ += Vector3::ONE;
    InitializeTetrahedralMesh(lightProbesMesh_, boundingBox);
    AddTetrahedralMeshVertices(lightProbesMesh_, positions);
}


}
