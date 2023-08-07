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

#include "Attribute.h"

#include "../../Resource/XMLElement.h"
#include "../Helpers.h"
#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
namespace
{

template <typename T> struct CopyValues
{
    void operator()(const UpdateContext& context, const ParticleGraphPin& pin0, const ParticleGraphPin& pin1)
    {
        const unsigned numParticles = context.indices_.size();

        auto src = context.GetSpan<T>(pin1.GetMemoryReference());
        auto dst = context.GetSpan<T>(pin0.GetMemoryReference());
        for (unsigned i = 0; i < numParticles; ++i)
        {
            dst[i] = src[i];
        }
    }
};

} // namespace

Attribute::Attribute(Context* context)
    : ParticleGraphNode(context)
{
}

void Attribute::SetAttributeType(VariantType valueType)
{
    SetPinValueType(0, valueType);
}

GetAttribute::GetAttribute(Context* context)
    : Attribute(context)
    , pins_{ ParticleGraphPin(ParticleGraphPinFlag::MutableName | ParticleGraphPinFlag::MutableType, "attr", VAR_FLOAT,
                                              ParticleGraphContainerType::Sparse)}
{
}

void GetAttribute::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<GetAttribute>();
}

ParticleGraphPin* GetAttribute::LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin)
{
    SetPinName(0, pin.GetName());
    SetPinValueType(0, pin.GetType());
    return &pins_[0];
}

SetAttribute::SetAttribute(Context* context)
    : Attribute(context)
    , pins_{
          ParticleGraphPin(ParticleGraphPinFlag::MutableName | ParticleGraphPinFlag::MutableType, "attr", VAR_FLOAT,
              ParticleGraphContainerType::Sparse),
          ParticleGraphPin(ParticleGraphPinFlag::Input | ParticleGraphPinFlag::MutableType, "", VAR_FLOAT),
    }
{
}

void SetAttribute::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<SetAttribute>();
}

void SetAttribute::SetAttributeType(VariantType valueType)
{
    SetPinValueType(0, valueType);
    SetPinValueType(1, valueType);
}

ParticleGraphPin* SetAttribute::LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin)
{
    SetPinName(0, pin.GetName());
    SetPinValueType(0, pin.GetType());
    return &pins_[0];
}

SetAttribute::Instance::Instance(SetAttribute* node)
    : node_(node)
{
}
void SetAttribute::Instance::Update(UpdateContext& context)
{
    const ParticleGraphPin& pin0 = node_->pins_[0];
    const ParticleGraphPin& pin1 = node_->pins_[1];
    SelectByVariantType<CopyValues>(pin0.GetValueType(), context, pin0, pin1);
};

} // namespace ParticleGraphNodes
} // namespace Urho3D
