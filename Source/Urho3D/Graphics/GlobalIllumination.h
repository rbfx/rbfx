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

#include "../Graphics/LightProbeGroup.h"
#include "../Math/Matrix3.h"
#include "../Math/Sphere.h"
#include "../Math/Vector3.h"
#include "../Scene/Component.h"

namespace Urho3D
{

/// Tetrahedron with adjacency information.
struct AdjacentTetrahedron
{
    /// Indices of tetrahedron vertices.
    unsigned indices_[4]{};
    /// Indices of neighbor tetrahedrons. M_MAX_UNSIGNED if missing.
    unsigned neighbors_[4]{ M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED };
    /// Pre-computed matrix for calculating barycentric coordinates.
    Matrix3 barycentricInverse_;
};

/// Tetrahedral mesh.
struct TetrahedralMesh
{
    /// Vertices.
    ea::vector<Vector3> vertices_;
    /// Cells.
    ea::vector<AdjacentTetrahedron> cells_;
};

/// Global illumination manager.
class URHO3D_API GlobalIllumination : public Component
{
    URHO3D_OBJECT(GlobalIllumination, Component);

public:
    /// Construct.
    explicit GlobalIllumination(Context* context);
    /// Destruct.
    ~GlobalIllumination() override;
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);
    /// Visualize the component as debug geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Reset light probes.
    void ResetLightProbes();
    /// Compile all enabled light probe groups in the scene.
    void CompileLightProbes();

    /// Sample light probe mesh. Return barycentric coordinates and tetrahedron.
    Vector4 SampleLightProbeMesh(const Vector3& position, unsigned& hint) const;
    /// Sample average ambient lighting.
    Vector3 SampleAverageAmbient(const Vector3& position, unsigned& hint) const;

private:
    /// Light probes mesh.
    TetrahedralMesh lightProbesMesh_;
    /// Light probes collection.
    LightProbeCollection lightProbesCollection_;
};

}
