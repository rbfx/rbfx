//
// Copyright (c) 2008-2020 the Urho3D project.
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
class URHO3D_API Context : public RefCounted
{
    friend class Object;

public:
    /// Construct.
    Context();
    /// Destruct.
    ~Context() override;

    /// Return global instance of context object. Only one context may exist within application.
    static Context* GetInstance();

    /// Create an object by type. Return pointer to it or null if no factory found.
    template <class T> inline SharedPtr<T> CreateObject()
    {
        return StaticCast<T>(CreateObject(T::GetTypeStatic()));
    }
    /// Create an object by type hash. Return pointer to it or null if no factory found.
    SharedPtr<Object> CreateObject(StringHash objectType);
    /// Register a factory for an object type.
    void RegisterFactory(ObjectFactory* factory);
    /// Register a factory for an object type and specify the object category.
    void RegisterFactory(ObjectFactory* factory, const char* category);
    /// Remove object factory.
    void RemoveFactory(StringHash type);
    /// remove object factory.
    void RemoveFactory(StringHash type, const char* category);
    /// Register a subsystem by explicitly using a type. Type must belong to inheritance hierarchy.
    void RegisterSubsystem(Object* object, StringHash type);
    /// Register a subsystem.
    void RegisterSubsystem(Object* object);
    /// Remove a subsystem.
    void RemoveSubsystem(StringHash objectType);
    /// Register object attribute.
    AttributeHandle RegisterAttribute(StringHash objectType, const AttributeInfo& attr);
    /// Remove object attribute.
    void RemoveAttribute(StringHash objectType, const char* name);
    /// Remove all object attributes.
    void RemoveAllAttributes(StringHash objectType);
    /// Update object attribute's default value.
    void UpdateAttributeDefaultValue(StringHash objectType, const char* name, const Variant& defaultValue);
    /// Return a preallocated map for event data. Used for optimization to avoid constant re-allocation of event data maps.
    VariantMap& GetEventDataMap();
    /// Initialises the specified SDL systems, if not already. Returns true if successful. This call must be matched with ReleaseSDL() when SDL functions are no longer required, even if this call fails.
    bool RequireSDL(unsigned int sdlFlags);
    /// Indicate that you are done with using SDL. Must be called after using RequireSDL().
    void ReleaseSDL();
#ifdef URHO3D_IK
    /// Initialises the IK library, if not already. This call must be matched with ReleaseIK() when the IK library is no longer required.
    void RequireIK();
    /// Indicate that you are done with using the IK library.
    void ReleaseIK();
#endif

    /// Copy base class attributes to derived class.
    void CopyBaseAttributes(StringHash baseType, StringHash derivedType);
    /// Template version of registering an object factory.
    template <class T = void, class... Rest> void RegisterFactory();
    /// Template version of registering an object factory with category.
    template <class T = void, class... Rest> void RegisterFactory(const char* category);
    /// Template version of unregistering an object factory.
    template <class T = void, class... Rest> void RemoveFactory();
    /// Template version of unregistering an object factory with category.
    template <class T = void, class... Rest> void RemoveFactory(const char* category);
    /// Template version of registering subsystem.
    template <class T> T* RegisterSubsystem();
    /// Template version of removing a subsystem.
    template <class T> void RemoveSubsystem();
    /// Template version of registering an object attribute.
    template <class T> AttributeHandle RegisterAttribute(const AttributeInfo& attr);
    /// Template version of removing an object attribute.
    template <class T> void RemoveAttribute(const char* name);
    /// Template version of removing all object attributes.
    template <class T> void RemoveAllAttributes();
    /// Template version of copying base class attributes to derived class.
    template <class T, class U> void CopyBaseAttributes();
    /// Template version of updating an object attribute's default value.
    template <class T> void UpdateAttributeDefaultValue(const char* name, const Variant& defaultValue);

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

    /// Return all object factories.
    const ea::unordered_map<StringHash, SharedPtr<ObjectFactory> >& GetObjectFactories() const { return factories_; }

    /// Return all object categories.
    const ea::unordered_map<ea::string, ea::vector<StringHash> >& GetObjectCategories() const { return objectCategories_; }

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
        auto i = attributes_.find(type);
        return i != attributes_.end() ? &i->second : nullptr;
    }

    /// Return network replication attribute descriptions for an object type, or null if none defined.
    const ea::vector<AttributeInfo>* GetNetworkAttributes(StringHash type) const
    {
        auto i = networkAttributes_.find(type);
        return i != networkAttributes_.end() ? &i->second : nullptr;
    }

    /// Return all registered attributes.
    const ea::unordered_map<StringHash, ea::vector<AttributeInfo> >& GetAllAttributes() const { return attributes_; }

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

    /// Object factories.
    ea::unordered_map<StringHash, SharedPtr<ObjectFactory> > factories_;
    /// Subsystems.
    SubsystemCache subsystems_;
    /// Attribute descriptions per object type.
    ea::unordered_map<StringHash, ea::vector<AttributeInfo> > attributes_;
    /// Network replication attribute descriptions per object type.
    ea::unordered_map<StringHash, ea::vector<AttributeInfo> > networkAttributes_;
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
    /// Object categories.
    ea::unordered_map<ea::string, ea::vector<StringHash> > objectCategories_;
    /// Variant map for global variables that can persist throughout application execution.
    VariantMap globalVars_;
};

// Helper functions that terminate looping of argument list.
template <> inline void Context::RegisterFactory() { }
template <> inline void Context::RegisterFactory(const char* category) { }
template <> inline void Context::RemoveFactory<>() { }
template <> inline void Context::RemoveFactory<>(const char* category) { }

template <class T, class... Rest> void Context::RegisterFactory()
{
    RegisterFactory(new ObjectFactoryImpl<T>(this));
    RegisterFactory<Rest...>();
}

template <class T, class... Rest> void Context::RegisterFactory(const char* category)
{
    RegisterFactory(new ObjectFactoryImpl<T>(this), category);
    RegisterFactory<Rest...>(category);
}


template <class T, class... Rest> void Context::RemoveFactory()
{
    RemoveFactory(T::GetTypeStatic());
    RemoveFactory<Rest...>();
}

template <class T, class... Rest> void Context::RemoveFactory(const char* category)
{
    RemoveFactory(T::GetTypeStatic(), category);
    RemoveFactory<Rest...>(category);
}

template <class T> T* Context::RegisterSubsystem()
{
    auto* subsystem = new T(this);
    RegisterSubsystem(subsystem);
    return subsystem;
}

template <class T> void Context::RemoveSubsystem() { RemoveSubsystem(T::GetTypeStatic()); }

template <class T> AttributeHandle Context::RegisterAttribute(const AttributeInfo& attr) { return RegisterAttribute(T::GetTypeStatic(), attr); }

template <class T> void Context::RemoveAttribute(const char* name) { RemoveAttribute(T::GetTypeStatic(), name); }

template <class T> void Context::RemoveAllAttributes() { RemoveAllAttributes(T::GetTypeStatic()); }

template <class T, class U> void Context::CopyBaseAttributes() { CopyBaseAttributes(T::GetTypeStatic(), U::GetTypeStatic()); }

template <class T> T* Context::GetSubsystem() const { return subsystems_.Get<T>(); }

template <class T> AttributeInfo* Context::GetAttribute(const char* name) { return GetAttribute(T::GetTypeStatic(), name); }

template <class T> void Context::UpdateAttributeDefaultValue(const char* name, const Variant& defaultValue)
{
    UpdateAttributeDefaultValue(T::GetTypeStatic(), name, defaultValue);
}

}
