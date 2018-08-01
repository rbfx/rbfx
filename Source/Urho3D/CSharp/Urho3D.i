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

%typemap(csvarout) void* VOID_INT_PTR %{
  get {
    var ret = $imcall;$excode
    return ret;
  }
%}

%typemap(csvarin, excode=SWIGEXCODE2) void* VOID_INT_PTR %{
  set {
    $imcall;$excode
  }
%}

// Map void* to IntPtr
%apply void* VOID_INT_PTR { void* }

// Speed boost
%pragma(csharp) imclassclassmodifiers="[System.Security.SuppressUnmanagedCodeSecurity]\ninternal class"
%typemap(csclassmodifiers) SWIGTYPE "public partial class"

%include "cmalloc.i"
%include "arrays_csharp.i"
%include "swiginterface.i"
%include "InstanceCache.i"
%include "Helpers.i"
%include "Math.i"
%include "_constants.i"
%include "String.i"

// Container
%include "Container/RefCounted.i"
%include "Container/Vector.i"
%include "Container/HashMap.i"

%include "attribute.i"
//%include "_properties.i"
%include "_enums.i"

%interface_custom("%s", "I%s", Urho3D::Serializer);
%interface_custom("%s", "I%s", Urho3D::Deserializer);
%interface_custom("%s", "I%s", Urho3D::AbstractFile);
//%interface_custom("%s", "I%s", Urho3D::GPUObject);
//%interface_custom("%s", "I%s", Urho3D::RefCounted);
%interface_custom("%s", "I%s", Urho3D::Octant);

%ignore Urho3D::GPUObject::OnDeviceLost;
%ignore Urho3D::GPUObject::OnDeviceReset;
%ignore Urho3D::GPUObject::Release;
%ignore Urho3D::VertexBuffer::OnDeviceLost;
%ignore Urho3D::VertexBuffer::OnDeviceReset;
%ignore Urho3D::VertexBuffer::Release;
%ignore Urho3D::IndexBuffer::OnDeviceLost;
%ignore Urho3D::IndexBuffer::OnDeviceReset;
%ignore Urho3D::IndexBuffer::Release;
%ignore Urho3D::ConstantBuffer::OnDeviceLost;
%ignore Urho3D::ConstantBuffer::OnDeviceReset;
%ignore Urho3D::ConstantBuffer::Release;
%ignore Urho3D::ShaderVariation::OnDeviceLost;
%ignore Urho3D::ShaderVariation::OnDeviceReset;
%ignore Urho3D::ShaderVariation::Release;
%ignore Urho3D::Texture::OnDeviceLost;
%ignore Urho3D::Texture::OnDeviceReset;
%ignore Urho3D::Texture::Release;
%ignore Urho3D::ShaderProgram::OnDeviceLost;
%ignore Urho3D::ShaderProgram::OnDeviceReset;
%ignore Urho3D::ShaderProgram::Release;

%feature("flatnested");			// UI::DragData needs this

%feature("director") Urho3D::Object;
%feature("director") Urho3D::Application;

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
%csmethodmodifiers ToString "public new"
%include "Urho3D/Core/Variant.h"

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

// --------------------------------------- IO ---------------------------------------
%ignore Urho3D::Deserializer::ReadBuffer;                 // FIXME: implement return-by-value in PODVector marshalling

%include "Urho3D/IO/Serializer.h"
%include "Urho3D/IO/Deserializer.h"
%include "Urho3D/IO/AbstractFile.h"
%include "Urho3D/IO/Compression.h"
%include "Urho3D/IO/File.h"
%include "Urho3D/IO/Log.h"
%include "Urho3D/IO/MemoryBuffer.h"
%include "Urho3D/IO/PackageFile.h"
%include "Urho3D/IO/VectorBuffer.h"

// --------------------------------------- Resource ---------------------------------------
%include "Urho3D/Resource/Resource.h"
%include "Urho3D/Resource/BackgroundLoader.h"
%include "Urho3D/Resource/Image.h"
%include "Urho3D/Resource/YAMLFile.h"
%include "Urho3D/Resource/JSONFile.h"
%include "Urho3D/Resource/JSONValue.h"
%include "Urho3D/Resource/Localization.h"
%include "Urho3D/Resource/PListFile.h"
%include "Urho3D/Resource/ResourceCache.h"
%include "Urho3D/Resource/XMLElement.h"
%include "Urho3D/Resource/XMLFile.h"

