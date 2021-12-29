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

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Get time step of the current frame.
class URHO3D_API TimeStep : public AbstractNode<TimeStep, float>
{
    URHO3D_OBJECT(TimeStep, ParticleGraphNode)
public:
    /// Construct.
    explicit TimeStep(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& pin0 = ea::get<0>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin0[i] = context.timeStep_;
        }
    }
};

/// Get time passed since start of the emitter.
class URHO3D_API EffectTime : public AbstractNode<EffectTime, float>
{
    URHO3D_OBJECT(EffectTime, ParticleGraphNode)
public:
    /// Construct.
    explicit EffectTime(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& pin0 = ea::get<0>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin0[i] = context.time_;
        }
    }
};


/// Get time passed since start of the emitter.
class URHO3D_API NormalizedEffectTime : public AbstractNode<NormalizedEffectTime, float>
{
    URHO3D_OBJECT(NormalizedEffectTime, ParticleGraphNode)
public:
    /// Construct.
    explicit NormalizedEffectTime(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& pin0 = ea::get<0>(spans);
        const float value = context.time_ / instance->GetLayer()->GetDuration();
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin0[i] = value;
        }
    }
};

/// Move position according to velocity and time step of the current frame.
class URHO3D_API Move : public AbstractNode<Move, Vector3, Vector3, Vector3>
{
    URHO3D_OBJECT(Move, ParticleGraphNode)
public:
    /// Construct.
    explicit Move(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& pin0 = ea::get<0>(spans);
        auto& pin1 = ea::get<1>(spans);
        auto& pin2 = ea::get<2>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin2[i] = pin0[i] + context.timeStep_ * pin1[i];
        }
    }
};

/// Move position according to velocity and time step of the current frame.
class URHO3D_API LimitVelocity : public AbstractNode<LimitVelocity, Vector3, float, Vector3>
{
    URHO3D_OBJECT(LimitVelocity, ParticleGraphNode)
public:
    /// Construct.
    explicit LimitVelocity(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& vel = ea::get<0>(spans);
        auto& limit = ea::get<1>(spans);
        auto& result = ea::get<2>(spans);
        const float dampen = instance->GetGraphNodeInstace()->dampen_;
        if (dampen <= 1e-6f || context.timeStep_ < 1e-6f)
        {
            for (unsigned i = 0; i < numParticles; ++i)
            {
                result[i] = vel[i];
            }
        }
        else
        {
            const auto t = 1.0f - powf(1.0f - dampen, 20.0f * context.timeStep_);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                const float speed = vel[i].Length();
                const float limitVal = limit[i];
                if (speed > limitVal + 1e-6f)
                {
                    result[i] = vel[i] * (Urho3D::Lerp(speed, limitVal, t) / speed);
                }
            }
        }
    }

    /// Set dampen.
    /// @property
    void SetDampen(float value);
    /// Get dampen.
    /// @property
    float GetDampen() const;
protected:

    float dampen_{0.0f};
};

/// Move position according to velocity and time step of the current frame.
class URHO3D_API ApplyForce : public AbstractNode<ApplyForce, Vector3, Vector3, Vector3>
{
    URHO3D_OBJECT(ApplyForce, ParticleGraphNode)
public:
    /// Construct.
    explicit ApplyForce(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& vel = ea::get<0>(spans);
        auto& force = ea::get<1>(spans);
        auto& result = ea::get<2>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            result[i] = vel[i] + force[i]*context.timeStep_;
        }
    }

};

} // namespace ParticleGraphNodes

} // namespace Urho3D
