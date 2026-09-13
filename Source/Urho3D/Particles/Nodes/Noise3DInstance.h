// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Noise3D.h"
#include "../../Math/PerlinNoise.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class Noise3DInstance final : public Noise3D::InstanceBase
{
public:
    Noise3DInstance();

    void Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer) override;

    void operator()(
        const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& x, const SparseSpan<float>& out)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = Generate(x[i]);
        }
    }

    float Generate(const Vector3& pos) const;

    PerlinNoise noise_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
