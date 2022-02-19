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
    template <typename Pos, typename Vel>
    void operator()(UpdateContext& context, unsigned numParticles, Pos pos, Vel vel)
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
