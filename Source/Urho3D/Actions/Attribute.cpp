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

#include "Attribute.h"

#include "../Core/Context.h"
#include "../IO/ArchiveSerializationBasic.h"
#include "../IO/ArchiveSerializationVariant.h"
#include "../IO/Log.h"
#include "AttributeActionState.h"
#include "FiniteTimeActionState.h"

namespace Urho3D
{
namespace Actions
{

namespace
{
class AttributeFromToState : public AttributeActionState
{
    Variant from_;
    Variant to_;

public:
    AttributeFromToState(AttributeFromTo* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
        , from_(action->GetFrom())
        , to_(action->GetTo())
    {
    }

    void Update(float time, Variant& value) override { value = from_.Lerp(to_, time); }
};

class AttributeToState : public AttributeActionState
{
    Variant from_;
    Variant to_;

public:
    AttributeToState(AttributeTo* action, Object* target, AttributeInfo* attribute)
        : AttributeActionState(action, target, attribute)
        , to_(action->GetTo())
    {
        if (attribute_)
        {
            attribute_->accessor_->Get(static_cast<const Serializable*>(target), from_);
        }
    }

    void Update(float time, Variant& value) override { value = from_.Lerp(to_, time); }
};
} // namespace

/// Construct.
AttributeFromTo::AttributeFromTo(Context* context)
    : BaseClassName(context)
{
}

// Set "from" value.
void AttributeFromTo::SetFrom(const Variant& variant) { from_ = variant; }

// Get "to" value.
void AttributeFromTo::SetTo(const Variant& variant) { to_ = variant; }

/// Create reversed action.
SharedPtr<FiniteTimeAction> AttributeFromTo::Reverse() const
{
    auto result = MakeShared<AttributeFromTo>(context_);
    result->SetDuration(GetDuration());
    result->SetAttributeName(GetAttributeName());
    result->SetFrom(to_);
    result->SetTo(from_);
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void AttributeFromTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "from", from_, Variant::EMPTY);
    SerializeOptionalValue(archive, "to", to_, Variant::EMPTY);
}

/// Create new action state from the action.
SharedPtr<ActionState> AttributeFromTo::StartAction(Object* target)
{
    return MakeShared<AttributeFromToState>(this, target, GetAttribute(target));
}

/// Construct.
AttributeTo::AttributeTo(Context* context)
    : BaseClassName(context)
{
}

// Get "to" value.
void AttributeTo::SetTo(const Variant& variant) { to_ = variant; }

/// Serialize content from/to archive. May throw ArchiveException.
void AttributeTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "to", to_, Variant::EMPTY);
}

/// Create new action state from the action.
SharedPtr<ActionState> AttributeTo::StartAction(Object* target)
{
    return MakeShared<AttributeToState>(this, target, GetAttribute(target));
}

/// Construct.
AttributeBlink::AttributeBlink(Context* context)
    : BaseClassName(context)
{
}

// Set "from" value.
void AttributeBlink::SetFrom(const Variant& variant) { from_ = variant; }

// Get "to" value.
void AttributeBlink::SetTo(const Variant& variant) { to_ = variant; }

/// Create reversed action.
SharedPtr<FiniteTimeAction> AttributeBlink::Reverse() const
{
    auto result = MakeShared<AttributeFromTo>(context_);
    result->SetDuration(GetDuration());
    result->SetAttributeName(GetAttributeName());
    result->SetFrom(to_);
    result->SetTo(from_);
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void AttributeBlink::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "from", from_, Variant::EMPTY);
    SerializeOptionalValue(archive, "to", to_, Variant::EMPTY);
}

/// Create new action state from the action.
SharedPtr<ActionState> AttributeBlink::StartAction(Object* target)
{
    return MakeShared<AttributeBlinkState>(this, target, GetAttribute(target), from_, to_, times_);
}

} // namespace Actions
} // namespace Urho3D
