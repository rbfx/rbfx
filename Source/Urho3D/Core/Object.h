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

#include <EASTL/intrusive_list.h>

#include "../Container/Allocator.h"
#include "../Core/Mutex.h"
#include "../Core/Profiler.h"
#include "../Core/StringHashRegister.h"
#include "../Core/SubsystemCache.h"
#include "../Core/Variant.h"
#include <functional>
#include <utility>

namespace Urho3D
{

class Archive;
class ArchiveBlock;
class Context;
class EventHandler;

/// Type info.
/// @nobind
class URHO3D_API TypeInfo
{
public:
    /// Construct.
    TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo);
    /// Destruct.
    ~TypeInfo();

    /// Check current type is type of specified type.
    bool IsTypeOf(StringHash type) const;
    /// Check current type is type of specified type.
    bool IsTypeOf(const TypeInfo* typeInfo) const;
    /// Check current type is type of specified class type.
    template<typename T> bool IsTypeOf() const { return IsTypeOf(T::GetTypeInfoStatic()); }

    /// Return type.
    StringHash GetType() const { return type_; }
    /// Return type name.
    const ea::string& GetTypeName() const { return typeName_;}
    /// Return base type info.
    const TypeInfo* GetBaseTypeInfo() const { return baseTypeInfo_; }

private:
    /// Type.
    StringHash type_;
    /// Type name.
    ea::string typeName_;
    /// Base class type info.
    const TypeInfo* baseTypeInfo_;
};

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
    void SubscribeToEvent(StringHash eventType, EventHandler* handler);
    /// Subscribe to a specific sender's event.
    void SubscribeToEvent(Object* sender, StringHash eventType, EventHandler* handler);
    /// Subscribe to an event that can be sent by any sender.
    void SubscribeToEvent(StringHash eventType, const std::function<void(StringHash, VariantMap&)>& function, void* userData = nullptr);
    /// Subscribe to a specific sender's event.
    void SubscribeToEvent(Object* sender, StringHash eventType, const std::function<void(StringHash, VariantMap&)>& function, void* userData = nullptr);
    /// Subscribe to an event that can be sent by any sender.
    template<typename T>
    void SubscribeToEvent(StringHash eventType, void(T::*handler)(StringHash, VariantMap&));
    /// Subscribe to a specific sender's event.
    template<typename T>
    void SubscribeToEvent(Object* sender, StringHash eventType, void(T::*handler)(StringHash, VariantMap&));
    /// Unsubscribe from an event.
    void UnsubscribeFromEvent(StringHash eventType);
    /// Unsubscribe from a specific sender's event.
    void UnsubscribeFromEvent(Object* sender, StringHash eventType);
    /// Unsubscribe from a specific sender's events.
    void UnsubscribeFromEvents(Object* sender);
    /// Unsubscribe from all events.
    void UnsubscribeFromAllEvents();
    /// Unsubscribe from all events except those listed, and optionally only those with userdata (script registered events).
    void UnsubscribeFromAllEventsExcept(const ea::vector<StringHash>& exceptions, bool onlyUserData);
    /// Unsubscribe from all events except those with listed senders, and optionally only those with userdata (script registered events.)
    void UnsubscribeFromAllEventsExcept(const ea::vector<Object*>& exceptions, bool onlyUserData);
    /// Send event to all subscribers.
    void SendEvent(StringHash eventType);
    /// Send event with parameters to all subscribers.
    void SendEvent(StringHash eventType, VariantMap& eventData);
    /// Return a preallocated map for event data. Used for optimization to avoid constant re-allocation of event data maps.
    VariantMap& GetEventDataMap() const;
    /// Send event with variadic parameter pairs to all subscribers. The parameter pairs is a list of paramID and paramValue separated by comma, one pair after another.
    template <typename... Args> void SendEvent(StringHash eventType, Args... args)
    {
        SendEvent(eventType, GetEventDataMap().populate(args...));
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
    /// Construct with specified receiver and userdata.
    explicit EventHandler(Object* receiver, void* userData = nullptr) :
        receiver_(receiver),
        sender_(nullptr),
        userData_(userData)
    {
    }

    /// Destruct.
    virtual ~EventHandler() = default;

    /// Set sender and event type.
    void SetSenderAndEventType(Object* sender, StringHash eventType)
    {
        sender_ = sender;
        eventType_ = eventType;
    }

    /// Invoke event handler function.
    virtual void Invoke(VariantMap& eventData) = 0;
    /// Return a unique copy of the event handler.
    virtual EventHandler* Clone() const = 0;

    /// Return event receiver.
    Object* GetReceiver() const { return receiver_; }

    /// Return event sender. Null if the handler is non-specific.
    Object* GetSender() const { return sender_; }

    /// Return event type.
    const StringHash& GetEventType() const { return eventType_; }

    /// Return userdata.
    void* GetUserData() const { return userData_; }

protected:
    /// Event receiver.
    Object* receiver_;
    /// Event sender.
    Object* sender_;
    /// Event type.
    StringHash eventType_;
    /// Userdata.
    void* userData_;
};

