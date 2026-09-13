// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
template <typename... Values> struct MakeInstance
{
};

template <typename Value0, typename Value1, typename Value2> struct MakeInstance<Value0, Value1, Value2>
{
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Value0>& x,
        const SparseSpan<Value1>& y, const SparseSpan<Value2>& out)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = Value2(x[i], y[i]);
        }
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3>
struct MakeInstance<Value0, Value1, Value2, Value3>
{
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Value0>& x,
        const SparseSpan<Value1>& y, const SparseSpan<Value2>& z, const SparseSpan<Value3>& out)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = Value3(x[i], y[i], z[i]);
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
