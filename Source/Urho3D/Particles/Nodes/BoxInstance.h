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

#include "Box.h"
#include "../Emitter.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class BoxInstance final : public Box::InstanceBase
{
public:
    void operator()(
        const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& pos, const SparseSpan<Vector3>& vel)
    {
        const Box* box = static_cast<Box*>(GetGraphNode());
        const Matrix3x4 m = Matrix3x4(box->GetTranslation(), box->GetRotation(), box->GetScale());
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
        const Box* box = static_cast<Box*>(GetGraphNode());

        switch (static_cast<EmitFrom>(box->GetFrom()))
        {
        case EmitFrom::Edge:
        {
            const float x = Urho3D::Random(-1.0f, 1.0f);
            switch (Urho3D::Random(12))
            {
            case 0: pos = Vector3{x, -1.0f, -1.0f}; break;
            case 1: pos = Vector3{x, -1.0f, +1.0f}; break;
            case 2: pos = Vector3{x, +1.0f, -1.0f}; break;
            case 3: pos = Vector3{x, +1.0f, +1.0f}; break;
            case 4: pos = Vector3{-1.0f, x, -1.0f}; break;
            case 5: pos = Vector3{-1.0f, x, +1.0f}; break;
            case 6: pos = Vector3{+1.0f, x, -1.0f}; break;
            case 7: pos = Vector3{+1.0f, x, +1.0f}; break;
            case 8: pos = Vector3{-1.0f, -1.0f, x}; break;
            case 9: pos = Vector3{-1.0f, +1.0f, x}; break;
            case 10: pos = Vector3{+1.0f, -1.0f, x}; break;
            case 11: pos = Vector3{+1.0f, +1.0f, x}; break;
            }
            vel = pos.Normalized();
            break;
        }
        case EmitFrom::Surface:
        {
            const float x = Urho3D::Random(-1.0f, 1.0f);
            const float y = Urho3D::Random(-1.0f, 1.0f);
            switch (Urho3D::Random(6))
            {
            case 0: pos = Vector3{x, y, -1.0f}; break;
            case 1: pos = Vector3{x, y, 1.0f}; break;
            case 2: pos = Vector3{x, -1.0f, y}; break;
            case 3: pos = Vector3{x, 1.0f, y}; break;
            case 4: pos = Vector3{-1.0f, x, y}; break;
            case 5: pos = Vector3{1.0f, x, y}; break;
            }
            vel = pos.Normalized();
            break;
        }
        default:
        {
            pos = Vector3{Urho3D::Random(-1.0f, 1.0f), Urho3D::Random(-1.0f, 1.0f), Urho3D::Random(-1.0f, 1.0f)};
            vel = pos.Normalized();
            break;
        }
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
