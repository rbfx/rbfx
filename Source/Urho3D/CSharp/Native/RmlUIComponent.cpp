// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#if URHO3D_RMLUI
#include "Urho3D/RmlUI/RmlUI.h"

#include <Urho3D/RmlUI/RmlUIComponent.h>
#include <Urho3D/Script/Script.h>

#include <EASTL/shared_ptr.h>

namespace Urho3D
{

typedef void(SWIGSTDCALL* GetterCallback)(void*, Variant*);
typedef void(SWIGSTDCALL* SetterCallback)(void*, const Variant*);
typedef void(SWIGSTDCALL* EventCallback)(void*, const VariantVector*);

RmlUIComponent::GetterFunc WrapCSharpHandler(GetterCallback callback, void* callbackHandle)
{
    const ea::shared_ptr<void> callbackHandlePtr(callbackHandle,
        [](void* handle)
    {
        if (handle)
            Script::GetRuntimeApi()->FreeGCHandle(handle);
    });

    return [=](Variant& variant)
    {
        callback(callbackHandlePtr.get(), &variant);
    };
}

RmlUIComponent::SetterFunc WrapCSharpHandler(SetterCallback callback, void* callbackHandle)
{
    const ea::shared_ptr<void> callbackHandlePtr(callbackHandle,
        [](void* handle)
    {
        if (handle)
            Script::GetRuntimeApi()->FreeGCHandle(handle);
    });

    return [=](const Variant& variant)
    {
        callback(callbackHandlePtr.get(), &variant);
    };
}

RmlUIComponent::EventFunc WrapCSharpHandler(EventCallback callback, void* callbackHandle)
{
    const ea::shared_ptr<void> callbackHandlePtr(callbackHandle,
        [](void* handle)
        {
        if (handle)
            Script::GetRuntimeApi()->FreeGCHandle(handle);
    });

    return [=](const VariantVector& args)
    {
        callback(callbackHandlePtr.get(), &args);
    };
}
extern "C"
{
    URHO3D_EXPORT_API bool SWIGSTDCALL Urho3D_RmlUIComponent_BindDataModelProperty(RmlUIComponent* receiver,
        char* jarg2, GetterCallback getter, void* getterHandle, SetterCallback setter, void* setterHandle)
    {
        eastl::string* arg2 = 0;

        eastl::string name(jarg2 ? jarg2 : "");

        const auto getterHandler = WrapCSharpHandler(getter, getterHandle);
        const auto setterHandler = WrapCSharpHandler(setter, setterHandle);
        return receiver->BindDataModelProperty(name, getterHandler, setterHandler);
    }

    URHO3D_EXPORT_API bool SWIGSTDCALL Urho3D_RmlUIComponent_BindDataModelEvent(
        RmlUIComponent* receiver, char* jarg2, EventCallback callback, void* callbackHandle)
    {
        eastl::string* arg2 = 0;

        eastl::string name(jarg2 ? jarg2 : "");

        const auto callbackHandler = WrapCSharpHandler(callback, callbackHandle);
        return receiver->BindDataModelEvent(name, callbackHandler);
    }
}

} // namespace Urho3D
#endif  // URHO3D_RMLUI
