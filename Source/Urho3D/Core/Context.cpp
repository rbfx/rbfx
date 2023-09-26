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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../IO/Log.h"

#include "../Audio/Audio.h"
#include "../Engine/Engine.h"
#include "../Core/WorkQueue.h"
#include "../Core/Thread.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Renderer.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/Localization.h"
#if URHO3D_NETWORK
#include "../Network/Network.h"
#endif
#include "../Input/Input.h"
#include "../UI/UI.h"
#if URHO3D_SYSTEMUI
#include "../SystemUI/SystemUI.h"
#endif

#include <SDL.h>

#include "../DebugNew.h"

namespace Urho3D
{

// Keeps track of how many times SDL was initialised so we know when to call SDL_Quit().
static int sdlInitCounter = 0;

// Global context instance. Set in Context constructor.
static Context* contextInstance = nullptr;

void EventReceiverGroup::BeginSendEvent()
{
    ++inSend_;
}

void EventReceiverGroup::EndSendEvent()
{
    assert(inSend_ > 0);
    --inSend_;

    if (inSend_ == 0 && dirty_)
    {
        /// \todo Could be optimized by erase-swap, but this keeps the receiver order
        for (unsigned i = receivers_.size() - 1; i < receivers_.size(); --i)
        {
            if (!receivers_[i])
                receivers_.erase_at(i);
        }

        dirty_ = false;
    }
}

void EventReceiverGroup::Add(Object* object)
{
    if (object)
        receivers_.push_back(object);
}

void EventReceiverGroup::Remove(Object* object)
{
    if (inSend_ > 0)
    {
        auto i = receivers_.find(object);
        if (i != receivers_.end())
        {
            (*i) = nullptr;
            dirty_ = true;
        }
    }
    else
        receivers_.erase_first(object);
}

void RemoveNamedAttribute(ea::unordered_map<StringHash, ea::vector<AttributeInfo> >& attributes, StringHash objectType, const char* name)
{
    auto i = attributes.find(objectType);
    if (i == attributes.end())
        return;

    ea::vector<AttributeInfo>& infos = i->second;

    for (auto j = infos.begin(); j != infos.end(); ++j)
    {
        if (!j->name_.comparei(name))
        {
            infos.erase(j);
            break;
        }
    }

    // If the vector became empty, erase the object type from the map
    if (infos.empty())
        attributes.erase(i);
}

Context::Context()
    : ObjectReflectionRegistry(this)
    , eventHandler_(nullptr)
{
    assert(contextInstance == nullptr);
    contextInstance = this;

#ifdef __ANDROID__
    // Always reset the random seed on Android, as the Urho3D library might not be unloaded between runs
    SetRandomSeed(1);
#endif

    // Set the main thread ID (assuming the Context is created in it)
    Thread::SetMainThread();
}

Context::~Context()
{
    // Destroying resource cache does clear it, however some resources depend on resource cache being available when
    // destructor executes.
    if (auto* cache = GetSubsystem<ResourceCache>())
        cache->Clear();

    // Destroy PluginManager last because it may unload DLLs and make some classes destructors unavailable.
    SharedPtr<Object> pluginManager{GetSubsystem("PluginManager")};

    // Remove subsystems that use SDL in reverse order of construction, so that Graphics can shut down SDL last
    /// \todo Context should not need to know about subsystems
    RemoveSubsystem("VirtualReality");
    RemoveSubsystem("PluginManager");
    RemoveSubsystem("Audio");
    RemoveSubsystem("UI");
    RemoveSubsystem("SystemUI");
    RemoveSubsystem("ResourceCache");
    RemoveSubsystem("Input");
    RemoveSubsystem("Renderer");
    RemoveSubsystem("Graphics");
    RemoveSubsystem("StateManager");

    subsystems_.Clear();
    pluginManager = nullptr;

    // Delete allocated event data maps
    for (auto i = eventDataMaps_.begin(); i != eventDataMaps_.end(); ++i)
        delete *i;
    eventDataMaps_.clear();

    assert(contextInstance == this);
    contextInstance = nullptr;
}

Context* Context::GetInstance()
{
    return contextInstance;
}

void Context::RegisterSubsystem(Object* object, StringHash type)
{
    if (!object)
        return;

    bool isTypeValid = false;
    for (const TypeInfo* typeInfo = object->GetTypeInfo(); typeInfo != nullptr && !isTypeValid; typeInfo = typeInfo->GetBaseTypeInfo())
        isTypeValid = typeInfo->GetType() == type;

    if (isTypeValid)
        subsystems_.Add(type, object);
    else
        URHO3D_LOGERROR("Type supplied to RegisterSubsystem() does not belong to object inheritance hierarchy.");
}

void Context::RegisterSubsystem(Object* object)
{
    if (!object)
        return;

    subsystems_.Add(object->GetType(), object);
}

void Context::RemoveSubsystem(StringHash objectType)
{
    subsystems_.Remove(objectType);
}

VariantMap& Context::GetEventDataMap()
{
    unsigned nestingLevel = eventSenders_.size();
    while (eventDataMaps_.size() < nestingLevel + 1)
        eventDataMaps_.push_back(new VariantMap());

    VariantMap& ret = *eventDataMaps_[nestingLevel];
    ret.clear();
    return ret;
}

bool Context::RequireSDL(unsigned int sdlFlags)
{
    // Always increment, the caller must match with ReleaseSDL(), regardless of
    // what happens.
    ++sdlInitCounter;

    // Need to call SDL_Init() at least once before SDL_InitSubsystem()
    if (sdlInitCounter == 1)
    {
        URHO3D_LOGDEBUG("Initialising SDL");
        if (SDL_Init(0) != 0)
        {
            URHO3D_LOGERRORF("Failed to initialise SDL: %s", SDL_GetError());
            return false;
        }
    }

    Uint32 remainingFlags = sdlFlags & ~SDL_WasInit(0);
    if (remainingFlags != 0)
    {
        if (SDL_InitSubSystem(remainingFlags) != 0)
        {
            URHO3D_LOGERRORF("Failed to initialise SDL subsystem: %s", SDL_GetError());
            return false;
        }
    }

    return true;
}

void Context::ReleaseSDL()
{
    --sdlInitCounter;

    if (sdlInitCounter == 0)
    {
        URHO3D_LOGDEBUG("Quitting SDL");
        SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
        SDL_Quit();
    }

    if (sdlInitCounter < 0)
        URHO3D_LOGERROR("Too many calls to Context::ReleaseSDL()!");
}

void Context::SetUnitTest(bool isUnitTest)
{
    isUnitTest_ = isUnitTest;
}

bool Context::IsUnitTest() const
{
    return isUnitTest_;
}

Object* Context::GetSubsystem(StringHash type) const
{
    return subsystems_.Get(type);
}

const Variant& Context::GetGlobalVar(StringHash key) const
{
    auto i = globalVars_.find(key);
    return i != globalVars_.end() ? i->second : Variant::EMPTY;
}

void Context::SetGlobalVar(StringHash key, const Variant& value)
{
    globalVars_[key] = value;
}

Object* Context::GetEventSender() const
{
    if (!eventSenders_.empty())
        return eventSenders_.back();
    else
        return nullptr;
}

const ea::string& Context::GetTypeName(StringHash objectType) const
{
    // Search factories to find the hash-to-name mapping
    auto reflection = GetReflection(objectType);
    return reflection ? reflection->GetTypeName() : EMPTY_STRING;
}

AttributeInfo* Context::GetAttribute(StringHash objectType, const char* name)
{
    ObjectReflection* reflection = GetReflection(objectType);
    return reflection ? reflection->GetAttribute(name) : nullptr;
}

void Context::AddEventReceiver(Object* receiver, StringHash eventType)
{
    SharedPtr<EventReceiverGroup>& group = eventReceivers_[eventType];
    if (!group)
        group = new EventReceiverGroup();
    group->Add(receiver);
}

void Context::AddEventReceiver(Object* receiver, Object* sender, StringHash eventType)
{
    SharedPtr<EventReceiverGroup>& group = specificEventReceivers_[sender][eventType];
    if (!group)
        group = new EventReceiverGroup();
    group->Add(receiver);
}

void Context::RemoveEventSender(Object* sender)
{
    auto i = specificEventReceivers_.find(
        sender);
    if (i != specificEventReceivers_.end())
    {
        for (auto j = i->second.begin(); j != i->second.end(); ++j)
        {
            for (auto k = j->second->receivers_.begin(); k !=
                j->second->receivers_.end(); ++k)
            {
                Object* receiver = *k;
                if (receiver)
                    receiver->RemoveEventSender(sender);
            }
        }
        specificEventReceivers_.erase(i);
    }
}

void Context::RemoveEventReceiver(Object* receiver, StringHash eventType)
{
    EventReceiverGroup* group = GetEventReceivers(eventType);
    if (group)
        group->Remove(receiver);
}

void Context::RemoveEventReceiver(Object* receiver, Object* sender, StringHash eventType)
{
    EventReceiverGroup* group = GetEventReceivers(sender, eventType);
    if (group)
        group->Remove(receiver);
}

void Context::BeginSendEvent(Object* sender, StringHash eventType)
{
    eventSenders_.push_back(sender);
}

void Context::EndSendEvent()
{
    eventSenders_.pop_back();
}

}
