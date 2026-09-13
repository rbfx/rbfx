// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/Nodes/ApplyForce.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
template <typename... Value> struct BreakInstance
{
};

template <> struct BreakInstance<Vector3, float, float, float>
{
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& vec,
        const SparseSpan<float>& x, const SparseSpan<float>& y, const SparseSpan<float>& z)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            x[i] = vec[i].x_;
            y[i] = vec[i].y_;
            z[i] = vec[i].z_;
        }
    }
};

template <> struct BreakInstance<Vector2, float, float>
{
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector2>& vec,
        const SparseSpan<float>& x, const SparseSpan<float>& y)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            x[i] = vec[i].x_;
            y[i] = vec[i].y_;
        }
    }
};

template <> struct BreakInstance<Quaternion, float, float, float, float>
{
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Quaternion>& vec,
        const SparseSpan<float>& x, const SparseSpan<float>& y, const SparseSpan<float>& z, const SparseSpan<float>& w)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            x[i] = vec[i].x_;
            y[i] = vec[i].y_;
            z[i] = vec[i].z_;
            w[i] = vec[i].w_;
        }
    }
};

template <> struct BreakInstance<Quaternion, Vector3, float>
{
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Quaternion>& vec,
        const SparseSpan<Vector3>& axis, const SparseSpan<float>& angle)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            const Quaternion q = vec[i];
            angle[i] = q.Angle();
            axis[i] = q.Axis();
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
