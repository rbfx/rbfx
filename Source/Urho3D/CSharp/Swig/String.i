// Based on swig's std_string.i

%pragma(csharp) modulecode=%{
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    public struct Urho3DString
    {
        public int length;
        public int capacity;
        public System.IntPtr buffer;

        public void SetString(string value)
        {
            SetString(this, value);
        }

        [System.Runtime.InteropServices.DllImport("$dllimport", EntryPoint="CSharp_Urho3D_String_Set")]
        private static extern void SetString([param: System.Runtime.InteropServices.MarshalAs(
            System.Runtime.InteropServices.UnmanagedType.LPStruct)]Urho3DString str,
            [param: System.Runtime.InteropServices.MarshalAs(
                System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]string val);
    }

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

namespace Urho3D {

class String;
// Helpers
%csexposefunc(runtime, CreateString, const char*, const char*) %{
    [return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]
    private static string CreateString([param: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]string str)
    {
        return str;
    }
    internal delegate string CreateStringDelegate(string str);
}%}

%wrapper %{
    SWIGEXPORT void SWIGSTDCALL CSharp_Urho3D_String_Set(eastl::string* str, const char* val) { *str = val; }
%}

// String
%typemap(ctype, out="void*")   String, const String &, String & "char *"
%typemap(imtype)               String, const String &, String & "System.IntPtr"
%typemap(cstype)               String, const String &           "string"
%typemap(cstype, out="string")                         String & "ref string"

%typemap(typecheck) String, const String &, String & = char *;

// Parameters
%typemap(in, canthrow=1) String %{
    if (!$input) {
        SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
        return $null;
    }
    $1 = $input;
%}
%typemap(in, canthrow=1) const String & %{
    if (!$input) {
        SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
        return $null;
    }
    $*1_ltype $inputRef($input);
    $1 = &$inputRef;
%}
%typemap(in, canthrow=1) String & %{
    if (!$input) {
        SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
        return $null;
    }
    $*1_ltype $inputRef(*(const char**)$input);
    $1 = &$inputRef;
%}
%typemap(argout)       String & %{ *(const char**)$input = Urho3D_CSharpCreateString($inputRef.CString()); %}
%typemap(argout) const String & ""
%typemap(csin, pre="
    var $csinputBytes = System.Text.Encoding.UTF8.GetBytes($csinput);
    unsafe {
        fixed (byte* $csinputPtr = $csinputBytes) {
    ",
    terminator = "
        }
    }
") String, const String & "(System.IntPtr)$csinputPtr"
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
") String & "(System.IntPtr)$csinputPtr"
%typemap(directorin)   String & "$input = (void*)addr($1);"
%typemap(directorin)   const String &, String %{ $input = (void*)Urho3D_CSharpCreateString(addr($1)->CString()); %}
%typemap(csdirectorin) const String &, String %{ $module.ToString($iminput) %}
%typemap(csdirectorin, pre="
    unsafe {
        var $iminputStr = ($module.Urho3DString*)$iminput;
        var $iminputRef = $module.ToString((byte*)$iminputStr->buffer);
", post="
        $iminputStr->SetString($iminputRef);
    }
") String &     "ref $iminputRef"


// Returns
%typemap(out)                      const String &, String &, String %{ $result = (void*)Urho3D_CSharpCreateString(addr($1)->CString()); %}
%typemap(csout, excode=SWIGEXCODE) const String &, String &, String  {
    unsafe {
        var str = $imcall;$excode
        return $module.ToString(str);
    }
}
%typemap(csdirectorout)           String, const String &, String & %{ Urho3DNet.Urho3D.strdup_string($cscall) %}
%typemap(directorout, canthrow=1) String %{
    if (!$input) {
        SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
        return $null;
    }
    $result = $input;
    free((void*)$input); // strdup'ed in csdirectorout
%}
%typemap(directorout, canthrow=1) const String &, String & %{
    if (!$input) {
        SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
        return $null;
    }
    static thread_local $*1_ltype $inputStatic($input);
    $result = &$inputStatic;
    free((void*)$input); // strdup'ed in csdirectorout
%}

// Variables
%typemap(csvarin, excode=SWIGEXCODE2) String &, String * %{
    set {
        unsafe {
            fixed (byte* $csinputPtr = System.Text.Encoding.UTF8.GetBytes($csinput)) {
                $imcall;
            }
        }
        $excode
    }
%}

%typemap(csvarout, excode=SWIGEXCODE2) String &, String * %{
    get {
        unsafe {
            var str = $imcall;$excode
            return $module.ToString(str);
        }
    }
%}


// Ptr types
%apply       String & {       String * };
%apply const String & { const String * };

};

%typemap(ctype)   const char* INPUT[] "char**"
%typemap(cstype)  const char* INPUT[] "string[]"
%typemap(imtype,
    inattributes="[global::System.Runtime.InteropServices.In, global::System.Runtime.InteropServices.Out, global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]",
    outattributes="[return: global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]") const char* INPUT[] "string[]"
%typemap(csin)    const char* INPUT[] "$csinput"
%typemap(csvarin) const char* INPUT[] %{
    set {
        $imcall;$excode
    }
%}
%typemap(in)      const char* INPUT[] "$1 = $input;"
%typemap(out)     const char* INPUT[] "$result = $1;"
%typemap(csvarout, excode=SWIGEXCODE2) const char* INPUT[] %{
    get {
        var res = $imcall;$excode
        return res;
    }
%}
%typemap(freearg) const char* INPUT[] ""
%typemap(argout)  const char* INPUT[] ""
