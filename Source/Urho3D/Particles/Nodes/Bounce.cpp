
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

#include "../../Precompiled.h"

#include "Bounce.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "BounceInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Bounce::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Bounce>();
    URHO3D_ACCESSOR_ATTRIBUTE("Dampen", GetDampen, SetDampen, float, float{}, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("BounceFactor", GetBounceFactor, SetBounceFactor, float, float{}, AM_DEFAULT);
}


Bounce::Bounce(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "position", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "velocity", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "newPosition", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "newVelocity", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Bounce::EvaluateInstanceSize() const
{
    return sizeof(BounceInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Bounce::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    BounceInstance* instance = new (ptr) BounceInstance();
    instance->Init(this, layer);
    return instance;
}

void Bounce::SetDampen(float value) { dampen_ = value; }

float Bounce::GetDampen() const { return dampen_; }

void Bounce::SetBounceFactor(float value) { bounceFactor_ = value; }

float Bounce::GetBounceFactor() const { return bounceFactor_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
