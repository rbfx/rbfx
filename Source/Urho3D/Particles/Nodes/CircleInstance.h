// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Circle.h"
#include "../Emitter.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class CircleInstance final : public Circle::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& pos,
        const SparseSpan<Vector3>& vel)
    {
        const Circle* circle = static_cast<Circle*>(GetGraphNode());
        const Matrix3x4 m = Matrix3x4(circle->GetTranslation(), circle->GetRotation(), circle->GetScale());
        const Matrix3 md = m.RotationMatrix();

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
        const Circle* circle = static_cast<Circle*>(GetGraphNode());

        const float angle = Urho3D::Random(360.0f);
        const float cosinus = Cos(angle);
        const float sinus = Sin(angle);
        const Vector3 direction = Vector3(cosinus, sinus, 0.0f);

        float r = circle->GetRadius();
        if (circle->GetRadiusThickness() > 0.0f)
        {
            r *= 1.0f - Urho3D::Random() * circle->GetRadiusThickness();
        }
        vel = direction;
        pos = Vector3(cosinus * (r), sinus * (r), 0.0f);
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
