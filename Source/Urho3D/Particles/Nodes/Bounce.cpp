// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Bounce.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/BounceInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Bounce::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Bounce>();
    URHO3D_ACCESSOR_ATTRIBUTE("Dampen", GetDampen, SetDampen, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("BounceFactor", GetBounceFactor, SetBounceFactor, float, float{}, AM_DEFAULT);
}


Bounce::Bounce(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "position", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "velocity", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "newPosition", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "newVelocity", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Bounce::EvaluateInstanceSize() const
{
    return sizeof(BounceInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Bounce::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    BounceInstance* instance = new (ptr) BounceInstance();
    instance->Init(this, layer);
    return instance;
}

void Bounce::SetDampen(float value) { dampen_ = value; }

float Bounce::GetDampen() const { return dampen_; }

void Bounce::SetBounceFactor(float value) { bounceFactor_ = value; }

float Bounce::GetBounceFactor() const { return bounceFactor_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
