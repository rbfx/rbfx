%module(naturalvar=1) TestBindings
%{
#include <Urho3D/Urho3DAll.h>
using namespace Urho3D;
#include <Urho3D/CSharp/Native/SWIGHelpers.h>
%}
#define PLUGIN_CORE_SAMPLEPLUGIN_API
%import "../../Urho3D/CSharp/Swig/Urho3D.i"

%inline %{

int TestBindingsFunc()
{
    return 42;
}

ea::string GetObjectTypeName(Urho3D::Object* obj)
{
    return obj->GetTypeName();
}

Urho3D::StringHash GetObjectTypeHash(Urho3D::Object* obj)
{
    return obj->GetType();
}

%}

//%include "SampleComponent.h"
