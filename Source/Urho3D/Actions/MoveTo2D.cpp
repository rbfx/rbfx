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

#include "MoveTo2D.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "AttributeActionState.h"
#include "FiniteTimeActionState.h"

using namespace Urho3D;

namespace
{
class MoveTo2DState : public AttributeActionState
{
    Vector2 positionDelta_;
    Vector2 startPosition_;

public:
    MoveTo2DState(MoveTo2D* action, Object* target)
        : AttributeActionState(action, target, "Position")
    {
        positionDelta_ = action->GetPosition();
        if (attribute_)
        {
            if (attribute_->type_ == VAR_VECTOR2)
                startPosition_ = Get<Vector2>();
            else if (attribute_->type_ == VAR_INTVECTOR2)
                startPosition_ = Vector2(Get<IntVector2>());
            else
            {
                URHO3D_LOGERROR(
                    Format("Attribute {} is not of type VAR_VECTOR2 or VAR_INTVECTOR2.", attribute_->name_));
                attribute_ = nullptr;
                return;
            }
        }
    }

    ~MoveTo2DState() override = default;

    void Update(float time, Variant& value) override
    {
        if (attribute_->type_ == VAR_VECTOR2)
        {
            const auto newPos = startPosition_ + positionDelta_ * time;
            value = newPos;
        }
        else
        {
            const auto newPos = startPosition_ + positionDelta_ * time;
            const IntVector2 newIntPos = IntVector2(newPos.x_, newPos.y_);
            value = newIntPos;
        }
    }
};

} // namespace

/// Construct.
MoveTo2D::MoveTo2D(Context* context)
    : FiniteTimeAction(context)
{
}

/// Construct.
MoveTo2D::MoveTo2D(Context* context, float duration, const Vector2& position)
    : FiniteTimeAction(context, duration)
    , position_(position)
{
}

/// Destruct.
MoveTo2D::~MoveTo2D() {}

/// Register object factory.
void MoveTo2D::RegisterObject(Context* context) { context->RegisterFactory<MoveTo2D>(); }

SharedPtr<ActionState> MoveTo2D::StartAction(Object* target) { return MakeShared<MoveTo2DState>(this, target); }
