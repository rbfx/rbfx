// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "TimeStep.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "TimeStepInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void TimeStep::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<TimeStep>();
}


TimeStep::TimeStep(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Scalar),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned TimeStep::EvaluateInstanceSize() const
{
    return sizeof(TimeStepInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* TimeStep::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    TimeStepInstance* instance = new (ptr) TimeStepInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
