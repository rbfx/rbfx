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

#pragma once

#include "Helpers.h"
#include "ParticleGraphLayerInstance.h"
#include "Urho3D/Physics/PhysicsWorld.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Get time step of the current frame.
class URHO3D_API Bounce : public AbstractNode<Bounce, Vector3, Vector3, Vector3, Vector3>
{
    URHO3D_OBJECT(Bounce, ParticleGraphNode)

    class Instance : public AbstractNodeType::Instance
    {
    public:
        Instance(Bounce* node, ParticleGraphLayerInstance* layer)
            : AbstractNodeType::Instance(node, layer)
        {
        }

        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& pin0 = ea::get<0>(spans);
            auto& pin1 = ea::get<1>(spans);
            auto& pin2 = ea::get<2>(spans);
            auto& pin3 = ea::get<3>(spans);
            Bounce* bounce = GetGraphNodeInstace();
            auto node = GetNode();
            auto physics = GetScene()->GetComponent<PhysicsWorld>();
            for (unsigned i = 0; i < numParticles; ++i)
            {
                pin2[i] = pin0[i];
                pin3[i] = pin1[i];
                bounce->RayCastAndBounce(context, node, physics, pin2[i], pin3[i]);
            }
        }
    };

public:
    /// Construct.
    explicit Bounce(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    void RayCastAndBounce(UpdateContext& context, Node* node, PhysicsWorld* physics, Vector3& pos, Vector3& velocity);

    float GetDampen() const { return dampen_; }
    void SetDampen(float dampen) { dampen_ = dampen; }
    float GetBounceFactor() const { return bounceFactor_; }
    void SetBounceFactor(float bounceFactor) { bounceFactor_ = bounceFactor; }

protected:
    float dampen_{0.0f};
    float bounceFactor_{1.0f};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
