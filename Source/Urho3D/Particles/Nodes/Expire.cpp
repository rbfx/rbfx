
// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Expire.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "ExpireInstance.h"

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
