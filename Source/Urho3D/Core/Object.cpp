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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/ProcessUtils.h"
#include "../Core/Thread.h"
#include "../Core/Profiler.h"
#include "../IO/Log.h"

#include "../DebugNew.h"


namespace Urho3D
{

TypeInfo::TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo) :
    type_(typeName),
    typeName_(typeName),
    baseTypeInfo_(baseTypeInfo)
{
}

TypeInfo::~TypeInfo() = default;

bool TypeInfo::IsTypeOf(StringHash type) const
{
    const TypeInfo* current = this;
    while (current)
    {
        if (current->GetType() == type)
            return true;

        current = current->GetBaseTypeInfo();
    }

    return false;
}

bool TypeInfo::IsTypeOf(const TypeInfo* typeInfo) const
{
    if (typeInfo == nullptr)
        return false;

    const TypeInfo* current = this;
    while (current)
    {
        if (current == typeInfo || current->GetType() == typeInfo->GetType())
            return true;

        current = current->GetBaseTypeInfo();
    }

    return false;
}

Object::Object(Context* context) :
    context_(context),
    blockEvents_(false)
{
    assert(context_);
}

Object::~Object()
{
    if (!context_.Expired())
    {
        UnsubscribeFromAllEvents();
        context_->RemoveEventSender(this);
    }
}

void Object::OnEvent(Object* sender, StringHash eventType, VariantMap& eventData)
{
    if (blockEvents_)
        return;

    // Make a copy of the context pointer in case the object is destroyed during event handler invocation
    Context* context = context_;
    EventHandler* specific = nullptr;
    EventHandler* nonSpecific = nullptr;

    for (auto& handler : eventHandlers_)
    {
        if (handler.GetEventType() == eventType)
        {
            if (!handler.GetSender())
                nonSpecific = &handler;
            else if (handler.GetSender() == sender)
            {
                specific = &handler;
                break;
            }
        }
    }

    // Specific event handlers have priority, so if found, invoke first
    if (specific)
    {
        context->SetEventHandler(specific);
        specific->Invoke(eventData);
        context->SetEventHandler(nullptr);
        return;
    }

    if (nonSpecific)
    {
        context->SetEventHandler(nonSpecific);
        nonSpecific->Invoke(eventData);
        context->SetEventHandler(nullptr);
    }
}

bool Object::Serialize(Archive& /*archive*/)
{
    URHO3D_LOGERROR("Serialization is not supported for " + GetTypeInfo()->GetTypeName());
    assert(0);
    return false;
}

bool Object::Serialize(Archive& /*archive*/, ArchiveBlock& /*block*/)
{
    URHO3D_LOGERROR("Serialization is not supported for " + GetTypeInfo()->GetTypeName());
    assert(0);
    return false;
}

bool Object::IsInstanceOf(StringHash type) const
{
    return GetTypeInfo()->IsTypeOf(type);
}

bool Object::IsInstanceOf(const TypeInfo* typeInfo) const
{
    return GetTypeInfo()->IsTypeOf(typeInfo);
}

void Object::SubscribeToEvent(StringHash eventType, EventHandler* handler)
{
    if (!handler)
        return;

    handler->SetSenderAndEventType(nullptr, eventType);
    // Remove old event handler first
    auto oldHandler = FindSpecificEventHandler(nullptr, eventType);
    if (oldHandler != eventHandlers_.end())
    {
        EraseEventHandler(oldHandler);
        eventHandlers_.insert(eventHandlers_.begin(), *handler);
    }
    else
    {
        eventHandlers_.insert(eventHandlers_.begin(), *handler);
        context_->AddEventReceiver(this, eventType);
    }
}

void Object::SubscribeToEvent(Object* sender, StringHash eventType, EventHandler* handler)
{
    // If a null sender was specified, the event can not be subscribed to. Delete the handler in that case
    if (!sender || !handler)
    {
        delete handler;
        return;
    }

    handler->SetSenderAndEventType(sender, eventType);
    // Remove old event handler first
    auto oldHandler = FindSpecificEventHandler(sender, eventType);
    if (oldHandler != eventHandlers_.end())
    {
        EraseEventHandler(oldHandler);
        eventHandlers_.insert(eventHandlers_.begin(), *handler);
    }
    else
    {
        eventHandlers_.insert(eventHandlers_.begin(), *handler);
        context_->AddEventReceiver(this, sender, eventType);
    }
}

void Object::SubscribeToEvent(StringHash eventType, const std::function<void(StringHash, VariantMap&)>& function, void* userData/*=0*/)
{
    SubscribeToEvent(eventType, new EventHandler11Impl(function, userData));
}

