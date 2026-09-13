// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Move.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/MoveInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Move::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Move>();
}


Move::Move(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "position", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "velocity", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "newPosition", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Move::EvaluateInstanceSize() const
{
    return sizeof(MoveInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Move::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    MoveInstance* instance = new (ptr) MoveInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
