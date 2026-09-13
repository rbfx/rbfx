// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/EffectTime.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/EffectTimeInstance.h"

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
