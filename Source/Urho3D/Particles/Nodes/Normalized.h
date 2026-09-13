// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/PatternMatchingNode.h"
#include "Urho3D/Particles/ParticleGraphNode.h"
#include "Urho3D/Particles/ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class URHO3D_API Normalized : public PatternMatchingNode
{
    URHO3D_OBJECT(Normalized, ParticleGraphNode)
public:
    /// Construct Normalized.
    explicit Normalized(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

protected:
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
