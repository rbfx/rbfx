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

#include "Environment.h"

#include "Helpers.h"
#include "ParticleGraphSystem.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
TimeStep::TimeStep(Context* context)
    : AbstractNodeType(context, PinArray{ParticleGraphPin(PGPIN_NONE, "out", PGCONTAINER_SCALAR)})
{
}
void TimeStep::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<TimeStep>();
}

EffectTime::EffectTime(Context* context)
    : AbstractNodeType(context, PinArray{ParticleGraphPin(PGPIN_NONE, "out",PGCONTAINER_SCALAR)})
{
}

void EffectTime::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<EffectTime>();
}

NormalizedEffectTime::NormalizedEffectTime(Context* context)
    : AbstractNodeType(context, PinArray{ParticleGraphPin(PGPIN_NONE, "out", PGCONTAINER_SCALAR)})
{
}

void NormalizedEffectTime::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<NormalizedEffectTime>();
}

Move::Move(Context* context)
    : AbstractNodeType(context, PinArray{
                                    ParticleGraphPin(PGPIN_INPUT, "position"),
                                    ParticleGraphPin(PGPIN_INPUT, "velocity"),
                                    ParticleGraphPin(PGPIN_NONE, "newPosition"),
                                })
{
}

void Move::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Move>();
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
