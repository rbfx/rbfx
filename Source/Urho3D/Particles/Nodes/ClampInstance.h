// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
template <typename Value0, typename Value1> struct ClampInstance
{
    void operator()(
        const UpdateContext& context, unsigned numParticles, const SparseSpan<Value0>& x, const SparseSpan<Value1>& out)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = Urho3D::VectorClamp(x[i], Value1::ZERO, Value1::ONE);
        }
    }
};

template <> struct ClampInstance<float, float>
{
    void operator()(
        const UpdateContext& context, unsigned numParticles, const SparseSpan<float>& x, const SparseSpan<float>& out)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = Urho3D::Clamp(x[i], 0.0f, 1.0f);
        }
    }
};
} // namespace ParticleGraphNodes

} // namespace Urho3D
