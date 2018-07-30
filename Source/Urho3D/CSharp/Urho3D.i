%module(directors="1", dirprot="1", allprotected="1", naturalvar=1) Urho3D

#define URHO3D_STATIC
#define URHO3D_API
#define final
#define static_assert(...)

%include "stl.i"

%{
#include <Urho3D/Urho3DAll.h>
%}

// Map void* to IntPtr
%apply void* VOID_INT_PTR { void* }

// Speed boost
%pragma(csharp) imclassclassmodifiers="[System.Security.SuppressUnmanagedCodeSecurity]\ninternal class"
%typemap(csclassmodifiers) SWIGTYPE "public partial class"

%include "cmalloc.i"
%include "Helpers.i"
%include "Math.i"
%include "_constants.i"
%include "String.i"

// Container
%include "Container/RefCounted.i"
%include "Container/Vector.i"
%include "Container/HashMap.i"

%include "_properties.i"

%include "Core.i"
