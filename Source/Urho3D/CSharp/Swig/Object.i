
%define IGNORE_SUBSYSTEM(name)
    %ignore Urho3D::Object::Get##name;
    %ignore Urho3D::Context::Get##name;
    %ignore Urho3D::Context::RegisterSubsystem(name*);
%enddef

IGNORE_SUBSYSTEM(FileSystem)
IGNORE_SUBSYSTEM(WorkQueue)

%typemap(csout, excode=SWIGEXCODE) Urho3D::StringHash GetType {
    return new $typemap(cstype, Urho3D::StringHash)(GetType().Name);
}

%typemap(csout, excode=SWIGEXCODE) const Urho3D::String& GetTypeName, const Urho3D::String& GetTypeName {
    return GetType().Name;
}

// Not all RefCounted are Object descendants, but most are.
// To implement these functions we need access to enclosing class type so we can use it with typeof().
%ignore GetTypeStatic;
%ignore GetTypeNameStatic;
// TODO: These can be implemented by having each class store a static instance of TypeInfo.
%ignore GetTypeInfoStatic;
%ignore GetTypeInfo;
%rename(GetTypeHash) GetType;

%ignore Urho3D::EventHandler;
%ignore Urho3D::EventHandlerImpl;
%ignore Urho3D::EventHandler11Impl;
%ignore Urho3D::ObjectFactory;
%ignore Urho3D::Object::GetEventHandler;
%ignore Urho3D::Object::SubscribeToEvent;
%ignore Urho3D::Object::context_;

// Extend Context with extra code
%typemap(csconstruct, excode=SWIGEXCODE,directorconnect="\n    SwigDirectorConnect();") Context %{: this($imcall, true) {$excode$directorconnect
        OnSetupInstance();
    }
%}

%typemap(csdestruct_derived, methodname="Dispose", methodmodifiers="public") Urho3D::Context {
    OnDispose();
    $typemap(csdestruct_derived, SWIGTYPE)
}


%csexposefunc(runtime, CloneGCHandle, void*, void*) %{
    private static System.IntPtr CloneGCHandle(System.IntPtr handle)
    {
        return System.Runtime.InteropServices.GCHandle.ToIntPtr(
            System.Runtime.InteropServices.GCHandle.Alloc(
                System.Runtime.InteropServices.GCHandle.FromIntPtr(handle).Target));
    }
    private static System.Delegate CloneGCHandleDelegateInstance = new System.Func<System.IntPtr, System.IntPtr>(CloneGCHandle);
}%}

%csexposefunc(runtime, FreeGCHandle, void, void*) %{
    private static void FreeGCHandle(System.IntPtr handle)
    {
        System.Runtime.InteropServices.GCHandle.FromIntPtr(handle).Free();
    }
    private static System.Delegate FreeGCHandleDelegateInstance = new System.Action<System.IntPtr>(FreeGCHandle);
}%}

