// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Emit.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class EmitInstance final : public Emit::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<float>& span)
    {
        // Can't use iterator here as it may use ScalarSpan with infinite iterator.
        float sum = 0.0f;
        for (unsigned i = 0; i < numParticles; ++i)
        {
            sum += span[i];
        }
        context.layer_->EmitNewParticles(sum);
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
