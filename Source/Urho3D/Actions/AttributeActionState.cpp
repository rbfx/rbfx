// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    FiniteTimeAction* action, Object* target, AttributeInfo* attribute)
    : FiniteTimeActionState(action, target)
    , attribute_(attribute)
{
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
    Update(dt, dst);
    Set(dst);
}

SetAttributeState::SetAttributeState(
    FiniteTimeAction* action, Object* target, AttributeInfo* attribute, const Variant& value)
    : AttributeActionState(action, target, attribute)
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

AttributeBlinkState::AttributeBlinkState(
    FiniteTimeAction* action, Object* target, AttributeInfo* attribute,
    Variant from,
    Variant to, unsigned times)
    : AttributeActionState(action, target, attribute)
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
