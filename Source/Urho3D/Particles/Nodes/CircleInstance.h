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
