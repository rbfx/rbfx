// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Emit.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/EmitInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Emit::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Emit>();
}


Emit::Emit(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "count", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Emit::EvaluateInstanceSize() const
{
    return sizeof(EmitInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Emit::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    EmitInstance* instance = new (ptr) EmitInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
