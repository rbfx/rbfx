// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/EmitterRotation.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/EmitterRotationInstance.h"

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
