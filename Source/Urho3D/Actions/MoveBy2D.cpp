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

#include "MoveBy2D.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "AttributeActionState.h"
#include "FiniteTimeActionState.h"

using namespace Urho3D;

namespace
{
class MoveBy2DState : public AttributeActionState
{
    Vector2 positionDelta_;
    Vector2 startPosition_;
    Vector2 previousPosition_;

public:
    MoveBy2DState(MoveBy2D* action, Object* target)
        : AttributeActionState(action, target, "Position")
    {
        positionDelta_ = action->GetPositionDelta();
        if (attribute_)
        {
            if (attribute_->type_ == VAR_VECTOR2)
                previousPosition_ = startPosition_ = Get<Vector2>();
            else if (attribute_->type_ == VAR_INTVECTOR2)
                previousPosition_ = startPosition_ = Vector2(Get<IntVector2>());
            else
            {
                URHO3D_LOGERROR(Format("Attribute {} is not of type VAR_VECTOR2 or VAR_INTVECTOR2.", attribute_->name_));
                attribute_ = nullptr;
                return;
            }
        }
    }

    ~MoveBy2DState() override = default;

    void Update(float time, Variant& value) override
    {
        if (attribute_->type_ == VAR_VECTOR2)
        {
            const auto currentPos = value.GetVector2();
            auto diff = currentPos - previousPosition_;
            startPosition_ = startPosition_ + diff;
            const auto newPos = startPosition_ + positionDelta_ * time;
            value = newPos;
            previousPosition_ = newPos;
        }
        else
        {
            const auto currentPos = Vector2(value.GetIntVector2());
            auto diff = currentPos - previousPosition_;
            startPosition_ = startPosition_ + diff;
            const auto newPos = startPosition_ + positionDelta_ * time;
            const IntVector2 newIntPos = IntVector2(newPos.x_, newPos.y_);
            value = newIntPos;
            previousPosition_ = Vector2(newIntPos);
        }
    }
};

} // namespace

/// Construct.
MoveBy2D::MoveBy2D(Context* context)
    : FiniteTimeAction(context)
{
}

/// Construct.
MoveBy2D::MoveBy2D(Context* context, float duration, const Vector2& position)
    : FiniteTimeAction(context, duration)
    , position_(position)
{
}

/// Destruct.
MoveBy2D::~MoveBy2D() {}

/// Register object factory.
void MoveBy2D::RegisterObject(Context* context) { context->RegisterFactory<MoveBy2D>(); }

SharedPtr<FiniteTimeAction> MoveBy2D::Reverse() const { return MakeShared<MoveBy2D>(context_, GetDuration(), -position_); }

SharedPtr<ActionState> MoveBy2D::StartAction(Object* target) { return MakeShared<MoveBy2DState>(this, target); }
