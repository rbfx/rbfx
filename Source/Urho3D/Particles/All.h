// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Helpers.h"
#include "Nodes/Add.h"
#include "Nodes/ApplyForce.h"
#include "Nodes/BurstTimer.h"
#include "Nodes/LimitVelocity.h"
#include "Nodes/Slerp.h"
#include "Nodes/Attribute.h"
#include "Nodes/Bounce.h"
#include "Nodes/Cone.h"
#include "Nodes/Hemisphere.h"
#include "Nodes/Sphere.h"
#include "Nodes/Constant.h"
#include "Nodes/Curve.h"
#include "Nodes/Destroy.h"
#include "Nodes/Emit.h"
#include "Nodes/Print.h"
#include "Nodes/Random.h"
#include "Nodes/RenderBillboard.h"
#include "Nodes/RenderMesh.h"
#include "Nodes/Uniform.h"
#include "Nodes/ApplyForce.h"
#include "Nodes/BurstTimer.h"
#include "Nodes/Expire.h"
#include "Nodes/LimitVelocity.h"
#include "Nodes/EffectTime.h"
#include "Nodes/NormalizedEffectTime.h"
#include "Nodes/TimeStep.h"
#include "Nodes/Divide.h"
#include "Nodes/Lerp.h"
#include "Nodes/Make.h"
#include "Nodes/Move.h"
#include "Nodes/Multiply.h"
#include "Nodes/Negate.h"
#include "Nodes/Subtract.h"
#include "Nodes/TimeStepScale.h"
#include "Nodes/Normalized.h"
#include "Nodes/Length.h"
#include "Nodes/Break.h"
#include "Nodes/Circle.h"
#include "Nodes/Box.h"
#include "Nodes/Cast.h"
#include "Nodes/CurlNoise3D.h"
#include "Nodes/Noise3D.h"

namespace Urho3D
{

class ParticleGraphSystem;

namespace ParticleGraphNodes
{

void RegisterGraphNodes(ParticleGraphSystem* context);

}

} // namespace Urho3D
