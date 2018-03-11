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

URHO3D_EXPORT_API int Urho3D_StringVector_Count(StringVector* instance)
{
    return instance->Size();
}

URHO3D_EXPORT_API int Urho3D_StringVector_IndexOf(StringVector* instance, const char* value)
{
    auto index = instance->IndexOf(value);
    if (index == instance->Size())
        return -1;
    return index;
}

}
