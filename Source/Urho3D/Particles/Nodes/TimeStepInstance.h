// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "TimeStep.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class TimeStepInstance final : public TimeStep::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<float>& pin0)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin0[i] = context.timeStep_;
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
