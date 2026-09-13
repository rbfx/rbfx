// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "EmitterPosition.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "EmitterPositionInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void EmitterPosition::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<EmitterPosition>();
}


EmitterPosition::EmitterPosition(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Scalar),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned EmitterPosition::EvaluateInstanceSize() const
{
    return sizeof(EmitterPositionInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* EmitterPosition::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    EmitterPositionInstance* instance = new (ptr) EmitterPositionInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
