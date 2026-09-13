// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "WorldToLocal.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "WorldToLocalInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void WorldToLocal::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<WorldToLocal>();
}


WorldToLocal::WorldToLocal(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Scalar),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned WorldToLocal::EvaluateInstanceSize() const
{
    return sizeof(WorldToLocalInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* WorldToLocal::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    WorldToLocalInstance* instance = new (ptr) WorldToLocalInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
