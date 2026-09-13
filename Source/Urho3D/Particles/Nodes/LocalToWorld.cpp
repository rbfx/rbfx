
// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "LocalToWorld.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "LocalToWorldInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void LocalToWorld::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<LocalToWorld>();
}


LocalToWorld::LocalToWorld(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Scalar),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned LocalToWorld::EvaluateInstanceSize() const
{
    return sizeof(LocalToWorldInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* LocalToWorld::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    LocalToWorldInstance* instance = new (ptr) LocalToWorldInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
