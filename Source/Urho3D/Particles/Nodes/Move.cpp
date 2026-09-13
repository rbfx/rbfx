
// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Move.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "MoveInstance.h"

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
