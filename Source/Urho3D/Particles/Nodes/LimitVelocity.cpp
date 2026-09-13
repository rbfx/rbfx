
// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "LimitVelocity.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "LimitVelocityInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void LimitVelocity::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<LimitVelocity>();
    URHO3D_ACCESSOR_ATTRIBUTE("Dampen", GetDampen, SetDampen, float, float{}, AM_DEFAULT);
}


LimitVelocity::LimitVelocity(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "velocity", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "limit", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned LimitVelocity::EvaluateInstanceSize() const
{
    return sizeof(LimitVelocityInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* LimitVelocity::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    LimitVelocityInstance* instance = new (ptr) LimitVelocityInstance();
    instance->Init(this, layer);
    return instance;
}

void LimitVelocity::SetDampen(float value) { dampen_ = value; }

float LimitVelocity::GetDampen() const { return dampen_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
