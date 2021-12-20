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

/// Emit particles at current layer.
class URHO3D_API Emit : public AbstractNode<Emit, float>
{
    URHO3D_OBJECT(Emit, ParticleGraphNode)
public:
    /// Construct.
    explicit Emit(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& span = ea::get<0>(spans);
        // Can't use iterator here as it may use ScalarSpan with infinite iterator.
        float sum = 0.0f;
        for (unsigned i=0; i<numParticles; ++i)
        {
            sum += span[i];
        }
        context.layer_->EmitNewParticles(sum);
    }
};


/// Emit particles at current layer.
class URHO3D_API BurstTimer : public AbstractNode<BurstTimer, float, float>
{
    URHO3D_OBJECT(BurstTimer, ParticleGraphNode)
public:
    /// Construct.
    explicit BurstTimer(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);

    struct Instance : public AbstractNodeType::Instance
    {
        /// Construct instance.
        Instance(BurstTimer* node, ParticleGraphLayerInstance* layer)
            : AbstractNodeType::Instance(node, layer)
        {
        }

        void Reset() override
        {
            timeToBurst_ = node_->GetDelay();
            counter_ = node_->GetCycles();
        }

        float timeToBurst_{};
        unsigned counter_{};
    };

    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        instance->timeToBurst_ -= context.timeStep_;
        if (instance->counter_ > 0 && instance->timeToBurst_ <= 0.0f)
        {
            auto& span = ea::get<0>(spans);
            // Can't use iterator here as it may use ScalarSpan with infinite iterator.
            float sum = 0.0f;
            for (unsigned i = 0; i < numParticles; ++i)
            {
                sum += span[i];
            }
            context.layer_->EmitNewParticles(sum);
            instance->timeToBurst_ += instance->GetGraphNodeInstace()->interval_;
            --instance->counter_;
        }
    }

    /// Get delay in seconds.
    /// @property
    float GetDelay() const { return delay_; }

    /// Set delay in seconds.
    /// @property 
    void SetDelay(float delay) { delay_ = delay; }

    /// Get interval in seconds.
    /// @property
    float GetInterval() const { return interval_; }

    /// Set interval in seconds.
    /// @property
    void SetInterval(float interval) { interval_ = interval; }

    /// Get number of cycles.
    /// @property
    unsigned GetCycles() const { return cycles_; }

    /// Set interval in seconds.
    /// @property
    void SetCycles(unsigned cycles) { cycles_ = cycles; }

private:
    float delay_{};
    float interval_{0.01f};
    unsigned cycles_{1};
};
} // namespace ParticleGraphNodes

} // namespace Urho3D
