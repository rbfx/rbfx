// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/Nodes/Cone.h"
#include "Urho3D/Particles/Emitter.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class ConeInstance final : public Cone::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& pos,
        const SparseSpan<Vector3>& vel)
    {
        const Cone* cone = static_cast<Cone*>(GetGraphNode());
        const Matrix3x4 m = Matrix3x4(cone->GetTranslation(), cone->GetRotation(), cone->GetScale());
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
        const Cone* cone = static_cast<Cone*>(GetGraphNode());

        const float angle = Urho3D::Random(360.0f);
        const float radius = Sqrt(Urho3D::Random()) * Sin(Min(Max(cone->GetAngle(), 0.0f), 89.999f));
        const float height = Sqrt(1.0f - radius * radius);
        const float cosinus = Cos(angle);
        const float sinus = Sin(angle);
        const Vector3 direction = Vector3(cosinus * radius, sinus * radius, height);

        float r = cone->GetRadius();
        if (cone->GetRadiusThickness() > 0.0f && static_cast<EmitFrom>(cone->GetFrom()) != EmitFrom::Surface)
        {
            r *= 1.0f - Urho3D::Random() * cone->GetRadiusThickness();
        }
        switch (static_cast<EmitFrom>(cone->GetFrom()))
        {
        case EmitFrom::Base:
            vel = direction;
            pos = Vector3(cosinus * (r), sinus * (r), 0.0f);
            break;
        default:
            vel = direction;
            pos = direction * Urho3D::Random(cone->GetLength()) + Vector3(cosinus * r, sinus * r, 0.0f);
            break;
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
