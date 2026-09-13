// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/Nodes/CurlNoise3D.h"
#include "Urho3D/Math/PerlinNoise.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class CurlNoise3DInstance final : public CurlNoise3D::InstanceBase
{
public:
    CurlNoise3DInstance();

    void Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer) override;

    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& x,
        const SparseSpan<Vector3>& out)
    {
        scrollPos_ += context.timeStep_;
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = Generate(x[i]);
        }
    }

    Vector3 Generate(const Vector3& pos);

    PerlinNoise noise_;
    double scrollPos_{};
    //FastNoiseLite noise_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
