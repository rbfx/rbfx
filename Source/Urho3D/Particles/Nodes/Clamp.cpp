// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Clamp.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "ClampInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Clamp::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Clamp>();
}

namespace {
static const ea::vector<NodePattern> ClampPatterns{
    MakePattern(
        ClampInstance<float, float>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        ClampInstance<Vector2, Vector2>()
        , ParticleGraphTypedPin<Vector2>("x")
        , ParticleGraphTypedPin<Vector2>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        ClampInstance<Vector3, Vector3>()
        , ParticleGraphTypedPin<Vector3>("x")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        ClampInstance<Vector4, Vector4>()
        , ParticleGraphTypedPin<Vector4>("x")
        , ParticleGraphTypedPin<Vector4>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Clamp::Clamp(Context* context)
    : PatternMatchingNode(context, ClampPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
