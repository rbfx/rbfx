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

#include "Curve.h"

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
