// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "BurstTimer.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class BurstTimerInstance final : public BurstTimer::InstanceBase
{
public:
    void Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer) override
    {
        InstanceBase::Init(node, layer);
        BurstTimer* burstTimer = static_cast<BurstTimer*>(node);
        timeToBurst_ = burstTimer->GetDelay();
        counter_ = burstTimer->GetCycles();
    }

    void operator()(
        const UpdateContext& context, unsigned numParticles, const SparseSpan<float>& count, const SparseSpan<float>& out)
    {
        timeToBurst_ -= context.timeStep_;
        if (counter_ > 0 && timeToBurst_ <= 0.0f)
        {
            // Can't use iterator here as it may use ScalarSpan with infinite iterator.
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = count[i];
            }
            timeToBurst_ += static_cast<BurstTimer*>(GetGraphNode())->GetInterval();
            --counter_;
        }
        else
        {
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = 0.0f;
            }
        }
    }

    float timeToBurst_{};
    unsigned counter_{};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
