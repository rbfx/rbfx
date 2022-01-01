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
//template <typename T> struct In
//{
//    typedef T Type;
//    const char* name_;
//};
//template <typename T> struct Out
//{
//    typedef T Type;
//    const char* name_;
//};
//template <typename Lambda, typename... Args> NodePattern MakePattern(Lambda&& lambda, Args... args)
//{
//    return NodePattern([lambda](UpdateContext& context, ParticleGraphPinRef* pinRefs)
//        { RunUpdate<Lambda, Args>(context, lambda, pinRefs); });
//}

static ea::vector<NodePattern> MakePatterns{
    /*MakePattern(
        [](UpdateContext& context, unsigned numParticles, auto&& spans)
        {
            auto& x = ea::get<0>(spans);
            auto& y = ea::get<1>(spans);
            auto& out = ea::get<2>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = Vector2(x[i], y[i]);
            }
        },
        In<int>{"x"}, In<int>{"y"}, Out<int>{"y"}),*/
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
        },
        PinPattern("x", VAR_FLOAT), PinPattern("y", VAR_FLOAT),
        PinPattern(ParticleGraphPinFlag::Output, "out", VAR_VECTOR2)),

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
        },
        PinPattern("x", VAR_FLOAT), PinPattern("y", VAR_FLOAT), PinPattern("z", VAR_FLOAT),
        PinPattern(ParticleGraphPinFlag::Output, "out", VAR_VECTOR3)),

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
        },
        PinPattern("translation", VAR_VECTOR3), PinPattern("rotation", VAR_QUATERNION),
        PinPattern("scale", VAR_VECTOR3), PinPattern(ParticleGraphPinFlag::Output, "out", VAR_MATRIX3X4)),
};

}
Slerp::Slerp(Context* context)
    : AbstractNodeType(context, PinArray{
                                    ParticleGraphPin(ParticleGraphPinFlag::Input, "x"),
                                    ParticleGraphPin(ParticleGraphPinFlag::Input, "y"),
                                    ParticleGraphPin(ParticleGraphPinFlag::Input, "t"),
                                    ParticleGraphPin(ParticleGraphPinFlag::None, "out"),
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
