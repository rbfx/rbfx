//
// Copyright (c) 2021 the rbfx project.
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

#include "Bounce.h"
#include "ParticleGraphSystem.h"

#include "Helpers.h"
#include "Urho3D/Math/Ray.h"
#include "Urho3D/Physics/PhysicsWorld.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
Bounce::Bounce(Context* context)
    : AbstractNodeType(context, PinArray
        {
            ParticleGraphPin(ParticleGraphPinFlag::Input, "position"),
            ParticleGraphPin(ParticleGraphPinFlag::Input, "velocity"),
            ParticleGraphPin(ParticleGraphPinFlag::None, "newPosition"),
            ParticleGraphPin(ParticleGraphPinFlag::None, "newVelocity"),
        })
{
}

void Bounce::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Bounce>();
}

void Bounce::RayCastAndBounce(UpdateContext& context, Node* node, PhysicsWorld* physics, Vector3& pos, Vector3& velocity)
{
    if (physics)
    {
        const auto gravity = physics->GetGravity();
        velocity += gravity * context.timeStep_;
        PhysicsRaycastResult res;
        Vector3 offset = velocity * context.timeStep_;

        auto distance = offset.Length();
        if (distance > 1e-6f)
        {
            auto wp = node->LocalToWorld(pos);
            physics->RaycastSingle(res, Ray(wp, offset * (1.0f / distance)), distance);
            if (res.body_)
            {
                wp = wp.Lerp(res.position_, 0.99f);
                pos = node->WorldToLocal(wp);
                const float bounceFactor = 1.0f;
                const auto bounceScale = (1.0f + bounceFactor) * velocity.DotProduct(res.normal_);
                velocity -= res.normal_ * bounceScale;
            }
            else
            {
                pos += offset;
            }
        }
        return;
    }
    pos += velocity * context.timeStep_;
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