// --------------------------------------- Scene ---------------------------------------
%include "Urho3D/Scene/ValueAnimationInfo.h"
%include "Urho3D/Scene/Serializable.h"
%include "Urho3D/Scene/Animatable.h"
%include "Urho3D/Scene/Component.h"
%include "Urho3D/Scene/Node.h"
%include "Urho3D/Scene/ReplicationState.h"
%include "Urho3D/Scene/Scene.h"
%include "Urho3D/Scene/SplinePath.h"
%include "Urho3D/Scene/ValueAnimation.h"
%include "Urho3D/Scene/AnimationDefs.h"
%include "Urho3D/Scene/LogicComponent.h"
%include "Urho3D/Scene/ObjectAnimation.h"
%include "Urho3D/Scene/SceneResolver.h"
%include "Urho3D/Scene/SmoothedTransform.h"
%include "Urho3D/Scene/UnknownComponent.h"

// --------------------------------------- Audio ---------------------------------------
%apply void* VOID_INT_PTR { signed char* }
%apply int FIXED[]  { int *dest }
%typemap(cstype) int *dest "ref int[]"
%typemap(imtype) int *dest "global::System.IntPtr"
%csmethodmodifiers Urho3D::SoundSource::Mix "public unsafe";
%ignore Urho3D::BufferedSoundStream::AddData(const SharedArrayPtr<signed char>& data, unsigned numBytes);
%ignore Urho3D::BufferedSoundStream::AddData(const SharedArrayPtr<signed short>& data, unsigned numBytes);
%ignore Urho3D::Sound::GetData;

%include "Urho3D/Audio/AudioDefs.h"
%include "Urho3D/Audio/Audio.h"
%include "Urho3D/Audio/Sound.h"
%include "Urho3D/Audio/SoundStream.h"
%include "Urho3D/Audio/BufferedSoundStream.h"
%include "Urho3D/Audio/OggVorbisSoundStream.h"
%include "Urho3D/Audio/SoundListener.h"
%include "Urho3D/Audio/SoundSource.h"
%include "Urho3D/Audio/SoundSource3D.h"

// --------------------------------------- IK ---------------------------------------
%{ using Algorithm = Urho3D::IKSolver::Algorithm; %}

%include "Urho3D/IK/IKConstraint.h"
%include "Urho3D/IK/IKEffector.h"
%include "Urho3D/IK/IK.h"
%include "Urho3D/IK/IKSolver.h"

// --------------------------------------- Graphics ---------------------------------------
%include "Urho3D/Graphics/GPUObject.h"
%include "Urho3D/Graphics/Octree.h"
%include "Urho3D/Graphics/Drawable.h"
%include "Urho3D/Graphics/Texture.h"
%include "Urho3D/Graphics/Texture2D.h"
%include "Urho3D/Graphics/Texture2DArray.h"
%include "Urho3D/Graphics/Texture3D.h"
%include "Urho3D/Graphics/TextureCube.h"
%include "Urho3D/Graphics/StaticModel.h"
%include "Urho3D/Graphics/AnimatedModel.h"
%include "Urho3D/Graphics/BillboardSet.h"
%include "Urho3D/Graphics/DecalSet.h"
%include "Urho3D/Graphics/GraphicsDefs.h"
%include "Urho3D/Graphics/Light.h"
%include "Urho3D/Graphics/OctreeQuery.h"
%include "Urho3D/Graphics/RenderSurface.h"
%include "Urho3D/Graphics/ShaderVariation.h"
%include "Urho3D/Graphics/Tangent.h"
%include "Urho3D/Graphics/VertexDeclaration.h"
%include "Urho3D/Graphics/AnimationController.h"
%include "Urho3D/Graphics/Camera.h"
%include "Urho3D/Graphics/Material.h"
%include "Urho3D/Graphics/ParticleEffect.h"
%include "Urho3D/Graphics/RibbonTrail.h"
%include "Urho3D/Graphics/Skeleton.h"
%include "Urho3D/Graphics/Technique.h"
%include "Urho3D/Graphics/View.h"
%include "Urho3D/Graphics/Animation.h"
%include "Urho3D/Graphics/ConstantBuffer.h"
%include "Urho3D/Graphics/Graphics.h"
%include "Urho3D/Graphics/Model.h"
%include "Urho3D/Graphics/ParticleEmitter.h"
%include "Urho3D/Graphics/Shader.h"
%include "Urho3D/Graphics/Skybox.h"
%include "Urho3D/Graphics/Terrain.h"
%include "Urho3D/Graphics/Viewport.h"
%include "Urho3D/Graphics/AnimationState.h"
%include "Urho3D/Graphics/CustomGeometry.h"
%include "Urho3D/Graphics/Geometry.h"
%include "Urho3D/Graphics/OcclusionBuffer.h"
%include "Urho3D/Graphics/Renderer.h"
%include "Urho3D/Graphics/ShaderPrecache.h"
%include "Urho3D/Graphics/StaticModelGroup.h"
%include "Urho3D/Graphics/TerrainPatch.h"
%include "Urho3D/Graphics/Zone.h"
%include "Urho3D/Graphics/Batch.h"
%include "Urho3D/Graphics/DebugRenderer.h"
%include "Urho3D/Graphics/IndexBuffer.h"
%include "Urho3D/Graphics/RenderPath.h"
%include "Urho3D/Graphics/ShaderProgram.h"
%include "Urho3D/Graphics/VertexBuffer.h"

