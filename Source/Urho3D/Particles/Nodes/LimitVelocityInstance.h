// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "LimitVelocity.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class LimitVelocityInstance final : public LimitVelocity::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& vel,
        const SparseSpan<float>& limit, const SparseSpan<Vector3>& result)
    {
        const float dampen = static_cast<LimitVelocity*>(GetGraphNode())->GetDampen();
        if (dampen <= 1e-6f || context.timeStep_ < 1e-6f)
        {
            for (unsigned i = 0; i < numParticles; ++i)
            {
                result[i] = vel[i];
            }
        }
        else
        {
            const auto t = 1.0f - powf(1.0f - dampen, 20.0f * context.timeStep_);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                const float speed = vel[i].Length();
                const float limitVal = limit[i];
                if (speed > limitVal + 1e-6f)
                {
                    result[i] = vel[i] * (Urho3D::Lerp(speed, limitVal, t) / speed);
                }
            }
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
