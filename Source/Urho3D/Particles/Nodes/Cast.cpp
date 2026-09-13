// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Cast.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/CastInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Cast::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Cast>();
}

namespace {
static const ea::vector<NodePattern> CastPatterns{
    MakePattern(
        CastInstance<float, int>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<int>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        CastInstance<int, float>()
        , ParticleGraphTypedPin<int>("x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Cast::Cast(Context* context)
    : PatternMatchingNode(context, CastPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
