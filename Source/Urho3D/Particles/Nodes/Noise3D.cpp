
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

#include "Noise3D.h"

#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"
#include "Noise3DInstance.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
void Noise3D::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Noise3D>();
}


Noise3D::Noise3D(Context* context)
    : BaseNodeType(context
    , PinArray {
        ParticleGraphPin(ParticleGraphPinFlag::Input, "x", ParticleGraphContainerType::Auto),
        ParticleGraphPin(ParticleGraphPinFlag::Output, "out", ParticleGraphContainerType::Auto),
    })
{
}

/// Evaluate size required to place new node instance.
unsigned Noise3D::EvaluateInstanceSize() const
{
    return sizeof(Noise3DInstance);
}

/// Place new instance at the provided address.
ParticleGraphNodeInstance* Noise3D::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    Noise3DInstance* instance = new (ptr) Noise3DInstance();
    instance->Init(this, layer);
    return instance;
}

} // namespace ParticleGraphNodes
} // namespace Urho3D
