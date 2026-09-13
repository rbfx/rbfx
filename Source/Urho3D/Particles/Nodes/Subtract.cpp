// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Subtract.h"

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"
#include "Urho3D/Particles/Nodes/SubtractInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Subtract::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Subtract>();
}

namespace {
static const ea::vector<NodePattern> SubtractPatterns{
    MakePattern(
        SubtractInstance<float, float, float>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<float>("y")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        SubtractInstance<Vector2, Vector2, Vector2>()
        , ParticleGraphTypedPin<Vector2>("x")
        , ParticleGraphTypedPin<Vector2>("y")
        , ParticleGraphTypedPin<Vector2>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        SubtractInstance<Vector3, Vector3, Vector3>()
        , ParticleGraphTypedPin<Vector3>("x")
        , ParticleGraphTypedPin<Vector3>("y")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        SubtractInstance<Vector4, Vector4, Vector4>()
        , ParticleGraphTypedPin<Vector4>("x")
        , ParticleGraphTypedPin<Vector4>("y")
        , ParticleGraphTypedPin<Vector4>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        SubtractInstance<Color, Color, Color>()
        , ParticleGraphTypedPin<Color>("x")
        , ParticleGraphTypedPin<Color>("y")
        , ParticleGraphTypedPin<Color>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Subtract::Subtract(Context* context)
    : PatternMatchingNode(context, SubtractPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
