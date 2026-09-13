// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/Nodes/Box.h"
#include "Urho3D/Particles/Emitter.h"

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
