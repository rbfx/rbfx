// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Expire.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/ExpireInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Expire::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Expire>();
}


Expire::Expire(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "time", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "lifetime", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Expire::EvaluateInstanceSize() const
{
    return sizeof(ExpireInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Expire::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    ExpireInstance* instance = new (ptr) ExpireInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
