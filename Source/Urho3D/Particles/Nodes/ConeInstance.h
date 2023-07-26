//
// Copyright (c) 2021-2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "Cone.h"
#include "../Emitter.h"

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
