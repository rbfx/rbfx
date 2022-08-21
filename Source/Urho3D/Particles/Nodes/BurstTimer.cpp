
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

#include "../../Precompiled.h"

#include "BurstTimer.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "BurstTimerInstance.h"

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
