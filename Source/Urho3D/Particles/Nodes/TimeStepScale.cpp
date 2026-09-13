// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "TimeStepScale.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "TimeStepScaleInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void TimeStepScale::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<TimeStepScale>();
}

namespace {
static const ea::vector<NodePattern> TimeStepScalePatterns{
    MakePattern(
        TimeStepScaleInstance<float, float>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        TimeStepScaleInstance<Vector2, Vector2>()
        , ParticleGraphTypedPin<Vector2>("x")
        , ParticleGraphTypedPin<Vector2>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        TimeStepScaleInstance<Vector3, Vector3>()
        , ParticleGraphTypedPin<Vector3>("x")
        , ParticleGraphTypedPin<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        TimeStepScaleInstance<Vector4, Vector4>()
        , ParticleGraphTypedPin<Vector4>("x")
        , ParticleGraphTypedPin<Vector4>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        TimeStepScaleInstance<Color, Color>()
        , ParticleGraphTypedPin<Color>("x")
        , ParticleGraphTypedPin<Color>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

TimeStepScale::TimeStepScale(Context* context)
    : PatternMatchingNode(context, TimeStepScalePatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
