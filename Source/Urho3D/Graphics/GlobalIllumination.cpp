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
Sphere CalculateTetrahedronCircumsphere(const Tetrahedron& cell, const ea::vector<Vector3>& vertices)
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
    const Vector3 num = d01 * u2.CrossProduct(u3) + d02 * u3.CrossProduct(u1) + d03 * u1.CrossProduct(u2);
    const float den = 2 * u1.DotProduct(u2.CrossProduct(u3));
    assert(Abs(den) > M_EPSILON);
    const Vector3 r0 = num / den;
    const Vector3 center = p0 + r0;

    const float eps = M_LARGE_EPSILON;
    const float radius = ea::max({ (p1 - center).Length(), (p2 - center).Length(), (p3 - center).Length()});
    //assert(Equals((p1 - center).Length(), r0.Length(), eps / 2));
    //assert(Equals((p2 - center).Length(), r0.Length(), eps / 2));
    //assert(Equals((p3 - center).Length(), r0.Length(), eps / 2));

    return { center, radius + eps };
}

/// Return barycentric coordinates withing tetrahedron.
Vector4 GetBarycentricCoords(const TetrahedralMesh& mesh, unsigned cellIndex, const Vector3& position)
{
    const Tetrahedron& cell = mesh.tetrahedrons_[cellIndex];
    const Vector3& basePosition = mesh.vertices_[cell.indices_[0]];
    const Vector3 coords = cell.matrix_ * (position - basePosition);
    return { 1.0f - coords.x_ - coords.y_ - coords.z_, coords.x_, coords.y_, coords.z_ };
};

