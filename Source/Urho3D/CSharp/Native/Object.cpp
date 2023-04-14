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

#include <EASTL/shared_ptr.h>

namespace Urho3D
{

typedef void(SWIGSTDCALL* EventHandlerCallback)(void*, unsigned, VariantMap*);

auto WrapCSharpHandler(EventHandlerCallback callback, void* callbackHandle)
{
    // callbackHandle is a handle to Action<> which references receiver object. We have to ensure object is alive as
    // long as engine will be sending events to it. On the other hand pinning receiver object is not required as it's
    // lifetime is managed by user or engine. If such object is deallocated it will simply stop sending events.
    ea::shared_ptr<void> callbackHandlePtr(callbackHandle,
        [](void* handle)
    {
        if (handle)
            Script::GetRuntimeApi()->FreeGCHandle(handle);
    });

    return [=](Object* receiver, StringHash eventType, VariantMap& eventData)
    { callback(callbackHandlePtr.get(), eventType.Value(), &eventData); };
}

extern "C"
{

URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_Object_SubscribeToEvent(Object* receiver, Object* sender, unsigned eventType,
    EventHandlerCallback callback, void* callbackHandle)
{
    const auto eventHandler = WrapCSharpHandler(callback, callbackHandle);
    if (sender == nullptr)
        receiver->SubscribeToEvent(StringHash{eventType}, eventHandler);
    else
        receiver->SubscribeToEvent(sender, StringHash{eventType}, eventHandler);
}

}

}
