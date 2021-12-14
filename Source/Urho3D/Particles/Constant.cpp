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

#include "../Precompiled.h"

#include "Constant.h"
#include "ParticleGraphSystem.h"
#include "../IO/ArchiveSerialization.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{

const Variant& Constant::GetValue() const { return value_; }

void Constant::SetValue(const Variant& value)
{
    value_ = value;
    SetPinValueType(0, value.GetType());
}


Constant::Constant(Context* context)
    : ParticleGraphNode(context)
    , pins_{ParticleGraphPin(PGPIN_TYPE_MUTABLE, "out", VAR_NONE, PGCONTAINER_SCALAR)}
{
}

void Constant::RegisterObject(ParticleGraphSystem* context)
{
    context->RegisterParticleGraphNodeFactory<Constant>();

    context->RegisterAttribute<Constant>(Urho3D::AttributeInfo(
        VAR_NONE, "Value",
        Urho3D::MakeVariantAttributeAccessor<Constant>(
            [](const Constant& self, Urho3D::Variant& value) { value = self.GetValue(); },
            [](Constant& self, const Urho3D::Variant& value) { self.SetValue(value); }),
        nullptr, Variant(), AM_DEFAULT));
}

Constant::Instance::Instance(Constant* node)
    : node_(node)
{
}
//bool Constant::Serialize(Archive& archive)
//{
//    SerializeValue(archive, "value", value_);
//    return ParticleGraphNode::Serialize(archive);
//}
void Constant::Instance::Update(UpdateContext& context)
{
    const auto& pin0 = node_->pins_[0];
    switch (node_->value_.GetType())
    {
    case VAR_INT:
        context.GetScalar<int>(pin0.GetMemoryReference())[0] = node_->value_.GetInt();
        break;
    case VAR_BOOL:
        context.GetScalar<bool>(pin0.GetMemoryReference())[0] = node_->value_.GetBool();
        break;
    case VAR_INT64:
        context.GetScalar<long long>(pin0.GetMemoryReference())[0] = node_->value_.GetInt64();
        break;
    case VAR_FLOAT:
        context.GetScalar<float>(pin0.GetMemoryReference())[0] = node_->value_.GetFloat();
        break;
    case VAR_VECTOR2:
        context.GetScalar<Vector2>(pin0.GetMemoryReference())[0] = node_->value_.GetVector2();
        break;
    case VAR_VECTOR3:
        context.GetScalar<Vector3>(pin0.GetMemoryReference())[0] = node_->value_.GetVector3();
        break;
    case VAR_VECTOR4:
        context.GetScalar<Vector4>(pin0.GetMemoryReference())[0] = node_->value_.GetVector4();
        break;
    case VAR_COLOR:
        context.GetScalar<Color>(pin0.GetMemoryReference())[0] = node_->value_.GetColor();
        break;
    case VAR_VARIANTCURVE:
        context.GetScalar<const VariantCurve*>(pin0.GetMemoryReference())[0] = &node_->value_.GetVariantCurve();
        break;
    default:
        assert(!"Not implemented yet");
        break;
    }
};
} // namespace ParticleGraphNodes
} // namespace Urho3D
