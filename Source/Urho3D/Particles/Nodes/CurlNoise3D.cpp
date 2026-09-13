// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/CurlNoise3D.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/CurlNoise3DInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void CurlNoise3D::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<CurlNoise3D>();
}


CurlNoise3D::CurlNoise3D(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "x", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned CurlNoise3D::EvaluateInstanceSize() const
{
    return sizeof(CurlNoise3DInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* CurlNoise3D::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    CurlNoise3DInstance* instance = new (ptr) CurlNoise3DInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
