// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Emitter.h"
#include "Sphere.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class SphereInstance final : public Sphere::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& pos,
        const SparseSpan<Vector3>& vel)
    {
        const Sphere* sphere = static_cast<Sphere*>(GetGraphNode());
        const Matrix3x4 m = Matrix3x4(sphere->GetTranslation(), sphere->GetRotation(), sphere->GetScale());
        const Matrix3 md = m.ToMatrix3();

        for (unsigned i = 0; i < numParticles; ++i)
        {
            Vector3 p, v;
            Generate(p, v);
            pos[i] = m * p;
            vel[i] = md * v;
        }
    }

    void Generate(Vector3& pos, Vector3& vel) const
    {
        const Sphere* sphere = static_cast<Sphere*>(GetGraphNode());

        Vector3 direction(Urho3D::Random(2.0f) - 1.0f, Urho3D::Random(2.0f) - 1.0f, Urho3D::Random(2.0f) - 1.0f);
        direction.Normalize();

        float r = sphere->GetRadius();
        float radiusThickness_ = sphere->GetRadiusThickness();
        auto emitFrom_ = static_cast<EmitFrom>(sphere->GetFrom());
        if (radiusThickness_ > 0.0f && emitFrom_ != EmitFrom::Surface)
        {
            r *= 1.0f - Urho3D::Random() * radiusThickness_;
        }
        switch (emitFrom_)
        {
        case EmitFrom::Base:
            vel = direction;
            pos = Vector3::ZERO;
            break;
        case EmitFrom::Surface:
            vel = direction;
            pos = direction * sphere->GetRadius();
            break;
        default:
            vel = direction;
            pos = direction * Pow(Urho3D::Random(), 1.0f / 3.0f) * 0.5f;
            break;
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
