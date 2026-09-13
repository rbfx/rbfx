// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/Nodes/ApplyForce.h"
#if URHO3D_PHYSICS
#include "Urho3D/Physics/PhysicsWorld.h"
#endif
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Math/Ray.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{
class ParticleGraphSystem;
class PhysicsWorld;

namespace ParticleGraphNodes
{

class BounceInstance final : public Bounce::InstanceBase
{
public:
    void RayCastAndBounce(
        const UpdateContext& context, Node* node, PhysicsWorld* physics, Vector3& pos, Vector3& velocity);

    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& pin0,
        const SparseSpan<Vector3>& pin1, const SparseSpan<Vector3>& pin2, const SparseSpan<Vector3>& pin3)
    {
#if URHO3D_PHYSICS
        auto node = GetNode();
        auto physics = GetScene()->GetComponent<PhysicsWorld>();
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin2[i] = pin0[i];
            pin3[i] = pin1[i];
            RayCastAndBounce(context, node, physics, pin2[i], pin3[i]);
        }
#endif
    }
};

inline void BounceInstance::RayCastAndBounce(
    const UpdateContext& context, Node* node, PhysicsWorld* physics, Vector3& pos,
    Vector3& velocity)
{
#if URHO3D_PHYSICS
    if (physics)
    {
        auto bounce = static_cast<Bounce*>(GetGraphNode());

        const auto gravity = physics->GetGravity();
        velocity += gravity * context.timeStep_;
        PhysicsRaycastResult res;
        Vector3 offset = velocity * context.timeStep_;

        auto distance = offset.Length();
        if (distance > 1e-6f)
        {
            auto wp = node->LocalToWorld(pos);
            const float radius = 0.1f;
            physics->SphereCast(res, Ray(wp, offset * (1.0f / distance)), radius, distance);
            if (res.body_)
            {
                wp = wp.Lerp(res.position_, 0.99f);
                pos = node->WorldToLocal(wp);
                const auto bounceScale = (1.0f + bounce->GetBounceFactor()) * velocity.DotProduct(res.normal_);
                velocity -= res.normal_ * bounceScale;
                if (bounce->GetDampen() > 0.0f)
                {
                    velocity *= (1.0f - bounce->GetDampen());
                }
            }
            else
            {
                pos += offset;
            }
        }
        return;
    }
#endif
    pos += velocity * context.timeStep_;
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
