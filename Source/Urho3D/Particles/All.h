//
// Copyright (c) 2021 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
