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

%wrapper %{
    SWIGEXPORT void SWIGSTDCALL CSharp_Urho3D_String_Set(eastl::string* str, const char* val) { *str = val; }
    SWIGEXPORT const char* SWIGSTDCALL CSharp_EaStl_String_Get(eastl::string* str) { return str->data(); }
%}

%pragma(csharp) modulecode=%{
    [System.Runtime.InteropServices.DllImport($dllimport, EntryPoint="CSharp_Urho3D_String_Set")]
    internal static extern void SetString(global::System.IntPtr str,
        [param: System.Runtime.InteropServices.MarshalAs(global::Urho3DNet.Urho3DPINVOKE.LPStr)]
        string val);

    [System.Runtime.InteropServices.DllImport($dllimport, EntryPoint="CSharp_Urho3D_String_Get")]
    [return: System.Runtime.InteropServices.MarshalAs(global::Urho3DNet.Urho3DPINVOKE.LPStr)]
    internal static extern string GetString(global::System.IntPtr str);

    public static unsafe byte[] mstrdup(string str)
    {
        int len = System.Text.Encoding.UTF8.GetByteCount(str);
        var ptr = new byte[len + 1];
        fixed(byte* pBytes = ptr)
        {
            fixed (char* pChar = str)
            {
                System.Text.Encoding.UTF8.GetBytes(pChar, str.Length, pBytes, len);
            }
            pBytes[len] = 0;
        }
        return ptr;
    }

    public static unsafe System.IntPtr strdup(string str)
    {
        int len = System.Text.Encoding.UTF8.GetByteCount(str);
        var ptr = $module.malloc(len + 1);
        byte* pBytes = (byte*)ptr;
        fixed (char* pChar = str)
        {
            System.Text.Encoding.UTF8.GetBytes(pChar, str.Length, pBytes, len);
        }
        pBytes[len] = 0;
        return ptr;
    }

    public static unsafe string ToString(byte* str)
    {
        return System.Text.Encoding.UTF8.GetString(str, $module.strlen((System.IntPtr)str));
    }

    public static unsafe string ToString(System.IntPtr str)
    {
        return System.Text.Encoding.UTF8.GetString((byte*)str, $module.strlen(str));
    }

    public static string ToString(string str)
    {
        return str;
    }
%}

%{
#include <string>
#include <string_view>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
%}

namespace std {

%naturalvar string;
class string;

// string
%typemap(ctype) string "char *"
#if defined(UWP)
// UWP does not support UnmanagedType.CustomMarshaler
%typemap(imtype) string "string"
#else
%typemap(imtype,
         inattributes="[global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof($imclassname.SWIGUTF8StringMarshaller))]",
         outattributes="[return: global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof($imclassname.SWIGUTF8StringMarshaller))]",
         directorinattributes="[global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof($imclassname.SWIGUTF8StringMarshaller))]",
         directoroutattributes="[return: global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof($imclassname.SWIGUTF8StringMarshaller))]"
               ) string "string"
#endif
%typemap(cstype) string "string"

%typemap(typecheck, precedence=SWIG_TYPECHECK_POINTER, equivalent="const char*") string ""

%typemap(csdirectorin) string "$iminput"
%typemap(csdirectorout) string "$cscall"

%typemap(in, canthrow=1) string
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $1 = {$input}; %}
%typemap(out) string %{ $result = SWIG_csharp_string_callback($1.data()); %}

%typemap(directorout, canthrow=1) string
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $result = {$input}; %}

%typemap(directorin) string %{ $input = SWIG_csharp_string_callback($1.data()); %}

%typemap(csin) string "$csinput"
%typemap(csout, excode=SWIGEXCODE) string {
    string ret = $imcall;$excode
    return ret;
  }

%typemap(typecheck) string = char *;

%typemap(throws, canthrow=1) string
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.data());
   return $null; %}

// const string &
%typemap(ctype) const string & "char *"
#if defined(UWP)
%typemap(imtype) const string & "string"
#else
%typemap(imtype,
         inattributes="[global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof($imclassname.SWIGUTF8StringMarshaller))]",
         outattributes="[return: global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof($imclassname.SWIGUTF8StringMarshaller))]",
         directorinattributes="[global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof($imclassname.SWIGUTF8StringMarshaller))]",
         directoroutattributes="[return: global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.CustomMarshaler, MarshalTypeRef=typeof($imclassname.SWIGUTF8StringMarshaller))]"
               ) const string & "string"
#endif
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
%typemap(out) const string & %{ $result = SWIG_csharp_string_callback($1->data()); %}

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

%typemap(directorin) const string & %{ $input = SWIG_csharp_string_callback($1.data()); %}

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
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.data());
   return $null; %}

// string&
%apply const string & { string & };
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
%typemap(argout) string& %{ *(const char**)$input = SWIG_csharp_string_callback($input_str.data()); %}
%typemap(argout) const string&, string ""
%typemap(csin, pre="
    var $csinputBytes = Urho3DNet.Urho3D.mstrdup($csinput);
    fixed (byte* $csinputBytesPtr = $csinputBytes) {
        System.IntPtr $csinputPtr = new System.IntPtr(&$csinputBytesPtr);
        try {
    ",
    terminator = "
        } finally {
            $csinput = Urho3DNet.Urho3D.ToString(*(byte**)$csinputPtr);
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
%typemap(csvarin, excode=SWIGEXCODE2) string& %{
    set {
        unsafe {
            System.IntPtr $csinputStr = Urho3DNet.Urho3D.strdup($csinput);
            System.IntPtr $csinputPtr = new System.IntPtr(&$csinputStr);
            try {
                $imcall;$excode
            } finally {
                $module.free($csinputStr);
            }
        }
    }
%}

%typemap(csvarout, excode=SWIGEXCODE2) string& %{
    get {
        System.IntPtr str = System.IntPtr.Zero;
        try {
            str = $imcall;$excode
            return Urho3DNet.Urho3D.ToString(str);
        } finally {
            if (str != System.IntPtr.Zero)
                $module.free(str);
        }
    }
%}

}   // namespace std

// Ptr types
%apply       std::string& {       std::string* };
%apply const std::string& { const std::string* };

%apply       std::string  {       std::string_view  }
%apply const std::string& { const std::string_view& }

namespace std {

%naturalvar string;
class string_view;

// string_view
%apply string { string_view };
%typemap(out) string_view %{ $result = SWIG_csharp_string_callback($1.data(), $1.size()); %}
%typemap(in, canthrow=1) string_view
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $1_ltype $input_view((const char*)$input);
   $1.swap($input_view); %}
%typemap(out) string_view %{ $result = SWIG_csharp_string_callback($1.data(), $1.size()); %}

%typemap(directorin) string_view %{ $input = SWIG_csharp_string_callback($1.data(), $1.size()); %}
%typemap(directorout, canthrow=1) string_view
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $1_ltype $input_view((const char*)$input);
   $result.swap($input_view); %}


%typemap(typecheck)       string_view  = char *;
%typemap(typecheck) const string_view& = char *;

}   // namespace std

namespace eastl {

%naturalvar string;
%naturalvar string_view;
class string;
class string_view;

}   // namespace eastl

%apply       std::string       {       eastl::string       }
%apply       std::string&      {       eastl::string&      }
%apply const std::string&      { const eastl::string&      }
%apply       std::string*      {       eastl::string*      }
%apply const std::string*      { const eastl::string*      }
%apply       std::string_view  {       eastl::string_view  }
%apply const std::string_view& { const eastl::string_view& }
