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

#include "Bounce.h"
#include "ParticleGraphSystem.h"

#include "Helpers.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
Bounce::Bounce(Context* context)
    : AbstractNodeType(context, PinArray
        {
            ParticleGraphPin(PGPIN_INPUT, "position"),
            ParticleGraphPin(PGPIN_INPUT, "velocity"),
            ParticleGraphPin(PGPIN_NONE, "newPosition"),
            ParticleGraphPin(PGPIN_NONE, "newVelocity"),
        })
{
}

void Bounce::RegisterObject(ParticleGraphSystem* context)
{
    context->RegisterParticleGraphNodeFactory<Bounce>();
}

void Bounce::RayCastAndBounce(UpdateContext& context, Vector3& pos, Vector3& velocity)
{
    pos += velocity * context.timeStep_;
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
