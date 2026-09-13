
// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "EffectTime.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "EffectTimeInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void EffectTime::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<EffectTime>();
}


EffectTime::EffectTime(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Scalar),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned EffectTime::EvaluateInstanceSize() const
{
    return sizeof(EffectTimeInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* EffectTime::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    EffectTimeInstance* instance = new (ptr) EffectTimeInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
