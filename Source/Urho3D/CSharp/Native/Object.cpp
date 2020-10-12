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

#include <Urho3D/Core/Object.h>
#include <Urho3D/Script/Script.h>

namespace Urho3D
{

typedef void(SWIGSTDCALL*EventHandlerCallback)(void*, unsigned, VariantMap*);

class ManagedEventHandler : public EventHandler
{
public:
    ManagedEventHandler(Object* receiver, EventHandlerCallback callback, void* callbackHandle)
        : EventHandler(receiver, nullptr)
        , callback_(callback)
        , callbackHandle_(callbackHandle)
    {
    }

    ~ManagedEventHandler() override
    {
        Script::GetRuntimeApi()->FreeGCHandle(callbackHandle_);
        callbackHandle_ = nullptr;
    }

    void Invoke(VariantMap& eventData) override
    {
        callback_(callbackHandle_, eventType_.Value(), &eventData);
    }

    EventHandler* Clone() const override
    {
        return new ManagedEventHandler(receiver_, callback_, Script::GetRuntimeApi()->CloneGCHandle(callbackHandle_));
    }

public:

protected:
    EventHandlerCallback callback_ = nullptr;
    void* callbackHandle_ = nullptr;
};

extern "C"
{

URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_Object_SubscribeToEvent(Object* receiver, Object* sender, unsigned eventType,
    EventHandlerCallback callback, void* callbackHandle)
{
    // callbackHandle is a handle to Action<> which references receiver object. We have to ensure object is alive as long as
    // engine will be sending events to it. On the other hand pinning receiver object is not required as it's lifetime
    // is managed by user or engine. If such object is deallocated it will simply stop sending events.
    StringHash event(eventType);
    if (sender == nullptr)
        receiver->SubscribeToEvent(event, new ManagedEventHandler(receiver, callback, callbackHandle));
    else
        receiver->SubscribeToEvent(sender, event, new ManagedEventHandler(receiver, callback, callbackHandle));
}

}

}
