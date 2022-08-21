
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

#include "LimitVelocity.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "LimitVelocityInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void LimitVelocity::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<LimitVelocity>();
    URHO3D_ACCESSOR_ATTRIBUTE("Dampen", GetDampen, SetDampen, float, float{}, AM_DEFAULT);
}


LimitVelocity::LimitVelocity(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "velocity", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Input, "limit", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned LimitVelocity::EvaluateInstanceSize() const
{
    return sizeof(LimitVelocityInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* LimitVelocity::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    LimitVelocityInstance* instance = new (ptr) LimitVelocityInstance();
    instance->Init(this, layer);
    return instance;
}

void LimitVelocity::SetDampen(float value) { dampen_ = value; }

float LimitVelocity::GetDampen() const { return dampen_; }

} // namespace ParticleGraphNodes
} // namespace Urho3D
