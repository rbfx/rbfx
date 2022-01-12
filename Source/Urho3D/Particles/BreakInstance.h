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

#pragma once

#include "ApplyForce.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
template <typename... Value>
struct BreakInstance
{};

template <> struct BreakInstance<Vector3, float, float, float>
{
    template <typename Vec, typename X, typename Y, typename Z>
    void operator()(UpdateContext& context, unsigned numParticles, Vec vec, X x, Y y, Z z)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            x[i] = vec[i].x_;
            y[i] = vec[i].y_;
            z[i] = vec[i].z_;
        }
    }
};

template <> struct BreakInstance<Vector2, float, float>
{
    template <typename Vec, typename X, typename Y>
    void operator()(UpdateContext& context, unsigned numParticles, Vec vec, X x, Y y)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            x[i] = vec[i].x_;
            y[i] = vec[i].y_;
        }
    }
};

template <> struct BreakInstance<Quaternion, float, float, float, float>
{
    template <typename Vec, typename X, typename Y, typename Z, typename W>
    void operator()(UpdateContext& context, unsigned numParticles, Vec vec, X x, Y y, Z z, W w)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            x[i] = vec[i].x_;
            y[i] = vec[i].y_;
            z[i] = vec[i].z_;
            w[i] = vec[i].w_;
        }
    }
};

template <> struct BreakInstance<Quaternion, Vector3, float>
{
    template <typename Vec, typename X, typename Y>
    void operator()(UpdateContext& context, unsigned numParticles, Vec vec, X axis, Y angle)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            const Quaternion q = vec[i];
            angle[i] = q.Angle();
            axis[i] = q.Axis();
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
