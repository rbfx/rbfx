// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Destroy.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class DestroyInstance final : public Destroy::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<bool>& pin0)
    {
        // Iterate all particles even if all pins are scalar.
        for (unsigned i = 0; i < context.indices_.size(); ++i)
        {
            if (pin0[i])
            {
                context.layer_->MarkForDeletion(i);
            }
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