/// Template implementation of the event handler invoke helper (stores a function pointer of specific class).
template <class T> class EventHandlerImpl : public EventHandler
{
public:
    using HandlerFunctionPtr = void (T::*)(StringHash, VariantMap&);

    /// Construct with receiver and function pointers and userdata.
    EventHandlerImpl(T* receiver, HandlerFunctionPtr function, void* userData = nullptr) :
        EventHandler(receiver, userData),
        function_(function)
    {
        assert(receiver_);
        assert(function_);
    }

    /// Invoke event handler function.
    void Invoke(VariantMap& eventData) override
    {
        auto* receiver = static_cast<T*>(receiver_);
        (receiver->*function_)(eventType_, eventData);
    }

    /// Return a unique copy of the event handler.
    EventHandler* Clone() const override
    {
        return new EventHandlerImpl(static_cast<T*>(receiver_), function_, userData_);
    }

private:
    /// Class-specific pointer to handler function.
    HandlerFunctionPtr function_;
};

/// Template implementation of the event handler invoke helper (std::function instance).
/// @nobind
class EventHandler11Impl : public EventHandler
{
public:
    /// Construct with receiver and function pointers and userdata.
    explicit EventHandler11Impl(std::function<void(StringHash, VariantMap&)> function, void* userData = nullptr) :
        EventHandler(nullptr, userData),
        function_(std::move(function))
    {
        assert(function_);
    }

    /// Invoke event handler function.
    void Invoke(VariantMap& eventData) override
    {
        function_(eventType_, eventData);
    }

    /// Return a unique copy of the event handler.
    EventHandler* Clone() const override
    {
        return new EventHandler11Impl(function_, userData_);
    }

private:
    /// Class-specific pointer to handler function.
    std::function<void(StringHash, VariantMap&)> function_;
};

template<typename T>
inline void Object::SubscribeToEvent(StringHash eventType, void(T::*handler)(StringHash, VariantMap&))
{
    SubscribeToEvent(eventType, new Urho3D::EventHandlerImpl<T>((T*)this, handler));
}

template<typename T>
inline void Object::SubscribeToEvent(Object* sender, StringHash eventType, void(T::*handler)(StringHash, VariantMap&))
{
    SubscribeToEvent(sender, eventType, new Urho3D::EventHandlerImpl<T>((T*)this, handler));
}

/// Get register of event names.
URHO3D_API StringHashRegister& GetEventNameRegister();

/// Describe an event's hash ID and begin a namespace in which to define its parameters.
#define URHO3D_EVENT(eventID, eventName) static const Urho3D::StringHash eventID(Urho3D::GetEventNameRegister().RegisterString(#eventName)); namespace eventName
/// Describe an event's parameter hash ID. Should be used inside an event namespace.
#define URHO3D_PARAM(paramID, paramName) static const Urho3D::StringHash paramID = #paramName
/// Convenience macro to construct an EventHandler that points to a receiver object and its member function.
#define URHO3D_HANDLER(className, function) (new Urho3D::EventHandlerImpl<className>(this, &className::function))
/// Convenience macro to construct an EventHandler that points to a receiver object and its member function, and also defines a userdata pointer.
#define URHO3D_HANDLER_USERDATA(className, function, userData) (new Urho3D::EventHandlerImpl<className>(this, &className::function, userData))

}
