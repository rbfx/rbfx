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
#include "../IO/Log.h"
#include "../Scene/Serializable.h"

using namespace Urho3D;

/// Construct.
AttributeActionState::AttributeActionState(
    FiniteTimeAction* action, Object* target, const char* attribute, VariantType type)
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

void AttributeActionState::Update(float dt)
{
    if (!attribute_)
        return;

    auto* serializable = static_cast<Serializable*>(GetTarget());
    Variant dst;
    attribute_->accessor_->Get(serializable, dst);
    if (attributeType_ != VAR_NONE && dst.GetType() != attributeType_)
    {
        URHO3D_LOGERROR(
            Format("Attribute {} value is not of type {}.", attribute_->name_, Variant::GetTypeName(attributeType_)));
        attribute_ = nullptr;
        return;
    }

    Update(dt, dst);

    attribute_->accessor_->Set(serializable, dst);
}
