// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Actions/Attribute.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/ArchiveSerializationBasic.h"
#include "Urho3D/IO/ArchiveSerializationVariant.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Actions/AttributeActionState.h"
#include "Urho3D/Actions/FiniteTimeActionState.h"

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
