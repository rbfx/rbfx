// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Slerp.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class SlerpInstance final : public Slerp::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Quaternion>& x,
        const SparseSpan<Quaternion>& y, const SparseSpan<float>& t, const SparseSpan<Quaternion>& out) const
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = x[i].Slerp(y[i], t[i]);
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
