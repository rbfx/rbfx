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

#include "AttributeAction.h"
#include "FiniteTimeAction.h"
#include "../Core/Context.h"
#include "../Core/Object.h"
#include "../IO/Log.h"
#include "../Scene/Serializable.h"

namespace Urho3D
{
namespace Actions
{
/// Construct.
AttributeActionState::AttributeActionState(AttributeAction* action, Object* target)
    : FiniteTimeActionState(action, target)
    , attribute_(action->GetAttribute(target))
{
}

AttributeActionState::AttributeActionState(AttributeActionInstant* action, Object* target)
    : FiniteTimeActionState(action, target)
    , attribute_(action->GetAttribute(target))
{
}

/// Destruct.
AttributeActionState::~AttributeActionState() = default;

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
    Update(dt, dst);
    Set(dst);
}


} // namespace Actions
} // namespace Urho3D
