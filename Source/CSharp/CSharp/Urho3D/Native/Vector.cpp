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

#include "CSharp.h"

extern "C"
{

void Urho3D_StringVector_Add(StringVector* instance, MonoString* value)
{
    instance->Push(CSharpConverter<MonoString>::FromCSharp<Urho3D::String>(value));
}

void Urho3D_StringVector_InsertAt(StringVector* instance, int index, MonoString* value)
{
    instance->Insert(index, CSharpConverter<MonoString>::FromCSharp<Urho3D::String>(value));
}

void Urho3D_StringVector_Set(StringVector* instance, int index, MonoString* value)
{
    instance->operator[](index) = CSharpConverter<MonoString>::FromCSharp<Urho3D::String>(value);
}

MonoString* Urho3D_StringVector_Get(StringVector* instance, int index)
{
    return CSharpConverter<MonoString>::ToCSharp(instance->operator[](index));
}

bool Urho3D_StringVector_Remove(StringVector* instance, MonoString* value)
{
    return instance->Remove(CSharpConverter<MonoString>::FromCSharp<Urho3D::String>(value));
}

bool Urho3D_StringVector_RemoveAt(StringVector* instance, int index)
{
    auto oldSize = instance->Size();
    instance->Erase(index, 1);
    return instance->Size() < oldSize;
}

void Urho3D_StringVector_Clear(StringVector* instance)
{
    instance->Clear();
}

bool Urho3D_StringVector_Contains(StringVector* instance, MonoString* value)
{
    return instance->Contains(CSharpConverter<MonoString>::FromCSharp<Urho3D::String>(value));
}

int Urho3D_StringVector_IndexOf(StringVector* instance, MonoString* value)
{
    auto index = instance->IndexOf(CSharpConverter<MonoString>::FromCSharp<Urho3D::String>(value));
    if (index == instance->Size())
        return -1;
    return index;
}

void Urho3D_StringVector_destructor(StringVector* instance)
{
    delete instance;
}

void RegisterVectorInternalCalls(Context* context)
{
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_Add);
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_InsertAt);
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_Set);
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_Get);
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_Remove);
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_RemoveAt);
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_Clear);
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_Contains);
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_IndexOf);
    MONO_INTERNAL_CALL(Urho3D.StringVector, Urho3D_StringVector_destructor);
}

}
