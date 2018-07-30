// Various marshalling helpers

%ignore SafeArray;
%inline %{
    struct SafeArray
    {
        void* data;
        int length;
    };
%}

%pragma(csharp) modulecode=%{
    [global::System.Runtime.InteropServices.StructLayout(global::System.Runtime.InteropServices.LayoutKind.Sequential)]
    internal struct SafeArray
    {
        public global::System.IntPtr data;
        public int length;

        public SafeArray(global::System.IntPtr d, int l)
        {
            data = d;
            length = l;
        }
    }

    internal static global::System.IntPtr strdup_string(string str)
    {
        var res = global::System.Text.Encoding.UTF8.GetBytes(str);
        unsafe {
            fixed (byte* p_res = res) {
                return strdup((global::System.IntPtr)p_res);
            }
        }
    }
%}

%typemap(ctype)  char* strdup "char*"
%typemap(imtype) char* strdup "global::System.IntPtr"
%typemap(cstype) char* strdup "global::System.IntPtr"
%typemap(in)     char* strdup "$1 = $input;"
%typemap(out)    char* strdup "$result = $1;"
%typemap(csin)   char* strdup "$csinput"
%typemap(csout, excode=SWIGEXCODE) char* strdup {
    var ret = $imcall;$excode
    return ret;
  }
%apply char* strdup { const char* void_ptr_string }

void free(void* ptr);
void* malloc(int size);
char* strdup(const char* void_ptr_string);
int strlen(const char* void_ptr_string);


%define CS_CONSTANT(fqn, name, value)
    %csconst(1) name;
    #define name value
    %ignore fqn;
%enddef
