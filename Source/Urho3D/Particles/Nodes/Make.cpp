
//
// Copyright (c) 2021-2022 the rbfx project.
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
