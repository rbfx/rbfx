%ignore Urho3D::Context::RegisterFactory;
%ignore Urho3D::Context::GetFactory;
%ignore Urho3D::Context::GetEventHandler;
%ignore Urho3D::Context::RegisterAttribute;            // AttributeHandle copy ctor is private
%ignore Urho3D::Context::GetAttributes;
%ignore Urho3D::Context::GetNetworkAttributes;
%ignore Urho3D::Context::GetObjectCategories;
%ignore Urho3D::Context::GetSubsystems;
%ignore Urho3D::Context::GetObjectFactories;
%ignore Urho3D::Context::GetAllAttributes;
%ignore Urho3D::Context::GetAttribute;


%wrapper %{
  SWIGEXPORT Urho3D::HashMap<Urho3D::StringHash, const Urho3D::TypeInfo*> Urho3DDirectorTypes;
  #ifdef __cplusplus
  extern "C"
  #endif
  SWIGEXPORT const Urho3D::TypeInfo* Urho3DGetDirectorTypeInfo(Urho3D::StringHash type) {
    auto it = Urho3DDirectorTypes.Find(type);
    if (it != Urho3DDirectorTypes.End())
      return it->second_;
    return nullptr;
  }

  #ifdef __cplusplus
  extern "C"
  #endif
  SWIGEXPORT void SWIGSTDCALL $moduleRegisterDirectorFactories(Urho3D::Context* context) { %}

// Inheritable classes go here
%inheritable(Urho3D, Object);
%inheritable(Urho3D, Application);
//%inheritable(Urho3D, Component);
//%inheritable(Urho3D, LogicComponent);

%wrapper %{  }  // end of RegisterDirectorFactories%}

%csexposefunc(wrapper, CreateObject, void*, %arg(Urho3D::Context* context, unsigned type)) %{
  private static System.Delegate CreateObjectDelegateInstance = new System.Func<global::System.IntPtr, uint, global::System.Runtime.InteropServices.HandleRef>((context, type) => {
    return Context.wrap(context, false).CreateObject(type);
  });
}%}


%csexposefunc(runtime, CreateString, const char*, %arg(const char* str)) %{
    [return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]
    public static string CreateString([param: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]string str) {
        return str;
    }
    private static System.Delegate CreateStringDelegateInstance = new System.Func<string, string>(CreateString);
}%}

%inline %{
    const char* test_string()
    {
        const char* in = "foobar";
        const char* out = SWIG_CSharpUrho3DCreateString(in);
        return out;
    }

%}