/// Tetrahedron for Delaunay triangulation.
struct DelaunayTetrahedron
{
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
TetrahedralMeshHoleFace GetTetrahedronHoleFace(const Tetrahedron& tet, unsigned faceIndex, unsigned cellIndex)
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
    for (unsigned cellIndex = 0; cellIndex < mesh.tetrahedrons_.size(); ++cellIndex)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            const unsigned neighborIndex = mesh.tetrahedrons_[cellIndex].neighbors_[i];
            if (neighborIndex != M_MAX_UNSIGNED)
            {
                const Tetrahedron& neighborCell = mesh.tetrahedrons_[neighborIndex];
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
    ea::vector<DelaunayTetrahedron> auxilary(mesh.tetrahedrons_.size());
    for (unsigned i = 0; i < mesh.tetrahedrons_.size(); ++i)
    {
        mesh.tetrahedrons_[i] = mesh.tetrahedrons_[i];
        auxilary[i].circumsphere_ = CalculateTetrahedronCircumsphere(mesh.tetrahedrons_[i], mesh.vertices_);
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
        for (unsigned cellIndex = 0; cellIndex < mesh.tetrahedrons_.size(); ++cellIndex)
        {
            DelaunayTetrahedron& cell = auxilary[cellIndex];
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
                const Tetrahedron& tetrahedron = mesh.tetrahedrons_[searchQueue[i]];
                const DelaunayTetrahedron& cell = auxilary[searchQueue[i]];

                // Process neighbors
                for (unsigned j = 0; j < 4; ++j)
                {
                    const unsigned nextIndex = tetrahedron.neighbors_[j];
                    if (nextIndex == M_MAX_UNSIGNED)
                    {
                        // Missing neighbor closes hole
                        const TetrahedralMeshHoleFace newFace = GetTetrahedronHoleFace(tetrahedron, j, M_MAX_UNSIGNED);
                        AddTriangleToHole(holeMesh, newFace);
                        continue;
                    }

                    // Ignore bad cells, they are already processed
                    Tetrahedron& nextTetrahedron = mesh.tetrahedrons_[nextIndex];
                    DelaunayTetrahedron& nextCell = auxilary[nextIndex];
                    if (nextCell.bad_)
                        continue;

                    if (auxilary[nextIndex].circumsphere_.IsInside(position) != OUTSIDE)
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
                            ea::begin(nextTetrahedron.neighbors_), ea::end(nextTetrahedron.neighbors_), searchQueue[i])
                            - ea::begin(nextTetrahedron.neighbors_);
                        const TetrahedralMeshHoleFace newFace = GetTetrahedronHoleFace(nextTetrahedron, nextFaceIndex, nextIndex);
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
            badCells.push_back(mesh.tetrahedrons_.size());
            mesh.tetrahedrons_.push_back();
            auxilary.push_back(placeholder);
        }

        for (unsigned i = 0; i < holeMesh.size(); ++i)
        {
            const unsigned newCellIndex = badCells[i];
            DelaunayTetrahedron& cell = auxilary[newCellIndex];
            Tetrahedron& tetrahedron = mesh.tetrahedrons_[newCellIndex];
            const TetrahedralMeshHoleFace& face = holeMesh[i];

            for (unsigned j = 0; j < 3; ++j)
            {
                tetrahedron.indices_[j] = face.indices_[j];
                tetrahedron.neighbors_[j] = badCells[face.neighbors_[j]];
            }
            tetrahedron.indices_[3] = newIndex;
            tetrahedron.neighbors_[3] = face.cell_;
            if (face.cell_ != M_MAX_UNSIGNED)
                mesh.tetrahedrons_[face.cell_].neighbors_[face.face_] = newCellIndex;
            cell.bad_ = false;
            cell.circumsphere_ = CalculateTetrahedronCircumsphere(tetrahedron, mesh.vertices_);
        }
    }

    for (unsigned i = 0; i < mesh.tetrahedrons_.size(); ++i)
    {
        for (unsigned j = 0; j < 4; ++j)
        {
            if (mesh.tetrahedrons_[i].indices_[j] < 8)
            {
                auxilary[i].bad_ = true;
                break;
            }
        }
        if (auxilary[i].bad_)
        {
            for (unsigned j = 0; j < 4; ++j)
            {
                const unsigned neighborIndex = mesh.tetrahedrons_[i].neighbors_[j];
                if (neighborIndex != M_MAX_UNSIGNED)
                {
                    ea::replace(ea::begin(mesh.tetrahedrons_[neighborIndex].neighbors_),
                        ea::end(mesh.tetrahedrons_[neighborIndex].neighbors_), i, M_MAX_UNSIGNED);
                }
            }
        }
    }

    // Copy output
    ea::vector<Tetrahedron> cells = ea::move(mesh.tetrahedrons_);
    ea::vector<unsigned> newIndices(cells.size());
    mesh.tetrahedrons_.clear();
    for (unsigned i = 0; i < cells.size(); ++i)
    {
        if (auxilary[i].bad_)
        {
            newIndices[i] = M_MAX_UNSIGNED;
            continue;
        }

        newIndices[i] = mesh.tetrahedrons_.size();
        mesh.tetrahedrons_.push_back(cells[i]);
    }
    for (Tetrahedron& cell : mesh.tetrahedrons_)
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
    for (unsigned i = 0; i < mesh.tetrahedrons_.size(); ++i)
    {
        for (unsigned j = 0; j < 4; ++j)
            mesh.tetrahedrons_[i].indices_[j] -= 8;
    }

    for (unsigned i = 0; i < mesh.tetrahedrons_.size(); ++i)
    {
        Tetrahedron& cell = mesh.tetrahedrons_[i];
        const Vector3 p0 = mesh.vertices_[cell.indices_[0]];
        const Vector3 p1 = mesh.vertices_[cell.indices_[1]];
        const Vector3 p2 = mesh.vertices_[cell.indices_[2]];
        const Vector3 p3 = mesh.vertices_[cell.indices_[3]];
        const Vector3 u1 = p1 - p0;
        const Vector3 u2 = p2 - p0;
        const Vector3 u3 = p3 - p0;
        cell.matrix_ = Matrix3(u1.x_, u2.x_, u3.x_, u1.y_, u2.y_, u3.y_, u1.z_, u2.z_, u3.z_).Inverse();
        assert(GetBarycentricCoords(mesh, i, p0).Equals(Vector4(1, 0, 0, 0)));
        assert(GetBarycentricCoords(mesh, i, p1).Equals(Vector4(0, 1, 0, 0)));
        assert(GetBarycentricCoords(mesh, i, p2).Equals(Vector4(0, 0, 1, 0)));
        assert(GetBarycentricCoords(mesh, i, p3).Equals(Vector4(0, 0, 0, 1)));
    }

    assert(IsTetrahedralMeshAdjacencyValid(mesh));
}

void GenerateHullNormals(TetrahedralMesh& mesh)
{
    mesh.hullNormals_.resize(mesh.vertices_.size());
    for (const Tetrahedron& cell : mesh.tetrahedrons_)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            if (cell.neighbors_[i] != M_MAX_UNSIGNED)
                continue;

            unsigned triangle[3];
            unsigned baseIndex;
            unsigned k = 0;
            for (unsigned j = 0; j < 4; ++j)
            {
                if (i != j)
                    triangle[k++] = cell.indices_[j];
                else
                    baseIndex = cell.indices_[j];
            }

            const Vector3 p0 = mesh.vertices_[baseIndex];
            const Vector3 p1 = mesh.vertices_[triangle[0]];
            const Vector3 p2 = mesh.vertices_[triangle[1]];
            const Vector3 p3 = mesh.vertices_[triangle[2]];
            const Vector3 orientation = p1 - p0;
            const Vector3 cross = (p2 - p1).CrossProduct(p3 - p1);
            const Vector3 normal = cross.DotProduct(orientation) >= 0.0f ? cross : -cross;

            for (unsigned j = 0; j < 3; ++j)
                mesh.hullNormals_[triangle[j]] += normal;
        }
    }

    for (Vector3& normal : mesh.hullNormals_)
    {
        if (normal != Vector3::ZERO)
            normal.Normalize();
    }
}

}

