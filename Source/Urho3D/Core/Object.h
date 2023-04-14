//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Container/Allocator.h"
#include "../Core/Mutex.h"
#include "../Core/ObjectCategory.h"
#include "../Core/Profiler.h"
#include "../Core/StringHashRegister.h"
#include "../Core/SubsystemCache.h"
#include "../Core/TypeInfo.h"
#include "../Core/TypeTrait.h"
#include "../Core/Variant.h"

#include <EASTL/functional.h>
#include <EASTL/intrusive_list.h>

namespace Urho3D
{

class Archive;
class ArchiveBlock;
class Context;
class EventHandler;

#define URHO3D_OBJECT(typeName, baseTypeName) \
    public: \
        using ClassName = typeName; \
        using BaseClassName = baseTypeName; \
        virtual Urho3D::StringHash GetType() const override { return GetTypeInfoStatic()->GetType(); } \
        virtual const ea::string& GetTypeName() const override { return GetTypeInfoStatic()->GetTypeName(); } \
        virtual const Urho3D::TypeInfo* GetTypeInfo() const override { return GetTypeInfoStatic(); } \
        static Urho3D::StringHash GetTypeStatic() { return GetTypeInfoStatic()->GetType(); } \
        static const ea::string& GetTypeNameStatic() { return GetTypeInfoStatic()->GetTypeName(); } \
        static const Urho3D::TypeInfo* GetTypeInfoStatic() { static const Urho3D::TypeInfo typeInfoStatic(#typeName, BaseClassName::GetTypeInfoStatic()); return &typeInfoStatic; }

/// Base class for objects with type identification, subsystem access and event sending/receiving capability.
/// @templateversion
class URHO3D_API Object : public RefCounted
{
    friend class Context;

public:
    /// Construct.
    explicit Object(Context* context);
    /// Destruct. Clean up self from event sender & receiver structures.
    ~Object() override;

    /// Return type hash.
    /// @property
    virtual StringHash GetType() const = 0;
    /// Return type name.
    /// @property
    virtual const ea::string& GetTypeName() const = 0;
    /// Return type info.
    virtual const TypeInfo* GetTypeInfo() const = 0;
    /// Handle event.
    virtual void OnEvent(Object* sender, StringHash eventType, VariantMap& eventData);

    /// Serialize content from/to archive. May throw ArchiveException.
    virtual void SerializeInBlock(Archive& archive);

    /// Return type info static.
    static const TypeInfo* GetTypeInfoStatic() { return nullptr; }
    /// Check current instance is type of specified type.
    bool IsInstanceOf(StringHash type) const;
    /// Check current instance is type of specified type.
    bool IsInstanceOf(const TypeInfo* typeInfo) const;
    /// Check current instance is type of specified class.
    template<typename T> bool IsInstanceOf() const { return IsInstanceOf(T::GetTypeInfoStatic()); }
    /// Cast the object to specified most derived class.
    template<typename T> T* Cast() { return IsInstanceOf<T>() ? static_cast<T*>(this) : nullptr; }
    /// Cast the object to specified most derived class.
    template<typename T> const T* Cast() const { return IsInstanceOf<T>() ? static_cast<const T*>(this) : nullptr; }

    /// Subscribe to an event that can be sent by any sender.
    void SubscribeToEventManual(StringHash eventType, EventHandler* handler);
    /// Subscribe to a specific sender's event.
    void SubscribeToEventManual(Object* sender, StringHash eventType, EventHandler* handler);
    /// Subscribe to an event that can be sent by any sender.
    template <class T> void SubscribeToEvent(StringHash eventType, T handler);
    /// Subscribe to a specific sender's event.
    template <class T> void SubscribeToEvent(Object* sender, StringHash eventType, T handler);
    /// Unsubscribe from an event.
    void UnsubscribeFromEvent(StringHash eventType);
    /// Unsubscribe from a specific sender's event.
    void UnsubscribeFromEvent(Object* sender, StringHash eventType);
    /// Unsubscribe from a specific sender's events.
    void UnsubscribeFromEvents(Object* sender);
    /// Unsubscribe from all events.
    void UnsubscribeFromAllEvents();
    /// Unsubscribe from all events except those listed.
    void UnsubscribeFromAllEventsExcept(const ea::vector<StringHash>& exceptions);
    /// Unsubscribe from all events except those with listed senders.
    void UnsubscribeFromAllEventsExcept(const ea::vector<Object*>& exceptions);
    /// Send event to all subscribers.
    void SendEvent(StringHash eventType);
    /// Send event with parameters to all subscribers.
    void SendEvent(StringHash eventType, VariantMap& eventData);
    /// Return a preallocated map for event data. Used for optimization to avoid constant re-allocation of event data maps.
    VariantMap& GetEventDataMap() const;
    /// Send event with variadic parameter pairs to all subscribers. The parameters are (paramID, paramValue) pairs.
    template <typename... Args> void SendEvent(StringHash eventType, const Args&... args)
    {
        VariantMap& eventData = GetEventDataMap();
        ((void)eventData.emplace(ea::get<0>(args), Variant(ea::get<1>(args))), ...);
        SendEvent(eventType, eventData);
    }

    /// Return execution context.
    Context* GetContext() const { return context_; }
    /// Return global variable based on key.
    /// @property
    const Variant& GetGlobalVar(StringHash key) const;
    /// Return all global variables.
    /// @property
    const VariantMap& GetGlobalVars() const;
    /// Set global variable with the respective key and value.
    /// @property
    void SetGlobalVar(StringHash key, const Variant& value);
    /// Return subsystem by type.
    Object* GetSubsystem(StringHash type) const;
    /// Return active event sender. Null outside event handling.
    Object* GetEventSender() const;
    /// Return active event handler. Null outside event handling.
    EventHandler* GetEventHandler() const;
    /// Return whether has subscribed to an event without specific sender.
    bool HasSubscribedToEvent(StringHash eventType) const;
    /// Return whether has subscribed to a specific sender's event.
    bool HasSubscribedToEvent(Object* sender, StringHash eventType) const;

    /// Return whether has subscribed to any event.
    bool HasEventHandlers() const { return !eventHandlers_.empty(); }

    /// Template version of returning a subsystem.
    template <class T> T* GetSubsystem() const;
    /// Return object category. Categories are (optionally) registered along with the object factory. Return an empty string if the object category is not registered.
    /// @property
    const ea::string& GetCategory() const;

    /// Send event with parameters to all subscribers.
    void SendEvent(StringHash eventType, const VariantMap& eventData);
    /// Block object from sending and receiving events.
    void SetBlockEvents(bool block) { blockEvents_ = block; }
    /// Return sending and receiving events blocking status.
    bool GetBlockEvents() const { return blockEvents_; }

protected:
    /// Execution context.
    WeakPtr<Context> context_;

private:
    /// Return all subsystems from Context.
    const SubsystemCache& GetSubsystems() const;
    /// Find the first event handler with no specific sender.
    ea::intrusive_list<EventHandler>::iterator FindEventHandler(StringHash eventType);
    /// Find the first event handler with no specific sender.
    ea::intrusive_list<EventHandler>::iterator FindEventHandler(StringHash eventType) const { return const_cast<Object*>(this)->FindEventHandler(eventType); }
    /// Find the first event handler with specific sender.
    ea::intrusive_list<EventHandler>::iterator FindSpecificEventHandler(Object* sender);
    /// Find the first event handler with specific sender.
    ea::intrusive_list<EventHandler>::iterator FindSpecificEventHandler(Object* sender) const { return const_cast<Object*>(this)->FindSpecificEventHandler(sender); }
    /// Find the first event handler with specific sender and event type.
    ea::intrusive_list<EventHandler>::iterator FindSpecificEventHandler(Object* sender, StringHash eventType);
    /// Find the first event handler with specific sender and event type.
    ea::intrusive_list<EventHandler>::iterator FindSpecificEventHandler(Object* sender, StringHash eventType) const { return const_cast<Object*>(this)->FindSpecificEventHandler(sender, eventType); }
    /// Erase event handler from the list.
    ea::intrusive_list<EventHandler>::iterator EraseEventHandler(ea::intrusive_list<EventHandler>::iterator handlerIter);
    /// Remove event handlers related to a specific sender.
    void RemoveEventSender(Object* sender);

    /// Event handlers. Sender is null for non-specific handlers.
    ea::intrusive_list<EventHandler> eventHandlers_;

    /// Block object from sending and receiving any events.
    bool blockEvents_;
};

template <class T> T* Object::GetSubsystem() const { return GetSubsystems().Get<T>(); }

/// Internal helper class for invoking event handler functions.
class URHO3D_API EventHandler : public ea::intrusive_list_node
{
public:
    using HandlerFunction = ea::function<void(Object* receiver, StringHash eventType, VariantMap& eventData)>;

    /// Construct with specified receiver and handler.
    EventHandler(Object* receiver, HandlerFunction handler)
        : receiver_(receiver)
        , sender_(nullptr)
        , handler_(ea::move(handler))
    {
    }

    /// Construct with specified receiver and handler of flexible signature.
    template <class T>
    EventHandler(Object* receiver, T handler)
        : receiver_(receiver)
        , sender_(nullptr)
        , handler_(WrapHandler(ea::move(handler)))
    {
    }

    /// Set sender and event type.
    void SetSenderAndEventType(Object* sender, StringHash eventType)
    {
        sender_ = sender;
        eventType_ = eventType;
    }

    /// Invoke event handler function.
    void Invoke(VariantMap& eventData) const
    {
        handler_(receiver_, eventType_, eventData);
    }

    /// Return event receiver.
    Object* GetReceiver() const { return receiver_; }

    /// Return event sender. Null if the handler is non-specific.
    Object* GetSender() const { return sender_; }

    /// Return event type.
    const StringHash& GetEventType() const { return eventType_; }

private:
    template <class T> static HandlerFunction WrapHandler(T&& handler)
    {
        if constexpr (ea::is_member_function_pointer_v<T>)
            return WrapMemberHandler(ea::move(handler));
        else
            return WrapGenericHandler(ea::move(handler));
    }

    template <class T> static HandlerFunction WrapGenericHandler(T&& handler)
    {
        static constexpr bool hasObjectTypeData = ea::is_invocable_r_v<void, T, Object*, StringHash, VariantMap&>;
        static constexpr bool hasTypeData = ea::is_invocable_r_v<void, T, StringHash, VariantMap&>;
        static constexpr bool hasType = ea::is_invocable_r_v<void, T, StringHash>;
        static constexpr bool hasData = ea::is_invocable_r_v<void, T, VariantMap&>;
        static constexpr bool hasNone = ea::is_invocable_r_v<void, T>;
        static_assert(hasObjectTypeData || hasTypeData || hasType || hasData || hasNone, "Invalid handler signature");

        // clang-format off
        if constexpr (hasObjectTypeData)
            return [handler = ea::move(handler)](Object* receiver, StringHash eventType, VariantMap& eventData) mutable { handler(receiver, eventType, eventData); };
        else if constexpr (hasTypeData)
            return [handler = ea::move(handler)](Object*, StringHash eventType, VariantMap& eventData) mutable { handler(eventType, eventData); };
        else if constexpr (hasType)
            return [handler = ea::move(handler)](Object*, StringHash eventType, VariantMap&) mutable { handler(eventType); };
        else if constexpr (hasData)
            return [handler = ea::move(handler)](Object*, StringHash, VariantMap& eventData) mutable { handler(eventData); };
        else
            return [handler = ea::move(handler)](Object*, StringHash, VariantMap&) mutable { handler(); };
        // clang-format on
    }

    template <class T> static HandlerFunction WrapMemberHandler(T&& handler)
    {
        using ObjectType = MemberFunctionObject<T>;

        static constexpr bool hasTypeData = ea::is_invocable_r_v<void, T, ObjectType*, StringHash, VariantMap&>;
        static constexpr bool hasType = ea::is_invocable_r_v<void, T, ObjectType*, StringHash>;
        static constexpr bool hasData = ea::is_invocable_r_v<void, T, ObjectType*, VariantMap&>;
        static constexpr bool hasNone = ea::is_invocable_r_v<void, T, ObjectType*>;
        static_assert(hasTypeData || hasType || hasData || hasNone, "Invalid handler signature");

        // clang-format off
        if constexpr (hasTypeData)
            return [handler = ea::move(handler)](Object* receiver, StringHash eventType, VariantMap& eventData) mutable { (static_cast<ObjectType*>(receiver)->*handler)(eventType, eventData); };
        else if constexpr (hasType)
            return [handler = ea::move(handler)](Object* receiver, StringHash eventType, VariantMap&) mutable { (static_cast<ObjectType*>(receiver)->*handler)(eventType); };
        else if constexpr (hasData)
            return [handler = ea::move(handler)](Object* receiver, StringHash, VariantMap& eventData) mutable { (static_cast<ObjectType*>(receiver)->*handler)(eventData); };
        else
            return [handler = ea::move(handler)](Object* receiver, StringHash, VariantMap&) mutable { (static_cast<ObjectType*>(receiver)->*handler)(); };
        // clang-format on
    }

    Object* receiver_;
    Object* sender_;
    StringHash eventType_;
    HandlerFunction handler_;
};

template<typename T>
inline void Object::SubscribeToEvent(StringHash eventType, T handler)
{
    SubscribeToEventManual(eventType, new Urho3D::EventHandler(this, ea::move(handler)));
}

template<typename T>
inline void Object::SubscribeToEvent(Object* sender, StringHash eventType, T handler)
{
    SubscribeToEventManual(sender, eventType, new Urho3D::EventHandler(this, ea::move(handler)));
}

/// Get register of event names.
URHO3D_API StringHashRegister& GetEventNameRegister();
URHO3D_API StringHashRegister& GetEventParamRegister();

/// Describe an event's hash ID and begin a namespace in which to define its parameters.
#define URHO3D_EVENT(eventID, eventName) static const Urho3D::StringHash eventID(Urho3D::GetEventNameRegister().RegisterString(#eventName)); namespace eventName
/// Describe an event's parameter hash ID. Should be used inside an event namespace.
#define URHO3D_PARAM(paramID, paramName) static const Urho3D::StringHash paramID(Urho3D::GetEventParamRegister().RegisterString(#paramName))
/// Deprecated. Just use &className::function instead.
#define URHO3D_HANDLER(className, function) (&className::function)

}
