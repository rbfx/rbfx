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
#include "../Graphics/LightProbeGroup.h"
#include "../Scene/Scene.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

namespace
{

/// Calculate circumsphere of the tetrahedron.
Sphere CalculateTetrahedronCircumsphere(const TetrahedralMesh& mesh, unsigned cellIndex)
{
    const AdjacentTetrahedron& cell = mesh.cells_[cellIndex];
    const Vector3 p0 = mesh.vertices_[cell.indices_[0]];
    const Vector3 p1 = mesh.vertices_[cell.indices_[1]];
    const Vector3 p2 = mesh.vertices_[cell.indices_[2]];
    const Vector3 p3 = mesh.vertices_[cell.indices_[3]];
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

    return { center, r0.Length() };
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

    static const int neighbors[numTetrahedrons][4] =
    {
        { 4, -1, -1, -1 },
        { 4, -1, -1, -1 },
        { 4, -1, -1, -1 },
        { 4, -1, -1, -1 },
        { 3, 2, 1, 0 }, // Tetrahedrons with corners at (6, 5, 3, 0)
    };

    // Initialize vertices
    mesh.vertices_.resize(numVertices);
    for (unsigned i = 0; i < numVertices; ++i)
        mesh.vertices_[i] = volume.min_ + volume.Size() * offsets[i];

    // Initialize cells
    mesh.cells_.resize(numTetrahedrons);
    mesh.circumspheres_.resize(numTetrahedrons);
    for (unsigned i = 0; i < numTetrahedrons; ++i)
    {
        AdjacentTetrahedron cell;
        for (unsigned j = 0; j < 4; ++j)
        {
            cell.indices_[j] = indices[i][j];
            cell.neighbors_[j] = neighbors[i][j];
        }
        mesh.cells_[i] = cell;
        mesh.circumspheres_[i] = CalculateTetrahedronCircumsphere(mesh, i);
    }
};

/// Add vertices to tetrahedral mesh.
void AddTetrahedralMeshVertices(TetrahedralMesh& mesh, ea::span<const Vector3> positions)
{
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
                const Vector3& start = lightProbesMesh_.vertices_[cell.indices_[i]];
                const Vector3& end = lightProbesMesh_.vertices_[cell.indices_[j]];
                debug->AddLine(start, end, Color::YELLOW);
            }
        }
    }
}

void GlobalIllumination::ResetLightProbes()
{
    lightProbesMesh_ = {};
}

void GlobalIllumination::CompileLightProbes()
{
    ResetLightProbes();

    // Collect light probes
    ea::vector<LightProbeGroup*> groups;
    GetScene()->GetComponents<LightProbeGroup>(groups, true);

    ea::vector<Vector3> positions;
    ea::vector<LightProbe> probes;

    for (LightProbeGroup* group : groups)
    {
        Node* node = group->GetNode();
        for (const LightProbe& probe : group->GetLightProbes())
        {
            const Vector3 worldPosition = node->LocalToWorld(probe.position_);
            positions.push_back(worldPosition);
            probes.push_back(LightProbe{ worldPosition, probe.bakedLight_ });
        }
    }

    // Build mesh
    if (probes.empty())
        return;

    // Add padding to avoid vertex collision
    BoundingBox boundingBox(positions.data(), positions.size());
    boundingBox.min_ -= Vector3::ONE;
    boundingBox.min_ += Vector3::ONE;
    InitializeTetrahedralMesh(lightProbesMesh_, boundingBox);
    AddTetrahedralMeshVertices(lightProbesMesh_, positions);
}


}
