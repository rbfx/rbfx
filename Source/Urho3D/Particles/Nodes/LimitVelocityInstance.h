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

#include "LimitVelocity.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

class LimitVelocityInstance final : public LimitVelocity::InstanceBase
{
public:
    void operator()(const UpdateContext& context, unsigned numParticles, const SparseSpan<Vector3>& vel,
        const SparseSpan<float>& limit, const SparseSpan<Vector3>& result)
    {
        const float dampen = static_cast<LimitVelocity*>(GetGraphNode())->GetDampen();
        if (dampen <= 1e-6f || context.timeStep_ < 1e-6f)
        {
            for (unsigned i = 0; i < numParticles; ++i)
            {
                result[i] = vel[i];
            }
        }
        else
        {
            const auto t = 1.0f - powf(1.0f - dampen, 20.0f * context.timeStep_);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                const float speed = vel[i].Length();
                const float limitVal = limit[i];
                if (speed > limitVal + 1e-6f)
                {
                    result[i] = vel[i] * (Urho3D::Lerp(speed, limitVal, t) / speed);
                }
            }
        }
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
