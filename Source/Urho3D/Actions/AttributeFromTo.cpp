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

#include "AttributeFromTo.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "AttributeActionState.h"
#include "FiniteTimeActionState.h"

using namespace Urho3D;

namespace
{
class AttributeFromToState : public AttributeActionState
{
    Variant from_;
    Variant to_;

public:
    AttributeFromToState(AttributeFromTo* action, Object* target)
        : AttributeActionState(action, target, action->GetName().c_str())
        , from_(action->GetFrom())
        , to_(action->GetTo())
    {
    }

    ~AttributeFromToState() override = default;

    void Update(float time, Variant& value) override
    {
        value = from_.Lerp(to_, time);
    }
};

} // namespace

/// Construct.
AttributeFromTo::AttributeFromTo(Context* context)
    : FiniteTimeAction(context)
{
}

/// Construct.
AttributeFromTo::AttributeFromTo(
    Context* context, float duration, const ea::string& attribute, const Variant& from, const Variant& to)
    : FiniteTimeAction(context, duration)
    , name_(attribute)
    , from_(from)
    , to_(to)
{
}

/// Destruct.
AttributeFromTo::~AttributeFromTo() {}

/// Register object factory.
void AttributeFromTo::RegisterObject(Context* context) { context->RegisterFactory<AttributeFromTo>(); }

SharedPtr<ActionState> AttributeFromTo::StartAction(Object* target) { return MakeShared<AttributeFromToState>(this, target); }

/// Create reversed action.
SharedPtr<FiniteTimeAction> AttributeFromTo::Reverse() const
{
    return MakeShared<AttributeFromTo>(context_, GetDuration(), name_, to_, from_);
}