// --------------------------------------- Navigation ---------------------------------------
%apply void* VOID_INT_PTR {
	rcContext*,
	dtTileCacheContourSet*,
	dtTileCachePolyMesh*,
	dtTileCacheAlloc*,
	dtQueryFilter*,
	rcCompactHeightfield*,
	rcContourSet*,
	rcHeightfield*,
	rcHeightfieldLayerSet*,
	rcPolyMesh*,
	rcPolyMeshDetail*
}

%include "Urho3D/Navigation/CrowdAgent.h"
%include "Urho3D/Navigation/CrowdManager.h"
%include "Urho3D/Navigation/NavigationMesh.h"
%include "Urho3D/Navigation/DynamicNavigationMesh.h"
%include "Urho3D/Navigation/NavArea.h"
%include "Urho3D/Navigation/NavBuildData.h"
%include "Urho3D/Navigation/Navigable.h"
%include "Urho3D/Navigation/Obstacle.h"
%include "Urho3D/Navigation/OffMeshConnection.h"

// --------------------------------------- Network ---------------------------------------
%ignore Urho3D::Network::MakeHttpRequest;

%include "Urho3D/Network/Connection.h"
%include "Urho3D/Network/Network.h"
%include "Urho3D/Network/NetworkPriority.h"
%include "Urho3D/Network/Protocol.h"

// --------------------------------------- Physics ---------------------------------------
%ignore Urho3D::TriangleMeshData::meshInterface_;
%ignore Urho3D::TriangleMeshData::shape_;
%ignore Urho3D::TriangleMeshData::infoMap_;
%ignore Urho3D::GImpactMeshData::meshInterface_;

%include "Urho3D/Physics/CollisionShape.h"
%include "Urho3D/Physics/Constraint.h"
%include "Urho3D/Physics/PhysicsWorld.h"
%include "Urho3D/Physics/RaycastVehicle.h"
%include "Urho3D/Physics/RigidBody.h"

// --------------------------------------- SystemUI ---------------------------------------
%include "Urho3D/SystemUI/Console.h"
%include "Urho3D/SystemUI/DebugHud.h"
%include "Urho3D/SystemUI/SystemMessageBox.h"
%include "Urho3D/SystemUI/SystemUI.h"

// --------------------------------------- UI ---------------------------------------
%include "Urho3D/UI/UI.h"
%include "Urho3D/UI/UIBatch.h"
%include "Urho3D/UI/UIElement.h"
%include "Urho3D/UI/BorderImage.h"
%include "Urho3D/UI/UISelectable.h"
%include "Urho3D/UI/CheckBox.h"
%include "Urho3D/UI/FontFace.h"
%include "Urho3D/UI/FontFaceBitmap.h"
%include "Urho3D/UI/FontFaceFreeType.h"
%include "Urho3D/UI/Font.h"
%include "Urho3D/UI/LineEdit.h"
%include "Urho3D/UI/ProgressBar.h"
%include "Urho3D/UI/ScrollView.h"
%include "Urho3D/UI/Sprite.h"
%include "Urho3D/UI/Text.h"
%include "Urho3D/UI/Button.h"
%include "Urho3D/UI/Menu.h"
%include "Urho3D/UI/DropDownList.h"
%include "Urho3D/UI/Cursor.h"
%include "Urho3D/UI/FileSelector.h"
%include "Urho3D/UI/ListView.h"
%include "Urho3D/UI/MessageBox.h"
%include "Urho3D/UI/ScrollBar.h"
%include "Urho3D/UI/Slider.h"
%include "Urho3D/UI/Text3D.h"
%include "Urho3D/UI/ToolTip.h"
%include "Urho3D/UI/UIComponent.h"
%include "Urho3D/UI/Window.h"
%include "Urho3D/UI/View3D.h"
