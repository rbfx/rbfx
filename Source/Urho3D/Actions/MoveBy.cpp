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

#include "MoveBy.h"

#include "FiniteTimeActionState.h"
#include "../Core/Context.h"

using namespace Urho3D;

namespace
{
class MoveByState: public FiniteTimeActionState
{
    Vector3 positionDelta_;
    Vector3 startPosition_;
    Vector3 previousPosition_;

public:
    MoveByState(MoveBy* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        positionDelta_ = action->GetPositionDelta();
        Node* node = target->Cast<Node>();
        if (node)
            previousPosition_ = startPosition_ = node->GetPosition();
    }

    void Update(float time) override
    {
        Node* node = GetTarget()->Cast<Node>();
        if (node)
        {
            const auto currentPos = node->GetPosition();
            auto diff = currentPos - previousPosition_;
            startPosition_ = startPosition_ + diff;
            const auto newPos = startPosition_ + positionDelta_ * time;
            node->SetPosition(newPos);
            previousPosition_ = newPos;
        }
    }
};

}

/// Construct.
MoveBy::MoveBy(Context* context)
    : FiniteTimeAction(context)
{
}

    /// Construct.
MoveBy::MoveBy(Context* context, float duration, const Vector3& position)
    : FiniteTimeAction(context, duration)
    , position_(position)
{
}


/// Destruct.
MoveBy::~MoveBy() {
    
}

/// Register object factory.
void MoveBy::RegisterObject(Context* context) { context->RegisterFactory<MoveBy>(); }

FiniteTimeAction* MoveBy::Reverse() const
{
    return new MoveBy(context_, GetDuration(), -position_);
}

SharedPtr<ActionState> MoveBy::StartAction(Object* target)
{
    return MakeShared<MoveByState>(this, target);
}
