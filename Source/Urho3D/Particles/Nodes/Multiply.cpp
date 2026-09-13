// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Multiply.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "MultiplyInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Multiply::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Multiply>();
}

namespace {
static const ea::vector<NodePattern> MultiplyPatterns{
    MakePattern(
        MultiplyInstance<float, float, float>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<float>("y")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MultiplyInstance<Vector3, float, Vector3>()
        , ParticleGraphTypedPin<Vector3>("x")
        , ParticleGraphTypedPin<float>("y")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MultiplyInstance<float, Vector3, Vector3>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<Vector3>("y")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MultiplyInstance<Color, Color, Color>()
        , ParticleGraphTypedPin<Color>("x")
        , ParticleGraphTypedPin<Color>("y")
        , ParticleGraphTypedPin<Color>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Multiply::Multiply(Context* context)
    : PatternMatchingNode(context, MultiplyPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
