%module(directors="1", allprotected="1") Urho3D

#define URHO3D_STATIC
#define URHO3D_API
#define final
#define static_assert(...)

%include "stl.i"

%{
#include <Urho3D/Urho3DAll.h>
%}

// Map void* to IntPtr
%apply void *VOID_INT_PTR { void * }

%define URHO3D_SHARED_PTR(TYPE)
	%template(TYPE##Ref)     Urho3D::SharedPtr<Urho3D::TYPE>;
	%template(TYPE##WeakRef) Urho3D::WeakPtr<Urho3D::TYPE>;
%enddef

// StringHash
%typemap(ctype)  StringHash "unsigned"								// c layer type
%typemap(imtype) StringHash "uint"									// pinvoke type
%typemap(cstype) StringHash "global::Urh3DNet.StringHash"			// c# type
%typemap(in)     StringHash %{ $1 = Urho3D::StringHash($input); %}	// c to cpp
%typemap(out)    StringHash %{ $result = $1.Value(); %} 			// cpp to c
%typemap(csin)   StringHash "$csinput.Hash"							// convert C# to pinvoke
%typemap(csout, excode=SWIGEXCODE) StringHash {						// convert pinvoke to C#
	//csout
    var ret = new StringHash($imcall);$excode
    return ret;
  }
%typemap(csdirectorin) StringHash "$iminput /*csdirectorin*/"
%typemap(csdirectorout) StringHash "$cscall /*csdirectorout*/"

%define URHO3D_BINARY_COMPATIBLE_TYPE(CPP_TYPE)
	%typemap(ctype)  CPP_TYPE& "Urho3D::CPP_TYPE*"					// c layer type
	%typemap(imtype) CPP_TYPE& "ref global::Urh3DNet.CPP_TYPE"		// pinvoke type
	%typemap(cstype) CPP_TYPE& "global::Urh3DNet.CPP_TYPE"			// c# type
	%typemap(in)     CPP_TYPE& %{ $1 = $input; %}					// c to cpp
	%typemap(out)    CPP_TYPE& %{ $result = $1; %} 					// cpp to c
	%typemap(csin)   CPP_TYPE& "ref $csinput"						// convert C# to pinvoke
	%typemap(csout, excode=SWIGEXCODE) CPP_TYPE& {					// convert pinvoke to C#
	    var ret = $imcall;$excode
	    return ret;
	  }
%enddef

URHO3D_BINARY_COMPATIBLE_TYPE(Color);
URHO3D_BINARY_COMPATIBLE_TYPE(Rect);
URHO3D_BINARY_COMPATIBLE_TYPE(IntRect);
URHO3D_BINARY_COMPATIBLE_TYPE(Vector2);
URHO3D_BINARY_COMPATIBLE_TYPE(IntVector2);
URHO3D_BINARY_COMPATIBLE_TYPE(Vector3);
URHO3D_BINARY_COMPATIBLE_TYPE(IntVector3);
URHO3D_BINARY_COMPATIBLE_TYPE(Vector4);
URHO3D_BINARY_COMPATIBLE_TYPE(IntVector4);
URHO3D_BINARY_COMPATIBLE_TYPE(Matrix3);
URHO3D_BINARY_COMPATIBLE_TYPE(Matrix3x4);
URHO3D_BINARY_COMPATIBLE_TYPE(Matrix4);
URHO3D_BINARY_COMPATIBLE_TYPE(Quaternion);


%rename ("$ignore", fullname=1) Urho3D::SharedPtr::SharedPtr(std::nullptr_t);
%rename ("$ignore", fullname=1) Urho3D::WeakPtr::SharedPtr(std::nullptr_t);
%rename ("$ignore", fullname=1) Urho3D::UniquePtr::SharedPtr(std::nullptr_t);

%include "Urho3D/Container/RefCounted.h"
%include "Urho3D/Container/Ptr.h"

%ignore Urho3D::Intersection;
%ignore Urho3D::CustomVariantValue;
%ignore Urho3D::CustomVariantValueTraits;
%ignore Urho3D::CustomVariantValueImpl;
%ignore Urho3D::MakeCustomValue;
%ignore Urho3D::VariantValue;
%include "Urho3D/Core/Variant.h"

%ignore Urho3D::EventHandler;
%ignore Urho3D::EventHandlerImpl;
%ignore Urho3D::EventHandler11Impl;
%ignore Urho3D::ObjectFactory;
%include "Urho3D/Core/Object.h"
%feature("director") Urho3D::Object;
URHO3D_SHARED_PTR(Object)
