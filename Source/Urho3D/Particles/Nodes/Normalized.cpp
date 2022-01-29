
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

#include "../Span.h"
#include "../ParticleGraphLayerInstance.h"
#include "../UpdateContext.h"
#include "../../Precompiled.h"
#include "Normalized.h"
#include "NormalizedInstance.h"
#include "../ParticleGraphSystem.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Normalized::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Normalized>();
}

namespace {
static ea::vector<NodePattern> NormalizedPatterns{
    MakePattern(
        NormalizedInstance<Vector3, Vector3>()
        , PinPattern<Vector3>("x")
        , PinPattern<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        NormalizedInstance<Vector2, Vector2>()
        , PinPattern<Vector2>("x")
        , PinPattern<Vector2>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        NormalizedInstance<Quaternion, Quaternion>()
        , PinPattern<Quaternion>("x")
        , PinPattern<Quaternion>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Normalized::Normalized(Context* context)
    : PatternMatchingNode(context, NormalizedPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
