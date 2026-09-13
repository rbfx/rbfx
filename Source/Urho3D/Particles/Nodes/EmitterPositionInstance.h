// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "EmitterPosition.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class EmitterPositionInstance final : public EmitterPosition::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& pin0)
    {
        auto* node = GetNode();
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin0[i] = node->GetWorldPosition();
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
