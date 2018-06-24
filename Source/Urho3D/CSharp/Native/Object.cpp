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

#include <Urho3D/Core/Object.h>
#include "Urho3DClassWrappers.hpp"


namespace Urho3D
{

class ManagedObjectFactory : public ObjectFactory
{
public:
    ManagedObjectFactory(Context* context, const char* typeName, StringHash baseType)
        : ObjectFactory(context)
        , baseType_(baseType)
        , managedType_(typeName)
    {
        typeInfo_ = new TypeInfo(typeName, context_->GetScripts()->GetRegisteredType(baseType));
    }

    ~ManagedObjectFactory() override
    {
        delete typeInfo_;
    }

public:
    SharedPtr<Object> CreateObject() override
    {
        return SharedPtr<Object>(ScriptSubsystem::managed_.CreateObject(context_, managedType_.Value()));
    }

protected:
    StringHash baseType_;
    StringHash managedType_;
};

class ManagedEventHandler : public EventHandler
{
public:
    ManagedEventHandler(Object* receiver, gchandle gcHandle)
        : EventHandler(receiver, nullptr)
        , gcHandle_(gcHandle)
    {
    }

    ~ManagedEventHandler() override
    {
        ScriptSubsystem::managed_.Unlock(gcHandle_);
        gcHandle_ = 0;
    }

    void Invoke(VariantMap& eventData) override
    {
        ScriptSubsystem::managed_.HandleEvent(gcHandle_, eventType_.Value(), &eventData);
    }

