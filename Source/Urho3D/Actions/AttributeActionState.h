// Copyright (c) 2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "FiniteTimeActionState.h"
#include "../Core/Attribute.h"
#include "../Scene/Serializable.h"

namespace Urho3D
{
namespace Actions
{

/// Attribute action state.
class URHO3D_API AttributeActionState : public FiniteTimeActionState
{
public:
    /// Construct.
    AttributeActionState(FiniteTimeAction* action, Object* target, AttributeInfo* attribute);
    /// Destruct.
    ~AttributeActionState() override;

protected:
    /// Called every frame with it's delta time and attribute value.
    virtual void Update(float dt, Variant& value);

    void Get(Variant& value) const;

    void Set(const Variant& value);

    /// Get attribute value or default value.
    template <typename T> T Get() const
    {
        Variant tmp;
        if (attribute_)
        {
            attribute_->accessor_->Get(static_cast<const Serializable*>(GetTarget()), tmp);
        }
        return tmp.Get<T>();
    }

private:
    void Update(float dt) override;

protected:
    AttributeInfo* attribute_{};
};

class SetAttributeState : public AttributeActionState
{
public:
    SetAttributeState(FiniteTimeAction* action, Object* target, AttributeInfo* attribute, const Variant& value);

private:
    void Update(float time, Variant& var) override;

private:
    Variant value_;
    bool triggered_{};
};


class AttributeBlinkState : public AttributeActionState
{
public:
    AttributeBlinkState(
        FiniteTimeAction* action, Object* target, AttributeInfo* attribute, Variant from,
        Variant to, unsigned times);

    void Update(float time, Variant& var) override;

    void Stop() override;

private:
    unsigned times_;
    Variant originalState_;
    Variant from_;
    Variant to_;
};

} // namespace Actions
} // namespace Urho3D
