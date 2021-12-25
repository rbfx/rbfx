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

Slerp::Slerp(Context* context)
    : AbstractNodeType(context, PinArray{
                                    ParticleGraphPin(PGPIN_INPUT, "x"),
                                    ParticleGraphPin(PGPIN_INPUT, "y"),
                                    ParticleGraphPin(PGPIN_INPUT, "t"),
                                    ParticleGraphPin(PGPIN_NONE, "out"),
                                })
{
}

void Slerp::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Slerp>();
}

MakeVec2::MakeVec2(Context* context)
    : AbstractNodeType(context,
        PinArray{
            ParticleGraphPin(PGPIN_INPUT, "x"),
            ParticleGraphPin(PGPIN_INPUT, "y"),
            ParticleGraphPin(PGPIN_NONE, "out"),
        })
{
}

void MakeVec2::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<MakeVec2>(); }

MakeVec3::MakeVec3(Context* context)
    : AbstractNodeType(context,
        PinArray{
            ParticleGraphPin(PGPIN_INPUT, "x"),
            ParticleGraphPin(PGPIN_INPUT, "y"),
            ParticleGraphPin(PGPIN_INPUT, "z"),
            ParticleGraphPin(PGPIN_NONE, "out"),
        })
{
}

void MakeVec3::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<MakeVec3>(); }
}

} // namespace Urho3D
