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

#include <EASTL/unique_ptr.h>

#include "../Container/Ptr.h"
#include "../Core/Attribute.h"
#include "../Core/Object.h"
#include "../Core/ObjectReflection.h"
#include "../Core/SubsystemCache.h"

namespace Urho3D
{

/// Tracking structure for event receivers.
class URHO3D_API EventReceiverGroup : public RefCounted
{
public:
    /// Construct.
    EventReceiverGroup() :
        inSend_(0),
        dirty_(false)
    {
    }

    /// Begin event send. When receivers are removed during send, group has to be cleaned up afterward.
    void BeginSendEvent();

    /// End event send. Clean up if necessary.
    void EndSendEvent();

    /// Add receiver. Same receiver must not be double-added!
    void Add(Object* object);

    /// Remove receiver. Leave holes during send, which requires later cleanup.
    void Remove(Object* object);

    /// Receivers. May contain holes during sending.
    ea::vector<Object*> receivers_;

private:
    /// "In send" recursion counter.
    unsigned inSend_;
    /// Cleanup required flag.
    bool dirty_;
};

/// Urho3D execution context. Provides access to subsystems, object factories and attributes, and event receivers.
class URHO3D_API Context : public RefCounted, public ObjectReflectionRegistry
{
    friend class Object;

public:
    /// Construct.
    Context();
    /// Destruct.
    ~Context() override;

    /// Return global instance of context object. Only one context may exist within application.
    static Context* GetInstance();

    /// Register a subsystem by explicitly using a type. Type must belong to inheritance hierarchy.
    void RegisterSubsystem(Object* object, StringHash type);
    /// Register a subsystem.
    void RegisterSubsystem(Object* object);
    /// Remove a subsystem.
    void RemoveSubsystem(StringHash objectType);
    /// Return a preallocated map for event data. Used for optimization to avoid constant re-allocation of event data maps.
    VariantMap& GetEventDataMap();
    /// Initialises the specified SDL systems, if not already. Returns true if successful. This call must be matched with ReleaseSDL() when SDL functions are no longer required, even if this call fails.
    bool RequireSDL(unsigned int sdlFlags);
    /// Indicate that you are done with using SDL. Must be called after using RequireSDL().
    void ReleaseSDL();

    /// Deprecated. Use AddFactoryReflection, AddAbstractReflection or AddReflection instead.
    template <class T> void RegisterFactory(ea::string_view category = {}) { AddFactoryReflection<T>(category); }
    /// Template version of registering subsystem.
    template <class T, class U = T> T* RegisterSubsystem();
    /// Template version of removing a subsystem.
    template <class T> void RemoveSubsystem();

    /// Return subsystem by type.
    Object* GetSubsystem(StringHash type) const;

    /// Return global variable based on key.
    const Variant& GetGlobalVar(StringHash key) const;

    /// Return all global variables.
    const VariantMap& GetGlobalVars() const { return globalVars_; }

    /// Set global variable with the respective key and value.
    void SetGlobalVar(StringHash key, const Variant& value);

    /// Return all subsystems.
    const SubsystemCache& GetSubsystems() const { return subsystems_; }

    /// Return active event sender. Null outside event handling.
    Object* GetEventSender() const;

    /// Return active event handler. Set by Object. Null outside event handling.
    EventHandler* GetEventHandler() const { return eventHandler_; }

    /// Return object type name from hash, or empty if unknown.
    const ea::string& GetTypeName(StringHash objectType) const;
    /// Return a specific attribute description for an object, or null if not found.
    AttributeInfo* GetAttribute(StringHash objectType, const char* name);
    /// Template version of returning a subsystem.
    template <class T> T* GetSubsystem() const;
    /// Template version of returning a specific attribute description.
    template <class T> AttributeInfo* GetAttribute(const char* name);

    /// Return attribute descriptions for an object type, or null if none defined.
    const ea::vector<AttributeInfo>* GetAttributes(StringHash type) const
    {
        auto reflection = GetReflection(type);
        return reflection ? &reflection->GetAttributes() : nullptr;
    }

    /// Return event receivers for a sender and event type, or null if they do not exist.
    EventReceiverGroup* GetEventReceivers(Object* sender, StringHash eventType)
    {
        auto i = specificEventReceivers_.find(
            sender);
        if (i != specificEventReceivers_.end())
        {
            auto j = i->second.find(eventType);
            return j != i->second.end() ? j->second : nullptr;
        }
        else
            return nullptr;
    }

    /// Return event receivers for an event type, or null if they do not exist.
    EventReceiverGroup* GetEventReceivers(StringHash eventType)
    {
        auto i = eventReceivers_.find(
            eventType);
        return i != eventReceivers_.end() ? i->second : nullptr;
    }

private:
    /// Add event receiver.
    void AddEventReceiver(Object* receiver, StringHash eventType);
    /// Add event receiver for specific event.
    void AddEventReceiver(Object* receiver, Object* sender, StringHash eventType);
    /// Remove an event sender from all receivers. Called on its destruction.
    void RemoveEventSender(Object* sender);
    /// Remove event receiver from specific events.
    void RemoveEventReceiver(Object* receiver, Object* sender, StringHash eventType);
    /// Remove event receiver from non-specific events.
    void RemoveEventReceiver(Object* receiver, StringHash eventType);
    /// Begin event send.
    void BeginSendEvent(Object* sender, StringHash eventType);
    /// End event send. Clean up event receivers removed in the meanwhile.
    void EndSendEvent();

    /// Set current event handler. Called by Object.
    void SetEventHandler(EventHandler* handler) { eventHandler_ = handler; }

    /// Subsystems.
    SubsystemCache subsystems_;
    /// Event receivers for non-specific events.
    ea::unordered_map<StringHash, SharedPtr<EventReceiverGroup> > eventReceivers_;
    /// Event receivers for specific senders' events.
    ea::unordered_map<Object*, ea::unordered_map<StringHash, SharedPtr<EventReceiverGroup> > > specificEventReceivers_;
    /// Event sender stack.
    ea::vector<Object*> eventSenders_;
    /// Event data stack.
    ea::vector<VariantMap*> eventDataMaps_;
    /// Active event handler. Not stored in a stack for performance reasons; is needed only in esoteric cases.
    EventHandler* eventHandler_;
    /// Variant map for global variables that can persist throughout application execution.
    VariantMap globalVars_;
};

template <class T, class U> T* Context::RegisterSubsystem()
{
    auto* subsystem = new T(this);
    RegisterSubsystem(subsystem, U::GetTypeStatic());
    return subsystem;
}

template <class T> void Context::RemoveSubsystem() { RemoveSubsystem(T::GetTypeStatic()); }

template <class T> T* Context::GetSubsystem() const { return subsystems_.Get<T>(); }

template <class T> AttributeInfo* Context::GetAttribute(const char* name)
{
    ObjectReflection* reflection = GetReflection<T>();
    return reflection ? reflection->GetAttribute(name) : nullptr;
}

}
