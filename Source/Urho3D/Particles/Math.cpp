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

#include "Math.h"
#include "ParticleGraphSystem.h"

#include "Urho3D/Core/Context.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
namespace
{


template <typename PinType> struct PinPatternBuilder
{
    typedef typename PinType::Type Type;
};

template <typename Lambda, typename... Args> NodePattern MakePattern(Lambda lambda, Args... args)
{
    auto l = [&](UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        RunUpdate<Lambda, Args...>(context, lambda, pinRefs);
    };
    NodePattern pattern(l);
    pattern.SetPins(args...);
    return pattern;
}

static ea::vector<NodePattern> MakePatterns{
    MakePattern(
        [](UpdateContext& context, unsigned numParticles, auto x, auto y, auto out)
        {
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = IntVector2(x[i], y[i]);
            }
        },
        PinPattern<int>("x"), PinPattern<int>("y"), PinPattern<IntVector2>(ParticleGraphPinFlag::Output, "out")),
    NodePattern(
        [](UpdateContext& context, ParticleGraphPinRef* pinRefs)
        {
            auto lambda = [](UpdateContext& context, unsigned numParticles, auto x, auto y, auto out)
            {
                for (unsigned i = 0; i < numParticles; ++i)
                {
                    out[i] = Vector2(x[i], y[i]);
                }
            };
            RunUpdate<decltype(lambda), float, float, Vector2>(context, lambda, pinRefs);
        })
        .WithPin(PinPattern<float>("x"))
        .WithPin(PinPattern<float>("y"))
        .WithPin(PinPattern<Vector2>(ParticleGraphPinFlag::Output, "out")),
    NodePattern(
        [](UpdateContext& context, ParticleGraphPinRef* pinRefs)
        {
            auto lambda = [](UpdateContext& context, unsigned numParticles, auto x, auto y, auto z, auto out)
            {
                for (unsigned i = 0; i < numParticles; ++i)
                {
                    out[i] = Vector3(x[i], y[i], z[i]);
                }
            };
            RunUpdate<decltype(lambda), float, float, float, Vector3>(context, lambda, pinRefs);
        })
        .WithPin(PinPattern<float>("x"))
        .WithPin(PinPattern<float>("y"))
        .WithPin(PinPattern<float>("z"))
        .WithPin(PinPattern<Vector3>(ParticleGraphPinFlag::Output, "out")),

    NodePattern(
        [](UpdateContext& context, ParticleGraphPinRef* pinRefs)
        {
            auto lambda =
                [](UpdateContext& context, unsigned numParticles, auto translation, auto rotation, auto scale, auto out)
            {
                for (unsigned i = 0; i < numParticles; ++i)
                {
                    out[i] = Matrix3x4(translation[i], rotation[i], scale[i]);
                }
            };
            RunUpdate<decltype(lambda), Vector3, Quaternion, Vector3, Matrix3x4>(context, lambda, pinRefs);
        })
        .WithPin(PinPattern<Vector3>("translation"))
        .WithPin(PinPattern<Quaternion>("rotation"))
        .WithPin(PinPattern<Vector3>("scale"))
        .WithPin(PinPattern<Matrix3x4>(ParticleGraphPinFlag::Output, "out")),
};

}
Slerp::Slerp(Context* context)
    : AbstractNodeType(context, PinArray{
                                    ParticleGraphPin(ParticleGraphPinFlag::Input, "x"),
                                    ParticleGraphPin(ParticleGraphPinFlag::Input, "y"),
                                    ParticleGraphPin(ParticleGraphPinFlag::Input, "t"),
            ParticleGraphPin(ParticleGraphPinFlag::Output, "out"),
                                })
{
}

void Slerp::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Slerp>();
}

Make::Make(Context* context)
    : PatternMatchingNode(context, MakePatterns)
{
}

void Make::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<Make>(); }

}

} // namespace Urho3D
