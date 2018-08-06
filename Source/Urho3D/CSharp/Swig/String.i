// Based on swig's std_string.i

namespace Urho3D {

%naturalvar String;

class String;

// String
%typemap(ctype)  String  "char *"
%typemap(imtype) String  "global::System.IntPtr"
%typemap(cstype) String  "string"

%typemap(csdirectorin)  String         "$iminput"
%typemap(csdirectorout) String         "global::Urho3DNet.Urho3D.strdup_string($cscall)"

%typemap(in, canthrow=1) String %{
  if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $1 = $input;
 %}

%typemap(out) String         %{ $result = SWIG_csharp_string_callback($1.CString()); %}

%typemap(directorout, canthrow=1) String %{
  if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
  }
  $result = $input;
%}

%typemap(directorin) String %{ $input = SWIG_csharp_string_callback($1.CString()); %}

%typemap(csin, pre=         "    var $csinput_bytes = global::System.Text.Encoding.UTF8.GetBytes($csinput);\n"
                            "    unsafe {\n      fixed (byte* p_$csinput_bytes = $csinput_bytes) {\n",
               terminator = "    }\n      }\n") 
               String "(global::System.IntPtr)p_$csinput_bytes"

%typemap(csout, excode=SWIGEXCODE) String {
    unsafe {
        var str = $imcall;$excode
        return global::System.Text.Encoding.UTF8.GetString((byte*)str, global::Urho3DNet.Urho3D.strlen(str));
    }
  }

%typemap(typecheck) String         = char *;

%typemap(throws, canthrow=1) String %{
  SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.CString());
  return $null;
%}

%apply String { const String &, String & };

// const String &

%typemap(typecheck) const String & = char *;

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

// References
%typemap(in, canthrow=1) const String & %{
  if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $*1_ltype $1_str($input);
   $1 = &$1_str;
 %}
%typemap(out) const String & %{ $result = SWIG_csharp_string_callback($1->CString()); %}
%typemap(directorout, canthrow=1, warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const String & %{
 if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   static thread_local $*1_ltype $1_str($input);
   free($input);  // strdup'ed in csdirectorout
   $result = &$1_str;
%}


// Output parameters

%typemap(ctype)  String& "char **"
%typemap(cstype) String& "ref string"

%typemap(in, canthrow=1) String & %{
  if (!$input || !*$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $*1_ltype $1_str(*$input);
   $1 = &$1_str;
 %}

%typemap(csin, pre=         "    var $csinput_bytes = global::System.Text.Encoding.UTF8.GetBytes($csinput);
                                 unsafe {
                                   global::System.IntPtr ref_$csinput = global::System.IntPtr.Zero;
                                   try {
                                     fixed (byte* p_$csinput_bytes = $csinput_bytes) {
                                       ref_$csinput = new global::System.IntPtr(&p_$csinput_bytes);
                            ",
               terminator = "        }
                                   } finally {
                                     $csinput = global::System.Text.Encoding.UTF8.GetString(*(byte**)ref_$csinput, global::Urho3DNet.Urho3D.strlen(new global::System.IntPtr(*(byte**)ref_$csinput)));
                                   }
                                 }
                            ")
               String &     "ref_$csinput"

%typemap(directorin) String& %{
  char* p_$input = SWIG_csharp_string_callback($1.CString());
  $input = &p_$input;
%}

}
