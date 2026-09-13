// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "NormalizedEffectTime.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "NormalizedEffectTimeInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void NormalizedEffectTime::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<NormalizedEffectTime>();
}


NormalizedEffectTime::NormalizedEffectTime(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Scalar),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned NormalizedEffectTime::EvaluateInstanceSize() const
{
    return sizeof(NormalizedEffectTimeInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* NormalizedEffectTime::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    NormalizedEffectTimeInstance* instance = new (ptr) NormalizedEffectTimeInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