    EventHandler* Clone() const override
    {
        return new ManagedEventHandler(receiver_, ScriptSubsystem::managed_.CloneHandle(gcHandle_));
    }

public:

protected:
    gchandle gcHandle_ = 0;
};

extern "C" {

EXPORT_API void Urho3D_Context_RegisterFactory(Context* context, const char* typeName, unsigned baseType, const char* category)
{
    context->RegisterFactory(new ManagedObjectFactory(context, typeName, StringHash(baseType)), category);
}

EXPORT_API void Urho3D_Object_SubscribeToEvent(Object* receiver, gchandle gcHandle, unsigned eventType, Object* sender)
{
    // gcHandle is a handle to Action<> which references receiver object. We have to ensure object is alive as long as
    // engine will be sending events to it. On the other hand pinning receiver object is not required as it's lifetime
    // is managed by user or engine. If such object is deallocated it will simply stop sending events.
    if (sender == nullptr)
        receiver->SubscribeToEvent(StringHash(eventType), new ManagedEventHandler(receiver, gcHandle));
    else
        receiver->SubscribeToEvent(sender, StringHash(eventType),
                                   new ManagedEventHandler(receiver, gcHandle));
}

///////////////////////////////////////////////////// Event ////////////////////////////////////////////////////////////


EXPORT_API void Urho3D_Object_Event_GetInt32(const VariantMap* map, unsigned key, int& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetInt();
}

EXPORT_API void Urho3D_Object_Event_SetInt32(VariantMap* map, unsigned key, int value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetBool(const VariantMap* map, unsigned key, bool& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetBool();
}

EXPORT_API void Urho3D_Object_Event_SetBool(VariantMap* map, unsigned key, bool value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetFloat(const VariantMap* map, unsigned key, float& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetFloat();
}

EXPORT_API void Urho3D_Object_Event_SetFloat(VariantMap* map, unsigned key, float value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetDouble(const VariantMap* map, unsigned key, double& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetDouble();
}

EXPORT_API void Urho3D_Object_Event_SetDouble(VariantMap* map, unsigned key, double value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetInt64(const VariantMap* map, unsigned key, long long& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetInt64();
}

EXPORT_API void Urho3D_Object_Event_SetInt64(VariantMap* map, unsigned key, long long value)
{
    map->operator[](StringHash(key)) = value;
}


EXPORT_API void Urho3D_Object_Event_GetObject(const VariantMap* map, unsigned key, int& type, void*& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
    {
        if (auto* gcHandleContainer = variant->GetCustomPtr<GcHandleContainer>())
        {
            type = 1;
            value = gcHandleContainer->handle_;
        }
        else if (auto* ptr = variant->GetPtr())
        {
            type = 2;
            value = (void*)ptr;
        }
    }
}

EXPORT_API void Urho3D_Object_Event_SetManagedObject(VariantMap* map, unsigned key, int type, void* value)
{
    if (type == 1)
        map->operator[](StringHash(key)) = MakeCustomValue(GcHandleContainer(value));
    else if (type == 2)
        map->operator[](StringHash(key)) = (RefCounted*)value;

}

EXPORT_API void Urho3D_Object_Event_GetNativeObject(const VariantMap* map, unsigned key, RefCounted*& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetPtr();
}

EXPORT_API void Urho3D_Object_Event_SetNativeObject(VariantMap* map, unsigned key, RefCounted* value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetVector2(const VariantMap* map, unsigned key, Vector2& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetVector2();
}

EXPORT_API void Urho3D_Object_Event_SetVector2(VariantMap* map, unsigned key, Vector2& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetVector3(const VariantMap* map, unsigned key, Vector3& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetVector3();
}

EXPORT_API void Urho3D_Object_Event_SetVector3(VariantMap* map, unsigned key, Vector3& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetVector4(const VariantMap* map, unsigned key, Vector4& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetVector4();
}

EXPORT_API void Urho3D_Object_Event_SetVector4(VariantMap* map, unsigned key, Vector4& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetRect(const VariantMap* map, unsigned key, Rect& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetRect();
}

EXPORT_API void Urho3D_Object_Event_SetRect(VariantMap* map, unsigned key, Rect& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetIntVector2(const VariantMap* map, unsigned key, IntVector2& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetIntVector2();
}

EXPORT_API void Urho3D_Object_Event_SetIntVector2(VariantMap* map, unsigned key, IntVector2& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetIntVector3(const VariantMap* map, unsigned key, IntVector3& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetIntVector3();
}

EXPORT_API void Urho3D_Object_Event_SetIntVector3(VariantMap* map, unsigned key, IntVector3& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetIntRect(const VariantMap* map, unsigned key, IntRect& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetIntRect();
}

EXPORT_API void Urho3D_Object_Event_SetIntRect(VariantMap* map, unsigned key, IntRect& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetMatrix3(const VariantMap* map, unsigned key, Matrix3& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetMatrix3();
}

EXPORT_API void Urho3D_Object_Event_SetMatrix3(VariantMap* map, unsigned key, Matrix3& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetMatrix3x4(const VariantMap* map, unsigned key, Matrix3x4& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetMatrix3x4();
}

EXPORT_API void Urho3D_Object_Event_SetMatrix3x4(VariantMap* map, unsigned key, Matrix3x4& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetMatrix4(const VariantMap* map, unsigned key, Matrix4& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetMatrix4();
}

EXPORT_API void Urho3D_Object_Event_SetMatrix4(VariantMap* map, unsigned key, Matrix4& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetQuaternion(const VariantMap* map, unsigned key, Quaternion& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetQuaternion();
}

EXPORT_API void Urho3D_Object_Event_SetQuaternion(VariantMap* map, unsigned key, Quaternion& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetColor(const VariantMap* map, unsigned key, Color& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = variant->GetColor();
}

EXPORT_API void Urho3D_Object_Event_SetColor(VariantMap* map, unsigned key, Color& value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetString(const VariantMap* map, unsigned key, const char*& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = CSharpConverter<String>::ToCSharp(variant->GetString());
}

EXPORT_API void Urho3D_Object_Event_SetString(VariantMap* map, unsigned key, const char* value)
{
    map->operator[](StringHash(key)) = value;
}

EXPORT_API void Urho3D_Object_Event_GetStrings(const VariantMap* map, unsigned key, const char**& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = CSharpConverter<StringVector>::ToCSharp(variant->GetStringVector());
}

EXPORT_API void Urho3D_Object_Event_SetStrings(VariantMap* map, unsigned key, const char** value)
{
    map->operator[](StringHash(key)) = CSharpConverter<StringVector>::FromCSharp(value);
}

EXPORT_API void Urho3D_Object_Event_GetBuffer(const VariantMap* map, unsigned key, uint8_t*& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = CSharpConverter<PODVector<uint8_t>>::ToCSharp(variant->GetBuffer());
}

EXPORT_API void Urho3D_Object_Event_SetBuffer(VariantMap* map, unsigned key, uint8_t* value)
{
    map->operator[](StringHash(key)) = CSharpConverter<PODVector<uint8_t>>::FromCSharp(value);
}

EXPORT_API void Urho3D_Object_Event_GetResourceRef(const VariantMap* map, unsigned key, ResourceRef*& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = CSharpObjConverter::ToCSharp<ResourceRef>(variant->GetResourceRef());
}

EXPORT_API void Urho3D_Object_Event_SetResourceRef(VariantMap* map, unsigned key, ResourceRef* value)
{
    map->operator[](StringHash(key)) = *value;
}

EXPORT_API void Urho3D_Object_Event_GetResourceRefList(const VariantMap* map, unsigned key, ResourceRefList*& value)
{
    if (auto* variant = map->operator[](StringHash(key)))
        value = CSharpObjConverter::ToCSharp<ResourceRefList>(variant->GetResourceRefList());
}

EXPORT_API void Urho3D_Object_Event_SetResourceRefList(VariantMap* map, unsigned key, ResourceRefList* value)
{
    map->operator[](StringHash(key)) = *value;
}

}

}
