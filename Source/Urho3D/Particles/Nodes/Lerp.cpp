// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Lerp.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/LerpInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Lerp::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Lerp>();
}

namespace {
static const ea::vector<NodePattern> LerpPatterns{
    MakePattern(
        LerpInstance<float, float, float, float>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<float>("y")
        , ParticleGraphTypedPin<float>("t")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        LerpInstance<Vector2, Vector2, float, Vector2>()
        , ParticleGraphTypedPin<Vector2>("x")
        , ParticleGraphTypedPin<Vector2>("y")
        , ParticleGraphTypedPin<float>("t")
        , ParticleGraphTypedPin<Vector2>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        LerpInstance<Vector3, Vector3, float, Vector3>()
        , ParticleGraphTypedPin<Vector3>("x")
        , ParticleGraphTypedPin<Vector3>("y")
        , ParticleGraphTypedPin<float>("t")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        LerpInstance<Vector4, Vector4, float, Vector4>()
        , ParticleGraphTypedPin<Vector4>("x")
        , ParticleGraphTypedPin<Vector4>("y")
        , ParticleGraphTypedPin<float>("t")
        , ParticleGraphTypedPin<Vector4>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        LerpInstance<Color, Color, float, Color>()
        , ParticleGraphTypedPin<Color>("x")
        , ParticleGraphTypedPin<Color>("y")
        , ParticleGraphTypedPin<float>("t")
        , ParticleGraphTypedPin<Color>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Lerp::Lerp(Context* context)
    : PatternMatchingNode(context, LerpPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