void Object::SubscribeToEvent(Object* sender, StringHash eventType, const std::function<void(StringHash, VariantMap&)>& function, void* userData/*=0*/)
{
    SubscribeToEvent(sender, eventType, new EventHandler11Impl(function, userData));
}

void Object::UnsubscribeFromEvent(StringHash eventType)
{
    for (;;)
    {
        auto handler = FindEventHandler(eventType);
        if (handler != eventHandlers_.end())
        {
            if (handler->GetSender())
                context_->RemoveEventReceiver(this, handler->GetSender(), eventType);
            else
                context_->RemoveEventReceiver(this, eventType);
            EraseEventHandler(handler);
        }
        else
            break;
    }
}

void Object::UnsubscribeFromEvent(Object* sender, StringHash eventType)
{
    if (!sender)
        return;

    auto handler = FindSpecificEventHandler(sender, eventType);
    if (handler != eventHandlers_.end())
    {
        context_->RemoveEventReceiver(this, handler->GetSender(), eventType);
        EraseEventHandler(handler);
    }
}

void Object::UnsubscribeFromEvents(Object* sender)
{
    if (!sender)
        return;

    for (;;)
    {
        auto handler = FindSpecificEventHandler(sender);
        if (handler != eventHandlers_.end())
        {
            context_->RemoveEventReceiver(this, handler->GetSender(), handler->GetEventType());
            EraseEventHandler(handler);
        }
        else
            break;
    }
}

void Object::UnsubscribeFromAllEvents()
{
    for (;;)
    {
        auto handler = eventHandlers_.begin();
        if (handler != eventHandlers_.end())
        {
            if (handler->GetSender())
                context_->RemoveEventReceiver(this, handler->GetSender(), handler->GetEventType());
            else
                context_->RemoveEventReceiver(this, handler->GetEventType());
            EraseEventHandler(handler);
        }
        else
            break;
    }
}

void Object::UnsubscribeFromAllEventsExcept(const ea::vector<StringHash>& exceptions, bool onlyUserData)
{
    for (auto handler = eventHandlers_.begin(); handler != eventHandlers_.end(); )
    {
        if ((!onlyUserData || handler->GetUserData()) && !exceptions.contains(handler->GetEventType()))
        {
            if (handler->GetSender())
                context_->RemoveEventReceiver(this, handler->GetSender(), handler->GetEventType());
            else
                context_->RemoveEventReceiver(this, handler->GetEventType());

            handler = EraseEventHandler(handler);
        }
        else
            ++handler;
    }
}

void Object::UnsubscribeFromAllEventsExcept(const ea::vector<Object*>& exceptions, bool onlyUserData)
{
    for (auto handler = eventHandlers_.begin(); handler != eventHandlers_.end(); )
    {
        if ((!onlyUserData || handler->GetUserData()) && !exceptions.contains(handler->GetSender()))
        {
            if (handler->GetSender())
                context_->RemoveEventReceiver(this, handler->GetSender(), handler->GetEventType());
            else
                context_->RemoveEventReceiver(this, handler->GetEventType());

            handler = EraseEventHandler(handler);
        }
        else
            ++handler;
    }
}

void Object::SendEvent(StringHash eventType)
{
    VariantMap noEventData;

    SendEvent(eventType, noEventData);
}

void Object::SendEvent(StringHash eventType, VariantMap& eventData)
{
    if (!Thread::IsMainThread())
    {
        URHO3D_LOGERROR("Sending events is only supported from the main thread");
        return;
    }

    if (blockEvents_)
        return;

#if URHO3D_PROFILING
    URHO3D_PROFILE_C("SendEvent", PROFILER_COLOR_EVENTS);
    const auto& eventName = GetEventNameRegister().GetString(eventType);
    URHO3D_PROFILE_ZONENAME(eventName.c_str(), eventName.length());
#endif

    // Make a weak pointer to self to check for destruction during event handling
    WeakPtr<Object> self(this);
    Context* context = context_;

    context->BeginSendEvent(this, eventType);

    // Check first the specific event receivers
    // Note: group is held alive with a shared ptr, as it may get destroyed along with the sender
    SharedPtr<EventReceiverGroup> group(context->GetEventReceivers(this, eventType));
    if (group)
    {
        group->BeginSendEvent();

        const unsigned numReceivers = group->receivers_.size();
        for (unsigned i = 0; i < numReceivers; ++i)
        {
            Object* receiver = group->receivers_[i];
            // Holes may exist if receivers removed during send
            if (!receiver)
                continue;

            receiver->OnEvent(this, eventType, eventData);

            // If self has been destroyed as a result of event handling, exit
            if (self.Expired())
            {
                group->EndSendEvent();
                context->EndSendEvent();
                return;
            }
        }

        group->EndSendEvent();
    }

    // Then the non-specific receivers
    SharedPtr<EventReceiverGroup> groupNonSpec(context->GetEventReceivers(eventType));
    if (groupNonSpec)
    {
        groupNonSpec->BeginSendEvent();

        const unsigned numReceivers = groupNonSpec->receivers_.size();
        for (unsigned i = 0; i < numReceivers; ++i)
        {
            Object* receiver = groupNonSpec->receivers_[i];
            // If there were specific receivers, check that the event is not sent doubly to them
            if (!receiver || (group && group->receivers_.contains(receiver)))
                continue;

            receiver->OnEvent(this, eventType, eventData);

            if (self.Expired())
            {
                groupNonSpec->EndSendEvent();
                context->EndSendEvent();
                return;
            }
        }

        groupNonSpec->EndSendEvent();
    }

    context->EndSendEvent();
}

