%include "Helpers.i"

// Types that are binary-compatible
%define URHO3D_BINARY_COMPATIBLE_TYPE_EX(CS_TYPE, CPP_TYPE, POD_TYPE)
    %ignore CPP_TYPE;
    struct CPP_TYPE;

    %typemap(ctype, out="POD_TYPE") CPP_TYPE, const CPP_TYPE &, CPP_TYPE & "CPP_TYPE *"
    %typemap(imtype, out="CS_TYPE")
                                   CPP_TYPE, const CPP_TYPE &, CPP_TYPE & "ref CS_TYPE"
    %typemap(cstype)               CPP_TYPE, const CPP_TYPE &             "CS_TYPE"
    %typemap(cstype, out="CS_TYPE")                            CPP_TYPE & "ref CS_TYPE"

    // Parameters
    %typemap(in, canthrow=1) CPP_TYPE %{
        if (!$input) {
            SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null", 0);
            return $null;
        }
        $1 = *$input;
    %}
    %typemap(in, canthrow=1) CPP_TYPE & %{
        if (!$input) {
            SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null", 0);
            return $null;
        }
        $*1_ltype $inputRef(*$input);
        $1 = &$inputRef;
    %}
    %typemap(argout)       CPP_TYPE & %{ *$input = $inputRef; %}
    %typemap(argout)                 const CPP_TYPE & ""
    %typemap(csin)         CPP_TYPE, const CPP_TYPE &, CPP_TYPE & "ref $csinput"
    %typemap(directorin)   CPP_TYPE & "$input = addr($1);"
    %typemap(directorin)   const CPP_TYPE &, CPP_TYPE %{ $input = addr($1); %}
    %typemap(csdirectorin) const CPP_TYPE &, CPP_TYPE "$iminput"
    %typemap(csdirectorin) CPP_TYPE &                 "ref $iminput"


    // Returns
    %typemap(out, null="POD_TYPE{}")   const CPP_TYPE &, CPP_TYPE &, CPP_TYPE %{ $result = pod::convert<CPP_TYPE, POD_TYPE>(deref($1)); %}
    %typemap(csout, excode=SWIGEXCODE) const CPP_TYPE &, CPP_TYPE &, CPP_TYPE  {
        unsafe {
            var res = $imcall;$excode
            return res;
        }
    }
    %typemap(csdirectorout)           CPP_TYPE, const CPP_TYPE &, CPP_TYPE & "$cscall"
    %typemap(directorout, canthrow=1) CPP_TYPE %{
        if (!$input) {
            SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null", 0);
            return $null;
        }
        $result = *$input;
    %}
    %typemap(directorout, canthrow=1) const CPP_TYPE &, CPP_TYPE & %{
        if (!$input) {
            SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null", 0);
            return $null;
        }
        static thread_local CPP_TYPE $inputStatic(*$input);
        $result = &$inputStatic;
    %}

    // Variables
    %typemap(csvarin, excode=SWIGEXCODE2) CPP_TYPE &, CPP_TYPE * %{
        set {
            $imcall;$excode
        }
    %}

    %typemap(csvarout, excode=SWIGEXCODE2) CPP_TYPE &, CPP_TYPE * %{
        get {
            var res = $imcall;$excode
            return res;
        }
    %}

    // Ptr types
    %apply       CPP_TYPE & {       CPP_TYPE * };
    %apply const CPP_TYPE & { const CPP_TYPE * };
%enddef

%define URHO3D_BINARY_COMPATIBLE_TYPE(TYPE, POD_TYPE)
    URHO3D_BINARY_COMPATIBLE_TYPE_EX(TYPE, TYPE, POD_TYPE);
%enddef

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
URHO3D_BINARY_COMPATIBLE_TYPE_EX(Vector4, ImVec4, pod::float4);
URHO3D_BINARY_COMPATIBLE_TYPE_EX(Vector4, btVector3, pod::float4);  // Bullet stores 4 floats despite the name.
URHO3D_BINARY_COMPATIBLE_TYPE_EX(Rect, ImRect, pod::float4);
