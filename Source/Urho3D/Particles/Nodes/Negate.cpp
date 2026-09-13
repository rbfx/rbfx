
// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Negate.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "NegateInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Negate::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Negate>();
}

namespace {
static const ea::vector<NodePattern> NegatePatterns{
    MakePattern(
        NegateInstance<float, float>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        NegateInstance<Vector2, Vector2>()
        , ParticleGraphTypedPin<Vector2>("x")
        , ParticleGraphTypedPin<Vector2>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        NegateInstance<Vector3, Vector3>()
        , ParticleGraphTypedPin<Vector3>("x")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        NegateInstance<Vector4, Vector4>()
        , ParticleGraphTypedPin<Vector4>("x")
        , ParticleGraphTypedPin<Vector4>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        NegateInstance<Color, Color>()
        , ParticleGraphTypedPin<Color>("x")
        , ParticleGraphTypedPin<Color>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Negate::Negate(Context* context)
    : PatternMatchingNode(context, NegatePatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
