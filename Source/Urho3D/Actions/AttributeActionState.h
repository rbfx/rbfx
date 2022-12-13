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
