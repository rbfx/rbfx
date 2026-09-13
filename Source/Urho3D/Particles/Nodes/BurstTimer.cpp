// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/BurstTimer.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/BurstTimerInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void BurstTimer::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<BurstTimer>();
    URHO3D_ACCESSOR_ATTRIBUTE("Delay", GetDelay, SetDelay, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Interval", GetInterval, SetInterval, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Cycles", GetCycles, SetCycles, int, int{}, AM_DEFAULT);
}


BurstTimer::BurstTimer(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "count", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned BurstTimer::EvaluateInstanceSize() const
{
    return sizeof(BurstTimerInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* BurstTimer::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    BurstTimerInstance* instance = new (ptr) BurstTimerInstance();
    instance->Init(this, layer);
    return instance;
}

void BurstTimer::SetDelay(float value) { delay_ = value; }

float BurstTimer::GetDelay() const { return delay_; }

void BurstTimer::SetInterval(float value) { interval_ = value; }

float BurstTimer::GetInterval() const { return interval_; }

void BurstTimer::SetCycles(int value) { cycles_ = value; }

int BurstTimer::GetCycles() const { return cycles_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
