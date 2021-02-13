// Various marshalling helpers
%include <typemaps.i>

#define %nocsattribute %feature("nocsattribute", "1")

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

%typemap(ctype)  const char* const* "char**"
%typemap(imtype) const char* const* "global::System.IntPtr"
%typemap(cstype) const char* const* "string[]"
// FIXME: %typemap(in)     const char* const* "$1 = $input;"
%typemap(out)    const char* const* "$result = $1;"
// FIXME: %typemap(csin)   const char* const* "$csinput"
%typemap(csout, excode=SWIGEXCODE) const char* const* {
    unsafe {
      var ptr = (byte**)$imcall;$excode
      var len = 0;
      if (ptr != null)
      {
          for (; ptr[len] != null; len++) { }
      }
      var ret = new string[len];
      for (var i = 0; i < len; i++)
        ret[i] = global::System.Text.Encoding.UTF8.GetString(ptr[i], Urho3D.strlen(new global::System.IntPtr(ptr[i])));
      return ret;
    }
  }

%define %stringify(token) #token %enddef

%define %csattribute(Class, AttributeType, AttributeName, GetMethod, SetMethod...)
  %csmethodmodifiers Class::GetMethod "private";
  %rename(_##GetMethod) Class::GetMethod;
#if #SetMethod != ""
  %csmethodmodifiers Class::SetMethod "private";
  %rename(_##SetMethod) Class::SetMethod;
  %extend Class {
    %insert("proxycode", cls="csattribute", depends1=%stringify(Class::GetMethod), depends2=%stringify(Class::SetMethod)) %{
      public $typemap(cstype, AttributeType) AttributeName {
        get { return _##GetMethod(); }
        set { _##SetMethod(value); }
      }
    %}
  }
#else
  %extend Class {
    %insert("proxycode", cls="csattribute", depends1=%stringify(Class::GetMethod)) %{
      public $typemap(cstype, AttributeType) AttributeName {
        get { return _##GetMethod(); }
      }
    %}
  }
#endif
%enddef

%define %inheritable(NS, CTYPE)
  %director NS::CTYPE;
  %wrapper %{
      $moduleDirectorTypes[SwigDirector_##CTYPE::GetTypeStatic()] = SwigDirector_##CTYPE::GetTypeInfoStatic();
      context->RegisterFactory<SwigDirector_##CTYPE>();%}
  %typemap(directorbody) NS::CTYPE %{
  public:
    using ClassName = SwigDirector_##CTYPE;
    using BaseClassName = NS::CTYPE;
    static Urho3D::StringHash GetTypeStatic() { return GetTypeInfoStatic()->GetType(); }
    static const eastl::string& GetTypeNameStatic() { return GetTypeInfoStatic()->GetTypeName(); }
    static const Urho3D::TypeInfo* GetTypeInfoStatic() { static const Urho3D::TypeInfo typeInfoStatic("SwigDirector_" #CTYPE, BaseClassName::GetTypeInfoStatic()); return &typeInfoStatic; }
  %}
%enddef

%define %csexposefunc(DEST, NAME, CRETURN, CPARAMS)
%DEST %{
  typedef CRETURN (SWIGSTDCALL* Urho3D_CSharp##NAME##Callback)(CPARAMS);
  SWIGEXPORT Urho3D_CSharp##NAME##Callback Urho3D_CSharp##NAME = nullptr;
  #ifdef __cplusplus
  extern "C"
  #endif
  SWIGEXPORT void SWIGSTDCALL Urho3DRegister##NAME##Callback(Urho3D_CSharp##NAME##Callback callback) {
    Urho3D_CSharp##NAME = callback;
  }
%}

%pragma(csharp) imclasscode=%{
  private static Urho3D_CSharp##NAME##Helper.NAME##Delegate Urho3D_CSharp##NAME##DelegateInstance = Urho3D_CSharp##NAME##Helper.Register();
  internal partial struct Urho3D_CSharp##NAME##Helper {
    [global::System.Runtime.InteropServices.DllImport($dllimport, EntryPoint="Urho3DRegister" + #NAME + "Callback")]
    private static extern void Urho3DRegister##NAME##Callback(System.Delegate fn);
    internal static NAME##Delegate Register() {
        var NAME##DelegateInstance = new NAME##Delegate(NAME);
        Urho3DRegister##NAME##Callback(NAME##DelegateInstance);
        return NAME##DelegateInstance;
    }
%}
%pragma(csharp) imclasscode=
%enddef
