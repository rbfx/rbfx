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

#pragma once

#include "ActionInstant.h"
#include "FiniteTimeAction.h"

namespace Urho3D
{
namespace Actions
{
/// Internal helper class for invoking action handler functions.
class URHO3D_API ActionCallHandler : public RefCounted
{
public:
    /// Construct with specified receiver and userdata.
    explicit ActionCallHandler(Object* receiver, void* userData = nullptr)
        : receiver_(receiver)
        , userData_(userData)
    {
    }

    /// Destruct.
    virtual ~ActionCallHandler() = default;

    /// Invoke event handler function.
    virtual void Invoke(Object* target) = 0;

    /// Return event receiver.
    Object* GetReceiver() const { return receiver_; }

    /// Return userdata.
    void* GetUserData() const { return userData_; }

protected:
    /// Event receiver.
    Object* receiver_;
    /// Userdata.
    void* userData_;
};

/// Template implementation of the action handler invoke helper (stores a function pointer of specific class).
template <class T> class ActionCallHandlerImpl : public ActionCallHandler
{
public:
    using HandlerFunctionPtr = void (T::*)(Object*);

        /// Construct with receiver and function pointers and userdata.
    ActionCallHandlerImpl(T* receiver, HandlerFunctionPtr function, void* userData = nullptr)
        : ActionCallHandler(receiver, userData)
        , function_(function)
    {
        assert(receiver_);
        assert(function_);
    }

    /// Invoke event handler function.
    void Invoke(Object* target) override
    {
        auto* receiver = static_cast<T*>(receiver_);
        (receiver->*function_)(target);
    }

private:
    /// Class-specific pointer to handler function.
    HandlerFunctionPtr function_;
};

/// Call function on each action tick.
/// This is not serializable action. If you want something you can save in a file please use SendEvent
class URHO3D_API CallFunc : public ActionInstant
{
    URHO3D_OBJECT(CallFunc, ActionInstant)
public:
    /// Construct.
    explicit CallFunc(Context* context);

    /// Get call handler
    ActionCallHandler* GetCallHandler() const { return actionCallHandler_; }

    /// Set call handler
    void SetCallHandler(ActionCallHandler* handler);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    SharedPtr<ActionCallHandler> actionCallHandler_;
};

/// Send event on target node.
class URHO3D_API SendEvent : public ActionInstant
{
    URHO3D_OBJECT(SendEvent, ActionInstant)
public:
    /// Construct.
    explicit SendEvent(Context* context);

    /// Get event name.
    const ea::string& GetEventType() const { return eventType_; }
    /// Set event name.
    void SetEventType(ea::string_view eventType) { eventType_ = eventType; }

    /// Get event args.
    const StringVariantMap& GetEventData() const { return eventData_; }
    /// Set event args.
    void SetEventData(const StringVariantMap& eventArgs) { eventData_ = eventArgs; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Name of the event to send.
    ea::string eventType_;
    /// Event arguments.
    StringVariantMap eventData_;
};

} // namespace Actions
} // namespace Urho3D
