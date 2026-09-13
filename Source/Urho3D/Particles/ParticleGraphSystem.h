// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/ParticleGraphNode.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Context.h"

namespace Urho3D
{
/// %Particle graph effect definition.
class URHO3D_API ParticleGraphSystem : public Object, public ObjectReflectionRegistry
{
    URHO3D_OBJECT(ParticleGraphSystem, Object);

public:
    ParticleGraphSystem(Context* context);

    ~ParticleGraphSystem() override;
};


/// Register Particle Graph library objects.
void URHO3D_API RegisterParticleGraphLibrary(Context* context, ParticleGraphSystem* system);

}
