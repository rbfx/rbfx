// Various marshalling helpers

%ignore SafeArray;
%inline %{
    struct SafeArray
    {
        void* data;
        int length;

        SafeArray() : data(nullptr), length(0) { }
        SafeArray(void* d, int len) : data(d), length(len) { }
        SafeArray(int){}
        void operator=(int)
        {
          data = nullptr;
          length = 0;
        }
    };
    
    // Helper for returning empty values
    #define _RET_NULL
    #define _RET_NULL0 {}
    #define RET_NULL(...) _RET_NULL##__VA_ARGS__
%}

%pragma(csharp) modulecode=%{
    [global::System.Runtime.InteropServices.StructLayout(global::System.Runtime.InteropServices.LayoutKind.Sequential)]
    public struct SafeArray
    {
        public global::System.IntPtr data;
        public int length;

        public SafeArray(global::System.IntPtr d, int l)
        {
            data = d;
            length = l;
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

%define %csattribute(Class, AttributeType, AttributeAccess, AttributeName, GetAccess, GetMethod, SetAccess, SetMethod)
  #if #GetMethod != #AttributeName
      %csmethodmodifiers Class::GetMethod "GetAccess";
      %typemap(cstype)   double Class::AttributeName "$typemap(cstype, AttributeType)"
      %typemap(csvarout) double Class::AttributeName "get { return GetMethod(); }"


      %{#define %mangle(Class) ##_## AttributeName ## _get(...) 0%}

      #if #SetMethod != "None"
        %csmethodmodifiers Class::SetMethod(AttributeType) "SetAccess";
        %typemap(csvarin) double Class::AttributeName "set { SetMethod(value); }"
        %{#define %mangle(Class) ##_## AttributeName ## _set(...)%}
      #else
        %immutable Class::AttributeName;
      #endif
      %extend Class {
        AttributeAccess:
          double AttributeName;
      }
  #endif
%enddef

%define %inheritable(NS, CTYPE)
  %director CTYPE;
  %wrapper %{
      $moduleDirectorTypes[SwigDirector_##CTYPE::GetTypeStatic()] = SwigDirector_##CTYPE::GetTypeInfoStatic();
      context->RegisterFactory<SwigDirector_##CTYPE>();%}
  %typemap(directorbody) NS::CTYPE %{
  public:
    using ClassName = SwigDirector_##CTYPE;
    using BaseClassName = NS::CTYPE;
    static Urho3D::StringHash GetTypeStatic() { return GetTypeInfoStatic()->GetType(); }
    static const Urho3D::String& GetTypeNameStatic() { return GetTypeInfoStatic()->GetTypeName(); }
    static const Urho3D::TypeInfo* GetTypeInfoStatic() { static const Urho3D::TypeInfo typeInfoStatic("SwigDirector_" #CTYPE, BaseClassName::GetTypeInfoStatic()); return &typeInfoStatic; }
  %}
%enddef

%define %csexposefunc(DEST, NAME, CRETURN, CPARAMS)
%DEST %{
  typedef CRETURN (SWIGSTDCALL* SWIG_CSharp$module##NAME##Callback)(CPARAMS);
  SWIG_CSharp$module##NAME##Callback SWIG_CSharp$module##NAME = nullptr;
  #ifdef __cplusplus
  extern "C"
  #endif
  SWIGEXPORT void SWIGSTDCALL SWIGRegister$module##NAME##Callback(SWIG_CSharp$module##NAME##Callback callback) {
    SWIG_CSharp$module##NAME = callback;
  }
%}

%pragma(csharp) imclasscode=%{
  static protected System.Delegate SWIG_CSharp$module##NAME##DelegateInstance = SWIG_CSharp$module##NAME##Helper.RegisterDelegate();
  internal partial struct SWIG_CSharp$module##NAME##Helper {
    [global::System.Runtime.InteropServices.DllImport("$dllimport", EntryPoint="SWIGRegister$module" + #NAME + "Callback")]
    private static extern void SWIGRegister$module##NAME##Callback(System.Delegate fn);
    public static System.Delegate RegisterDelegate() {
        SWIGRegister$module##NAME##Callback(NAME##DelegateInstance);
        return NAME##DelegateInstance;
    }
%}
%pragma(csharp) imclasscode=
%enddef
