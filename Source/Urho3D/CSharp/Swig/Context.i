%ignore Urho3D::Context::RegisterFactory;
%ignore Urho3D::Context::GetFactory;
%ignore Urho3D::Context::GetEventHandler;
%ignore Urho3D::Context::GetObjectCategories;
%ignore Urho3D::Context::GetSubsystems;
%ignore Urho3D::Context::GetObjectFactories;


// Extend Context with extra code
%typemap(csconstruct, excode=SWIGEXCODE,directorconnect="\n    SwigDirectorConnect();") Urho3D::Context %{: this($imcall, true) {$excode$directorconnect
    OnSetupInstance();
  }
%}

%typemap(csdestruct_derived, methodname="Dispose", methodmodifiers="public") Urho3D::Context {
    OnDispose();
    $typemap(csdestruct_derived, SWIGTYPE)
}

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
%inheritable(Urho3D, Serializable);
%inheritable(Urho3D, Animatable);
%inheritable(Urho3D, Component);
%inheritable(Urho3D, LogicComponent);

%wrapper %{  }  // end of RegisterDirectorFactories%}

%csexposefunc(wrapper, CreateObject, void*, %arg(Urho3D::Context* context, unsigned type)) %{
    private delegate System.IntPtr CreateObjectDelegate(System.IntPtr context, uint type);
    private static System.IntPtr CreateObject(System.IntPtr context, uint type)
    {
        var newObject = Context.wrap(context, false).CreateObject(type);
        System.GC.KeepAlive(newObject.Wrapper);
        return newObject.Handle;
    }
}%}
