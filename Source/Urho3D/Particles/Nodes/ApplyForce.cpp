// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/ApplyForce.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/Nodes/ApplyForceInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void ApplyForce::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<ApplyForce>();
}


ApplyForce::ApplyForce(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "velocity", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "force", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned ApplyForce::EvaluateInstanceSize() const
{
    return sizeof(ApplyForceInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* ApplyForce::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    ApplyForceInstance* instance = new (ptr) ApplyForceInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
