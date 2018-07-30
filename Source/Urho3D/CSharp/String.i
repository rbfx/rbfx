// Based on swig's std_string.i

namespace Urho3D {

%naturalvar String;

class String;

// String
%typemap(ctype) String "char *"
%typemap(imtype) String "global::System.IntPtr"
%typemap(cstype) String "string"

%typemap(csdirectorin) String "$iminput"
%typemap(csdirectorout) String "$cscall"

%typemap(in, canthrow=1) String 
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $1 = $input; %}
%typemap(out) String %{ $result = SWIG_csharp_string_callback($1.CString()); %}

%typemap(directorout, canthrow=1) String 
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $result = $input; %}

%typemap(directorin) String %{ $input = SWIG_csharp_string_callback($1.CString()); %}

%typemap(csin, pre=         "    var $csinput_bytes = global::System.Text.Encoding.UTF8.GetBytes($csinput);\n"
                            "    unsafe {fixed (byte* p_$csinput_bytes = $csinput_bytes) {",
               terminator = "    }}") 
               String "(global::System.IntPtr)p_$csinput_bytes"

%typemap(csout, excode=SWIGEXCODE) String {
    unsafe {
        var str = $imcall;$excode
        return global::System.Text.Encoding.UTF8.GetString((byte*)str, global::Urho3DNet.Urho3D.strlen(str));
    }
  }

%typemap(typecheck) String = char *;

%typemap(throws, canthrow=1) String
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.CString());
   return $null; %}

// const String &
%typemap(ctype) const String & "char *"
%typemap(imtype) const String & "global::System.IntPtr"
%typemap(cstype) const String & "string"

%typemap(csdirectorin) const String & "$iminput"
%typemap(csdirectorout) const String & "global::System.IntPtr.Zero"
  /*var res = global::System.Text.Encoding.UTF8.GetBytes($cscall);
  unsafe {
    fixed (byte* p_res = res) {
      return IntPtr.Zero; // TODO: fixme
    }
  }*/

%typemap(in, canthrow=1) const String &
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $*1_ltype $1_str($input);
   $1 = &$1_str; %}
%typemap(out) const String & %{ $result = SWIG_csharp_string_callback($1->CString()); %}

%typemap(csin, pre=         "    var $csinput_bytes = global::System.Text.Encoding.UTF8.GetBytes($csinput);\n"
                            "    unsafe {fixed (byte* p_$csinput_bytes = $csinput_bytes) {",
               terminator = "    }}") 
               const String & "(global::System.IntPtr)p_$csinput_bytes"

%typemap(csout, excode=SWIGEXCODE) const String& {
    unsafe {
        var str = $imcall;$excode
        return global::System.Text.Encoding.UTF8.GetString((byte*)str, global::Urho3DNet.Urho3D.strlen(str));
    }
  }

%typemap(directorout, canthrow=1, warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const String &
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   /* possible thread/reentrant code problem */
   static $*1_ltype $1_str;
   $1_str = $input;
   $result = &$1_str; %}

%typemap(directorin) const String & %{ $input = SWIG_csharp_string_callback($1.CString()); %}

%typemap(csvarin, excode=SWIGEXCODE2) const String& %{
    set {
      var $csinput_bytes = global::System.Text.Encoding.UTF8.GetBytes($csinput);
      unsafe {
        fixed (byte* p_$csinput_bytes = $csinput_bytes) {
          $imcall;
        }
      }
      $excode
    }
  %}

%typemap(csvarout, excode=SWIGEXCODE2) const String& %{
    get {
      unsafe {
          var str = $imcall;$excode
          return global::System.Text.Encoding.UTF8.GetString((byte*)str, global::Urho3DNet.Urho3D.strlen(str));
      }
    }
  %}

%typemap(typecheck) const String & = char *;

%typemap(throws, canthrow=1) const String &
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.CString());
   return $null; %}

%apply const String & {String &};

}
