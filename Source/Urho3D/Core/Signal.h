//
// Copyright (c) 2017-2020 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Container/Ptr.h"
#include "../Container/RefCounted.h"

#include <EASTL/type_traits.h>
#include <EASTL/fixed_function.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>
#include <EASTL/vector_multiset.h>

namespace Urho3D
{

namespace Detail
{

/// Signal subscription data without priority.
template <class Handler>
struct SignalSubscription
{
    /// Signal receiver. Handler is not invoked if receiver is expired.
    WeakPtr<RefCounted> receiver_;
    /// Signal handler callable.
    Handler handler_{};

    /// Construct default.
    SignalSubscription() = default;

    /// Construct valid.
    SignalSubscription(WeakPtr<RefCounted> receiver, Handler handler)
        : receiver_(ea::move(receiver))
        , handler_(ea::move(handler))
    {
    }
};

/// Signal subscription data with priority.
template <class Handler, class Priority>
struct PrioritySignalSubscription : public SignalSubscription<Handler>
{
    /// Signal priority.
    Priority priority_{};

    /// Construct default.
    PrioritySignalSubscription() = default;

    /// Construct valid.
    PrioritySignalSubscription(WeakPtr<RefCounted> receiver, Priority priority, Handler handler)
        : SignalSubscription<Handler>(ea::move(receiver), ea::move(handler))
        , priority_(priority)
    {
    }

    /// Compare subscriptions. Higher priority goes before.
    bool operator<(const PrioritySignalSubscription<Handler, Priority>& rhs) const
    {
        return !(priority_ < rhs.priority_);
    }
};

/// Base signal type.
template <class Priority, class Sender, class... Args>
class SignalBase
{
public:
    /// Small object optimization buffer size.
    static const unsigned HandlerSize = 4 * sizeof(void*);

    /// Signal handler type.
    using Handler = ea::fixed_function<HandlerSize, bool(RefCounted*, Sender*, Args...)>;

    /// Whether the signal is prioritized.
    static constexpr bool HasPriority = !ea::is_same_v<Priority, void>;

    /// Subscription type.
    using Subscription = ea::conditional_t<HasPriority,
        PrioritySignalSubscription<Handler, Priority>,
        SignalSubscription<Handler>>;

    /// Subscription collection type.
    using SubscriptionVector = ea::conditional_t<HasPriority,
        ea::vector_multiset<Subscription>,
        ea::vector<Subscription>>;

    /// Unsubscribe all handlers of specified receiver from this signal.
    void Unsubscribe(RefCounted* receiver)
    {
        for (auto it = subscriptions_.begin(); it != subscriptions_.end();)
        {
            Subscription& subscription = *it;
            if (subscription.receiver_.Expired() || subscription.receiver_ == receiver)
                it = subscriptions_.erase(it);
            else
                ++it;
        }
    }

    /// Invoke signal.
    template <typename... InvokeArgs>
    void operator()(Sender* sender, InvokeArgs&&... args)
    {
        for (auto it = subscriptions_.begin(); it != subscriptions_.end();)
        {
            Subscription& subscription = *it;
            RefCounted* receiver = subscription.receiver_.Get();
            if (receiver && subscription.handler_(receiver, sender, args...))
                ++it;
            else
                it = subscriptions_.erase(it);
        }
    }

    /// Returns true when event has at least one subscription.
    bool HasSubscriptions() const { return !subscriptions_.empty(); }

protected:
    /// Wrap callback into Handler.
    template <class Receiver, class Callback>
    Handler WrapHandler(Callback handler)
    {
        return [handler](RefCounted* receiverPtr, Sender* sender, Args... args)
        {
            static constexpr bool boolSenderArgs = ea::is_invocable_r_v<bool, Callback, Receiver*, Sender*, Args...>;
            static constexpr bool voidSenderArgs = ea::is_invocable_r_v<void, Callback, Receiver*, Sender*, Args...>;
            static constexpr bool boolArgs = ea::is_invocable_r_v<bool, Callback, Receiver*, Args...>;
            static constexpr bool voidArgs = ea::is_invocable_r_v<void, Callback, Receiver*, Args...>;
            static_assert(boolSenderArgs || voidSenderArgs || boolArgs || voidArgs,
                "Callback should return either bool or void. "
                "Callback should accept either (Args...) or (Sender*, Args...) as parameters.");

            auto receiver = static_cast<Receiver*>(receiverPtr);
            // MinGW build fails at ea::invoke ATM, call as member function
#ifdef __MINGW32__
            if constexpr (boolSenderArgs)
                return (receiver->*handler)(sender, args...);
            else if constexpr (voidSenderArgs)
                return (receiver->*handler)(sender, args...), true;
            else if constexpr (boolArgs)
                return (receiver->*handler)(args...);
            else if constexpr (voidArgs)
                return (receiver->*handler)(args...), true;
#else
            if constexpr (boolSenderArgs)
                return ea::invoke(handler, receiver, sender, args...);
            else if constexpr (voidSenderArgs)
                return ea::invoke(handler, receiver, sender, args...), true;
            else if constexpr (boolArgs)
                return ea::invoke(handler, receiver, args...);
            else if constexpr (voidArgs)
                return ea::invoke(handler, receiver, args...), true;
#endif
        };
    }

    /// Vector of subscriptions.
    SubscriptionVector subscriptions_;
};

}

/// Signal with specified or default sender type.
template <class Signature, class Sender = RefCounted>
class Signal;

template <class Sender, class... Args>
class Signal<void(Args...), Sender> : public Detail::SignalBase<void, Sender, Args...>
{
public:
    /// Subscribe to event.
    template <class Receiver, class Callback>
    void Subscribe(Receiver* receiver, Callback handler)
    {
        WeakPtr<RefCounted> weakReceiver(static_cast<RefCounted*>(receiver));
        auto wrappedHandler = this->template WrapHandler<Receiver>(handler);
        this->subscriptions_.emplace_back(ea::move(weakReceiver), ea::move(wrappedHandler));
    }
};

/// Signal with subscription priority and specified or default sender type.
template <class Signature, class Priority = int, class Sender = RefCounted>
class PrioritySignal;

template <class Sender, class Priority, class... Args>
class PrioritySignal<void(Args...), Priority, Sender> : public Detail::SignalBase<Priority, Sender, Args...>
{
public:
    /// Subscribe to event.
    template <class Receiver, class Callback>
    void Subscribe(Receiver* receiver, Priority priority, Callback handler)
    {
        WeakPtr<RefCounted> weakReceiver(static_cast<RefCounted*>(receiver));
        auto wrappedHandler = this->template WrapHandler<Receiver>(handler);
        this->subscriptions_.emplace(ea::move(weakReceiver), priority, ea::move(wrappedHandler));
    }
};

}
