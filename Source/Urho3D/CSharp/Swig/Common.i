#define final
#define static_assert(...)

%include "stl.i"
%include "stdint.i"
%include "typemaps.i"
%include "arrays_csharp.i"

%typemap(csvarout) void* VOID_INT_PTR %{
  get {
    var ret = $imcall;$excode
    return ret;
  }
%}

%apply void* VOID_INT_PTR {
	void*,
	signed char*,
	unsigned char*
}

%typemap(csvarin, excode=SWIGEXCODE2) void* VOID_INT_PTR %{
  set {
    $imcall;$excode
  }
%}

%apply void* { std::uintptr_t, uintptr_t };
%apply unsigned { time_t };
%apply float *INOUT        { float& sin, float& cos, float& accumulator };
%apply unsigned int* INOUT { unsigned int* randomRef, unsigned int* nearestRef }
%typemap(csvarout, excode=SWIGEXCODE2) float INOUT[] "get { var ret = $imcall;$excode return ret; }"
%apply float *OUTPUT   { float& out_r, float& out_g, float& out_b, float& out_u, float& out_v, float& out_w };
%apply double *INOUT   { double* v };
%apply bool *INOUT     { bool*, unsigned int* flags };
%apply int INOUT[]     { int*, int[ANY] };
%apply float INOUT[]   { float*, float[ANY] };
%apply unsigned char OUTPUT[] { unsigned char out_table[256] };
%apply unsigned char INPUT[]  { unsigned char const table[256], unsigned char *pixels };
%apply bool INOUT[]    { bool[5] };
%typemap(ctype)   const char* INPUT[] "char**"
%typemap(cstype)  const char* INPUT[] "string[]"
%typemap(imtype, inattributes="[global::System.Runtime.InteropServices.In, global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]") const char* INPUT[] "string[]"
%typemap(csin)    const char* INPUT[] "$csinput"
%typemap(in)      const char* INPUT[] "$1 = $input;"
%typemap(freearg) const char* INPUT[] ""
%typemap(argout)  const char* INPUT[] ""
%apply const char* INPUT[]   { char const *const items[] };

// ref global::System.IntPtr
%typemap(ctype, out="void *")                 void*& "void *"
%typemap(imtype, out="global::System.IntPtr") void*& "ref global::System.IntPtr"
%typemap(cstype, out="$csclassname")          void*& "ref global::System.IntPtr"
%typemap(csin)                                void*& "ref $csinput"
%typemap(in)                                  void*& %{ $1 = ($1_ltype)$input; %}
%typecheck(SWIG_TYPECHECK_CHAR_PTR)           void*& ""

// Speed boost
%pragma(csharp) imclassclassmodifiers="[System.Security.SuppressUnmanagedCodeSecurity]\ninternal class"
%pragma(csharp) moduleclassmodifiers="[System.Security.SuppressUnmanagedCodeSecurity]\npublic partial class"
%typemap(csclassmodifiers) SWIGTYPE "public partial class"

%include "cmalloc.i"
%include "swiginterface.i"
%include "attribute.i"

%include "Helpers.i"
%include "Operators.i"
%include "InstanceCache.i"

// --------------------------------------- Math ---------------------------------------
%include "Math.i"

namespace Urho3D
{

URHO3D_BINARY_COMPATIBLE_TYPE(Color, pod::float4);
URHO3D_BINARY_COMPATIBLE_TYPE(Rect, pod::float4);
URHO3D_BINARY_COMPATIBLE_TYPE(IntRect, pod::int4);
URHO3D_BINARY_COMPATIBLE_TYPE(Vector2, pod::float2);
URHO3D_BINARY_COMPATIBLE_TYPE(IntVector2, pod::int2);
URHO3D_BINARY_COMPATIBLE_TYPE(Vector3, pod::float3);
URHO3D_BINARY_COMPATIBLE_TYPE(IntVector3, pod::int3);
URHO3D_BINARY_COMPATIBLE_TYPE(Vector4, pod::float4);
URHO3D_BINARY_COMPATIBLE_TYPE(Matrix3, pod::float9);
URHO3D_BINARY_COMPATIBLE_TYPE(Matrix3x4, pod::float12);
URHO3D_BINARY_COMPATIBLE_TYPE(Matrix4, pod::float16);
URHO3D_BINARY_COMPATIBLE_TYPE(Quaternion, pod::float4);
URHO3D_BINARY_COMPATIBLE_TYPE(Plane, pod::float7);
URHO3D_BINARY_COMPATIBLE_TYPE(BoundingBox, pod::float8);
URHO3D_BINARY_COMPATIBLE_TYPE(Sphere, pod::float4);
//URHO3D_BINARY_COMPATIBLE_TYPE(Frustum);
URHO3D_BINARY_COMPATIBLE_TYPE(Ray, pod::float6);

}

URHO3D_BINARY_COMPATIBLE_TYPE_EX(Vector2, ImVec2, pod::float2);
URHO3D_BINARY_COMPATIBLE_TYPE_EX(Vector4, ImVec4, pod::float2);
URHO3D_BINARY_COMPATIBLE_TYPE_EX(Rect, ImRect, pod::float4);
