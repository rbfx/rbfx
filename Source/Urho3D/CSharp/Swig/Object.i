
%define IGNORE_SUBSYSTEM(name)
    %ignore Urho3D::Object::Get##name;
    %ignore Urho3D::Context::Get##name;
    %ignore Urho3D::Context::RegisterSubsystem(name*);
%enddef

IGNORE_SUBSYSTEM(WorkQueue)
IGNORE_SUBSYSTEM(Tasks)

%typemap(csout, excode=SWIGEXCODE) Urho3D::StringHash GetType {
    return new $typemap(cstype, Urho3D::StringHash)(GetType().Name);
}

%typemap(csout, excode=SWIGEXCODE) const eastl::string& GetTypeName, const eastl::string& GetTypeName {
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
