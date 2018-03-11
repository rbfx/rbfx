#include "CSharp.h"

extern "C"
{

URHO3D_EXPORT_API void Urho3D_StringVector_Add(StringVector* instance, const char* value)
{
    instance->Push(value);
}

URHO3D_EXPORT_API void Urho3D_StringVector_InsertAt(StringVector* instance, int index, const char* value)
{
    instance->Insert(index, value);
}

URHO3D_EXPORT_API void Urho3D_StringVector_Set(StringVector* instance, int index, const char* value)
{
    instance->operator[](index) = value;
}

URHO3D_EXPORT_API const char* Urho3D_StringVector_Get(StringVector* instance, int index)
{
    return strdup(instance->operator[](index).CString());
}

URHO3D_EXPORT_API bool Urho3D_StringVector_Remove(StringVector* instance, const char* value)
{
    return instance->Remove(value);
}

URHO3D_EXPORT_API bool Urho3D_StringVector_RemoveAt(StringVector* instance, int index)
{
    auto oldSize = instance->Size();
    instance->Erase(index, 1);
    return instance->Size() < oldSize;
}

URHO3D_EXPORT_API void Urho3D_StringVector_Clear(StringVector* instance)
{
    instance->Clear();
}

URHO3D_EXPORT_API bool Urho3D_StringVector_Contains(StringVector* instance, const char* value)
{
    return instance->Contains(value);
}

URHO3D_EXPORT_API int Urho3D_StringVector_IndexOf(StringVector* instance, const char* value)
{
    auto index = instance->IndexOf(value);
    if (index == instance->Size())
        return -1;
    return index;
}

///////////////////////////////////////////////// VectorBase ///////////////////////////////////////////////////////////
URHO3D_EXPORT_API unsigned char* Urho3D_VectorBase_AllocateBuffer(int size)
{
    return VectorBase::AllocateBuffer(size);
}

URHO3D_EXPORT_API unsigned char* Urho3D_VectorBase_FreeBuffer(unsigned char* buffer)
{
    VectorBase::FreeBuffer(buffer);
}

URHO3D_EXPORT_API VectorBase* Urho3D_VectorBase_VectorBase()
{
    return script->TakeOwnership<VectorBase>(new VectorBase());
}

class VectorBaseAccessor : public VectorBase
{
public:
    unsigned char* Buffer() { return buffer_; }
};

URHO3D_EXPORT_API void Urho3D_VectorBase_destructor(VectorBase* instance)
{
    if (auto handler = script->GetHandler(instance))
    {
        // Undefined behavior. Most likely safe but proper solution should be figured out some time.
        if (handler->deleter_ != nullptr)
            VectorBase::FreeBuffer(((VectorBaseAccessor*)instance)->Buffer());
    }
    script->ReleaseRef<VectorBase>(instance);
}

}
