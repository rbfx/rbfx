//
// Copyright (c) 2017-2019 the rbfx project.
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
#include "../Core/Function.h"

#include <EASTL/utility.h>
#include <EASTL/vector.h>


namespace Urho3D
{

template<typename T, typename Sender=RefCounted>
class Signal
{
public:
    /// Signal handler type.
    using Handler = Function<bool(RefCounted*, Sender*, T&)>;

    /// Subscribe to event.
    template<typename Receiver>
    void Subscribe(Receiver* receiver, void(Receiver::*handler)(Sender*, T&))
    {
        handlers_.emplace_back(WeakPtr<RefCounted>(static_cast<RefCounted*>(receiver)),
            [handler](RefCounted* receiver, Sender* sender, T& args)
            {
                (static_cast<Receiver*>(receiver)->*handler)(sender, args);
                return true;
            }
        );
    }

    /// Subscribe to event.
    template<typename Receiver>
    void Subscribe(Receiver* receiver, bool(Receiver::*handler)(RefCounted*, T&))
    {
        handlers_.emplace_back(WeakPtr<RefCounted>(static_cast<RefCounted*>(receiver)),
            [handler](RefCounted* receiver, Sender* sender, T& args)
            {
                return (static_cast<Receiver*>(receiver)->*handler)(sender, args);
            }
        );
    }

    /// Subscribe to event.
    template<typename Receiver>
    void Subscribe(Receiver* receiver, void(Receiver::*handler)(T&))
    {
        handlers_.emplace_back(WeakPtr<RefCounted>(static_cast<RefCounted*>(receiver)),
            [handler](RefCounted* receiver, Sender* sender, T& args)
            {
                (static_cast<Receiver*>(receiver)->*handler)(args);
                return true;
            }
        );
    }

    /// Subscribe to event.
    template<typename Receiver>
    void Subscribe(Receiver* receiver, bool(Receiver::*handler)(T&))
    {
        handlers_.emplace_back(WeakPtr<RefCounted>(static_cast<RefCounted*>(receiver)),
            [handler](RefCounted* receiver, Sender* sender, T& args)
            {
                return (static_cast<Receiver*>(receiver)->*handler)(args);
            }
        );
    }

    /// Unsubscribe all handlers of specified receiver from this events.
    void Unsubscribe(RefCounted* receiver)
    {
        for (auto it = handlers_.begin(); it != handlers_.end();)
        {
            ea::pair<WeakPtr<RefCounted>, Handler>& pair = *it;
            if (pair.first.Expired() || pair.first == receiver)
                it = handlers_.erase(it);
            else
                ++it;
        }
    }

    /// Invoke event.
    void operator()(Sender* sender, T& args)
    {
        for (auto it = handlers_.begin(); it != handlers_.end();)
        {
            ea::pair<WeakPtr<RefCounted>, Handler>& pair = *it;
            if (RefCounted* receiver = pair.first.Get())
            {
                if (pair.second(receiver, sender, args))
                    ++it;
                else
                    it = handlers_.erase(it);
            }
            else
                it = handlers_.erase(it);
        }
    }

    /// Returns true when event has at least one subscriber.
    bool HasSubscribers() const { return !handlers_.empty(); }

protected:
    /// A collection of event handlers.
    ea::vector<ea::pair<WeakPtr<RefCounted>, Handler>> handlers_;
};

template<typename Sender>
class Signal<void, Sender>
{
public:
    /// Signal handler type.
    using Handler = Function<bool(RefCounted*, Sender*)>;

    /// Subscribe to event.
    template<typename Receiver>
    void Subscribe(Receiver* receiver, void(Receiver::*handler)(Sender*))
    {
        handlers_.emplace_back(WeakPtr<RefCounted>(static_cast<RefCounted*>(receiver)),
            [handler](RefCounted* receiver, Sender* sender)
            {
                (static_cast<Receiver*>(receiver)->*handler)(sender);
                return true;
            }
        );
    }

    /// Subscribe to event.
    template<typename Receiver>
    void Subscribe(Receiver* receiver, bool(Receiver::*handler)(RefCounted*))
    {
        handlers_.emplace_back(WeakPtr<RefCounted>(static_cast<RefCounted*>(receiver)),
            [handler](RefCounted* receiver, Sender* sender)
            {
                return (static_cast<Receiver*>(receiver)->*handler)(sender);
            }
        );
    }

    /// Subscribe to event.
    template<typename Receiver>
    void Subscribe(Receiver* receiver, void(Receiver::*handler)())
    {
        handlers_.emplace_back(WeakPtr<RefCounted>(static_cast<RefCounted*>(receiver)),
            [handler](RefCounted* receiver, Sender* sender)
            {
                (static_cast<Receiver*>(receiver)->*handler)();
                return true;
            }
        );
    }

    /// Subscribe to event.
    template<typename Receiver>
    void Subscribe(Receiver* receiver, bool(Receiver::*handler)())
    {
        handlers_.emplace_back(WeakPtr<RefCounted>(static_cast<RefCounted*>(receiver)),
            [handler](RefCounted* receiver, Sender* sender)
            {
                return (static_cast<Receiver*>(receiver)->*handler)();
            }
        );
    }

    /// Unsubscribe all handlers of specified receiver from this events.
    void Unsubscribe(RefCounted* receiver)
    {
        for (auto it = handlers_.begin(); it != handlers_.end();)
        {
            ea::pair<WeakPtr<RefCounted>, Handler>& pair = *it;
            if (pair.first.Expired() || pair.first == receiver)
                it = handlers_.erase(it);
            else
                ++it;
        }
    }

    /// Invoke event.
    void operator()(Sender* sender)
    {
        for (auto it = handlers_.begin(); it != handlers_.end();)
        {
            ea::pair<WeakPtr<RefCounted>, Handler>& pair = *it;
            if (RefCounted* receiver = pair.first.Get())
            {
                if (pair.second(receiver, sender))
                    ++it;
                else
                    it = handlers_.erase(it);
            }
            else
                it = handlers_.erase(it);
        }
    }

    /// Returns true when event has at least one subscriber.
    bool HasSubscribers() const { return !handlers_.empty(); }

protected:
    /// A collection of event handlers.
    ea::vector<ea::pair<WeakPtr<RefCounted>, Handler>> handlers_;
};

}
