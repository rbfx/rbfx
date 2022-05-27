//
// Copyright (c) 2022 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "AttributeActionState.h"

#include "../Core/Context.h"
#include "../Core/Object.h"
#include "../IO/Log.h"
#include "../Scene/Serializable.h"

namespace Urho3D
{
namespace Actions
{
/// Construct.
AttributeActionState::AttributeActionState(
    FiniteTimeAction* action, Object* target, ea::string_view attribute, VariantType type)
    : FiniteTimeActionState(action, target)
    , attributeType_(type)
{
    auto serializable = target->Cast<Serializable>();
    if (!serializable)
    {
        URHO3D_LOGERROR(
            Format("Can animate only serializable class but {} is not serializable.", target->GetTypeName()));
        return;
    }
    attribute_ = target->GetContext()->GetReflection(target->GetType())->GetAttribute(attribute);
    if (!attribute_)
    {
        URHO3D_LOGERROR(Format("Attribute {} not found in {}.", attribute, target->GetTypeName()));
        return;
    }
    if (attributeType_ != VAR_NONE && attribute_->type_ != attributeType_)
    {
        URHO3D_LOGERROR(Format("Attribute {} is not of type {}.", attribute, Variant::GetTypeName(attributeType_)));
        attribute_ = nullptr;
        return;
    }
}

/// Destruct.
AttributeActionState::~AttributeActionState() {}

void AttributeActionState::Update(float dt, Variant& value) {}

void AttributeActionState::Get(Variant& value) const
{
    if (attribute_)
    {
        auto* serializable = static_cast<Serializable*>(GetTarget());
        return attribute_->accessor_->Get(serializable, value);
    }
    value.Clear();
}

void AttributeActionState::Set(const Variant& value)
{
    if (attribute_)
    {
        auto* serializable = static_cast<Serializable*>(GetTarget());
        attribute_->accessor_->Set(serializable, value);
    }
}

void AttributeActionState::Update(float dt)
{
    if (!attribute_)
        return;

    Variant dst;
    Get(dst);
    if (attributeType_ != VAR_NONE && dst.GetType() != attributeType_)
    {
        URHO3D_LOGERROR(
            Format("Attribute {} value is not of type {}.", attribute_->name_, Variant::GetTypeName(attributeType_)));
        attribute_ = nullptr;
        return;
    }

    Update(dt, dst);

    Set(dst);
}

SetAttributeState::SetAttributeState(
    FiniteTimeAction* action, Object* target, ea::string_view attribute, const Variant& value)
    : AttributeActionState(action, target, attribute, value.GetType())
    , value_(value)
    , triggered_(false)
{
}

void SetAttributeState::Update(float time, Variant& var)
{
    if (!triggered_)
    {
        var = value_;
        triggered_ = true;
    }
}

AttributeBlinkState::AttributeBlinkState(FiniteTimeAction* action, Object* target, ea::string_view attribute,
    Variant from,
    Variant to, unsigned times)
    : AttributeActionState(action, target, attribute, from.GetType())
    , from_(from)
    , to_(to)
{
    times_ = Max(1, times);
    Get(originalState_);
}

void AttributeBlinkState::Update(float time, Variant& var)
{
    const auto slice = 1.0f / static_cast<float>(times_);
    const auto m = Mod(time, slice);
    var = (m > slice / 2)?from_:to_;
}

void AttributeBlinkState::Stop()
{
    Set(originalState_);
}

} // namespace Actions
} // namespace Urho3D
