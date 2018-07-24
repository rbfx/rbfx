
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
%rename(GetVariantType) Urho3D::Variant::GetType;
%csmethodmodifiers Urho3D::Variant::ToString "public new"
%include "Urho3D/Core/Variant.h"

%ignore Urho3D::EventHandler;
%ignore Urho3D::EventHandlerImpl;
%ignore Urho3D::EventHandler11Impl;
//%ignore Urho3D::ObjectFactory;
%ignore Urho3D::Object::GetEventHandler;
%ignore Urho3D::Object::SubscribeToEvent;
%rename(GetTypeHash) Urho3D::TypeInfo::GetType;
%director Urho3D::Object;
%include "Urho3D/Core/Object.h"


%ignore Urho3D::Context::RegisterFactory;
%ignore Urho3D::Context::GetEventHandler;
%ignore Urho3D::Context::RegisterAttribute;            // AttributeHandle copy ctor is private
%ignore Urho3D::Context::GetAttributes;
%ignore Urho3D::Context::GetNetworkAttributes;
%include "Urho3D/Core/Context.h"

%include "Urho3D/Engine/EngineDefs.h"
%include "Urho3D/Engine/Engine.h"
%director Urho3D::Application;
%ignore Urho3D::Application::startupErrors_;
%include "Urho3D/Engine/Application.h"
