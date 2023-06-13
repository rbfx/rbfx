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

#include <Urho3D/RmlUI/RmlUIComponent.h>
#include <Urho3D/Script/Script.h>

#include <EASTL/shared_ptr.h>

namespace Urho3D
{

typedef void(SWIGSTDCALL* GetterCallback)(void*, Variant*);
typedef void(SWIGSTDCALL* SetterCallback)(void*, const Variant*);

RmlUIComponent::GetterFunc WrapCSharpHandler(GetterCallback callback, void* callbackHandle)
{
    const ea::shared_ptr<void> callbackHandlePtr(callbackHandle,
        [](void* handle)
    {
        if (handle)
            Script::GetRuntimeApi()->FreeGCHandle(handle);
    });

    return [callback, callbackHandlePtr](Variant& variant)
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

    return [callback, callbackHandlePtr](const Variant& variant)
    {
        callback(callbackHandlePtr.get(), &variant);
    };
}
extern "C"
{
    URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_RmlUIComponent_BindDataModelProperty(RmlUIComponent* receiver,
        char* jarg2,
        GetterCallback getter, void* getterHandle, SetterCallback setter, void* setterHandle)
    {
        eastl::string* arg2 = 0;

        if (!jarg2)
        {
            //SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
            return;
        }

        eastl::string name(jarg2);

        const auto getterHandler = WrapCSharpHandler(getter, getterHandle);
        const auto setterHandler = WrapCSharpHandler(setter, setterHandle);
        receiver->BindDataModelProperty(name, getterHandler, setterHandler);
    }
}

} // namespace Urho3D
