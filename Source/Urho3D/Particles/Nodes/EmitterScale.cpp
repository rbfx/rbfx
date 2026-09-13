// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "EmitterScale.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "EmitterScaleInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void EmitterScale::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<EmitterScale>();
}


EmitterScale::EmitterScale(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Scalar),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned EmitterScale::EvaluateInstanceSize() const
{
    return sizeof(EmitterScaleInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* EmitterScale::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    EmitterScaleInstance* instance = new (ptr) EmitterScaleInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
