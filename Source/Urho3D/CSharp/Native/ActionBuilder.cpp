// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#if URHO3D_ACTIONS
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
#endif  // URHO3D_ACTIONS
