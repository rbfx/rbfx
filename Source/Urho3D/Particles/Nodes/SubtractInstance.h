// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
template <typename Value0, typename Value1, typename Value2> struct SubtractInstance
{
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Value0>& x,
        const SparseSpan<Value1>& y, const SparseSpan<Value2>& out)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = x[i] - y[i];
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
