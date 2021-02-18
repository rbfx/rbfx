/* -----------------------------------------------------------------------------
 * copied from std_string.i from SWIG
 *
 * Typemaps for std::string and const std::string&
 * These are mapped to a C# String and are passed around by value.
 *
 * To use non-const std::string references use the following %apply.  Note
 * that they are passed by value.
 * %apply const std::string & {std::string &};
 * ----------------------------------------------------------------------------- */

%{
#include <EASTL/string.h>
%}

namespace eastl {

%naturalvar string;
%naturalvar string_view;

class string;
class string_view;

// string
%typemap(ctype) string "char *"
%typemap(imtype) string "string"
%typemap(cstype) string "string"

%typemap(csdirectorin) string "$iminput"
%typemap(csdirectorout) string "$cscall"

%typemap(in, canthrow=1) string
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $1.assign($input); %}
%typemap(out) string %{ $result = SWIG_csharp_string_callback($1.c_str()); %}

%typemap(directorout, canthrow=1) string
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $result.assign($input); %}

%typemap(directorin) string %{ $input = SWIG_csharp_string_callback($1.c_str()); %}

%typemap(csin) string "$csinput"
%typemap(csout, excode=SWIGEXCODE) string {
    string ret = $imcall;$excode
    return ret;
  }

%typemap(typecheck) string = char *;

%typemap(throws, canthrow=1) string
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.c_str());
   return $null; %}

// const string &
%typemap(ctype) const string & "char *"
%typemap(imtype) const string & "string"
%typemap(cstype) const string & "string"

%typemap(csdirectorin) const string & "$iminput"
%typemap(csdirectorout) const string & "$cscall"

%typemap(in, canthrow=1) const string &
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $*1_ltype $1_str($input);
   $1 = &$1_str; %}
%typemap(out) const string & %{ $result = SWIG_csharp_string_callback($1->c_str()); %}

%typemap(csin) const string & "$csinput"
%typemap(csout, excode=SWIGEXCODE) const string & {
    string ret = $imcall;$excode
    return ret;
  }

%typemap(directorout, canthrow=1, warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const string &
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   /* possible thread/reentrant code problem */
   static $*1_ltype $1_str;
   $1_str = $input;
   $result = &$1_str; %}

%typemap(directorin) const string & %{ $input = SWIG_csharp_string_callback($1.c_str()); %}

%typemap(csvarin, excode=SWIGEXCODE2) const string & %{
    set {
      $imcall;$excode
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) const string & %{
    get {
      string ret = $imcall;$excode
      return ret;
    } %}

%typemap(typecheck) const string & = char *;

%typemap(throws, canthrow=1) const string &
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.c_str());
   return $null; %}



// string&

// Helpers
%wrapper %{
    SWIGEXPORT void SWIGSTDCALL CSharp_Urho3D_String_Set(eastl::string* str, const char* val) { *str = val; }
    SWIGEXPORT const char* SWIGSTDCALL CSharp_EaStl_String_Get(eastl::string* str) { return str->c_str(); }
%}

%pragma(csharp) modulecode=%{
    [System.Runtime.InteropServices.DllImport($dllimport, EntryPoint="CSharp_Urho3D_String_Set")]
    internal static extern void SetString(global::System.IntPtr str,
        [param: System.Runtime.InteropServices.MarshalAs(global::Urho3DNet.Urho3DPINVOKE.LPStr)]
        string val);

    [System.Runtime.InteropServices.DllImport($dllimport, EntryPoint="CSharp_Urho3D_String_Get")]
    [return: System.Runtime.InteropServices.MarshalAs(global::Urho3DNet.Urho3DPINVOKE.LPStr)]
    internal static extern string GetString(global::System.IntPtr str);

    internal static System.IntPtr strdup_string(string str)
    {
        var res = System.Text.Encoding.UTF8.GetBytes(str);
        unsafe {
            fixed (byte* p_res = res) {
                return strdup((System.IntPtr)p_res);
            }
        }
    }

    internal static unsafe string ToString(byte* str)
    {
        return System.Text.Encoding.UTF8.GetString(str, Urho3DNet.Urho3D.strlen((System.IntPtr)str));
    }

    internal static unsafe string ToString(System.IntPtr str)
    {
        return System.Text.Encoding.UTF8.GetString((byte*)str, Urho3DNet.Urho3D.strlen(str));
    }
%}


%typemap(ctype, out="void*")   string& "char *"
%typemap(imtype)               string& "System.IntPtr"
%typemap(cstype, out="string") string& "ref string"
%typemap(in, canthrow=1) string& %{
    if (!$input) {
        SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
        return $null;
    }
    const char* p_$input = *(const char**)$input;
    $*1_ltype $input_str(p_$input ? p_$input : "");
    $1 = &$input_str;
%}
%typemap(argout) string& %{ *(const char**)$input = SWIG_csharp_string_callback($input_str.c_str()); %}
%typemap(argout) const string&, string ""
%typemap(csin, pre="
    var $csinputBytes = System.Text.Encoding.UTF8.GetBytes($csinput);
    unsafe {
        fixed (byte* $csinputBytesPtr = $csinputBytes) {
            System.IntPtr $csinputPtr = new System.IntPtr(&$csinputBytesPtr);
            try {
    ",
    terminator = "
            } finally {
                $csinput = $module.ToString($csinputBytesPtr);
            }
        }
    }
") string& "(System.IntPtr)$csinputPtr"
%typemap(directorin)   string& "$input = (void*)addr($1);"
%typemap(csdirectorin, pre="
    unsafe {
        var $iminputRef = $module.GetString($iminput);
", post="
        $module.SetString($iminput, $iminputRef);
    }
") string&     "ref $iminputRef"

// Variables
%typemap(csvarin, excode=SWIGEXCODE2) string&, string * %{
    set {
        unsafe {
            fixed (byte* $csinputPtr = System.Text.Encoding.UTF8.GetBytes($csinput)) {
                $imcall;
            }
        }
        $excode
    }
%}

%typemap(csvarout, excode=SWIGEXCODE2) string&, string * %{
    get {
        unsafe {
            var str = $imcall;$excode
            return $module.ToString(str);
        }
    }
%}


// Ptr types
%apply       string& {       string* };
%apply const string& { const string* };

// string_view
%apply string { string_view };
%typemap(out) string_view %{ $result = SWIG_csharp_string_callback($1.data()); %}
%typemap(in, canthrow=1) string_view
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   eastl::string_view $input_view((const char*)$input);
   $1.swap($input_view); %}
%typemap(out) string %{ $result = SWIG_csharp_string_callback($1.c_str()); %}

%typemap(directorout, canthrow=1) string_view
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   eastl::string_view $input_view((const char*)$input);
   $result.swap($input_view); %}
}

