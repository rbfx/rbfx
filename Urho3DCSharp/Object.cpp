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
        const auto& factories = context_->GetObjectFactories();
        SharedPtr<ObjectFactory> factory;
        if (factories.TryGetValue(baseType, factory))
            typeInfo_ = new TypeInfo(typeName, factory->GetTypeInfo());
        assert(typeInfo_ != nullptr);
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

}

extern "C"
{

URHO3D_EXPORT_API void Urho3D_Context_RegisterFactory(Context* context, const char* typeName, unsigned baseType,
    const char* category)
{
    context->RegisterFactory(new Urho3D::ManagedObjectFactory(context, typeName, StringHash(baseType)), category);
}

}
