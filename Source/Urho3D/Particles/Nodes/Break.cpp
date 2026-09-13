// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Break.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "BreakInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Break::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Break>();
}

namespace {
static const ea::vector<NodePattern> BreakPatterns{
    MakePattern(
        BreakInstance<Vector3, float, float, float>()
        , ParticleGraphTypedPin<Vector3>("vec")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "y")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "z")
    ),
    MakePattern(
        BreakInstance<Vector2, float, float>()
        , ParticleGraphTypedPin<Vector2>("vec")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "y")
    ),
    MakePattern(
        BreakInstance<Quaternion, float, float, float, float>()
        , ParticleGraphTypedPin<Quaternion>("q")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "y")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "z")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "w")
    ),
    MakePattern(
        BreakInstance<Quaternion, Vector3, float>()
        , ParticleGraphTypedPin<Quaternion>("q")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "axis")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "angle")
    ),
};
} // namespace

Break::Break(Context* context)
    : PatternMatchingNode(context, BreakPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
