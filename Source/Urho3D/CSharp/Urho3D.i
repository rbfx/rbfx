%module(directors="1", dirprot="1", allprotected="1") Urho3D

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

// Bug https://github.com/swig/swig/issues/1291
%ignore Urho3D::ResourceRef::type_;
%ignore Urho3D::ResourceRefList::type_;
// csin typemap pre and terminator strings are not applied to variable properties.
%ignore Urho3D::ResourceRef;

%include "cmalloc.i"
%include "Helpers.i"
%include "Math.i"
%include "String.i"
%include "Container.i"
%include "Core.i"