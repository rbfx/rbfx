%typemap(cscode) Urho3D::AttributeAnimationInfo %{
  public $typemap(cstype, const Urho3D::AttributeInfo &) AttributeInfo {
    get { return GetAttributeInfo(); }
  }
%}
%csmethodmodifiers Urho3D::AttributeAnimationInfo::GetAttributeInfo "private";
%typemap(cscode) Urho3D::Animatable %{
  public $typemap(cstype, bool) AnimationEnabled {
    get { return GetAnimationEnabled(); }
    set { SetAnimationEnabled(value); }
  }
  public $typemap(cstype, Urho3D::ObjectAnimation *) ObjectAnimation {
    get { return GetObjectAnimation(); }
    set { SetObjectAnimation(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) ObjectAnimationAttr {
    get { return GetObjectAnimationAttr(); }
  }
%}
%csmethodmodifiers Urho3D::Animatable::GetAnimationEnabled "private";
%csmethodmodifiers Urho3D::Animatable::SetAnimationEnabled "private";
%csmethodmodifiers Urho3D::Animatable::GetObjectAnimation "private";
%csmethodmodifiers Urho3D::Animatable::SetObjectAnimation "private";
%csmethodmodifiers Urho3D::Animatable::GetObjectAnimationAttr "private";
%typemap(cscode) Urho3D::CameraViewport %{
  public $typemap(cstype, Urho3D::Rect) NormalizedRect {
    get { return GetNormalizedRect(); }
  }
  public $typemap(cstype, const Urho3D::ResourceRef &) LastRenderPath {
    get { return GetLastRenderPath(); }
  }
  public $typemap(cstype, Urho3D::Viewport *) Viewport {
    get { return GetViewport(); }
  }
  public $typemap(cstype, Urho3D::IntRect) ScreenRect {
    get { return GetScreenRect(); }
  }
%}
%csmethodmodifiers Urho3D::CameraViewport::GetNormalizedRect "private";
%csmethodmodifiers Urho3D::CameraViewport::GetLastRenderPath "private";
%csmethodmodifiers Urho3D::CameraViewport::GetViewport "private";
%csmethodmodifiers Urho3D::CameraViewport::GetScreenRect "private";
%typemap(cscode) Urho3D::Component %{
  public $typemap(cstype, unsigned int) ID {
    get { return GetID(); }
  }
  public $typemap(cstype, Urho3D::Node *) Node {
    get { return GetNode(); }
  }
  public $typemap(cstype, Urho3D::Scene *) Scene {
    get { return GetScene(); }
  }
%}
%csmethodmodifiers Urho3D::Component::GetID "private";
%csmethodmodifiers Urho3D::Component::GetNode "private";
%csmethodmodifiers Urho3D::Component::GetScene "private";
%typemap(cscode) Urho3D::LogicComponent %{
  public $typemap(cstype, Urho3D::UpdateEvent) UpdateEventMask {
    get { return GetUpdateEventMask(); }
    set { SetUpdateEventMask(value); }
  }
%}
%csmethodmodifiers Urho3D::LogicComponent::GetUpdateEventMask "private";
%csmethodmodifiers Urho3D::LogicComponent::SetUpdateEventMask "private";
%typemap(cscode) Urho3D::Node %{
  public $typemap(cstype, unsigned int) ID {
    get { return GetID(); }
    set { SetID(value); }
  }
  /*public _typemap(cstype, entt::entity) Entity {
    get { return GetEntity(); }
    set { SetEntity(value); }
  }*/
  public $typemap(cstype, const eastl::string &) Name {
    get { return GetName(); }
    set { SetName(value); }
  }
  public $typemap(cstype, Urho3D::StringHash) NameHash {
    get { return GetNameHash(); }
  }
  public $typemap(cstype, const eastl::vector<eastl::string> &) Tags {
    get { return GetTags(); }
    set { SetTags(value); }
  }
  public $typemap(cstype, Urho3D::Node *) Parent {
    get { return GetParent(); }
    set { SetParent(value); }
  }
  public $typemap(cstype, Urho3D::Scene *) Scene {
    get { return GetScene(); }
    set { SetScene(value); }
  }
  public $typemap(cstype, Urho3D::Connection *) Owner {
    get { return GetOwner(); }
    set { SetOwner(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) Position {
    get { return GetPosition(); }
    set { SetPosition(value); }
  }
  public $typemap(cstype, Urho3D::Vector2) Position2D {
    get { return GetPosition2D(); }
  }
  public $typemap(cstype, const Urho3D::Quaternion &) Rotation {
    get { return GetRotation(); }
    set { SetRotation(value); }
  }
  public $typemap(cstype, float) Rotation2D {
    get { return GetRotation2D(); }
    set { SetRotation2D(value); }
  }
  public $typemap(cstype, Urho3D::Vector3) Direction {
    get { return GetDirection(); }
  }
  public $typemap(cstype, Urho3D::Vector3) Up {
    get { return GetUp(); }
  }
  public $typemap(cstype, Urho3D::Vector3) Right {
    get { return GetRight(); }
  }
  public $typemap(cstype, Urho3D::Matrix3x4) Transform {
    get { return GetTransform(); }
  }
  public $typemap(cstype, Urho3D::Vector3) WorldPosition {
    get { return GetWorldPosition(); }
  }
  public $typemap(cstype, Urho3D::Vector2) WorldPosition2D {
    get { return GetWorldPosition2D(); }
  }
  public $typemap(cstype, Urho3D::Quaternion) WorldRotation {
    get { return GetWorldRotation(); }
  }
  public $typemap(cstype, float) WorldRotation2D {
    get { return GetWorldRotation2D(); }
    set { SetWorldRotation2D(value); }
  }
  public $typemap(cstype, Urho3D::Vector3) WorldDirection {
    get { return GetWorldDirection(); }
  }
  public $typemap(cstype, Urho3D::Vector3) WorldUp {
    get { return GetWorldUp(); }
  }
  public $typemap(cstype, Urho3D::Vector3) WorldRight {
    get { return GetWorldRight(); }
  }
  public $typemap(cstype, Urho3D::Vector3) WorldScale {
    get { return GetWorldScale(); }
  }
  public $typemap(cstype, Urho3D::Vector3) SignedWorldScale {
    get { return GetSignedWorldScale(); }
  }
  public $typemap(cstype, Urho3D::Vector2) WorldScale2D {
    get { return GetWorldScale2D(); }
  }
  public $typemap(cstype, const Urho3D::Matrix3x4 &) WorldTransform {
    get { return GetWorldTransform(); }
    set { SetWorldTransform(value); }
  }
  public $typemap(cstype, unsigned int) NumComponents {
    get { return GetNumComponents(); }
  }
  public $typemap(cstype, unsigned int) NumNetworkComponents {
    get { return GetNumNetworkComponents(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::WeakPtr<Urho3D::Component>>) Listeners {
    get { return GetListeners(); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant> &) Vars {
    get { return GetVars(); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) NetPositionAttr {
    get { return GetNetPositionAttr(); }
    set { SetNetPositionAttr(value); }
  }
  public $typemap(cstype, const eastl::vector<unsigned char> &) NetRotationAttr {
    get { return GetNetRotationAttr(); }
    set { SetNetRotationAttr(value); }
  }
  public $typemap(cstype, const eastl::vector<unsigned char> &) NetParentAttr {
    get { return GetNetParentAttr(); }
    set { SetNetParentAttr(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Node *> &) DependencyNodes {
    get { return GetDependencyNodes(); }
  }
  public $typemap(cstype, unsigned int) NumPersistentChildren {
    get { return GetNumPersistentChildren(); }
  }
  public $typemap(cstype, unsigned int) NumPersistentComponents {
    get { return GetNumPersistentComponents(); }
  }
%}
%csmethodmodifiers Urho3D::Node::GetID "private";
%csmethodmodifiers Urho3D::Node::SetID "private";
%csmethodmodifiers Urho3D::Node::GetEntity "private";
%csmethodmodifiers Urho3D::Node::SetEntity "private";
%csmethodmodifiers Urho3D::Node::GetName "private";
%csmethodmodifiers Urho3D::Node::SetName "private";
%csmethodmodifiers Urho3D::Node::GetNameHash "private";
%csmethodmodifiers Urho3D::Node::GetTags "private";
%csmethodmodifiers Urho3D::Node::SetTags "private";
%csmethodmodifiers Urho3D::Node::GetParent "private";
%csmethodmodifiers Urho3D::Node::SetParent "private";
%csmethodmodifiers Urho3D::Node::GetScene "private";
%csmethodmodifiers Urho3D::Node::SetScene "private";
%csmethodmodifiers Urho3D::Node::GetOwner "private";
%csmethodmodifiers Urho3D::Node::SetOwner "private";
%csmethodmodifiers Urho3D::Node::GetPosition "private";
%csmethodmodifiers Urho3D::Node::SetPosition "private";
%csmethodmodifiers Urho3D::Node::GetPosition2D "private";
%csmethodmodifiers Urho3D::Node::GetRotation "private";
%csmethodmodifiers Urho3D::Node::SetRotation "private";
%csmethodmodifiers Urho3D::Node::GetRotation2D "private";
%csmethodmodifiers Urho3D::Node::SetRotation2D "private";
%csmethodmodifiers Urho3D::Node::GetDirection "private";
%csmethodmodifiers Urho3D::Node::GetUp "private";
%csmethodmodifiers Urho3D::Node::GetRight "private";
%csmethodmodifiers Urho3D::Node::GetTransform "private";
%csmethodmodifiers Urho3D::Node::GetWorldPosition "private";
%csmethodmodifiers Urho3D::Node::GetWorldPosition2D "private";
%csmethodmodifiers Urho3D::Node::GetWorldRotation "private";
%csmethodmodifiers Urho3D::Node::GetWorldRotation2D "private";
%csmethodmodifiers Urho3D::Node::SetWorldRotation2D "private";
%csmethodmodifiers Urho3D::Node::GetWorldDirection "private";
%csmethodmodifiers Urho3D::Node::GetWorldUp "private";
%csmethodmodifiers Urho3D::Node::GetWorldRight "private";
%csmethodmodifiers Urho3D::Node::GetWorldScale "private";
%csmethodmodifiers Urho3D::Node::GetSignedWorldScale "private";
%csmethodmodifiers Urho3D::Node::GetWorldScale2D "private";
%csmethodmodifiers Urho3D::Node::GetWorldTransform "private";
%csmethodmodifiers Urho3D::Node::SetWorldTransform "private";
%csmethodmodifiers Urho3D::Node::GetNumComponents "private";
%csmethodmodifiers Urho3D::Node::GetNumNetworkComponents "private";
%csmethodmodifiers Urho3D::Node::GetListeners "private";
%csmethodmodifiers Urho3D::Node::GetVars "private";
%csmethodmodifiers Urho3D::Node::GetNetPositionAttr "private";
%csmethodmodifiers Urho3D::Node::SetNetPositionAttr "private";
%csmethodmodifiers Urho3D::Node::GetNetRotationAttr "private";
%csmethodmodifiers Urho3D::Node::SetNetRotationAttr "private";
%csmethodmodifiers Urho3D::Node::GetNetParentAttr "private";
%csmethodmodifiers Urho3D::Node::SetNetParentAttr "private";
%csmethodmodifiers Urho3D::Node::GetDependencyNodes "private";
%csmethodmodifiers Urho3D::Node::GetNumPersistentChildren "private";
%csmethodmodifiers Urho3D::Node::GetNumPersistentComponents "private";
%typemap(cscode) Urho3D::ObjectAnimation %{
  public $typemap(cstype, const eastl::unordered_map<eastl::string, Urho3D::SharedPtr<Urho3D::ValueAnimationInfo>> &) AttributeAnimationInfos {
    get { return GetAttributeAnimationInfos(); }
  }
%}
%csmethodmodifiers Urho3D::ObjectAnimation::GetAttributeAnimationInfos "private";
%typemap(cscode) Urho3D::Scene %{
  /*public _typemap(cstype, entt::basic_registry<entt::entity> &) Registry {
    get { return GetRegistry(); }
  }*/
  public $typemap(cstype, float) AsyncProgress {
    get { return GetAsyncProgress(); }
  }
  public $typemap(cstype, Urho3D::LoadMode) AsyncLoadMode {
    get { return GetAsyncLoadMode(); }
  }
  public $typemap(cstype, const eastl::string &) FileName {
    get { return GetFileName(); }
  }
  public $typemap(cstype, unsigned int) Checksum {
    get { return GetChecksum(); }
  }
  public $typemap(cstype, float) TimeScale {
    get { return GetTimeScale(); }
    set { SetTimeScale(value); }
  }
  public $typemap(cstype, float) ElapsedTime {
    get { return GetElapsedTime(); }
    set { SetElapsedTime(value); }
  }
  public $typemap(cstype, float) SmoothingConstant {
    get { return GetSmoothingConstant(); }
    set { SetSmoothingConstant(value); }
  }
  public $typemap(cstype, float) SnapThreshold {
    get { return GetSnapThreshold(); }
    set { SetSnapThreshold(value); }
  }
  public $typemap(cstype, int) AsyncLoadingMs {
    get { return GetAsyncLoadingMs(); }
    set { SetAsyncLoadingMs(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::SharedPtr<Urho3D::PackageFile>> &) RequiredPackageFiles {
    get { return GetRequiredPackageFiles(); }
  }
  public $typemap(cstype, eastl::string) VarNamesAttr {
    get { return GetVarNamesAttr(); }
  }
%}
%csmethodmodifiers Urho3D::Scene::GetRegistry "private";
%csmethodmodifiers Urho3D::Scene::GetAsyncProgress "private";
%csmethodmodifiers Urho3D::Scene::GetAsyncLoadMode "private";
%csmethodmodifiers Urho3D::Scene::GetFileName "private";
%csmethodmodifiers Urho3D::Scene::GetChecksum "private";
%csmethodmodifiers Urho3D::Scene::GetTimeScale "private";
%csmethodmodifiers Urho3D::Scene::SetTimeScale "private";
%csmethodmodifiers Urho3D::Scene::GetElapsedTime "private";
%csmethodmodifiers Urho3D::Scene::SetElapsedTime "private";
%csmethodmodifiers Urho3D::Scene::GetSmoothingConstant "private";
%csmethodmodifiers Urho3D::Scene::SetSmoothingConstant "private";
%csmethodmodifiers Urho3D::Scene::GetSnapThreshold "private";
%csmethodmodifiers Urho3D::Scene::SetSnapThreshold "private";
%csmethodmodifiers Urho3D::Scene::GetAsyncLoadingMs "private";
%csmethodmodifiers Urho3D::Scene::SetAsyncLoadingMs "private";
%csmethodmodifiers Urho3D::Scene::GetRequiredPackageFiles "private";
%csmethodmodifiers Urho3D::Scene::GetVarNamesAttr "private";
%typemap(cscode) Urho3D::SceneManager %{
  public $typemap(cstype, Urho3D::Scene *) ActiveScene {
    get { return GetActiveScene(); }
    set { SetActiveScene(value); }
  }
%}
%csmethodmodifiers Urho3D::SceneManager::GetActiveScene "private";
%csmethodmodifiers Urho3D::SceneManager::SetActiveScene "private";
%typemap(cscode) Urho3D::Serializable %{
  public $typemap(cstype, unsigned int) NumAttributes {
    get { return GetNumAttributes(); }
  }
  public $typemap(cstype, unsigned int) NumNetworkAttributes {
    get { return GetNumNetworkAttributes(); }
  }
  public $typemap(cstype, Urho3D::NetworkState *) NetworkState {
    get { return GetNetworkState(); }
  }
%}
%csmethodmodifiers Urho3D::Serializable::GetNumAttributes "private";
%csmethodmodifiers Urho3D::Serializable::GetNumNetworkAttributes "private";
%csmethodmodifiers Urho3D::Serializable::GetNetworkState "private";
%typemap(cscode) Urho3D::SmoothedTransform %{
  public $typemap(cstype, const Urho3D::Vector3 &) TargetPosition {
    get { return GetTargetPosition(); }
    set { SetTargetPosition(value); }
  }
  public $typemap(cstype, const Urho3D::Quaternion &) TargetRotation {
    get { return GetTargetRotation(); }
    set { SetTargetRotation(value); }
  }
  public $typemap(cstype, Urho3D::Vector3) TargetWorldPosition {
    get { return GetTargetWorldPosition(); }
  }
  public $typemap(cstype, Urho3D::Quaternion) TargetWorldRotation {
    get { return GetTargetWorldRotation(); }
  }
%}
%csmethodmodifiers Urho3D::SmoothedTransform::GetTargetPosition "private";
%csmethodmodifiers Urho3D::SmoothedTransform::SetTargetPosition "private";
%csmethodmodifiers Urho3D::SmoothedTransform::GetTargetRotation "private";
%csmethodmodifiers Urho3D::SmoothedTransform::SetTargetRotation "private";
%csmethodmodifiers Urho3D::SmoothedTransform::GetTargetWorldPosition "private";
%csmethodmodifiers Urho3D::SmoothedTransform::GetTargetWorldRotation "private";
%typemap(cscode) Urho3D::SplinePath %{
  public $typemap(cstype, Urho3D::InterpolationMode) InterpolationMode {
    get { return GetInterpolationMode(); }
    set { SetInterpolationMode(value); }
  }
  public $typemap(cstype, float) Speed {
    get { return GetSpeed(); }
    set { SetSpeed(value); }
  }
  public $typemap(cstype, float) Length {
    get { return GetLength(); }
  }
  public $typemap(cstype, Urho3D::Vector3) Position {
    get { return GetPosition(); }
  }
  public $typemap(cstype, Urho3D::Node *) ControlledNode {
    get { return GetControlledNode(); }
    set { SetControlledNode(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Variant> &) ControlPointIdsAttr {
    get { return GetControlPointIdsAttr(); }
    set { SetControlPointIdsAttr(value); }
  }
  public $typemap(cstype, unsigned int) ControlledIdAttr {
    get { return GetControlledIdAttr(); }
    set { SetControlledIdAttr(value); }
  }
%}
%csmethodmodifiers Urho3D::SplinePath::GetInterpolationMode "private";
%csmethodmodifiers Urho3D::SplinePath::SetInterpolationMode "private";
%csmethodmodifiers Urho3D::SplinePath::GetSpeed "private";
%csmethodmodifiers Urho3D::SplinePath::SetSpeed "private";
%csmethodmodifiers Urho3D::SplinePath::GetLength "private";
%csmethodmodifiers Urho3D::SplinePath::GetPosition "private";
%csmethodmodifiers Urho3D::SplinePath::GetControlledNode "private";
%csmethodmodifiers Urho3D::SplinePath::SetControlledNode "private";
%csmethodmodifiers Urho3D::SplinePath::GetControlPointIdsAttr "private";
%csmethodmodifiers Urho3D::SplinePath::SetControlPointIdsAttr "private";
%csmethodmodifiers Urho3D::SplinePath::GetControlledIdAttr "private";
%csmethodmodifiers Urho3D::SplinePath::SetControlledIdAttr "private";
%typemap(cscode) Urho3D::UnknownComponent %{
  public $typemap(cstype, const eastl::vector<eastl::string> &) XMLAttributes {
    get { return GetXMLAttributes(); }
  }
  public $typemap(cstype, const eastl::vector<unsigned char> &) BinaryAttributes {
    get { return GetBinaryAttributes(); }
  }
  public $typemap(cstype, bool) UseXML {
    get { return GetUseXML(); }
  }
%}
%csmethodmodifiers Urho3D::UnknownComponent::GetXMLAttributes "private";
%csmethodmodifiers Urho3D::UnknownComponent::GetBinaryAttributes "private";
%csmethodmodifiers Urho3D::UnknownComponent::GetUseXML "private";
%typemap(cscode) Urho3D::ValueAnimation %{
  public $typemap(cstype, void *) Owner {
    get { return GetOwner(); }
    set { SetOwner(value); }
  }
  public $typemap(cstype, Urho3D::InterpMethod) InterpolationMethod {
    get { return GetInterpolationMethod(); }
    set { SetInterpolationMethod(value); }
  }
  public $typemap(cstype, float) SplineTension {
    get { return GetSplineTension(); }
    set { SetSplineTension(value); }
  }
  public $typemap(cstype, Urho3D::VariantType) ValueType {
    get { return GetValueType(); }
    set { SetValueType(value); }
  }
  public $typemap(cstype, float) BeginTime {
    get { return GetBeginTime(); }
  }
  public $typemap(cstype, float) EndTime {
    get { return GetEndTime(); }
  }
  /*public _typemap(cstype, const eastl::vector<Urho3D::VAnimKeyFrame> &) KeyFrames {
    get { return GetKeyFrames(); }
  }*/
%}
%csmethodmodifiers Urho3D::ValueAnimation::GetOwner "private";
%csmethodmodifiers Urho3D::ValueAnimation::SetOwner "private";
%csmethodmodifiers Urho3D::ValueAnimation::GetInterpolationMethod "private";
%csmethodmodifiers Urho3D::ValueAnimation::SetInterpolationMethod "private";
%csmethodmodifiers Urho3D::ValueAnimation::GetSplineTension "private";
%csmethodmodifiers Urho3D::ValueAnimation::SetSplineTension "private";
%csmethodmodifiers Urho3D::ValueAnimation::GetValueType "private";
%csmethodmodifiers Urho3D::ValueAnimation::SetValueType "private";
%csmethodmodifiers Urho3D::ValueAnimation::GetBeginTime "private";
%csmethodmodifiers Urho3D::ValueAnimation::GetEndTime "private";
%csmethodmodifiers Urho3D::ValueAnimation::GetKeyFrames "private";
%typemap(cscode) Urho3D::ValueAnimationInfo %{
  public $typemap(cstype, Urho3D::Object *) Target {
    get { return GetTarget(); }
  }
  public $typemap(cstype, Urho3D::ValueAnimation *) Animation {
    get { return GetAnimation(); }
  }
  public $typemap(cstype, Urho3D::WrapMode) WrapMode {
    get { return GetWrapMode(); }
    set { SetWrapMode(value); }
  }
  public $typemap(cstype, float) Time {
    get { return GetTime(); }
    set { SetTime(value); }
  }
  public $typemap(cstype, float) Speed {
    get { return GetSpeed(); }
    set { SetSpeed(value); }
  }
%}
%csmethodmodifiers Urho3D::ValueAnimationInfo::GetTarget "private";
%csmethodmodifiers Urho3D::ValueAnimationInfo::GetAnimation "private";
%csmethodmodifiers Urho3D::ValueAnimationInfo::GetWrapMode "private";
%csmethodmodifiers Urho3D::ValueAnimationInfo::SetWrapMode "private";
%csmethodmodifiers Urho3D::ValueAnimationInfo::GetTime "private";
%csmethodmodifiers Urho3D::ValueAnimationInfo::SetTime "private";
%csmethodmodifiers Urho3D::ValueAnimationInfo::GetSpeed "private";
%csmethodmodifiers Urho3D::ValueAnimationInfo::SetSpeed "private";
