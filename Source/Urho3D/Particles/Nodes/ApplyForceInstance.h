// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "ApplyForce.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class ApplyForceInstance final : public ApplyForce::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& vel,
        const SparseSpan<Vector3>& force, const SparseSpan<Vector3>& result) const
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            result[i] = vel[i] + force[i] * context.timeStep_;
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
