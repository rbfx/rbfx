
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
