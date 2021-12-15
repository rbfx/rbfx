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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/Thread.h>
#include <Urho3D/Script/Script.h>

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
}

URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_ParseArguments(int argc, char** argv)
{
    ParseArguments(argc, argv);
}

}

}

