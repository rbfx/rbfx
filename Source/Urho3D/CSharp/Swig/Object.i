
%define IGNORE_SUBSYSTEM(name)
    %ignore Urho3D::Object::Get##name;
    %ignore Urho3D::Context::Get##name;
    %ignore Urho3D::Context::RegisterSubsystem(name*);
%enddef

IGNORE_SUBSYSTEM(WorkQueue)
IGNORE_SUBSYSTEM(Tasks)

%csattributes Urho3D::Object::GetType "[Urho3DPINVOKE.OverrideNative]";
%typemap(csout, excode=SWIGEXCODE) Urho3D::StringHash Urho3D::Object::GetType {
    var ret = new $typemap(cstype, Urho3D::StringHash)(GetType().Name);$excode
    return ret;
}

%csattributes Urho3D::Object::GetTypeName "[Urho3DPINVOKE.OverrideNative]";
%typemap(csout, excode=SWIGEXCODE) const eastl::string& Urho3D::Object::GetTypeName {
    var ret = GetType().Name;$excode
    return ret;
}

%rename(GetTypeHash) Urho3D::Object::GetType;
%ignore Urho3D::Object::GetTypeInfo;    // TODO: All C# classes should implement it for C++ side to see.
%ignore Urho3D::Object::IsInstanceOf;   // TODO: All C# classes should implement it using metadata info (and possibly caching it).

%ignore Urho3D::EventHandler;
%ignore Urho3D::EventHandlerImpl;
%ignore Urho3D::EventHandler11Impl;
%ignore Urho3D::ObjectFactory;
%ignore Urho3D::Object::GetEventHandler;
%ignore Urho3D::Object::SubscribeToEvent;
%ignore Urho3D::Object::context_;
