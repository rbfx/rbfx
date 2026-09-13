// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Uniform.h"

#include "Urho3D/Particles/Helpers.h"
#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"

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
