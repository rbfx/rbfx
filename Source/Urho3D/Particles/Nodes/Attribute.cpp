// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Attribute.h"

#include "Urho3D/Resource/XMLElement.h"
#include "Urho3D/Particles/Helpers.h"
#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphSystem.h"
#include "Urho3D/Particles/Span.h"
#include "Urho3D/Particles/UpdateContext.h"

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
