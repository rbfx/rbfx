%module(directors="1", dirprot="1", allprotected="1", naturalvar=1) Urho3D

#define URHO3D_STATIC
#define URHO3D_API
#define final
#define static_assert(...)

%include "stl.i"

%{
#include <Urho3D/Urho3DAll.h>
#include <SDL/SDL_joystick.h>
#include <SDL/SDL_gamecontroller.h>
#include <SDL/SDL_keycode.h>
%}

// Map void* to IntPtr
%apply void* VOID_INT_PTR { void* }

// Speed boost
%pragma(csharp) imclassclassmodifiers="[System.Security.SuppressUnmanagedCodeSecurity]\ninternal class"
%typemap(csclassmodifiers) SWIGTYPE "public partial class"

%include "cmalloc.i"
%include "arrays_csharp.i"
%include "Helpers.i"
%include "Math.i"
%include "_constants.i"
%include "String.i"

// Container
%include "Container/RefCounted.i"
%include "Container/Vector.i"
%include "Container/HashMap.i"

%include "_properties.i"
%include "_enums.i"

// --------------------------------------- SDL ---------------------------------------
namespace SDL
{
  #include "../ThirdParty/SDL/include/SDL/SDL_joystick.h"
  #include "../ThirdParty/SDL/include/SDL/SDL_gamecontroller.h"
  #include "../ThirdParty/SDL/include/SDL/SDL_keycode.h"
}
// --------------------------------------- Core ---------------------------------------

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
%include "Urho3D/Core/Timer.h"
%include "Urho3D/Core/Spline.h"
%include "Urho3D/Core/Mutex.h"

// --------------------------------------- Engine ---------------------------------------

%include "Urho3D/Engine/Engine.h"

%ignore Urho3D::Application::engine_;
%director Urho3D::Application;
%include "Urho3D/Engine/Application.h"

// --------------------------------------- Input ---------------------------------------

%apply void* VOID_INT_PTR { SDL_GameController*, SDL_Joystick* }
%apply int { SDL_JoystickID };
%typemap(csvarout, excode=SWIGEXCODE2) SDL_GameController*, SDL_Joystick* %{
  get {
    var ret = $imcall;$excode
    return ret;
  }
%}

%ignore Urho3D::TouchState::touchedElement_;
%ignore Urho3D::TouchState::GetTouchedElement;
%ignore Urho3D::JoystickState::IsController;

%include "Urho3D/Input/InputConstants.h"
%include "Urho3D/Input/Controls.h"
%include "Urho3D/Input/Input.h"

// --------------------------------------- Audio ---------------------------------------
%apply void* VOID_INT_PTR { signed char* }
%apply int FIXED[]  { int *dest }
%typemap(cstype) int *dest "ref int[]"
%typemap(imtype) int *dest "global::System.IntPtr"
%csmethodmodifiers Urho3D::SoundSource::Mix "public unsafe";
%ignore Urho3D::BufferedSoundStream::AddData(const SharedArrayPtr<signed char>& data, unsigned numBytes);
%ignore Urho3D::BufferedSoundStream::AddData(const SharedArrayPtr<signed short>& data, unsigned numBytes);
%ignore Urho3D::Sound::GetData;

namespace Urho3D
{
  // Temporary until Component.h is wrapped
  enum AutoRemoveMode
  {
    REMOVE_DISABLED = 0,
    REMOVE_COMPONENT,
    REMOVE_NODE
  };
}

%include "Urho3D/Audio/AudioDefs.h"
%include "Urho3D/Audio/Audio.h"
%include "Urho3D/Audio/BufferedSoundStream.h"
%include "Urho3D/Audio/OggVorbisSoundStream.h"
%include "Urho3D/Audio/Sound.h"
%include "Urho3D/Audio/SoundListener.h"
%include "Urho3D/Audio/SoundSource3D.h"
%include "Urho3D/Audio/SoundSource.h"
%include "Urho3D/Audio/SoundStream.h"
