#include <Urho3D/Core/Object.h>
#include "ClassWrappers.hpp"


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
        typeInfo_ = new TypeInfo(typeName, script->GetRegisteredType(baseType));
    }

    ~ManagedObjectFactory() override
    {
        delete typeInfo_;
    }

public:
    SharedPtr<Object> CreateObject() override
    {
        return SharedPtr<Object>(script->net_.CreateObject(context_, managedType_.Value()));
    }

protected:
    StringHash baseType_;
    StringHash managedType_;
};

class ManagedEventHandler : public EventHandler
{
public:
    ManagedEventHandler(Object* receiver, void* gcHandle, void(*function)(void*, StringHash, VariantMap*))
        : EventHandler(receiver, nullptr)
        , function_(function)
        , gcHandle_(gcHandle)
    {
    }

    ~ManagedEventHandler() override
    {
        script->net_.FreeGCHandle(gcHandle_);
        gcHandle_ = nullptr;
    }

    void Invoke(VariantMap& eventData) override
    {
        function_(gcHandle_, eventType_, script->AddRef<Urho3D::VariantMap>(eventData));
    }

    EventHandler* Clone() const override
    {
        return new ManagedEventHandler(receiver_, script->net_.CloneGCHandle(gcHandle_), function_);
    }

public:

protected:
    void* gcHandle_ = nullptr;
    void(*function_)(void*, StringHash, VariantMap*) = nullptr;
};

}

extern "C"
{

URHO3D_EXPORT_API void Urho3D_Context_RegisterFactory(Context* context, const char* typeName, unsigned baseType,
    const char* category)
{
    context->RegisterFactory(new Urho3D::ManagedObjectFactory(context, typeName, StringHash(baseType)), category);
}

URHO3D_EXPORT_API void Urho3D_Object_SubscribeToEvent(Object* receiver, void* gcHandle, unsigned eventType,
    void(*function)(void*, StringHash, VariantMap*), Object* sender)
{
    // gcHandle is a handle to Action<> which references receiver object. We have to ensure object is alive as long as
    // engine will be sending events to it. On the other hand pinning receiver object is not required as it's lifetime
    // is managed by user or engine. If such object is deallocated it will simply stop sending events.
    if (sender == nullptr)
        receiver->SubscribeToEvent(StringHash(eventType), new ManagedEventHandler(receiver, gcHandle, function));
    else
        receiver->SubscribeToEvent(sender, StringHash(eventType), new ManagedEventHandler(receiver, gcHandle, function));
}

}
