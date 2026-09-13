// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
