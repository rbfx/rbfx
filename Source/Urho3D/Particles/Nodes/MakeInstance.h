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

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
template <typename... Values> struct MakeInstance
{
};

template <typename Value0, typename Value1, typename Value2> struct MakeInstance<Value0, Value1, Value2>
{
    template <typename X, typename Y, typename Out>
    void operator()(UpdateContext& context, unsigned numParticles, X x, Y y, Out out)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = Value2(x[i], y[i]);
        }
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3>
struct MakeInstance<Value0, Value1, Value2, Value3>
{
    template <typename X, typename Y, typename Z, typename Out>
    void operator()(UpdateContext& context, unsigned numParticles, X x, Y y, Z z, Out out)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = Value3(x[i], y[i], z[i]);
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
