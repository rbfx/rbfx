// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Expire.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class ExpireInstance final : public Expire::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<float>& time,
        const SparseSpan<float>& lifetime)
    {
        // Iterate all particles even if all pins are scalar.
        for (unsigned i = 0; i < context.indices_.size(); ++i)
        {
            if (time[i] >= lifetime[i])
            {
                context.layer_->MarkForDeletion(i);
            }
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
