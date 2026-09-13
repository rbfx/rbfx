// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Destroy.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/DestroyInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Destroy::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Destroy>();
}


Destroy::Destroy(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "condition", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Destroy::EvaluateInstanceSize() const
{
    return sizeof(DestroyInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Destroy::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    DestroyInstance* instance = new (ptr) DestroyInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
