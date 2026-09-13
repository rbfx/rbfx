// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Make.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "MakeInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Make::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Make>();
}

namespace {
static const ea::vector<NodePattern> MakePatterns{
    MakePattern(
        MakeInstance<float, float, Vector2>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<float>("y")
        , ParticleGraphTypedPin<Vector2>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MakeInstance<int, int, IntVector2>()
        , ParticleGraphTypedPin<int>("x")
        , ParticleGraphTypedPin<int>("y")
        , ParticleGraphTypedPin<IntVector2>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MakeInstance<float, float, float, Vector3>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<float>("y")
        , ParticleGraphTypedPin<float>("z")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MakeInstance<Vector3, Quaternion, Vector3, Matrix3x4>()
        , ParticleGraphTypedPin<Vector3>("translation")
        , ParticleGraphTypedPin<Quaternion>("rotation")
        , ParticleGraphTypedPin<Vector3>("scale")
        , ParticleGraphTypedPin<Matrix3x4>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MakeInstance<float, float, float, Quaternion>()
        , ParticleGraphTypedPin<float>("pitch")
        , ParticleGraphTypedPin<float>("yaw")
        , ParticleGraphTypedPin<float>("roll")
        , ParticleGraphTypedPin<Quaternion>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Make::Make(Context* context)
    : PatternMatchingNode(context, MakePatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
