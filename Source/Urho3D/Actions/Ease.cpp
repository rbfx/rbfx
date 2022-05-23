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

#include "Ease.h"

#include "../Math/EaseMath.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../IO/ArchiveSerializationBasic.h"
#include "FiniteTimeActionState.h"

using namespace Urho3D;

namespace Urho3D
{
class ActionEaseState : public FiniteTimeActionState
{

public:
    ActionEaseState(ActionEase* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        innerAction_.DynamicCast(action->GetInnerAction()->StartAction(target));
    }

    void StepInner(float dt)
    {
        if (innerAction_)
        {
            innerAction_->Step(BackIn(dt));
        }
    }

    private:
    SharedPtr<FiniteTimeActionState> innerAction_;
};

} // namespace

URHO3D_FINITETIMEACTIONDEF(ActionEase)

/// Construct.
ActionEase::ActionEase(Context* context, FiniteTimeAction* action)
    : FiniteTimeAction(context, action->GetDuration())
    , innerAction_(action)
{
}

SharedPtr<FiniteTimeAction> ActionEase::Reverse() const
{ return MakeShared<ActionEase>(context_, innerAction_->Reverse()); }

/// Serialize content from/to archive. May throw ArchiveException.
void ActionEase::SerializeInBlock(Archive& archive)
{
    FiniteTimeAction::SerializeInBlock(archive);
    SerializeValue(archive, "innerAction", innerAction_);
}

#define URHO3D_EASEACTIONDEF(typeName) \
namespace { \
struct Ease##typeName##State : public ActionEaseState { \
    Ease##typeName##State(ActionEase* action, Object* target): ActionEaseState(action, target) { } \
    void Step(float dt) override { StepInner(typeName(dt)); } \
    }; \
} \
URHO3D_FINITETIMEACTIONDEF(Ease##typeName) \
Ease##typeName::Ease##typeName(Context* context, FiniteTimeAction* action): ActionEase(context, action) {} \
void Ease##typeName::SerializeInBlock(Archive& archive) { ActionEase::SerializeInBlock(archive); }

URHO3D_EASEACTIONDEF(BackIn)

SharedPtr<FiniteTimeAction> EaseBackIn::Reverse() const
{
    return MakeShared<EaseBackOut>(context_, GetInnerAction()->Reverse());
}

URHO3D_EASEACTIONDEF(BackOut)

SharedPtr<FiniteTimeAction> EaseBackOut::Reverse() const
{
    return MakeShared<EaseBackIn>(context_, GetInnerAction()->Reverse());
}
