// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/Nodes/NormalizedEffectTime.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class NormalizedEffectTimeInstance final : public NormalizedEffectTime::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<float>& pin0)
    {
        const float value = context.time_ / GetLayer()->GetDuration();
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin0[i] = value;
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
