//
// Copyright (c) 2018 Rokas Kupstys
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

#include <Urho3D/Core/Object.h>
#include "Urho3DClassWrappers.hpp"


namespace Urho3D
{

class ManagedObjectFactory : public ObjectFactory
{
public:
    ManagedObjectFactory(Context* context, const char* typeName, StringHash baseType)
        : ObjectFactory(context)
        , baseType_(baseType)
        , managedType_(typeName)
    {
        typeInfo_ = new TypeInfo(typeName, context_->GetScripts()->GetRegisteredType(baseType));
    }

    ~ManagedObjectFactory() override
    {
        delete typeInfo_;
    }

public:
    SharedPtr<Object> CreateObject() override
    {
        return SharedPtr<Object>(ScriptSubsystem::managed_.CreateObject(context_, managedType_.Value()));
    }

protected:
    StringHash baseType_;
    StringHash managedType_;
};

class ManagedEventHandler : public EventHandler
{
public:
    ManagedEventHandler(Object* receiver, gchandle gcHandle, bool handleWithType)
        : EventHandler(receiver, nullptr)
        , handleWithType_(handleWithType)
        , gcHandle_(gcHandle)
    {
    }

    ~ManagedEventHandler() override
    {
        ScriptSubsystem::managed_.Unlock(gcHandle_);
        gcHandle_ = 0;
    }

    void Invoke(VariantMap& eventData) override
    {
        if (handleWithType_)
            ScriptSubsystem::managed_.HandleEventWithType(gcHandle_, eventType_.Value(), &eventData);
        else
            ScriptSubsystem::managed_.HandleEventWithoutType(gcHandle_, eventType_.Value(), &eventData);
    }

    EventHandler* Clone() const override
    {
        return new ManagedEventHandler(receiver_, ScriptSubsystem::managed_.CloneHandle(gcHandle_), handleWithType_);
    }

public:

protected:
    gchandle gcHandle_ = 0;
    bool handleWithType_ = false;
};

extern "C"
{

EXPORT_API void Urho3D_Context_RegisterFactory(Context* context, const char* typeName, unsigned baseType,
                                               const char* category)
{
    context->RegisterFactory(new ManagedObjectFactory(context, typeName, StringHash(baseType)), category);
}

EXPORT_API void Urho3D_Object_SubscribeToEvent(Object* receiver, gchandle gcHandle, unsigned eventType,
                                               bool handleWithType, Object* sender)
{
    // gcHandle is a handle to Action<> which references receiver object. We have to ensure object is alive as long as
    // engine will be sending events to it. On the other hand pinning receiver object is not required as it's lifetime
    // is managed by user or engine. If such object is deallocated it will simply stop sending events.
    if (sender == nullptr)
        receiver->SubscribeToEvent(StringHash(eventType), new ManagedEventHandler(receiver, gcHandle, handleWithType));
    else
        receiver->SubscribeToEvent(sender, StringHash(eventType), new ManagedEventHandler(receiver, gcHandle, handleWithType));
}

}

}
