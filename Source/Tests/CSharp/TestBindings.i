%module(naturalvar=1) TestBindings
%{
#include <Urho3D/Urho3DAll.h>
using namespace Urho3D;
#include <Urho3D/CSharp/Native/SWIGHelpers.h>
%}
#define PLUGIN_CORE_SAMPLEPLUGIN_API
%import "../../Urho3D/CSharp/Swig/Urho3D.i"

%{

int testBindingsFunc()
{
    return 42;
}

%}

//%include "SampleComponent.h"

int testBindingsFunc();
