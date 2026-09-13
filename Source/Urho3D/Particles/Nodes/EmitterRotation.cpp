
// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "EmitterRotation.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "EmitterRotationInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void EmitterRotation::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<EmitterRotation>();
}


EmitterRotation::EmitterRotation(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Scalar),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned EmitterRotation::EvaluateInstanceSize() const
{
    return sizeof(EmitterRotationInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* EmitterRotation::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    EmitterRotationInstance* instance = new (ptr) EmitterRotationInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
