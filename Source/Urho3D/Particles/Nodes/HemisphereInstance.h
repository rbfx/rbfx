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

#include "Hemisphere.h"
#include "../Emitter.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class HemisphereInstance final : public Hemisphere::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& pos,
        const SparseSpan<Vector3>& vel)
    {
        const Hemisphere* hemisphere = static_cast<Hemisphere*>(GetGraphNode());
        const Matrix3x4 m = Matrix3x4(hemisphere->GetTranslation(), hemisphere->GetRotation(), hemisphere->GetScale());
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
        const Hemisphere* hemisphere = static_cast<Hemisphere*>(GetGraphNode());

        Vector3 direction(Urho3D::Random(2.0f) - 1.0f, Urho3D::Random(2.0f) - 1.0f, Urho3D::Random(2.0f) - 1.0f);
        direction.Normalize();
        direction.z_ = Abs(direction.z_);

        float r = hemisphere->GetRadius();
        float radiusThickness_ = hemisphere->GetRadiusThickness();
        auto emitFrom_ = static_cast<EmitFrom>(hemisphere->GetFrom());

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
            pos = direction * hemisphere->GetRadius();
            break;
        default:
            vel = direction;
            pos = direction * hemisphere->GetRadius() * Pow(Urho3D::Random(), 1.0f / 3.0f) * 0.5f;
            break;
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
