// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "EmitterRotation.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class EmitterRotationInstance final : public EmitterRotation::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Quaternion>& pin0)
    {
        auto* node = GetNode();
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin0[i] = node->GetWorldRotation();
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
