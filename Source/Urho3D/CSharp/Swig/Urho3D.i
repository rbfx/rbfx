%module(directors="1", dirprot="1", allprotected="1", naturalvar=1) Urho3D

#ifndef URHO3D_STATIC
#   define URHO3D_STATIC
#endif
#ifndef URHO3D_API
#   define URHO3D_API
#endif
#define final
#define static_assert(...)

%include "stl.i"
%include "typemaps.i"

%{
#include <Urho3D/Urho3DAll.h>
#include <SDL/SDL_joystick.h>
#include <SDL/SDL_gamecontroller.h>
#include <SDL/SDL_keycode.h>
#undef GetObject
#undef SendMessage
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

%apply void* VOID_INT_PTR {
	void*,
	SDL_Cursor*,
	SDL_Surface*,
	SDL_Window*,
	Urho3D::GraphicsImpl*,
	signed char*,
	unsigned char*
}

// Speed boost
%pragma(csharp) imclassclassmodifiers="[System.Security.SuppressUnmanagedCodeSecurity]\ninternal class"
%typemap(csclassmodifiers) SWIGTYPE "public partial class"

%include "cmalloc.i"
%include "arrays_csharp.i"
%include "swiginterface.i"
%include "InstanceCache.i"
%include "Helpers.i"
%include "Operators.i"

// String typemap returns 0 if null string is passed. This fails to initialize SafeArray.
%ignore Urho3D::Node::GetChildrenWithTag(const String& tag, bool recursive = false) const;
%ignore Urho3D::UIElement::GetChildrenWithTag(const String& tag, bool recursive = false) const;
%ignore Urho3D::XMLElement::GetBuffer;

%include "StringHash.i"
%include "String.i"

// --------------------------------------- Math ---------------------------------------
%include "Math.i"

namespace Urho3D
{

URHO3D_BINARY_COMPATIBLE_TYPE(Color);
URHO3D_BINARY_COMPATIBLE_TYPE(Rect);
URHO3D_BINARY_COMPATIBLE_TYPE(IntRect);
URHO3D_BINARY_COMPATIBLE_TYPE(Vector2);
URHO3D_BINARY_COMPATIBLE_TYPE(IntVector2);
URHO3D_BINARY_COMPATIBLE_TYPE(Vector3);
URHO3D_BINARY_COMPATIBLE_TYPE(IntVector3);
URHO3D_BINARY_COMPATIBLE_TYPE(Vector4);
URHO3D_BINARY_COMPATIBLE_TYPE(Matrix3);
URHO3D_BINARY_COMPATIBLE_TYPE(Matrix3x4);
URHO3D_BINARY_COMPATIBLE_TYPE(Matrix4);
URHO3D_BINARY_COMPATIBLE_TYPE(Quaternion);
URHO3D_BINARY_COMPATIBLE_TYPE(Plane);
URHO3D_BINARY_COMPATIBLE_TYPE(BoundingBox);
URHO3D_BINARY_COMPATIBLE_TYPE(Sphere);
//URHO3D_BINARY_COMPATIBLE_TYPE(Frustum);
URHO3D_BINARY_COMPATIBLE_TYPE(Ray);

}

%include "_constants.i"
%include "_events.i"
%include "_enums.i"

%rename(Outside) OUTSIDE;
%rename(Intersects) INTERSECTS;
%rename(Inside) INSIDE;
%ignore Urho3D::M_PI;
%ignore Urho3D::M_HALF_PI;
%ignore Urho3D::M_MIN_INT;
%ignore Urho3D::M_MAX_INT;
%ignore Urho3D::M_MIN_UNSIGNED;
%ignore Urho3D::M_MAX_UNSIGNED;
%ignore Urho3D::M_EPSILON;
%ignore Urho3D::M_LARGE_EPSILON;
%ignore Urho3D::M_MIN_NEARCLIP;
%ignore Urho3D::M_MAX_FOV;
%ignore Urho3D::M_LARGE_VALUE;
%ignore Urho3D::M_INFINITY;
%ignore Urho3D::M_DEGTORAD;
%ignore Urho3D::M_DEGTORAD_2;
%ignore Urho3D::M_RADTODEG;

// These should be implemented in C# anyway.
%ignore Urho3D::Polyhedron::Polyhedron(const Vector<PODVector<Vector3> >& faces);
%ignore Urho3D::Polyhedron::faces_;

%apply float *INOUT        { float& sin, float& cos, float& accumulator };
%apply unsigned int* INOUT { unsigned int* randomRef, unsigned int* nearestRef }

// ref global::System.IntPtr
%typemap(ctype, out="void *")                 void*& "void *"
%typemap(imtype, out="global::System.IntPtr") void*& "ref global::System.IntPtr"
%typemap(cstype, out="$csclassname")          void*& "ref global::System.IntPtr"
%typemap(csin)                                void*& "ref $csinput"
%typemap(in)                                  void*& %{ $1 = ($1_ltype)$input; %}
%typecheck(SWIG_TYPECHECK_CHAR_PTR)           void*& ""

%include "Urho3D/Math/MathDefs.h"

// ---------------------------------------  ---------------------------------------

%ignore Urho3D::textureFilterModeNames;
%ignore Urho3D::textureUnitNames;
%ignore Urho3D::cullModeNames;
%ignore Urho3D::fillModeNames;
%ignore Urho3D::blendModeNames;
%ignore Urho3D::compareModeNames;
%ignore Urho3D::lightingModeNames;

%ignore Urho3D::Frustum::planes_;
%ignore Urho3D::Frustum::vertices_;

%include "RefCounted.i"
%include "Vector.i"
%include "HashMap.i"
%include "Urho3D/Math/Polyhedron.h"
%include "Urho3D/Math/Frustum.h"

// Declare inheritable classes in this file
%include "Context.i"

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

// --------------------------------------- SDL ---------------------------------------
namespace SDL
{
  #include "../ThirdParty/SDL/include/SDL/SDL_joystick.h"
  #include "../ThirdParty/SDL/include/SDL/SDL_gamecontroller.h"
  #include "../ThirdParty/SDL/include/SDL/SDL_keycode.h"
}
// --------------------------------------- Core ---------------------------------------

%ignore Urho3D::RegisterResourceLibrary;
%ignore Urho3D::RegisterSceneLibrary;
%ignore Urho3D::RegisterAudioLibrary;
%ignore Urho3D::RegisterIKLibrary;
%ignore Urho3D::RegisterGraphicsLibrary;
%ignore Urho3D::RegisterNavigationLibrary;
%ignore Urho3D::RegisterUILibrary;

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
%csmethodmodifiers ToString() "public override"
%ignore Urho3D::AttributeInfo::enumNamesStorage_;
%ignore Urho3D::AttributeInfo::enumNamesPointers_;
%ignore Urho3D::AttributeInfo::enumNames_;

%include "Urho3D/Core/Variant.h"
%include "Object.i"
%director Urho3D::AttributeAccessor;
%include "Urho3D/Core/Attribute.h"
%include "Urho3D/Core/Object.h"
%include "Urho3D/Core/Context.h"
%include "Urho3D/Core/Timer.h"
%include "Urho3D/Core/Spline.h"
%include "Urho3D/Core/Mutex.h"

// --------------------------------------- Engine ---------------------------------------
%ignore Urho3D::Engine::DefineParameters;

%include "Urho3D/Engine/EngineDefs.h"
%include "Urho3D/Engine/Engine.h"

%ignore Urho3D::Application::engine_;
%ignore Urho3D::Application::GetCommandLineParser;
%ignore Urho3D::PluginApplication::PluginMain;
%include "Urho3D/Engine/Application.h"
%include "Urho3D/Engine/PluginApplication.h"
#if URHO3D_CSHARP
%include "Urho3D/Script/Script.h"
#endif

// --------------------------------------- Input ---------------------------------------
%typemap(csbase) Urho3D::MouseButton "uint"
%csconstvalue("uint.MaxValue") MOUSEB_ANY;

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
%interface_custom("%s", "I%s", Urho3D::Serializer);
%include "Urho3D/IO/Serializer.h"
%interface_custom("%s", "I%s", Urho3D::Deserializer);
%include "Urho3D/IO/Deserializer.h"
%interface_custom("%s", "I%s", Urho3D::AbstractFile);
%include "Urho3D/IO/AbstractFile.h"
%include "Urho3D/IO/Compression.h"
%include "Urho3D/IO/File.h"
%include "Urho3D/IO/Log.h"
%include "Urho3D/IO/MemoryBuffer.h"
%include "Urho3D/IO/PackageFile.h"
%include "Urho3D/IO/VectorBuffer.h"

// --------------------------------------- Resource ---------------------------------------
%ignore Urho3D::XMLFile::GetDocument;
%ignore Urho3D::XMLElement::XMLElement(XMLFile* file, pugi::xml_node_struct* node);
%ignore Urho3D::XMLElement::XMLElement(XMLFile* file, const XPathResultSet* resultSet, const pugi::xpath_node* xpathNode, unsigned xpathResultIndex);
%ignore Urho3D::XMLElement::GetNode;
%ignore Urho3D::XMLElement::GetXPathNode;
%ignore Urho3D::XMLElement::Select;
%ignore Urho3D::XMLElement::SelectSingle;
%ignore Urho3D::XPathResultSet::XPathResultSet(XMLFile* file, pugi::xpath_node_set* resultSet);
%ignore Urho3D::XPathResultSet::GetXPathNodeSet;
%ignore Urho3D::XPathQuery::GetXPathQuery;
%ignore Urho3D::XPathQuery::GetXPathVariableSet;

%ignore Urho3D::Image::GetLevels(PODVector<Image*>& levels);
%ignore Urho3D::Image::GetLevels(PODVector<Image const*>& levels) const;
namespace Urho3D { class Image; }
%extend Urho3D::Image {
public:
	PODVector<Image*> GetLevels() {
		PODVector<Image*> result{};
		$self->GetLevels(result);
		return result;
	}
}

// These expose iterators of underlying collection. Iterate object through GetObject() instead.
%ignore Urho3D::JSONValue::Begin;
%ignore Urho3D::JSONValue::End;
%ignore Urho3D::BackgroundLoadItem;
%ignore Urho3D::BackgroundLoader::ThreadFunction;

%include "Urho3D/Resource/Resource.h"
#if defined(URHO3D_THREADING)
%include "Urho3D/Resource/BackgroundLoader.h"
#endif
%include "Urho3D/Resource/Image.h"
%include "Urho3D/Resource/JSONValue.h"
%include "Urho3D/Resource/JSONFile.h"
%include "Urho3D/Resource/Localization.h"
%include "Urho3D/Resource/PListFile.h"
%include "Urho3D/Resource/XMLElement.h"
%include "Urho3D/Resource/XMLFile.h"
%include "Urho3D/Resource/ResourceCache.h"

// --------------------------------------- Scene ---------------------------------------
%ignore Urho3D::DirtyBits::data_;
%ignore Urho3D::SceneReplicationState::dirtyNodes_;		// Needs HashSet wrapped
%ignore Urho3D::NodeReplicationState::dirtyVars_;		// Needs HashSet wrapped
%ignore Urho3D::Animatable::animatedNetworkAttributes_; // Needs HashSet wrapped
%ignore Urho3D::AsyncProgress::resources_;
%ignore Urho3D::ValueAnimation::GetKeyFrames;
%ignore Urho3D::Serializable::networkState_;
%ignore Urho3D::ReplicationState::connection_;
%ignore Urho3D::Node::SetOwner;
%ignore Urho3D::Node::GetOwner;
%ignore Urho3D::Component::CleanupConnection;
%ignore Urho3D::Scene::CleanupConnection;
%ignore Urho3D::Node::CleanupConnection;
%ignore Urho3D::NodeImpl;

%include "Urho3D/Scene/AnimationDefs.h"
%include "Urho3D/Scene/ValueAnimationInfo.h"
%include "Urho3D/Scene/Serializable.h"
%include "Urho3D/Scene/Animatable.h"
%include "Urho3D/Scene/Component.h"
%include "Urho3D/Scene/Node.h"
%include "Urho3D/Scene/ReplicationState.h"
%include "Urho3D/Scene/Scene.h"
%include "Urho3D/Scene/SplinePath.h"
%include "Urho3D/Scene/ValueAnimation.h"
%include "Urho3D/Scene/LogicComponent.h"
%include "Urho3D/Scene/ObjectAnimation.h"
%include "Urho3D/Scene/SceneResolver.h"
%include "Urho3D/Scene/SmoothedTransform.h"
%include "Urho3D/Scene/UnknownComponent.h"

// --------------------------------------- Audio ---------------------------------------
%apply int FIXED[]  { int *dest }
%apply int* OUTPUT  { int& x, int& y }
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
#if defined(URHO3D_IK)
%{ using Algorithm = Urho3D::IKSolver::Algorithm; %}

%include "Urho3D/IK/IKConstraint.h"
%include "Urho3D/IK/IKEffector.h"
%include "Urho3D/IK/IK.h"
%include "Urho3D/IK/IKSolver.h"
#endif
// --------------------------------------- Graphics ---------------------------------------
%ignore Urho3D::FrustumOctreeQuery::TestDrawables;
%ignore Urho3D::SphereOctreeQuery::TestDrawables;
%ignore Urho3D::AllContentOctreeQuery::TestDrawables;
%ignore Urho3D::PointOctreeQuery::TestDrawables;
%ignore Urho3D::BoxOctreeQuery::TestDrawables;
%ignore Urho3D::OctreeQuery::TestDrawables;
%ignore Urho3D::ELEMENT_TYPESIZES;
%ignore Urho3D::ScratchBuffer;
%ignore Urho3D::Drawable::GetBatches;
%ignore Urho3D::Light::SetLightQueue;
%ignore Urho3D::Light::GetLightQueue;
%ignore Urho3D::Renderer::SetShadowMapFilter;
%ignore Urho3D::Renderer::SetBatchShaders;
%ignore Urho3D::Renderer::SetLightVolumeBatchShaders;
%ignore Urho3D::IndexBufferDesc;
%ignore Urho3D::VertexBufferDesc;
%ignore Urho3D::GPUObject::GetGraphics;
%ignore Urho3D::Terrain::GetHeightData; // SharedArrayPtr<float>
%ignore Urho3D::Geometry::GetRawData;
%ignore Urho3D::Geometry::SetRawVertexData;
%ignore Urho3D::Geometry::SetRawIndexData;
%ignore Urho3D::Geometry::GetRawDataShared;
%ignore Urho3D::IndexBuffer::GetShadowDataShared;
%ignore Urho3D::VertexBuffer::GetShadowDataShared;
%ignore Urho3D::RenderPathCommand::outputs_;    // Needs Pair<String, CubeMapFace>
%ignore Urho3D::RenderPathCommand::textureNames_;    // Needs array of strings
%ignore Urho3D::VertexBufferMorph::morphData_;      // Needs SharedPtrArray
%ignore Urho3D::ShaderVariation::GetConstantBufferSizes;
%ignore Urho3D::ShaderProgram::GetConstantBuffers;
%ignore Urho3D::DecalVertex::blendIndices_;
%ignore Urho3D::DecalVertex::blendWeights_;
%ignore Urho3D::ShaderProgram::GetVertexAttributes;
%ignore Urho3D::ShaderVariation::elementSemanticNames;
%ignore Urho3D::CustomGeometry::DrawOcclusion;
%ignore Urho3D::CustomGeometry::ProcessRayQuery;
%ignore Urho3D::OcclusionBufferData::dataWithSafety_;
%ignore Urho3D::ScenePassInfo::batchQueue_;
%ignore Urho3D::LightQueryResult;
%ignore Urho3D::View::GetLightQueues;
%rename(DrawableFlags) Urho3D::DrawableFlag;


%apply unsigned *OUTPUT { unsigned& minVertex, unsigned& vertexCount };
%apply unsigned *INOUT  { unsigned& index };
%apply float INPUT[]    { const float* };
%apply unsigned char INPUT[] { const unsigned char* blendIndices };
%apply void* VOID_INT_PTR {
    int *data_,
    int *Urho3D::OcclusionBuffer::GetBuffer
}

%include "Urho3D/Graphics/GraphicsDefs.h"
%interface_custom("%s", "I%s", Urho3D::GPUObject);
%include "Urho3D/Graphics/GPUObject.h"
%include "Urho3D/Graphics/IndexBuffer.h"
%include "Urho3D/Graphics/VertexBuffer.h"
%include "Urho3D/Graphics/Geometry.h"
%include "Urho3D/Graphics/OcclusionBuffer.h"
%include "Urho3D/Graphics/Drawable.h"
%include "Urho3D/Graphics/OctreeQuery.h"
%interface_custom("%s", "I%s", Urho3D::Octant);
%include "Urho3D/Graphics/Octree.h"
%include "Urho3D/Graphics/RenderPath.h"
%include "Urho3D/Graphics/Viewport.h"
%include "Urho3D/Graphics/RenderSurface.h"
%include "Urho3D/Graphics/Texture.h"
%include "Urho3D/Graphics/Texture2D.h"
%include "Urho3D/Graphics/Texture2DArray.h"
%include "Urho3D/Graphics/Texture3D.h"
%include "Urho3D/Graphics/TextureCube.h"
//%include "Urho3D/Graphics/Batch.h"
%include "Urho3D/Graphics/Skeleton.h"
%include "Urho3D/Graphics/Model.h"
%include "Urho3D/Graphics/StaticModel.h"
%include "Urho3D/Graphics/StaticModelGroup.h"
%include "Urho3D/Graphics/Animation.h"
%include "Urho3D/Graphics/AnimationState.h"
%include "Urho3D/Graphics/AnimationController.h"
%include "Urho3D/Graphics/AnimatedModel.h"
%include "Urho3D/Graphics/BillboardSet.h"
%include "Urho3D/Graphics/DecalSet.h"
%include "Urho3D/Graphics/Light.h"
%include "Urho3D/Graphics/ConstantBuffer.h"
%include "Urho3D/Graphics/ShaderVariation.h"
%include "Urho3D/Graphics/ShaderPrecache.h"
#if defined(URHO3D_OPENGL)
%include "Urho3D/Graphics/OpenGL/OGLShaderProgram.h"
#elif defined(URHO3D_D3D11)
%include "Urho3D/Graphics/Direct3D11/D3D11ShaderProgram.h"
#else
%include "Urho3D/Graphics/Direct3D9/D3D9ShaderProgram.h"
#endif
%include "Urho3D/Graphics/Tangent.h"
//%include "Urho3D/Graphics/VertexDeclaration.h"
%include "Urho3D/Graphics/Camera.h"
%include "Urho3D/Graphics/View.h"
%include "Urho3D/Graphics/Material.h"
%include "Urho3D/Graphics/CustomGeometry.h"
%include "Urho3D/Graphics/ParticleEffect.h"
%include "Urho3D/Graphics/RibbonTrail.h"
%include "Urho3D/Graphics/Technique.h"
%include "Urho3D/Graphics/ParticleEmitter.h"
%include "Urho3D/Graphics/Shader.h"
%include "Urho3D/Graphics/Skybox.h"
%include "Urho3D/Graphics/TerrainPatch.h"
%include "Urho3D/Graphics/Terrain.h"
%include "Urho3D/Graphics/DebugRenderer.h"
%include "Urho3D/Graphics/Zone.h"
%include "Urho3D/Graphics/Renderer.h"
%include "Urho3D/Graphics/Graphics.h"

// --------------------------------------- Navigation ---------------------------------------
#if defined(URHO3D_NAVIGATION)
%template(CrowdAgentArray)       Urho3D::PODVector<Urho3D::CrowdAgent*>;

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
%ignore Urho3D::NavBuildData::navAreas_;
%ignore Urho3D::NavigationMesh::FindPath;
%include "Urho3D/Navigation/CrowdAgent.h"
%include "Urho3D/Navigation/CrowdManager.h"
%include "Urho3D/Navigation/NavigationMesh.h"
%include "Urho3D/Navigation/DynamicNavigationMesh.h"
%include "Urho3D/Navigation/NavArea.h"
%include "Urho3D/Navigation/NavBuildData.h"
%include "Urho3D/Navigation/Navigable.h"
%include "Urho3D/Navigation/Obstacle.h"
%include "Urho3D/Navigation/OffMeshConnection.h"
#endif

// --------------------------------------- Network ---------------------------------------
#if defined(URHO3D_NETWORK)
%ignore Urho3D::Network::MakeHttpRequest;
%ignore Urho3D::PackageDownload;
%ignore Urho3D::PackageUpload;

%template(ConnectionVector) Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Connection>>;

// These methods use forward-declared types from SLikeNet.
%ignore Urho3D::Connection::Connection;
%ignore Urho3D::Connection::GetAddressOrGUID;
%ignore Urho3D::Connection::SetAddressOrGUID;
%ignore Urho3D::Network::HandleMessage;
%ignore Urho3D::Network::NewConnectionEstablished;
%ignore Urho3D::Network::ClientDisconnected;
%ignore Urho3D::Network::GetConnection;
%ignore Urho3D::Network::OnServerConnect;
%ignore Urho3D::Network::HandleIncomingPacket;

%include "Urho3D/Network/Connection.h"
%include "Urho3D/Network/Network.h"
%include "Urho3D/Network/NetworkPriority.h"
%include "Urho3D/Network/Protocol.h"
#endif

//// --------------------------------------- Physics ---------------------------------------
#if defined(URHO3D_PHYSICS)
//%ignore Urho3D::TriangleMeshData::meshInterface_;
//%ignore Urho3D::TriangleMeshData::shape_;
//%ignore Urho3D::TriangleMeshData::infoMap_;
//%ignore Urho3D::GImpactMeshData::meshInterface_;
//
//%include "Urho3D/Physics/CollisionShape.h"
//%include "Urho3D/Physics/Constraint.h"
//%include "Urho3D/Physics/PhysicsWorld.h"
//%include "Urho3D/Physics/RaycastVehicle.h"
//%include "Urho3D/Physics/RigidBody.h"
//
#endif
// --------------------------------------- SystemUI ---------------------------------------
#if defined(URHO3D_SYSTEMUI)
%apply void* VOID_INT_PTR {
	ImFont*
}

%ignore ToImGui;
%ignore ToIntVector2;
%ignore ToIntRect;
%ignore ImGui::IsMouseDown;
%ignore ImGui::IsMouseDoubleClicked;
%ignore ImGui::IsMouseDragging;
%ignore ImGui::IsMouseReleased;
%ignore ImGui::IsMouseClicked;
%ignore ImGui::IsItemClicked;
%ignore ImGui::SetDragDropVariant;
%ignore ImGui::AcceptDragDropVariant;
%ignore ImGui::dpx;
%ignore ImGui::dpy;
%ignore ImGui::dp;
%ignore ImGui::pdpx;
%ignore ImGui::pdpy;
%ignore ImGui::pdp;
%apply unsigned short INPUT[] { ImWchar* };

%include "Urho3D/SystemUI/Console.h"
%include "Urho3D/SystemUI/DebugHud.h"
%include "Urho3D/SystemUI/SystemMessageBox.h"
%include "Urho3D/SystemUI/SystemUI.h"
#endif
// --------------------------------------- UI ---------------------------------------
%ignore Urho3D::UIElement::GetBatches;
%ignore Urho3D::UIElement::GetDebugDrawBatches;
%ignore Urho3D::UIElement::GetBatchesWithOffset;

%include "Urho3D/UI/UI.h"
//%include "Urho3D/UI/UIBatch.h"
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

// --------------------------------------- Urho2D ---------------------------------------
#if URHO3D_URHO2D
%template(Sprite2DMap) Urho3D::HashMap<Urho3D::String, Urho3D::SharedPtr<Urho3D::Sprite2D>>;
%template(PhysicsRaycastResult2DArray) Urho3D::PODVector<Urho3D::PhysicsRaycastResult2D>;
%template(RigitBody2DArray) Urho3D::PODVector<Urho3D::RigidBody2D*>;
%template(MaterialVector) Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Material>>;
%template(TileMapObject2DVector) Urho3D::Vector<Urho3D::SharedPtr<Urho3D::TileMapObject2D>>;

%ignore Urho3D::AnimationSet2D::GetSpriterData;
%ignore Urho3D::PhysicsWorld2D::DrawTransform;

// SWIG applies `override new` modifier by mistake.
%csmethodmodifiers Urho3D::Drawable2D::OnSceneSet "protected override";
%csmethodmodifiers Urho3D::Drawable2D::OnMarkedDirty "protected override";

%apply float { float32 };
%apply int { int32 };
%apply void* VOID_INT_PTR {
	b2Body*,
	b2Contact*,
	b2Fixture*,
	b2Joint*,
	b2Manifold*,
	b2World*
}

// b2Draw implementation
%ignore Urho3D::PhysicsWorld2D::DrawPolygon;
%ignore Urho3D::PhysicsWorld2D::DrawSolidPolygon;
%ignore Urho3D::PhysicsWorld2D::DrawCircle;
%ignore Urho3D::PhysicsWorld2D::DrawSolidCircle;
%ignore Urho3D::PhysicsWorld2D::DrawSegment;
%ignore Urho3D::PhysicsWorld2D::DrawTransform;
%ignore Urho3D::PhysicsWorld2D::DrawPoint;

%ignore Urho3D::ViewBatchInfo2D;
%ignore Urho3D::SourceBatch2D;
%ignore Urho3D::Vertex2D;
%ignore Urho3D::Drawable2D::GetSourceBatches;
%ignore Urho3D::TileMap2D::SetTmxFile;
%ignore Urho3D::TileMapLayer2D::Initialize;
%ignore Urho3D::TileMapLayer2D::GetTmxLayer;
%ignore Urho3D::Drawable2D::sourceBatches_;

%include "Urho3D/Urho2D/Drawable2D.h"
%include "Urho3D/Urho2D/StaticSprite2D.h"
%include "Urho3D/Urho2D/AnimatedSprite2D.h"
%include "Urho3D/Urho2D/TileMapDefs2D.h"
%include "Urho3D/Urho2D/AnimationSet2D.h"
%include "Urho3D/Urho2D/ParticleEffect2D.h"
%include "Urho3D/Urho2D/Renderer2D.h"
%include "Urho3D/Urho2D/SpriteSheet2D.h"
%include "Urho3D/Urho2D/TileMapLayer2D.h"
%include "Urho3D/Urho2D/ParticleEmitter2D.h"
%include "Urho3D/Urho2D/Sprite2D.h"
%include "Urho3D/Urho2D/StretchableSprite2D.h"
%include "Urho3D/Urho2D/TileMap2D.h"

%include "Urho3D/Urho2D/CollisionShape2D.h"
%include "Urho3D/Urho2D/CollisionPolygon2D.h"
%include "Urho3D/Urho2D/CollisionEdge2D.h"
%include "Urho3D/Urho2D/CollisionChain2D.h"
%include "Urho3D/Urho2D/CollisionCircle2D.h"
%include "Urho3D/Urho2D/CollisionBox2D.h"
%include "Urho3D/Urho2D/Constraint2D.h"
%include "Urho3D/Urho2D/ConstraintFriction2D.h"
%include "Urho3D/Urho2D/ConstraintPulley2D.h"
%include "Urho3D/Urho2D/ConstraintGear2D.h"
%include "Urho3D/Urho2D/ConstraintRevolute2D.h"
%include "Urho3D/Urho2D/ConstraintMotor2D.h"
%include "Urho3D/Urho2D/ConstraintRope2D.h"
%include "Urho3D/Urho2D/ConstraintMouse2D.h"
%include "Urho3D/Urho2D/ConstraintWeld2D.h"
%include "Urho3D/Urho2D/ConstraintDistance2D.h"
%include "Urho3D/Urho2D/ConstraintPrismatic2D.h"
%include "Urho3D/Urho2D/ConstraintWheel2D.h"
%include "Urho3D/Urho2D/RigidBody2D.h"
%include "Urho3D/Urho2D/PhysicsWorld2D.h"

#endif
