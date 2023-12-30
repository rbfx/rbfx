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

%insert(runtime) %{
static thread_local eastl::string gEaStringReturnValue;
static thread_local eastl::wstring gEaWStringReturnValue;
inline char* SWIG_csharp_string_callback(eastl::string&& value)
{
  std::swap(gEaStringReturnValue, value);
  return gEaStringReturnValue.data();
}
inline char* SWIG_csharp_string_callback(const eastl::string& value)
{
  gEaStringReturnValue = value;
  return gEaStringReturnValue.data();
}
inline wchar_t* SWIG_csharp_string_callback(eastl::wstring&& value)
{
  std::swap(gEaWStringReturnValue, value);
  return gEaWStringReturnValue.data();
}
inline wchar_t* SWIG_csharp_string_callback(const eastl::wstring& value)
{
  gEaWStringReturnValue = value;
  return gEaWStringReturnValue.data();
}
%}

namespace eastl {

%naturalvar string;
%naturalvar string_view;

class string;
class string_view;

// string
%typemap(ctype) string "const char*"
%typemap(imtype) string "System.IntPtr"
%typemap(cstype) string "string"

%typemap(csdirectorin) string "$iminput"
%typemap(csdirectorout) string "System.Runtime.InteropServices.Marshal.PtrToStringUTF8($cscall)"

%typemap(in, canthrow=1) string
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $1.assign($input); %}
%typemap(out) string %{ $result = SWIG_csharp_string_callback(std::move($1)); %}

%typemap(directorout, canthrow=1) string
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $result.assign($input); %}

%typemap(directorin) string %{ $input = SWIG_csharp_string_callback(std::move($1)); %}

%typemap(csin, pre="var p$csinput = System.Runtime.InteropServices.Marshal.StringToCoTaskMemUTF8($csinput);", post="System.Runtime.InteropServices.Marshal.FreeCoTaskMem(p$csinput);") string "p$csinput"
%typemap(csout, excode=SWIGEXCODE) string {
    string ret = System.Runtime.InteropServices.Marshal.PtrToStringUTF8($imcall);$excode
    return ret;
  }

%typemap(typecheck) string = char *;

%typemap(throws, canthrow=1) string
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.c_str());
   return $null; %}

// const string &
%typemap(ctype) const string & "const char *"
%typemap(imtype) const string & "System.IntPtr"
%typemap(cstype) const string & "string"

%typemap(csdirectorin) const string & "System.Runtime.InteropServices.Marshal.PtrToStringUTF8($iminput)"
%typemap(csdirectorout) const string & "System.Runtime.InteropServices.Marshal.PtrToStringUTF8($cscall)"

%typemap(in, canthrow=1) const string &
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $*1_ltype $1_str($input);
   $1 = &$1_str; %}
%typemap(out) const string & %{ $result = $1->c_str(); %}
%typemap(csin, pre="var p$csinput = System.Runtime.InteropServices.Marshal.StringToCoTaskMemUTF8($csinput);", post="System.Runtime.InteropServices.Marshal.FreeCoTaskMem(p$csinput);") const string & "p$csinput"
%typemap(csout, excode=SWIGEXCODE) const string & {
    string ret = System.Runtime.InteropServices.Marshal.PtrToStringUTF8($imcall);$excode
    return ret;
  }

%typemap(directorout, canthrow=1, warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const string &
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   /* possible thread/reentrant code problem */
   gEaStringReturnValue = $input;
   $result = &gEaStringReturnValue; %}

%typemap(directorin) const string & %{ $input = $1.c_str(); %}

%typemap(csvarin, excode=SWIGEXCODE2) const string & %{
    set {
      var p$csinput = System.Runtime.InteropServices.Marshal.StringToCoTaskMemUTF8($csinput);
      $imcall;$excode
      System.Runtime.InteropServices.Marshal.FreeCoTaskMem(p$csinput);
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) const string & %{
    get {
      string ret = System.Runtime.InteropServices.Marshal.PtrToStringUTF8($imcall);$excode
      return ret;
    } %}

%typemap(typecheck) const string & = char *;

%typemap(throws, canthrow=1) const string &
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.c_str());
   return $null; %}



// string&

// Helpers
%{
    SWIGEXPORT void        SWIGSTDCALL SetString(eastl::string* str, const char* val) { *str = val; }
    SWIGEXPORT const char* SWIGSTDCALL GetString(eastl::string* str)                  { return str->c_str(); }
%}

%pragma(csharp) modulecode=%{
    /*[System.Runtime.InteropServices.DllImport($dllimport, EntryPoint="CSharp_Urho3D_String_Set")]
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
    }*/

    //internal static unsafe string ToString(byte* str)
    //{
    //    return System.Text.Encoding.UTF8.GetString(str, Urho3DNet.Urho3D.strlen((System.IntPtr)str));
    //}

    //internal static unsafe string ToString(System.IntPtr str)
    //{
    //    return System.Text.Encoding.UTF8.GetString((byte*)str, Urho3DNet.Urho3D.strlen(str));
    //}
%}


%typemap(ctype, out="void*")   string& "const char *"
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
                $csinput = System.Runtime.InteropServices.Marshal.PtrToStringUTF8($csinputPtr);
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
            return System.Runtime.InteropServices.Marshal.PtrToStringUTF8(str);
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
%typemap(out) string %{ $result = SWIG_csharp_string_callback(std::move($1)); %}

%typemap(directorout, canthrow=1) string_view
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   eastl::string_view $input_view((const char*)$input);
   $result.swap($input_view); %}
}

