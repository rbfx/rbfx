//
// Copyright (c) 2015 Xamarin Inc.
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

#include "CallFunc.h"

#include "../Core/Context.h"
#include "../IO/ArchiveSerializationBasic.h"
#include "../IO/ArchiveSerializationVariant.h"
#include "FiniteTimeActionState.h"

namespace Urho3D
{
namespace Actions
{

namespace
{
class CallFuncState : public FiniteTimeActionState
{
public:
    CallFuncState(CallFunc* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        callHandler_ = action->GetCallHandler();
    }

    void Update(float time) override
    {
        if (callHandler_)
            callHandler_->Invoke(GetTarget());
    }

private:
    SharedPtr<ActionCallHandler> callHandler_;
};
} // namespace

/// Construct.
CallFunc::CallFunc(Context* context)
    : BaseClassName(context)
{
}

/// Set call handler
void CallFunc::SetCallHandler(ActionCallHandler* handler) { actionCallHandler_ = handler; }

/// Create new action state from the action.
SharedPtr<ActionState> CallFunc::StartAction(Object* target) { return MakeShared<CallFuncState>(this, target); }

namespace
{
class SendEventState : public FiniteTimeActionState
{
public:
    SendEventState(SendEvent* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
        eventType_ = action->GetEventType();
        eventArgs_.insert(action->GetEventData().begin(), action->GetEventData().end());

    }

    void Update(float time) override
    {
        GetTarget()->SendEvent(eventType_, eventArgs_);
    }

    StringHash eventType_;
    VariantMap eventArgs_;
};
} // namespace

/// Construct.
SendEvent::SendEvent(Context* context)
    : BaseClassName(context)
{
}

/// Serialize content from/to archive. May throw ArchiveException.
void SendEvent::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "event", eventType_);
    SerializeOptionalValue(archive, "args", eventData_, EmptyObject{});
}

/// Create new action state from the action.
SharedPtr<ActionState> SendEvent::StartAction(Object* target) { return MakeShared<SendEventState>(this, target); }

} // namespace Actions
} // namespace Urho3D
