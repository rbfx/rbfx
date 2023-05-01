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

#include "../../Precompiled.h"

#include "Uniform.h"

#include "../Helpers.h"
#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"

namespace Urho3D
{

namespace
{
template <typename T> struct GetValue
{
    void operator()(const UpdateContext& context, unsigned uniformIndex, const ParticleGraphPin& pin0)
    {
        auto dst = context.GetSpan<T>(pin0.GetMemoryReference());
        dst[0] = context.layer_->GetUniform(uniformIndex).Get<T>();
    }
};
template <typename T> struct SetValue
{
    void operator()(
        const UpdateContext& context, unsigned uniformIndex, const ParticleGraphPin& pin0, const ParticleGraphPin& pin1)
    {
        auto src = context.GetSpan<T>(pin1.GetMemoryReference());
        auto dst = context.GetSpan<T>(pin0.GetMemoryReference());
        context.layer_->GetUniform(uniformIndex) = src[0];
        dst[0] = src[0];
    }
};

} // namespace

namespace ParticleGraphNodes
{

Uniform::Uniform(Context* context)
    : ParticleGraphNode(context)
{
}

void Uniform::SetUniformType(VariantType valueType) { SetPinValueType(0, valueType); }

GetUniform::GetUniform(Context* context)
    : Uniform(context)
    , pins_{ParticleGraphPin(ParticleGraphPinFlag::MutableName | ParticleGraphPinFlag::MutableType, "uniform", VAR_FLOAT, ParticleGraphContainerType::Scalar)}
{
}

void GetUniform::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<GetUniform>(); }

GetUniform::Instance::Instance(GetUniform* node, unsigned uniformIndex)
    : node_(node)
    , uniformIndex_(uniformIndex)
{
}

void GetUniform::Instance::Update(UpdateContext& context)
{
    const ParticleGraphPin& pin0 = node_->pins_[0];
    SelectByVariantType<GetValue>(pin0.GetValueType(), context, uniformIndex_, pin0);
};

ParticleGraphPin* GetUniform::LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin)
{
    SetPinName(0, pin.GetName());
    SetPinValueType(0, pin.GetType());
    return &pins_[0];
}

SetUniform::SetUniform(Context* context)
    : Uniform(context)
    , pins_{
          ParticleGraphPin(ParticleGraphPinFlag::MutableName | ParticleGraphPinFlag::MutableType, "uniform", VAR_FLOAT, ParticleGraphContainerType::Scalar),
          ParticleGraphPin(ParticleGraphPinFlag::Input | ParticleGraphPinFlag::MutableType, "", VAR_FLOAT, ParticleGraphContainerType::Scalar),
      }
{
}

void SetUniform::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<SetUniform>(); }

void SetUniform::SetUniformType(VariantType valueType)
{
    SetPinValueType(0, valueType);
    SetPinValueType(1, valueType);
}

ParticleGraphPin* SetUniform::LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin)
{
    SetPinName(0, pin.GetName());
    SetPinValueType(0, pin.GetType());
    return &pins_[0];
}

SetUniform::Instance::Instance(SetUniform* node, unsigned uniformIndex)
    : node_(node)
    , uniformIndex_(uniformIndex)
{
}

void SetUniform::Instance::Update(UpdateContext& context)
{
    const ParticleGraphPin& pin0 = node_->pins_[0];
    const ParticleGraphPin& pin1 = node_->pins_[1];
    SelectByVariantType<SetValue>(pin0.GetValueType(), context, uniformIndex_, pin0, pin1);
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
