// Various marshalling helpers

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

%csmethodmodifiers free "internal";
%csmethodmodifiers malloc "internal";
%csmethodmodifiers strdup "internal";
%csmethodmodifiers strlen "internal";

void free(void* ptr);
void* malloc(int size);
char* strdup(const char* void_ptr_string);
int strlen(const char* void_ptr_string);


%define CS_CONSTANT(fqn, name, value)
  %csconst(1) fqn;
  %csconstvalue(value) fqn;
  %rename(name) fqn;
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

%pragma(csharp) imclasscode=%{
    internal static class DelegateRegistry
    {
        private class DelegateReference
        {
            public System.Delegate reference;
            public System.Func<System.Delegate> registerer;

            public void Refresh()
            {
                reference = registerer();
            }
        }
        private static System.Collections.Generic.List<DelegateReference> _delegateReferences = new System.Collections.Generic.List<DelegateReference>();

        internal static bool RegisterDelegate(System.Func<System.Delegate> registerer)
        {
            var record = new DelegateReference {registerer = registerer};
            record.Refresh();
            _delegateReferences.Add(record);
            return true;
        }

        internal static void RefreshDelegatePointers()
        {
            for (int i = 0; i < _delegateReferences.Count; i++)
                _delegateReferences[i].Refresh();
        }
    }
%}

%define %csexposefunc(DEST, NAME, CRETURN, CPARAMS)
%DEST %{
  typedef CRETURN (SWIGSTDCALL* SWIG_CSharp##NAME##Callback)(CPARAMS);
  SWIGEXPORT SWIG_CSharp##NAME##Callback SWIG_CSharp##NAME = nullptr;
  #ifdef __cplusplus
  extern "C"
  #endif
  SWIGEXPORT void SWIGSTDCALL SWIGRegister##NAME##Callback(SWIG_CSharp##NAME##Callback callback) {
    SWIG_CSharp##NAME = callback;
  }
%}

%pragma(csharp) imclasscode=%{
  static private bool SWIG_CSharp##NAME##DelegateRegistered = DelegateRegistry.RegisterDelegate(SWIG_CSharp##NAME##Helper.RegisterDelegate);
  internal partial struct SWIG_CSharp##NAME##Helper {
    private static NAME##Delegate NAME##DelegateInstance;
    [global::System.Runtime.InteropServices.DllImport("$dllimport", EntryPoint="SWIGRegister" + #NAME + "Callback")]
    private static extern void SWIGRegister##NAME##Callback(System.Delegate fn);
    public static System.Delegate RegisterDelegate() {
        NAME##DelegateInstance = new NAME##Delegate(NAME);
        SWIGRegister##NAME##Callback(NAME##DelegateInstance);
        return NAME##DelegateInstance;
    }
%}
%pragma(csharp) imclasscode=
%enddef

%runtime %{
    template<typename T> T* addr(T& ref)  { return &ref; }
    template<typename T> T* addr(T* ptr)  { return ptr;  }
    template<typename T> T* addr(SwigValueWrapper<T>& ref)  { return &(T&)ref; }
    template<typename T> T* addr(SwigValueWrapper<T>* ptr)  { return *ptr;  }
    template<typename T> T& defef(T& ref) { return ref;  }
    template<typename T> T& defef(T* ptr) { return *ptr; }
    template<typename T> T& defef(SwigValueWrapper<T>& ref) { return (T&)ref;  }
    template<typename T> T& defef(SwigValueWrapper<T>* ptr) { return (T&)*ptr; }
%}
