// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Math/Ray.h"
#include "../../Scene/Node.h"
#include "../../Scene/Scene.h"
#include "ApplyForce.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class MoveInstance final : public Move::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& pin0,
        const SparseSpan<Vector3>& pin1, const SparseSpan<Vector3>& pin2)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin2[i] = pin0[i] + context.timeStep_ * pin1[i];
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
