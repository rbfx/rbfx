
// Types that are binary-compatible
%define URHO3D_BINARY_COMPATIBLE_TYPE(CPP_TYPE)
  %csexposefunc(wrapper, Create##CPP_TYPE, Urho3D::CPP_TYPE*, Urho3D::CPP_TYPE*) %{
    [return: global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPStruct)]
    private static CPP_TYPE Create##CPP_TYPE([param: global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPStruct)]CPP_TYPE primitive)
    {
      return primitive;
    }
    private static System.Delegate Create##CPP_TYPE##DelegateInstance = new System.Func<CPP_TYPE, CPP_TYPE>(Create##CPP_TYPE);
  }%}

  struct CPP_TYPE;

  %typemap(ctype)  CPP_TYPE  "Urho3D::CPP_TYPE*"                    // c layer type
  %typemap(imtype) CPP_TYPE  "global::System.IntPtr"                // pinvoke type
  %typemap(cstype) CPP_TYPE  "global::Urho3DNet.CPP_TYPE"           // c# type
  %typemap(in)     CPP_TYPE  %{ $1 = *$input; /*3*/ %}                    // c to cpp
  %typemap(out)    CPP_TYPE  %{ $result = SWIG_CSharpUrho3DCreate##CPP_TYPE(&$1); /*1*/ %} // cpp to c
  %typemap(csin, pre=         "    unsafe {\n"
                              "      $typemap(cstype, Urho3D::CPP_TYPE)* swig_ptrTo_$csinput_bytes = &$csinput;\n",
                 terminator = "    }\n") 
                   CPP_TYPE "(global::System.IntPtr)swig_ptrTo_$csinput_bytes"

  %typemap(csout, excode=SWIGEXCODE) CPP_TYPE {                   // convert pinvoke to C#
    var ret = $imcall;$excode
    return global::System.Runtime.InteropServices.Marshal.PtrToStructure<$typemap(cstype, Urho3D::CPP_TYPE)>(ret);
  }

  %apply                 CPP_TYPE     { CPP_TYPE & }
  %typemap(in)           CPP_TYPE &  %{ $1 = $input; /*2*/ %}                                      // c to cpp
  %typemap(out)          CPP_TYPE &  %{ $result = SWIG_CSharpUrho3DCreate##CPP_TYPE($1); %}  // cpp to c

  %typemap(csvarin, excode=SWIGEXCODE2) CPP_TYPE & %{
    set {
      unsafe {
        $typemap(cstype, Urho3D::CPP_TYPE)* swig_ptrTo_$csinput_bytes = &value;
        $imcall;$excode
      }
    }
  %}
  %typemap(csvarout, excode=SWIGEXCODE2) CPP_TYPE & %{
    get {
      var ret = $imcall;$excode
      return global::System.Runtime.InteropServices.Marshal.PtrToStructure<$typemap(cstype, Urho3D::CPP_TYPE)>(ret);
    }
  %}

  %typemap(cstype) CPP_TYPE & "ref global::Urho3DNet.CPP_TYPE"           // c# type
  %typemap(csin, pre=         "    unsafe {\n"
                              "      fixed ($typemap(cstype, Urho3D::CPP_TYPE)* swig_ptrTo_$csinput_bytes = &$csinput) {\n",
                 terminator = "      }\n"
                              "    }\n") 
                 CPP_TYPE&    "(global::System.IntPtr)swig_ptrTo_$csinput_bytes"

  %apply CPP_TYPE& { CPP_TYPE* }

%enddef

namespace Urho3D
{
  URHO3D_BINARY_COMPATIBLE_TYPE(Color);
  URHO3D_BINARY_COMPATIBLE_TYPE(Rect);
  URHO3D_BINARY_COMPATIBLE_TYPE(IntRect);
  URHO3D_BINARY_COMPATIBLE_TYPE(Vector2);
  URHO3D_BINARY_COMPATIBLE_TYPE(IntVector2);
  URHO3D_BINARY_COMPATIBLE_TYPE(Vector3);
  URHO3D_BINARY_COMPATIBLE_TYPE(IntVector3);
  URHO3D_BINARY_COMPATIBLE_TYPE(Vector4);
  URHO3D_BINARY_COMPATIBLE_TYPE(Matrix3);
  URHO3D_BINARY_COMPATIBLE_TYPE(Matrix3x4);
  URHO3D_BINARY_COMPATIBLE_TYPE(Matrix4);
  URHO3D_BINARY_COMPATIBLE_TYPE(Quaternion);
  URHO3D_BINARY_COMPATIBLE_TYPE(Plane);
  URHO3D_BINARY_COMPATIBLE_TYPE(BoundingBox);
}

// These should be implemented in C# anyway.
%ignore Urho3D::Frustum::planes_;
%ignore Urho3D::Frustum::vertices_;
%ignore Urho3D::Polyhedron::Polyhedron(const Vector<PODVector<Vector3> >& faces);
%ignore Urho3D::Polyhedron::faces_;


%apply float *INOUT        { float& sin, float& cos, float& accumulator };
%apply unsigned int* INOUT { unsigned int* randomRef, unsigned int* nearestRef }

%ignore Urho3D::M_MIN_INT;
%ignore Urho3D::M_MAX_INT;
%ignore Urho3D::M_MIN_UNSIGNED;
%ignore Urho3D::M_MAX_UNSIGNED;

%include "Urho3D/Math/MathDefs.h"
%include "Urho3D/Math/Ray.h"
%include "Urho3D/Math/Frustum.h"
%include "Urho3D/Math/Sphere.h"
%include "Urho3D/Math/Polyhedron.h"
