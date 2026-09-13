// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/Helpers.h"
#include "Urho3D/Particles/Nodes/Add.h"
#include "Urho3D/Particles/Nodes/ApplyForce.h"
#include "Urho3D/Particles/Nodes/BurstTimer.h"
#include "Urho3D/Particles/Nodes/LimitVelocity.h"
#include "Urho3D/Particles/Nodes/Slerp.h"
#include "Urho3D/Particles/Nodes/Attribute.h"
#include "Urho3D/Particles/Nodes/Bounce.h"
#include "Urho3D/Particles/Nodes/Cone.h"
#include "Urho3D/Particles/Nodes/Hemisphere.h"
#include "Urho3D/Particles/Nodes/Sphere.h"
#include "Urho3D/Particles/Nodes/Constant.h"
#include "Urho3D/Particles/Nodes/Curve.h"
#include "Urho3D/Particles/Nodes/Destroy.h"
#include "Urho3D/Particles/Nodes/Emit.h"
#include "Urho3D/Particles/Nodes/Print.h"
#include "Urho3D/Particles/Nodes/Random.h"
#include "Urho3D/Particles/Nodes/RenderBillboard.h"
#include "Urho3D/Particles/Nodes/RenderMesh.h"
#include "Urho3D/Particles/Nodes/Uniform.h"
#include "Urho3D/Particles/Nodes/ApplyForce.h"
#include "Urho3D/Particles/Nodes/BurstTimer.h"
#include "Urho3D/Particles/Nodes/Expire.h"
#include "Urho3D/Particles/Nodes/LimitVelocity.h"
#include "Urho3D/Particles/Nodes/EffectTime.h"
#include "Urho3D/Particles/Nodes/NormalizedEffectTime.h"
#include "Urho3D/Particles/Nodes/TimeStep.h"
#include "Urho3D/Particles/Nodes/Divide.h"
#include "Urho3D/Particles/Nodes/Lerp.h"
#include "Urho3D/Particles/Nodes/Make.h"
#include "Urho3D/Particles/Nodes/Move.h"
#include "Urho3D/Particles/Nodes/Multiply.h"
#include "Urho3D/Particles/Nodes/Negate.h"
#include "Urho3D/Particles/Nodes/Subtract.h"
#include "Urho3D/Particles/Nodes/TimeStepScale.h"
#include "Urho3D/Particles/Nodes/Normalized.h"
#include "Urho3D/Particles/Nodes/Length.h"
#include "Urho3D/Particles/Nodes/Break.h"
#include "Urho3D/Particles/Nodes/Circle.h"
#include "Urho3D/Particles/Nodes/Box.h"
#include "Urho3D/Particles/Nodes/Cast.h"
#include "Urho3D/Particles/Nodes/CurlNoise3D.h"
#include "Urho3D/Particles/Nodes/Noise3D.h"

namespace Urho3D
{

class ParticleGraphSystem;

namespace ParticleGraphNodes
{

void RegisterGraphNodes(ParticleGraphSystem* context);

}

} // namespace Urho3D
