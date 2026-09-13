// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Length.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/LengthInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Length::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Length>();
}

namespace {
static const ea::vector<NodePattern> LengthPatterns{
    MakePattern(
        LengthInstance<Vector3, float>()
        , ParticleGraphTypedPin<Vector3>("x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        LengthInstance<Vector2, float>()
        , ParticleGraphTypedPin<Vector2>("x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Length::Length(Context* context)
    : PatternMatchingNode(context, LengthPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
