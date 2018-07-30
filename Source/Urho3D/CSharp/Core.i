
%ignore Urho3D::GetEventNameRegister;
%ignore Urho3D::CustomVariantValue;
%ignore Urho3D::CustomVariantValueTraits;
%ignore Urho3D::CustomVariantValueImpl;
%ignore Urho3D::MakeCustomValue;
%ignore Urho3D::VariantValue;
%ignore Urho3D::Variant::Variant(const VectorBuffer&);
%ignore Urho3D::Variant::GetVectorBuffer;
%ignore Urho3D::Variant::SetCustomVariantValue;
%ignore Urho3D::Variant::GetCustomVariantValuePtr;
%ignore Urho3D::Variant::GetStringVectorPtr;
%ignore Urho3D::Variant::GetVariantVectorPtr;
%ignore Urho3D::Variant::GetVariantMapPtr;
%ignore Urho3D::Variant::GetCustomPtr;
%ignore Urho3D::Variant::GetBufferPtr;
%ignore Urho3D::VARIANT_VALUE_SIZE;
%rename(GetVariantType) Urho3D::Variant::GetType;
%csmethodmodifiers Urho3D::Variant::ToString "public new"
%include "Urho3D/Core/Variant.h"

%define IGNORE_SUBSYSTEM(name)
  %ignore Urho3D::Object::Get##name;
  %ignore Urho3D::Context::Get##name;
  %ignore Urho3D::Context::RegisterSubsystem(name*);
%enddef

IGNORE_SUBSYSTEM(FileSystem)
IGNORE_SUBSYSTEM(Console)
%ignore Urho3D::Engine::CreateConsole;
IGNORE_SUBSYSTEM(Audio)
IGNORE_SUBSYSTEM(DebugHud)
%ignore Urho3D::Engine::CreateDebugHud;
IGNORE_SUBSYSTEM(Graphics)
IGNORE_SUBSYSTEM(Input)
IGNORE_SUBSYSTEM(Localization)
IGNORE_SUBSYSTEM(Renderer)
IGNORE_SUBSYSTEM(Time)
IGNORE_SUBSYSTEM(UI)
IGNORE_SUBSYSTEM(WorkQueue)
%ignore Urho3D::Object::GetCache;
%ignore Urho3D::Context::GetCache;
%ignore Urho3D::Context::RegisterSubsystem(ResourceCache*);

%typemap(csout, excode=SWIGEXCODE) Urho3D::StringHash GetType {
  return new $typemap(cstype, Urho3D::StringHash)(GetType().Name);
}

%typemap(csout, excode=SWIGEXCODE) const Urho3D::String& GetTypeName, const Urho3D::String& GetTypeName {
  return GetType().Name;
}

%ignore Urho3D::EventHandler;
%ignore Urho3D::EventHandlerImpl;
%ignore Urho3D::EventHandler11Impl;
%ignore Urho3D::ObjectFactory;
%ignore Urho3D::Object::GetEventHandler;
%ignore Urho3D::Object::SubscribeToEvent;
%ignore Urho3D::Object::context_;
%rename(GetTypeHash) Urho3D::TypeInfo::GetType;
%rename(GetTypeHash) Urho3D::Object::GetType;
%director Urho3D::Object;
%include "Urho3D/Core/Object.h"


%ignore Urho3D::Context::RegisterFactory;
%ignore Urho3D::Context::GetEventHandler;
%ignore Urho3D::Context::RegisterAttribute;            // AttributeHandle copy ctor is private
%ignore Urho3D::Context::GetAttributes;
%ignore Urho3D::Context::GetNetworkAttributes;
%ignore Urho3D::Context::GetObjectCategories;
%ignore Urho3D::Context::GetSubsystems;
%ignore Urho3D::Context::GetObjectFactories;
%ignore Urho3D::Context::GetAllAttributes;
%ignore Urho3D::Context::GetAttribute;
%include "Urho3D/Core/Context.h"

%include "Urho3D/Engine/Engine.h"

%ignore Urho3D::Application::engine_;
%director Urho3D::Application;
%include "Urho3D/Engine/Application.h"
