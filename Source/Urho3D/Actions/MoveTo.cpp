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

#include "MoveTo.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "AttributeActionState.h"
#include "FiniteTimeActionState.h"

using namespace Urho3D;

namespace
{
class MoveToState : public AttributeActionState
{
    Vector3 positionDelta_;
    Vector3 startPosition_;

public:
    MoveToState(MoveTo* action, Object* target)
        : AttributeActionState(action, target, "Position")
    {
        positionDelta_ = action->GetPosition();
        if (attribute_)
        {
            if (attribute_->type_ == VAR_VECTOR3)
                startPosition_ = Get<Vector3>();
            else if (attribute_->type_ == VAR_INTVECTOR3)
                startPosition_ = Vector3(Get<IntVector3>());
            else {
                URHO3D_LOGERROR(Format("Attribute {} is not of type VAR_VECTOR3 or VAR_INTVECTOR3.", attribute_->name_));
                attribute_ = nullptr;
                return;
            }
        }
    }

    ~MoveToState() override = default;

    void Update(float time, Variant& value) override
    {
        const Vector3 result = startPosition_ + positionDelta_ * time;
        if (attribute_->type_ == VAR_VECTOR3)
            value = result;
        else
            value = IntVector3(result.x_, result.y_, result.z_);
    }
};

} // namespace

/// Construct.
MoveTo::MoveTo(Context* context)
    : FiniteTimeAction(context)
{
}

/// Construct.
MoveTo::MoveTo(Context* context, float duration, const Vector3& position)
    : FiniteTimeAction(context, duration)
    , position_(position)
{
}

/// Destruct.
MoveTo::~MoveTo() {}

/// Register object factory.
void MoveTo::RegisterObject(Context* context) { context->RegisterFactory<MoveTo>(); }

SharedPtr<ActionState> MoveTo::StartAction(Object* target) { return MakeShared<MoveToState>(this, target); }
