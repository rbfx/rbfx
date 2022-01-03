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
#include "Add.h"
#include "ApplyForce.h"
#include "BurstTimer.h"
#include "LimitVelocity.h"
#include "Slerp.h"
#include "Attribute.h"
#include "Bounce.h"
#include "Cone.h"
#include "Hemisphere.h"
#include "Sphere.h"
#include "Constant.h"
#include "Curve.h"
#include "Destroy.h"
#include "Emit.h"
#include "Print.h"
#include "Random.h"
#include "RenderBillboard.h"
#include "RenderMesh.h"
#include "Uniform.h"
#include "ApplyForce.h"
#include "BurstTimer.h"
#include "Expire.h"
#include "LimitVelocity.h"
#include "EffectTime.h"
#include "NormalizedEffectTime.h"
#include "TimeStep.h"
#include "Divide.h"
#include "Lerp.h"
#include "Make.h"
#include "Move.h"
#include "Multiply.h"
#include "Negate.h"
#include "Subtract.h"
#include "TimeStepScale.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
void RegisterGraphNodes(ParticleGraphSystem* context);
}

} // namespace Urho3D
