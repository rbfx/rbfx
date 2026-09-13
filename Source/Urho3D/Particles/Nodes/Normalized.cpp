// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Normalized.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "NormalizedInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Normalized::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Normalized>();
}

namespace {
static const ea::vector<NodePattern> NormalizedPatterns{
    MakePattern(
        NormalizedInstance<Vector3, Vector3>()
        , ParticleGraphTypedPin<Vector3>("x")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        NormalizedInstance<Vector2, Vector2>()
        , ParticleGraphTypedPin<Vector2>("x")
        , ParticleGraphTypedPin<Vector2>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        NormalizedInstance<Quaternion, Quaternion>()
        , ParticleGraphTypedPin<Quaternion>("x")
        , ParticleGraphTypedPin<Quaternion>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Normalized::Normalized(Context* context)
    : PatternMatchingNode(context, NormalizedPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