VariantMap& Object::GetEventDataMap() const
{
    return context_->GetEventDataMap();
}

const Variant& Object::GetGlobalVar(StringHash key) const
{
    return context_->GetGlobalVar(key);
}

const VariantMap& Object::GetGlobalVars() const
{
    return context_->GetGlobalVars();
}

void Object::SetGlobalVar(StringHash key, const Variant& value)
{
    context_->SetGlobalVar(key, value);
}

Object* Object::GetSubsystem(StringHash type) const
{
    return context_->GetSubsystem(type);
}

Object* Object::GetEventSender() const
{
    return context_->GetEventSender();
}

EventHandler* Object::GetEventHandler() const
{
    return context_->GetEventHandler();
}

bool Object::HasSubscribedToEvent(StringHash eventType) const
{
    return FindEventHandler(eventType) != eventHandlers_.end();
}

bool Object::HasSubscribedToEvent(Object* sender, StringHash eventType) const
{
    if (!sender)
        return false;
    else
        return FindSpecificEventHandler(sender, eventType) != eventHandlers_.end();
}

const ea::string& Object::GetCategory() const
{
    const ea::unordered_map<ea::string, ea::vector<StringHash> >& objectCategories = context_->GetObjectCategories();
    for (auto i = objectCategories.begin(); i != objectCategories.end(); ++i)
    {
        if (i->second.contains(GetType()))
            return i->first;
    }

    return EMPTY_STRING;
}

const SubsystemCache& Object::GetSubsystems() const
{
    return context_->GetSubsystems();
}

ea::intrusive_list<EventHandler>::iterator Object::FindEventHandler(StringHash eventType)
{
    return ea::find_if(eventHandlers_.begin(), eventHandlers_.end(), [eventType](const EventHandler& e) {
        return e.GetEventType() == eventType;
    });
}

ea::intrusive_list<EventHandler>::iterator Object::FindSpecificEventHandler(Object* sender)
{
    return ea::find_if(eventHandlers_.begin(), eventHandlers_.end(), [sender](const EventHandler& e) {
        return e.GetSender() == sender;
    });
}

ea::intrusive_list<EventHandler>::iterator Object::FindSpecificEventHandler(Object* sender, StringHash eventType)
{
    return ea::find_if(eventHandlers_.begin(), eventHandlers_.end(), [sender, eventType](const EventHandler& e) {
        return e.GetSender() == sender && e.GetEventType() == eventType;
    });
}

ea::intrusive_list<EventHandler>::iterator Object::EraseEventHandler(ea::intrusive_list<EventHandler>::iterator handlerIter)
{
    assert(handlerIter != eventHandlers_.end());
    EventHandler* handler = &*handlerIter;
    auto nextIter = eventHandlers_.erase(handlerIter);
    delete handler;
    return nextIter;
}

void Object::RemoveEventSender(Object* sender)
{
    for (auto handler = eventHandlers_.begin(); handler != eventHandlers_.end(); )
    {
        if (handler->GetSender() == sender)
            handler = EraseEventHandler(handler);
        else
            ++handler;
    }
}

StringHashRegister& GetEventNameRegister()
{
    static StringHashRegister eventNameRegister(false /*non thread safe*/);
    return eventNameRegister;
}

void Object::SendEvent(StringHash eventType, const VariantMap& eventData)
{
    VariantMap eventDataCopy = eventData;
    SendEvent(eventType, eventDataCopy);
}

}
