// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/Nodes/Curve.h"

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
template <typename T> struct DispatchUpdate
{
    void operator()(const UpdateContext& context, Curve::Instance* instance, ParticleGraphPinRef* pinRefs)
    {
        RunUpdate<Curve::Instance, float, T>(context, *instance, pinRefs);
    }
};
} // namespace

Curve::Instance::Instance(Curve* node)
    :node_(node)
{
}

void Curve::Instance::Update(UpdateContext& context)
{
    ParticleGraphPinRef pinRefs[2];
    for (unsigned i = 0; i < 2; ++i)
    {
        pinRefs[i] = node_->pins_[i].GetMemoryReference();
    }
    SelectByVariantType<DispatchUpdate>(node_->pins_[1].GetValueType(), context, this, pinRefs);

}

Curve::Curve(Context* context)
    : ParticleGraphNode(context)
    , duration_(1)
    , isLooped_(false)
    , pins_{
        ParticleGraphPin(ParticleGraphPinFlag::Input, "t", VAR_FLOAT),
        ParticleGraphPin(ParticleGraphPinFlag::MutableType, "out")
    }
{
}

void Curve::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Curve>();

    URHO3D_ACCESSOR_ATTRIBUTE("Duration", GetDuration, SetDuration, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("IsLooped", IsLooped, SetLooped, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Curve", GetCurve, SetCurve, VariantCurve, VariantCurve(), AM_DEFAULT);
}

void Curve::SetCurve(const VariantCurve& curve)
{
    curve_ = curve;
    SetPinValueType(1, curve_.GetType());
}

Variant Curve::Sample(float time) const
{
    unsigned frameIndex;
    return curve_.Sample(time, duration_, isLooped_, frameIndex);
}

VariantType Curve::EvaluateOutputPinType(ParticleGraphPin& pin)
{
    return curve_.GetType();
}

unsigned Curve::EvaluateInstanceSize() const
{
    return sizeof(Instance);
}

ParticleGraphNodeInstance* Curve::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    return new (ptr) Instance(this);
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
