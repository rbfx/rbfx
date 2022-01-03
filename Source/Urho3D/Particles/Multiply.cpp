
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

#include "../Precompiled.h"
#include "Multiply.h"
#include "MultiplyInstance.h"
#include "ParticleGraphSystem.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Multiply::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Multiply>();
}

namespace {
static ea::vector<NodePattern> MultiplyPatterns{
    MakePattern(
        MultiplyInstance<float, float, float>()
        , PinPattern<float>("x")
        , PinPattern<float>("y")
        , PinPattern<float>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MultiplyInstance<Vector3, float, Vector3>()
        , PinPattern<Vector3>("x")
        , PinPattern<float>("y")
        , PinPattern<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MultiplyInstance<float, Vector3, Vector3>()
        , PinPattern<float>("x")
        , PinPattern<Vector3>("y")
        , PinPattern<Vector3>(ParticleGraphPinFlag::Output, "out")
    ),
    MakePattern(
        MultiplyInstance<Color, Color, Color>()
        , PinPattern<Color>("x")
        , PinPattern<Color>("y")
        , PinPattern<Color>(ParticleGraphPinFlag::Output, "out")
    ),
};
} // namespace

Multiply::Multiply(Context* context)
    : PatternMatchingNode(context, MultiplyPatterns)
{
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
