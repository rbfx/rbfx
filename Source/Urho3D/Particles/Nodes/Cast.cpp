
// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Cast.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "CastInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Cast::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Cast>();
}

namespace {
static const ea::vector<NodePattern> CastPatterns{
    MakePattern(
        CastInstance<float, int>()
        , ParticleGraphTypedPin<float>("x")
        , ParticleGraphTypedPin<int>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        CastInstance<int, float>()
        , ParticleGraphTypedPin<int>("x")
        , ParticleGraphTypedPin<float>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Cast::Cast(Context* context)
    : PatternMatchingNode(context, CastPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
