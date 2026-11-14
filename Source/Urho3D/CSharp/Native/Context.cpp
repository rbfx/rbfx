// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Core/Mutex.h"
#include "Urho3D/Core/ProcessUtils.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/Script/Script.h"

extern "C"
{

const Urho3D::TypeInfo* Urho3DGetDirectorTypeInfo(Urho3D::StringHash type);
typedef void* (SWIGSTDCALL* Urho3D_CSharpCreateObjectCallback)(Urho3D::Context* context, unsigned type);
extern Urho3D_CSharpCreateObjectCallback Urho3D_CSharpCreateObject;

}

namespace Urho3D
{

SharedPtr<Object> CreateManagedObject(const TypeInfo* typeInfo, Context* context)
{
    const StringHash managedType = typeInfo->GetType();
    auto object = reinterpret_cast<Object*>(Urho3D_CSharpCreateObject(context, managedType.Value()));
    return SharedPtr<Object>(object);
}

extern "C"
{

URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_Context_RegisterFactory(Context* context, const char* typeName, unsigned baseType, const char* category)
{
    auto typeInfo = ea::make_unique<TypeInfo>(typeName, Urho3DGetDirectorTypeInfo(StringHash(baseType)));
    auto reflection = context->ReflectCustomType(ea::move(typeInfo));
    reflection->SetObjectFactory(CreateManagedObject);
    if (category)
    {
        context->SetReflectionCategory(reflection->GetTypeInfo(), category);
    }
}

URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_ParseArguments(int argc, char** argv)
{
    ParseArguments(argc, argv);
}

}

}