void TetrahedralMesh::Define(ea::span<const Vector3> positions, float padding)
{
    BoundingBox boundingBox(positions.data(), positions.size());
    boundingBox.min_ -= Vector3::ONE * padding;
    boundingBox.max_ += Vector3::ONE * padding;
    InitializeSuperMesh(boundingBox);

    AddTetrahedralMeshVertices(*this, positions);
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
    for (const Tetrahedron& cell : lightProbesMesh_.tetrahedrons_)
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
                const Color startColor = lightProbesCollection_.lightProbes_[startIndex].GetDebugColor();
                const Color endColor = lightProbesCollection_.lightProbes_[startIndex].GetDebugColor();
                debug->AddLine(startPos, midPos, startColor);
                debug->AddLine(midPos, midPos, endColor);
            }
        }
    }

    for (unsigned i = 0; i < lightProbesMesh_.vertices_.size(); ++i)
    {
        const Vector3& pos = lightProbesMesh_.vertices_[i];
        const Vector3& normal = lightProbesMesh_.hullNormals_[i];
        if (normal != Vector3::ZERO)
            debug->AddLine(pos, pos + normal, Color::YELLOW);
    }
}

void GlobalIllumination::ResetLightProbes()
{
    lightProbesCollection_.Clear();
    lightProbesMesh_ = {};
}

void GlobalIllumination::CompileLightProbes()
{
    ResetLightProbes();

    // Collect light probes
    LightProbeGroup::CollectLightProbes(GetScene(), lightProbesCollection_);
    if (lightProbesCollection_.Empty())
        return;

    // Add padding to avoid vertex collision
    lightProbesMesh_.Define(lightProbesCollection_.worldPositions_);
    GenerateHullNormals(lightProbesMesh_);
}

Vector4 GlobalIllumination::SampleLightProbeMesh(const Vector3& position, unsigned& hint) const
{
    const unsigned maxIters = lightProbesCollection_.Size();
    if (hint >= maxIters)
        hint = 0;

    Vector4 weights;
    for (unsigned i = 0; i < maxIters; ++i)
    {
        weights = GetBarycentricCoords(lightProbesMesh_, hint, position);
        if (weights.x_ >= 0.0f && weights.y_ >= 0.0f && weights.z_ >= 0.0f && weights.w_ >= 0.0f)
            return weights;

        if (weights.x_ < weights.y_ && weights.x_ < weights.z_ && weights.x_ < weights.w_)
            hint = lightProbesMesh_.tetrahedrons_[hint].neighbors_[0];
        else if (weights.y_ < weights.z_ && weights.y_ < weights.w_)
            hint = lightProbesMesh_.tetrahedrons_[hint].neighbors_[1];
        else if (weights.z_ < weights.w_)
            hint = lightProbesMesh_.tetrahedrons_[hint].neighbors_[2];
        else
            hint = lightProbesMesh_.tetrahedrons_[hint].neighbors_[3];

        // TODO(glow): Need to handle it more gracefully
        if (hint == M_MAX_UNSIGNED)
            break;
    }
    return weights;
}

SphericalHarmonicsDot9 GlobalIllumination::SampleAmbientSH(const Vector3& position, unsigned& hint) const
{
    // TODO(glow): Use real ambient here
    const Vector4& weights = SampleLightProbeMesh(position, hint);
    if (hint >= lightProbesMesh_.tetrahedrons_.size())
        return SphericalHarmonicsDot9{};

    const Tetrahedron& tetrahedron = lightProbesMesh_.tetrahedrons_[hint];

    SphericalHarmonicsDot9 sh;
    for (unsigned i = 0; i < 4; ++i)
        sh += lightProbesCollection_.lightProbes_[tetrahedron.indices_[i]].bakedLight_ * weights[i];
    return sh;
}

Vector3 GlobalIllumination::SampleAverageAmbient(const Vector3& position, unsigned& hint) const
{
    // TODO(glow): Use real ambient here
    const Vector4& weights = SampleLightProbeMesh(position, hint);
    if (hint >= lightProbesMesh_.tetrahedrons_.size())
        return Vector3::ZERO;

    const Tetrahedron& tetrahedron = lightProbesMesh_.tetrahedrons_[hint];

    Vector3 ambient;
    for (unsigned i = 0; i < 4; ++i)
        ambient += lightProbesCollection_.lightProbes_[tetrahedron.indices_[i]].bakedLight_.EvaluateAverage() * weights[i];
    ambient.x_ = Pow(ambient.x_, 1 / 2.2f);
    ambient.y_ = Pow(ambient.y_, 1 / 2.2f);
    ambient.z_ = Pow(ambient.z_, 1 / 2.2f);
    return ambient;
}

}
