// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "ParticleGraphSystem.h"

#include "ParticleGraphEmitter.h"
#include "ParticleGraphLayer.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{

void RegisterGraphNodes(ParticleGraphSystem* system);

}

ParticleGraphSystem::ParticleGraphSystem(Context* context)
    : Object(context)
    , ObjectReflectionRegistry(context)
{
    RegisterParticleGraphLibrary(context, this);
}

ParticleGraphSystem::~ParticleGraphSystem()
{
}

void RegisterParticleGraphLibrary(Context* context, ParticleGraphSystem* system)
{
    ParticleGraphEffect::RegisterObject(context);
    ParticleGraphLayer::RegisterObject(context);
    ParticleGraphEmitter::RegisterObject(context);

    ParticleGraphNodes::RegisterGraphNodes(system);
}

} // namespace Urho3D
