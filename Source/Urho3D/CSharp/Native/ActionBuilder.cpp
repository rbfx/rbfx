//
// Copyright (c) 2022-2022 the rbfx project.
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

#include <Urho3D/Script/Script.h>
#include <Urho3D/Actions/ActionBuilder.h>

namespace Urho3D
{

typedef void(SWIGSTDCALL* ActionCallHandlerCallback)(void*, Object*);

class ManagedActionCallHandler : public Actions::ActionCallHandler
{
public:
    ManagedActionCallHandler(ActionCallHandlerCallback callback, void* callbackHandle)
        : ActionCallHandler(nullptr, nullptr)
        , callback_(callback)
        , callbackHandle_(callbackHandle)
    {
    }

    ~ManagedActionCallHandler() override
    {
        Script::GetRuntimeApi()->FreeGCHandle(callbackHandle_);
        callbackHandle_ = nullptr;
    }

    void Invoke(Object* eventData) override { callback_(callbackHandle_, eventData); }

public:
protected:
    ActionCallHandlerCallback callback_ = nullptr;
    void* callbackHandle_ = nullptr;
};

namespace
{
/* SwigValueWrapper is described in swig.swg */
template <typename T> class SwigValueWrapper
{
    struct SwigMovePointer
    {
        T* ptr;
        SwigMovePointer(T* p)
            : ptr(p)
        {
        }
        ~SwigMovePointer() { delete ptr; }
        SwigMovePointer& operator=(SwigMovePointer& rhs)
        {
            T* oldptr = ptr;
            ptr = 0;
            delete oldptr;
            ptr = rhs.ptr;
            rhs.ptr = 0;
            return *this;
        }
    } pointer;
    SwigValueWrapper& operator=(const SwigValueWrapper<T>& rhs);
    SwigValueWrapper(const SwigValueWrapper<T>& rhs);

public:
    SwigValueWrapper()
        : pointer(0)
    {
    }
    SwigValueWrapper& operator=(const T& t)
    {
        SwigMovePointer tmp(new T(t));
        pointer = tmp;
        return *this;
    }
    operator T&() const { return *pointer.ptr; }
    T* operator&() { return pointer.ptr; }
};

} // namespace

extern "C"
{

    URHO3D_EXPORT_API void* SWIGSTDCALL Urho3D_ActionBuilder_CallFunc(
        void* jarg1, ActionCallHandlerCallback callback, void* callbackHandle)
    {
        void* jresult;
        Urho3D::ActionBuilder* arg1 = (Urho3D::ActionBuilder*)0;
        SwigValueWrapper<Urho3D::ActionBuilder> result;

        arg1 = (Urho3D::ActionBuilder*)jarg1;
        result = arg1->CallFunc(new ManagedActionCallHandler(callback, callbackHandle));

        jresult = new Urho3D::ActionBuilder((const Urho3D::ActionBuilder&)result);

        return jresult;
    }
}

} // namespace Urho3D
