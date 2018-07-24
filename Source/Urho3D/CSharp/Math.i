
%ignore Urho3D::Intersection;   // Defined in C#

// StringHash
%typemap(ctype)  Urho3D::StringHash "unsigned"                                // c layer type
%typemap(imtype) Urho3D::StringHash "uint"                                    // pinvoke type
%typemap(cstype) Urho3D::StringHash "global::Urho3DNet.StringHash"            // c# type
%typemap(in)     Urho3D::StringHash %{ $1 = Urho3D::StringHash($input); %}    // c to cpp
%typemap(out)    Urho3D::StringHash %{ $result = $1.Value(); %}               // cpp to c
%typemap(csin)   Urho3D::StringHash "$csinput.Hash"                           // convert C# to pinvoke
%typemap(csout, excode=SWIGEXCODE) Urho3D::StringHash {                       // convert pinvoke to C#
    var ret = new StringHash($imcall);$excode
    return ret;
  }
%typemap(directorin)  Urho3D::StringHash "$input = $1.Value();"
%typemap(directorout)   Urho3D::StringHash "$result = StringHash($input);"
namespace Urho3D { class StringHash; }

%define CSHARP_VALUE_TYPE_HELPER(NAME, CSTYPE)
  %insert(runtime) %{
    /* Callback for returning NAME to C# without leaking memory */
    typedef void* (SWIGSTDCALL* SWIG_CSharp##NAME##HelperCallback)(const void*);
    static SWIG_CSharp##NAME##HelperCallback SWIG_csharp_##NAME##_callback = nullptr;
  %}

  %pragma(csharp) imclasscode=%{
    internal partial class SWIG##NAME##Helper {
      public delegate CSTYPE SWIG##NAME##Delegate(CSTYPE value);
      static SWIG##NAME##Delegate NAME##Delegate = new SWIG##NAME##Delegate(Create##NAME);
      [global::System.Runtime.InteropServices.DllImport("$dllimport", EntryPoint="SWIGRegister" + #NAME + "Callback_$module")]
      public static extern void SWIGRegister##NAME##Callback_$module(SWIG##NAME##Delegate NAME##Delegate);
      [return: global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPStruct)]
      static CSTYPE Create##NAME([param: global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPStruct)]CSTYPE value) { return value; }
      static SWIG##NAME##Helper() { SWIGRegister##NAME##Callback_$module(NAME##Delegate); }
    }
    static protected SWIG##NAME##Helper swig##NAME##Helper = new SWIG##NAME##Helper();
  %}

  %insert(runtime) %{
  #ifdef __cplusplus
  extern "C" 
  #endif
  SWIGEXPORT void SWIGSTDCALL SWIGRegister##NAME##Callback_$module(SWIG_CSharp##NAME##HelperCallback callback) {
    SWIG_csharp_##NAME##_callback = callback;
  }
  %}
%enddef

// Types that are binary-compatible
%define URHO3D_BINARY_COMPATIBLE_TYPE(CPP_TYPE)
  CSHARP_VALUE_TYPE_HELPER(CPP_TYPE, global::Urho3DNet.CPP_TYPE);

  %typemap(ctype)  CPP_TYPE "Urho3D::CPP_TYPE*"                    // c layer type
  %typemap(imtype) CPP_TYPE "global::System.IntPtr"                // pinvoke type
  %typemap(cstype) CPP_TYPE "global::Urho3DNet.CPP_TYPE"           // c# type
  %typemap(in)     CPP_TYPE %{ $1 = $input; %}                     // c to cpp
  %typemap(out)    CPP_TYPE %{ $result = (CPP_TYPE*)SWIG_csharp_##CPP_TYPE##_callback((const void*)&$1); %} // cpp to c
  %typemap(out)    CPP_TYPE& %{ $result = (CPP_TYPE*)SWIG_csharp_##CPP_TYPE##_callback((const void*)$1); %} // cpp to c
  %typemap(csin, pre=         "    unsafe {$typemap(cstype, CPP_TYPE)* swig_ptrTo_$csinput_bytes = &$csinput;",
                 terminator = "    }") 
                 CPP_TYPE& "(global::System.IntPtr)swig_ptrTo_$csinput_bytes"
  %typemap(csout, excode=SWIGEXCODE) CPP_TYPE {                   // convert pinvoke to C#
    var ret = $imcall;$excode
    return global::System.Runtime.InteropServices.Marshal.PtrToStructure<$typemap(cstype, CPP_TYPE)>(ret);
  }
  %apply CPP_TYPE { const CPP_TYPE&, CPP_TYPE& }
%enddef

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
