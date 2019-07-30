%typemap(cscode) Urho3D::AnimatedModel %{
  public $typemap(cstype, Urho3D::Skeleton &) Skeleton {
    get { return GetSkeleton(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::SharedPtr<Urho3D::AnimationState>> &) AnimationStates {
    get { return GetAnimationStates(); }
  }
  public $typemap(cstype, unsigned int) NumAnimationStates {
    get { return GetNumAnimationStates(); }
  }
  public $typemap(cstype, float) AnimationLodBias {
    get { return GetAnimationLodBias(); }
    set { SetAnimationLodBias(value); }
  }
  public $typemap(cstype, bool) UpdateInvisible {
    get { return GetUpdateInvisible(); }
    set { SetUpdateInvisible(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::ModelMorph> &) Morphs {
    get { return GetMorphs(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::SharedPtr<Urho3D::VertexBuffer>> &) MorphVertexBuffers {
    get { return GetMorphVertexBuffers(); }
  }
  public $typemap(cstype, unsigned int) NumMorphs {
    get { return GetNumMorphs(); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) ModelAttr {
    get { return GetModelAttr(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) BonesEnabledAttr {
    get { return GetBonesEnabledAttr(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) AnimationStatesAttr {
    get { return GetAnimationStatesAttr(); }
  }
  public $typemap(cstype, const eastl::vector<unsigned char> &) MorphsAttr {
    get { return GetMorphsAttr(); }
    set { SetMorphsAttr(value); }
  }
  public $typemap(cstype, const eastl::vector<eastl::vector<unsigned int>> &) GeometryBoneMappings {
    get { return GetGeometryBoneMappings(); }
  }
  public $typemap(cstype, const eastl::vector<eastl::vector<Urho3D::Matrix3x4>> &) GeometrySkinMatrices {
    get { return GetGeometrySkinMatrices(); }
  }
%}
%csmethodmodifiers Urho3D::AnimatedModel::GetSkeleton "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetAnimationStates "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetNumAnimationStates "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetAnimationLodBias "private";
%csmethodmodifiers Urho3D::AnimatedModel::SetAnimationLodBias "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetUpdateInvisible "private";
%csmethodmodifiers Urho3D::AnimatedModel::SetUpdateInvisible "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetMorphs "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetMorphVertexBuffers "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetNumMorphs "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetModelAttr "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetBonesEnabledAttr "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetAnimationStatesAttr "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetMorphsAttr "private";
%csmethodmodifiers Urho3D::AnimatedModel::SetMorphsAttr "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetGeometryBoneMappings "private";
%csmethodmodifiers Urho3D::AnimatedModel::GetGeometrySkinMatrices "private";
%typemap(cscode) Urho3D::Animation %{
  public $typemap(cstype, const eastl::string &) AnimationName {
    get { return GetAnimationName(); }
    set { SetAnimationName(value); }
  }
  public $typemap(cstype, Urho3D::StringHash) AnimationNameHash {
    get { return GetAnimationNameHash(); }
  }
  public $typemap(cstype, float) Length {
    get { return GetLength(); }
    set { SetLength(value); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::AnimationTrack> &) Tracks {
    get { return GetTracks(); }
  }
  public $typemap(cstype, unsigned int) NumTracks {
    get { return GetNumTracks(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::AnimationTriggerPoint> &) Triggers {
    get { return GetTriggers(); }
  }
  public $typemap(cstype, unsigned int) NumTriggers {
    get { return GetNumTriggers(); }
    set { SetNumTriggers(value); }
  }
%}
%csmethodmodifiers Urho3D::Animation::GetAnimationName "private";
%csmethodmodifiers Urho3D::Animation::SetAnimationName "private";
%csmethodmodifiers Urho3D::Animation::GetAnimationNameHash "private";
%csmethodmodifiers Urho3D::Animation::GetLength "private";
%csmethodmodifiers Urho3D::Animation::SetLength "private";
%csmethodmodifiers Urho3D::Animation::GetTracks "private";
%csmethodmodifiers Urho3D::Animation::GetNumTracks "private";
%csmethodmodifiers Urho3D::Animation::GetTriggers "private";
%csmethodmodifiers Urho3D::Animation::GetNumTriggers "private";
%csmethodmodifiers Urho3D::Animation::SetNumTriggers "private";
%typemap(cscode) Urho3D::AnimationController %{
  public $typemap(cstype, const eastl::vector<Urho3D::AnimationControl> &) Animations {
    get { return GetAnimations(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) AnimationsAttr {
    get { return GetAnimationsAttr(); }
  }
  public $typemap(cstype, const eastl::vector<unsigned char> &) NetAnimationsAttr {
    get { return GetNetAnimationsAttr(); }
    set { SetNetAnimationsAttr(value); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) NodeAnimationStatesAttr {
    get { return GetNodeAnimationStatesAttr(); }
  }
%}
%csmethodmodifiers Urho3D::AnimationController::GetAnimations "private";
%csmethodmodifiers Urho3D::AnimationController::GetAnimationsAttr "private";
%csmethodmodifiers Urho3D::AnimationController::GetNetAnimationsAttr "private";
%csmethodmodifiers Urho3D::AnimationController::SetNetAnimationsAttr "private";
%csmethodmodifiers Urho3D::AnimationController::GetNodeAnimationStatesAttr "private";
%typemap(cscode) Urho3D::AnimationState %{
  public $typemap(cstype, Urho3D::Animation *) Animation {
    get { return GetAnimation(); }
  }
  public $typemap(cstype, Urho3D::AnimatedModel *) Model {
    get { return GetModel(); }
  }
  public $typemap(cstype, Urho3D::Node *) Node {
    get { return GetNode(); }
  }
  public $typemap(cstype, Urho3D::Bone *) StartBone {
    get { return GetStartBone(); }
    set { SetStartBone(value); }
  }
  public $typemap(cstype, float) Weight {
    get { return GetWeight(); }
    set { SetWeight(value); }
  }
  public $typemap(cstype, Urho3D::AnimationBlendMode) BlendMode {
    get { return GetBlendMode(); }
    set { SetBlendMode(value); }
  }
  public $typemap(cstype, float) Time {
    get { return GetTime(); }
    set { SetTime(value); }
  }
  public $typemap(cstype, float) Length {
    get { return GetLength(); }
  }
  public $typemap(cstype, unsigned char) Layer {
    get { return GetLayer(); }
    set { SetLayer(value); }
  }
%}
%csmethodmodifiers Urho3D::AnimationState::GetAnimation "private";
%csmethodmodifiers Urho3D::AnimationState::GetModel "private";
%csmethodmodifiers Urho3D::AnimationState::GetNode "private";
%csmethodmodifiers Urho3D::AnimationState::GetStartBone "private";
%csmethodmodifiers Urho3D::AnimationState::SetStartBone "private";
%csmethodmodifiers Urho3D::AnimationState::GetWeight "private";
%csmethodmodifiers Urho3D::AnimationState::SetWeight "private";
%csmethodmodifiers Urho3D::AnimationState::GetBlendMode "private";
%csmethodmodifiers Urho3D::AnimationState::SetBlendMode "private";
%csmethodmodifiers Urho3D::AnimationState::GetTime "private";
%csmethodmodifiers Urho3D::AnimationState::SetTime "private";
%csmethodmodifiers Urho3D::AnimationState::GetLength "private";
%csmethodmodifiers Urho3D::AnimationState::GetLayer "private";
%csmethodmodifiers Urho3D::AnimationState::SetLayer "private";
%typemap(cscode) Urho3D::BillboardSet %{
  public $typemap(cstype, Urho3D::Material *) Material {
    get { return GetMaterial(); }
    set { SetMaterial(value); }
  }
  public $typemap(cstype, unsigned int) NumBillboards {
    get { return GetNumBillboards(); }
    set { SetNumBillboards(value); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Billboard> &) Billboards {
    get { return GetBillboards(); }
  }
  public $typemap(cstype, Urho3D::FaceCameraMode) FaceCameraMode {
    get { return GetFaceCameraMode(); }
    set { SetFaceCameraMode(value); }
  }
  public $typemap(cstype, float) MinAngle {
    get { return GetMinAngle(); }
    set { SetMinAngle(value); }
  }
  public $typemap(cstype, float) AnimationLodBias {
    get { return GetAnimationLodBias(); }
    set { SetAnimationLodBias(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) MaterialAttr {
    get { return GetMaterialAttr(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) BillboardsAttr {
    get { return GetBillboardsAttr(); }
  }
  public $typemap(cstype, const eastl::vector<unsigned char> &) NetBillboardsAttr {
    get { return GetNetBillboardsAttr(); }
    set { SetNetBillboardsAttr(value); }
  }
%}
%csmethodmodifiers Urho3D::BillboardSet::GetMaterial "private";
%csmethodmodifiers Urho3D::BillboardSet::SetMaterial "private";
%csmethodmodifiers Urho3D::BillboardSet::GetNumBillboards "private";
%csmethodmodifiers Urho3D::BillboardSet::SetNumBillboards "private";
%csmethodmodifiers Urho3D::BillboardSet::GetBillboards "private";
%csmethodmodifiers Urho3D::BillboardSet::GetFaceCameraMode "private";
%csmethodmodifiers Urho3D::BillboardSet::SetFaceCameraMode "private";
%csmethodmodifiers Urho3D::BillboardSet::GetMinAngle "private";
%csmethodmodifiers Urho3D::BillboardSet::SetMinAngle "private";
%csmethodmodifiers Urho3D::BillboardSet::GetAnimationLodBias "private";
%csmethodmodifiers Urho3D::BillboardSet::SetAnimationLodBias "private";
%csmethodmodifiers Urho3D::BillboardSet::GetMaterialAttr "private";
%csmethodmodifiers Urho3D::BillboardSet::GetBillboardsAttr "private";
%csmethodmodifiers Urho3D::BillboardSet::GetNetBillboardsAttr "private";
%csmethodmodifiers Urho3D::BillboardSet::SetNetBillboardsAttr "private";
%typemap(cscode) Urho3D::Camera %{
  public $typemap(cstype, float) FarClip {
    get { return GetFarClip(); }
    set { SetFarClip(value); }
  }
  public $typemap(cstype, float) NearClip {
    get { return GetNearClip(); }
    set { SetNearClip(value); }
  }
  public $typemap(cstype, float) Fov {
    get { return GetFov(); }
    set { SetFov(value); }
  }
  public $typemap(cstype, float) OrthoSize {
    get { return GetOrthoSize(); }
    set { SetOrthoSize(value); }
  }
  public $typemap(cstype, float) AspectRatio {
    get { return GetAspectRatio(); }
    set { SetAspectRatio(value); }
  }
  public $typemap(cstype, float) Zoom {
    get { return GetZoom(); }
    set { SetZoom(value); }
  }
  public $typemap(cstype, float) LodBias {
    get { return GetLodBias(); }
    set { SetLodBias(value); }
  }
  public $typemap(cstype, unsigned int) ViewMask {
    get { return GetViewMask(); }
    set { SetViewMask(value); }
  }
  public $typemap(cstype, Urho3D::ViewOverride) ViewOverrideFlags {
    get { return GetViewOverrideFlags(); }
    set { SetViewOverrideFlags(value); }
  }
  public $typemap(cstype, Urho3D::FillMode) FillMode {
    get { return GetFillMode(); }
    set { SetFillMode(value); }
  }
  public $typemap(cstype, bool) AutoAspectRatio {
    get { return GetAutoAspectRatio(); }
    set { SetAutoAspectRatio(value); }
  }
  public $typemap(cstype, const Urho3D::Frustum &) Frustum {
    get { return GetFrustum(); }
  }
  public $typemap(cstype, Urho3D::Matrix4) Projection {
    get { return GetProjection(); }
  }
  public $typemap(cstype, Urho3D::Matrix4) GPUProjection {
    get { return GetGPUProjection(); }
  }
  public $typemap(cstype, const Urho3D::Matrix3x4 &) View {
    get { return GetView(); }
  }
  public $typemap(cstype, float) HalfViewSize {
    get { return GetHalfViewSize(); }
  }
  public $typemap(cstype, Urho3D::Frustum) ViewSpaceFrustum {
    get { return GetViewSpaceFrustum(); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) ProjectionOffset {
    get { return GetProjectionOffset(); }
    set { SetProjectionOffset(value); }
  }
  public $typemap(cstype, bool) UseReflection {
    get { return GetUseReflection(); }
    set { SetUseReflection(value); }
  }
  public $typemap(cstype, const Urho3D::Plane &) ReflectionPlane {
    get { return GetReflectionPlane(); }
    set { SetReflectionPlane(value); }
  }
  public $typemap(cstype, bool) UseClipping {
    get { return GetUseClipping(); }
    set { SetUseClipping(value); }
  }
  public $typemap(cstype, const Urho3D::Plane &) ClipPlane {
    get { return GetClipPlane(); }
    set { SetClipPlane(value); }
  }
  public $typemap(cstype, bool) FlipVertical {
    get { return GetFlipVertical(); }
    set { SetFlipVertical(value); }
  }
  public $typemap(cstype, bool) ReverseCulling {
    get { return GetReverseCulling(); }
  }
  public $typemap(cstype, Urho3D::Matrix3x4) EffectiveWorldTransform {
    get { return GetEffectiveWorldTransform(); }
  }
  public $typemap(cstype, Urho3D::Vector4) ReflectionPlaneAttr {
    get { return GetReflectionPlaneAttr(); }
  }
  public $typemap(cstype, Urho3D::Vector4) ClipPlaneAttr {
    get { return GetClipPlaneAttr(); }
  }
%}
%csmethodmodifiers Urho3D::Camera::GetFarClip "private";
%csmethodmodifiers Urho3D::Camera::SetFarClip "private";
%csmethodmodifiers Urho3D::Camera::GetNearClip "private";
%csmethodmodifiers Urho3D::Camera::SetNearClip "private";
%csmethodmodifiers Urho3D::Camera::GetFov "private";
%csmethodmodifiers Urho3D::Camera::SetFov "private";
%csmethodmodifiers Urho3D::Camera::GetOrthoSize "private";
%csmethodmodifiers Urho3D::Camera::SetOrthoSize "private";
%csmethodmodifiers Urho3D::Camera::GetAspectRatio "private";
%csmethodmodifiers Urho3D::Camera::SetAspectRatio "private";
%csmethodmodifiers Urho3D::Camera::GetZoom "private";
%csmethodmodifiers Urho3D::Camera::SetZoom "private";
%csmethodmodifiers Urho3D::Camera::GetLodBias "private";
%csmethodmodifiers Urho3D::Camera::SetLodBias "private";
%csmethodmodifiers Urho3D::Camera::GetViewMask "private";
%csmethodmodifiers Urho3D::Camera::SetViewMask "private";
%csmethodmodifiers Urho3D::Camera::GetViewOverrideFlags "private";
%csmethodmodifiers Urho3D::Camera::SetViewOverrideFlags "private";
%csmethodmodifiers Urho3D::Camera::GetFillMode "private";
%csmethodmodifiers Urho3D::Camera::SetFillMode "private";
%csmethodmodifiers Urho3D::Camera::GetAutoAspectRatio "private";
%csmethodmodifiers Urho3D::Camera::SetAutoAspectRatio "private";
%csmethodmodifiers Urho3D::Camera::GetFrustum "private";
%csmethodmodifiers Urho3D::Camera::GetProjection "private";
%csmethodmodifiers Urho3D::Camera::GetGPUProjection "private";
%csmethodmodifiers Urho3D::Camera::GetView "private";
%csmethodmodifiers Urho3D::Camera::GetHalfViewSize "private";
%csmethodmodifiers Urho3D::Camera::GetViewSpaceFrustum "private";
%csmethodmodifiers Urho3D::Camera::GetProjectionOffset "private";
%csmethodmodifiers Urho3D::Camera::SetProjectionOffset "private";
%csmethodmodifiers Urho3D::Camera::GetUseReflection "private";
%csmethodmodifiers Urho3D::Camera::SetUseReflection "private";
%csmethodmodifiers Urho3D::Camera::GetReflectionPlane "private";
%csmethodmodifiers Urho3D::Camera::SetReflectionPlane "private";
%csmethodmodifiers Urho3D::Camera::GetUseClipping "private";
%csmethodmodifiers Urho3D::Camera::SetUseClipping "private";
%csmethodmodifiers Urho3D::Camera::GetClipPlane "private";
%csmethodmodifiers Urho3D::Camera::SetClipPlane "private";
%csmethodmodifiers Urho3D::Camera::GetFlipVertical "private";
%csmethodmodifiers Urho3D::Camera::SetFlipVertical "private";
%csmethodmodifiers Urho3D::Camera::GetReverseCulling "private";
%csmethodmodifiers Urho3D::Camera::GetEffectiveWorldTransform "private";
%csmethodmodifiers Urho3D::Camera::GetReflectionPlaneAttr "private";
%csmethodmodifiers Urho3D::Camera::GetClipPlaneAttr "private";
%typemap(cscode) Urho3D::ConstantBuffer %{
  public $typemap(cstype, unsigned int) Size {
    get { return GetSize(); }
    set { SetSize(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstantBuffer::GetSize "private";
%csmethodmodifiers Urho3D::ConstantBuffer::SetSize "private";
%typemap(cscode) Urho3D::CustomGeometry %{
  public $typemap(cstype, unsigned int) NumGeometries {
    get { return GetNumGeometries(); }
    set { SetNumGeometries(value); }
  }
  public $typemap(cstype, eastl::vector<eastl::vector<Urho3D::CustomGeometryVertex>> &) Vertices {
    get { return GetVertices(); }
  }
  public $typemap(cstype, eastl::vector<unsigned char>) GeometryDataAttr {
    get { return GetGeometryDataAttr(); }
  }
  public $typemap(cstype, const Urho3D::ResourceRefList &) MaterialsAttr {
    get { return GetMaterialsAttr(); }
    set { SetMaterialsAttr(value); }
  }
%}
%csmethodmodifiers Urho3D::CustomGeometry::GetNumGeometries "private";
%csmethodmodifiers Urho3D::CustomGeometry::SetNumGeometries "private";
%csmethodmodifiers Urho3D::CustomGeometry::GetVertices "private";
%csmethodmodifiers Urho3D::CustomGeometry::GetGeometryDataAttr "private";
%csmethodmodifiers Urho3D::CustomGeometry::GetMaterialsAttr "private";
%csmethodmodifiers Urho3D::CustomGeometry::SetMaterialsAttr "private";
%typemap(cscode) Urho3D::DebugRenderer %{
  public $typemap(cstype, bool) LineAntiAlias {
    get { return GetLineAntiAlias(); }
    set { SetLineAntiAlias(value); }
  }
  public $typemap(cstype, const Urho3D::Matrix3x4 &) View {
    get { return GetView(); }
  }
  public $typemap(cstype, const Urho3D::Matrix4 &) Projection {
    get { return GetProjection(); }
  }
  public $typemap(cstype, const Urho3D::Frustum &) Frustum {
    get { return GetFrustum(); }
  }
%}
%csmethodmodifiers Urho3D::DebugRenderer::GetLineAntiAlias "private";
%csmethodmodifiers Urho3D::DebugRenderer::SetLineAntiAlias "private";
%csmethodmodifiers Urho3D::DebugRenderer::GetView "private";
%csmethodmodifiers Urho3D::DebugRenderer::GetProjection "private";
%csmethodmodifiers Urho3D::DebugRenderer::GetFrustum "private";
%typemap(cscode) Urho3D::DecalSet %{
  public $typemap(cstype, Urho3D::Material *) Material {
    get { return GetMaterial(); }
    set { SetMaterial(value); }
  }
  public $typemap(cstype, unsigned int) NumDecals {
    get { return GetNumDecals(); }
  }
  public $typemap(cstype, unsigned int) NumVertices {
    get { return GetNumVertices(); }
  }
  public $typemap(cstype, unsigned int) NumIndices {
    get { return GetNumIndices(); }
  }
  public $typemap(cstype, unsigned int) MaxVertices {
    get { return GetMaxVertices(); }
    set { SetMaxVertices(value); }
  }
  public $typemap(cstype, unsigned int) MaxIndices {
    get { return GetMaxIndices(); }
    set { SetMaxIndices(value); }
  }
  public $typemap(cstype, bool) OptimizeBufferSize {
    get { return GetOptimizeBufferSize(); }
    set { SetOptimizeBufferSize(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) MaterialAttr {
    get { return GetMaterialAttr(); }
  }
  public $typemap(cstype, eastl::vector<unsigned char>) DecalsAttr {
    get { return GetDecalsAttr(); }
  }
%}
%csmethodmodifiers Urho3D::DecalSet::GetMaterial "private";
%csmethodmodifiers Urho3D::DecalSet::SetMaterial "private";
%csmethodmodifiers Urho3D::DecalSet::GetNumDecals "private";
%csmethodmodifiers Urho3D::DecalSet::GetNumVertices "private";
%csmethodmodifiers Urho3D::DecalSet::GetNumIndices "private";
%csmethodmodifiers Urho3D::DecalSet::GetMaxVertices "private";
%csmethodmodifiers Urho3D::DecalSet::SetMaxVertices "private";
%csmethodmodifiers Urho3D::DecalSet::GetMaxIndices "private";
%csmethodmodifiers Urho3D::DecalSet::SetMaxIndices "private";
%csmethodmodifiers Urho3D::DecalSet::GetOptimizeBufferSize "private";
%csmethodmodifiers Urho3D::DecalSet::SetOptimizeBufferSize "private";
%csmethodmodifiers Urho3D::DecalSet::GetMaterialAttr "private";
%csmethodmodifiers Urho3D::DecalSet::GetDecalsAttr "private";
%typemap(cscode) Urho3D::Drawable %{
  public $typemap(cstype, const Urho3D::BoundingBox &) BoundingBox {
    get { return GetBoundingBox(); }
  }
  public $typemap(cstype, const Urho3D::BoundingBox &) WorldBoundingBox {
    get { return GetWorldBoundingBox(); }
  }
  public $typemap(cstype, Urho3D::DrawableFlag) DrawableFlags {
    get { return GetDrawableFlags(); }
  }
  public $typemap(cstype, float) DrawDistance {
    get { return GetDrawDistance(); }
    set { SetDrawDistance(value); }
  }
  public $typemap(cstype, float) ShadowDistance {
    get { return GetShadowDistance(); }
    set { SetShadowDistance(value); }
  }
  public $typemap(cstype, float) LodBias {
    get { return GetLodBias(); }
    set { SetLodBias(value); }
  }
  public $typemap(cstype, unsigned int) ViewMask {
    get { return GetViewMask(); }
    set { SetViewMask(value); }
  }
  public $typemap(cstype, unsigned int) LightMask {
    get { return GetLightMask(); }
    set { SetLightMask(value); }
  }
  public $typemap(cstype, unsigned int) ShadowMask {
    get { return GetShadowMask(); }
    set { SetShadowMask(value); }
  }
  public $typemap(cstype, unsigned int) ZoneMask {
    get { return GetZoneMask(); }
    set { SetZoneMask(value); }
  }
  public $typemap(cstype, unsigned int) MaxLights {
    get { return GetMaxLights(); }
    set { SetMaxLights(value); }
  }
  public $typemap(cstype, bool) CastShadows {
    get { return GetCastShadows(); }
    set { SetCastShadows(value); }
  }
  /*public _typemap(cstype, const eastl::vector<Urho3D::SourceBatch> &) Batches {
    get { return GetBatches(); }
  }*/
  public $typemap(cstype, Urho3D::Octant *) Octant {
    get { return GetOctant(); }
  }
  public $typemap(cstype, Urho3D::Zone *) Zone {
    get { return GetZone(); }
  }
  public $typemap(cstype, float) Distance {
    get { return GetDistance(); }
  }
  public $typemap(cstype, float) LodDistance {
    get { return GetLodDistance(); }
  }
  public $typemap(cstype, float) SortValue {
    get { return GetSortValue(); }
    set { SetSortValue(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Light *> &) Lights {
    get { return GetLights(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Light *> &) VertexLights {
    get { return GetVertexLights(); }
  }
  public $typemap(cstype, Urho3D::Light *) FirstLight {
    get { return GetFirstLight(); }
  }
  public $typemap(cstype, float) MinZ {
    get { return GetMinZ(); }
  }
  public $typemap(cstype, float) MaxZ {
    get { return GetMaxZ(); }
  }
%}
%csmethodmodifiers Urho3D::Drawable::GetBoundingBox "private";
%csmethodmodifiers Urho3D::Drawable::GetWorldBoundingBox "private";
%csmethodmodifiers Urho3D::Drawable::GetDrawableFlags "private";
%csmethodmodifiers Urho3D::Drawable::GetDrawDistance "private";
%csmethodmodifiers Urho3D::Drawable::SetDrawDistance "private";
%csmethodmodifiers Urho3D::Drawable::GetShadowDistance "private";
%csmethodmodifiers Urho3D::Drawable::SetShadowDistance "private";
%csmethodmodifiers Urho3D::Drawable::GetLodBias "private";
%csmethodmodifiers Urho3D::Drawable::SetLodBias "private";
%csmethodmodifiers Urho3D::Drawable::GetViewMask "private";
%csmethodmodifiers Urho3D::Drawable::SetViewMask "private";
%csmethodmodifiers Urho3D::Drawable::GetLightMask "private";
%csmethodmodifiers Urho3D::Drawable::SetLightMask "private";
%csmethodmodifiers Urho3D::Drawable::GetShadowMask "private";
%csmethodmodifiers Urho3D::Drawable::SetShadowMask "private";
%csmethodmodifiers Urho3D::Drawable::GetZoneMask "private";
%csmethodmodifiers Urho3D::Drawable::SetZoneMask "private";
%csmethodmodifiers Urho3D::Drawable::GetMaxLights "private";
%csmethodmodifiers Urho3D::Drawable::SetMaxLights "private";
%csmethodmodifiers Urho3D::Drawable::GetCastShadows "private";
%csmethodmodifiers Urho3D::Drawable::SetCastShadows "private";
%csmethodmodifiers Urho3D::Drawable::GetBatches "private";
%csmethodmodifiers Urho3D::Drawable::GetOctant "private";
%csmethodmodifiers Urho3D::Drawable::GetZone "private";
%csmethodmodifiers Urho3D::Drawable::GetDistance "private";
%csmethodmodifiers Urho3D::Drawable::GetLodDistance "private";
%csmethodmodifiers Urho3D::Drawable::GetSortValue "private";
%csmethodmodifiers Urho3D::Drawable::SetSortValue "private";
%csmethodmodifiers Urho3D::Drawable::GetLights "private";
%csmethodmodifiers Urho3D::Drawable::GetVertexLights "private";
%csmethodmodifiers Urho3D::Drawable::GetFirstLight "private";
%csmethodmodifiers Urho3D::Drawable::GetMinZ "private";
%csmethodmodifiers Urho3D::Drawable::GetMaxZ "private";
%typemap(cscode) Urho3D::GPUObject %{
  /*public _typemap(cstype, Urho3D::Graphics *) Graphics {
    get { return GetGraphics(); }
  }*/
  public $typemap(cstype, unsigned int) GPUObjectName {
    get { return GetGPUObjectName(); }
  }
%}
%csmethodmodifiers Urho3D::GPUObject::GetGraphics "private";
//%csmethodmodifiers Urho3D::GPUObject::GetGPUObjectName "private";
%typemap(cscode) Urho3D::Geometry %{
  public $typemap(cstype, const eastl::vector<Urho3D::SharedPtr<Urho3D::VertexBuffer>> &) VertexBuffers {
    get { return GetVertexBuffers(); }
  }
  public $typemap(cstype, unsigned int) NumVertexBuffers {
    get { return GetNumVertexBuffers(); }
    set { SetNumVertexBuffers(value); }
  }
  public $typemap(cstype, Urho3D::IndexBuffer *) IndexBuffer {
    get { return GetIndexBuffer(); }
    set { SetIndexBuffer(value); }
  }
  public $typemap(cstype, Urho3D::PrimitiveType) PrimitiveType {
    get { return GetPrimitiveType(); }
  }
  public $typemap(cstype, unsigned int) IndexStart {
    get { return GetIndexStart(); }
  }
  public $typemap(cstype, unsigned int) IndexCount {
    get { return GetIndexCount(); }
  }
  public $typemap(cstype, unsigned int) VertexStart {
    get { return GetVertexStart(); }
  }
  public $typemap(cstype, unsigned int) VertexCount {
    get { return GetVertexCount(); }
  }
  public $typemap(cstype, float) LodDistance {
    get { return GetLodDistance(); }
    set { SetLodDistance(value); }
  }
  public $typemap(cstype, unsigned short) BufferHash {
    get { return GetBufferHash(); }
  }
%}
%csmethodmodifiers Urho3D::Geometry::GetVertexBuffers "private";
%csmethodmodifiers Urho3D::Geometry::GetNumVertexBuffers "private";
%csmethodmodifiers Urho3D::Geometry::SetNumVertexBuffers "private";
%csmethodmodifiers Urho3D::Geometry::GetIndexBuffer "private";
%csmethodmodifiers Urho3D::Geometry::SetIndexBuffer "private";
%csmethodmodifiers Urho3D::Geometry::GetPrimitiveType "private";
%csmethodmodifiers Urho3D::Geometry::GetIndexStart "private";
%csmethodmodifiers Urho3D::Geometry::GetIndexCount "private";
%csmethodmodifiers Urho3D::Geometry::GetVertexStart "private";
%csmethodmodifiers Urho3D::Geometry::GetVertexCount "private";
%csmethodmodifiers Urho3D::Geometry::GetLodDistance "private";
%csmethodmodifiers Urho3D::Geometry::SetLodDistance "private";
%csmethodmodifiers Urho3D::Geometry::GetBufferHash "private";
%typemap(cscode) Urho3D::Graphics %{
  public $typemap(cstype, Urho3D::GraphicsImpl *) Impl {
    get { return GetImpl(); }
  }
  public $typemap(cstype, void *) ExternalWindow {
    get { return GetExternalWindow(); }
    set { SetExternalWindow(value); }
  }
  public $typemap(cstype, SDL_Window *) Window {
    get { return GetWindow(); }
  }
  public $typemap(cstype, const eastl::string &) WindowTitle {
    get { return GetWindowTitle(); }
    set { SetWindowTitle(value); }
  }
  public $typemap(cstype, const eastl::string &) ApiName {
    get { return GetApiName(); }
  }
  public $typemap(cstype, Urho3D::IntVector2) WindowPosition {
    get { return GetWindowPosition(); }
  }
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
  }
  public $typemap(cstype, int) MultiSample {
    get { return GetMultiSample(); }
  }
  public $typemap(cstype, Urho3D::IntVector2) Size {
    get { return GetSize(); }
  }
  public $typemap(cstype, bool) Fullscreen {
    get { return GetFullscreen(); }
  }
  public $typemap(cstype, bool) Borderless {
    get { return GetBorderless(); }
  }
  public $typemap(cstype, bool) Resizable {
    get { return GetResizable(); }
  }
  public $typemap(cstype, bool) HighDPI {
    get { return GetHighDPI(); }
  }
  public $typemap(cstype, bool) VSync {
    get { return GetVSync(); }
  }
  public $typemap(cstype, int) RefreshRate {
    get { return GetRefreshRate(); }
  }
  public $typemap(cstype, int) Monitor {
    get { return GetMonitor(); }
  }
  public $typemap(cstype, bool) TripleBuffer {
    get { return GetTripleBuffer(); }
  }
  public $typemap(cstype, bool) SRGB {
    get { return GetSRGB(); }
    set { SetSRGB(value); }
  }
  public $typemap(cstype, bool) Dither {
    get { return GetDither(); }
    set { SetDither(value); }
  }
  public $typemap(cstype, bool) FlushGPU {
    get { return GetFlushGPU(); }
    set { SetFlushGPU(value); }
  }
  public $typemap(cstype, bool) ForceGL2 {
    get { return GetForceGL2(); }
    set { SetForceGL2(value); }
  }
  public $typemap(cstype, const eastl::string &) Orientations {
    get { return GetOrientations(); }
    set { SetOrientations(value); }
  }
  public $typemap(cstype, unsigned int) NumPrimitives {
    get { return GetNumPrimitives(); }
  }
  public $typemap(cstype, unsigned int) NumBatches {
    get { return GetNumBatches(); }
  }
  public $typemap(cstype, unsigned int) DummyColorFormat {
    get { return GetDummyColorFormat(); }
  }
  public $typemap(cstype, unsigned int) ShadowMapFormat {
    get { return GetShadowMapFormat(); }
  }
  public $typemap(cstype, unsigned int) HiresShadowMapFormat {
    get { return GetHiresShadowMapFormat(); }
  }
  public $typemap(cstype, bool) InstancingSupport {
    get { return GetInstancingSupport(); }
  }
  public $typemap(cstype, bool) LightPrepassSupport {
    get { return GetLightPrepassSupport(); }
  }
  public $typemap(cstype, bool) DeferredSupport {
    get { return GetDeferredSupport(); }
  }
  public $typemap(cstype, bool) AnisotropySupport {
    get { return GetAnisotropySupport(); }
  }
  public $typemap(cstype, bool) HardwareShadowSupport {
    get { return GetHardwareShadowSupport(); }
  }
  public $typemap(cstype, bool) ReadableDepthSupport {
    get { return GetReadableDepthSupport(); }
  }
  public $typemap(cstype, bool) SRGBSupport {
    get { return GetSRGBSupport(); }
  }
  public $typemap(cstype, bool) SRGBWriteSupport {
    get { return GetSRGBWriteSupport(); }
  }
  public $typemap(cstype, eastl::vector<int>) MultiSampleLevels {
    get { return GetMultiSampleLevels(); }
  }
  public $typemap(cstype, int) MonitorCount {
    get { return GetMonitorCount(); }
  }
  public $typemap(cstype, int) CurrentMonitor {
    get { return GetCurrentMonitor(); }
  }
  public $typemap(cstype, bool) Maximized {
    get { return GetMaximized(); }
  }
  public $typemap(cstype, Urho3D::IndexBuffer *) IndexBuffer {
    get { return GetIndexBuffer(); }
    set { SetIndexBuffer(value); }
  }
  public $typemap(cstype, Urho3D::ShaderVariation *) VertexShader {
    get { return GetVertexShader(); }
  }
  public $typemap(cstype, Urho3D::ShaderVariation *) PixelShader {
    get { return GetPixelShader(); }
  }
  public $typemap(cstype, Urho3D::ShaderProgram *) ShaderProgram {
    get { return GetShaderProgram(); }
  }
  public $typemap(cstype, Urho3D::TextureFilterMode) DefaultTextureFilterMode {
    get { return GetDefaultTextureFilterMode(); }
    set { SetDefaultTextureFilterMode(value); }
  }
  public $typemap(cstype, unsigned int) DefaultTextureAnisotropy {
    get { return GetDefaultTextureAnisotropy(); }
    set { SetDefaultTextureAnisotropy(value); }
  }
  public $typemap(cstype, Urho3D::RenderSurface *) DepthStencil {
    get { return GetDepthStencil(); }
    set { SetDepthStencil(value); }
  }
  public $typemap(cstype, Urho3D::IntRect) Viewport {
    get { return GetViewport(); }
  }
  public $typemap(cstype, Urho3D::BlendMode) BlendMode {
    get { return GetBlendMode(); }
  }
  public $typemap(cstype, bool) AlphaToCoverage {
    get { return GetAlphaToCoverage(); }
  }
  public $typemap(cstype, bool) ColorWrite {
    get { return GetColorWrite(); }
    set { SetColorWrite(value); }
  }
  public $typemap(cstype, Urho3D::CullMode) CullMode {
    get { return GetCullMode(); }
    set { SetCullMode(value); }
  }
  public $typemap(cstype, float) DepthConstantBias {
    get { return GetDepthConstantBias(); }
  }
  public $typemap(cstype, float) DepthSlopeScaledBias {
    get { return GetDepthSlopeScaledBias(); }
  }
  public $typemap(cstype, Urho3D::CompareMode) DepthTest {
    get { return GetDepthTest(); }
    set { SetDepthTest(value); }
  }
  public $typemap(cstype, bool) DepthWrite {
    get { return GetDepthWrite(); }
    set { SetDepthWrite(value); }
  }
  public $typemap(cstype, Urho3D::FillMode) FillMode {
    get { return GetFillMode(); }
    set { SetFillMode(value); }
  }
  public $typemap(cstype, bool) LineAntiAlias {
    get { return GetLineAntiAlias(); }
    set { SetLineAntiAlias(value); }
  }
  public $typemap(cstype, bool) StencilTest {
    get { return GetStencilTest(); }
  }
  public $typemap(cstype, bool) ScissorTest {
    get { return GetScissorTest(); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) ScissorRect {
    get { return GetScissorRect(); }
  }
  public $typemap(cstype, Urho3D::CompareMode) StencilTestMode {
    get { return GetStencilTestMode(); }
  }
  public $typemap(cstype, Urho3D::StencilOp) StencilPass {
    get { return GetStencilPass(); }
  }
  public $typemap(cstype, Urho3D::StencilOp) StencilFail {
    get { return GetStencilFail(); }
  }
  public $typemap(cstype, Urho3D::StencilOp) StencilZFail {
    get { return GetStencilZFail(); }
  }
  public $typemap(cstype, unsigned int) StencilRef {
    get { return GetStencilRef(); }
  }
  public $typemap(cstype, unsigned int) StencilCompareMask {
    get { return GetStencilCompareMask(); }
  }
  public $typemap(cstype, unsigned int) StencilWriteMask {
    get { return GetStencilWriteMask(); }
  }
  public $typemap(cstype, bool) UseClipPlane {
    get { return GetUseClipPlane(); }
  }
  public $typemap(cstype, const eastl::string &) ShaderCacheDir {
    get { return GetShaderCacheDir(); }
    set { SetShaderCacheDir(value); }
  }
  public $typemap(cstype, Urho3D::IntVector2) RenderTargetDimensions {
    get { return GetRenderTargetDimensions(); }
  }
  public $typemap(cstype, void *) SDLWindow {
    get { return GetSDLWindow(); }
  }
%}
%csmethodmodifiers Urho3D::Graphics::GetImpl "private";
%csmethodmodifiers Urho3D::Graphics::GetExternalWindow "private";
%csmethodmodifiers Urho3D::Graphics::SetExternalWindow "private";
%csmethodmodifiers Urho3D::Graphics::GetWindow "private";
%csmethodmodifiers Urho3D::Graphics::GetWindowTitle "private";
%csmethodmodifiers Urho3D::Graphics::SetWindowTitle "private";
%csmethodmodifiers Urho3D::Graphics::GetApiName "private";
%csmethodmodifiers Urho3D::Graphics::GetWindowPosition "private";
%csmethodmodifiers Urho3D::Graphics::GetWidth "private";
%csmethodmodifiers Urho3D::Graphics::GetHeight "private";
%csmethodmodifiers Urho3D::Graphics::GetMultiSample "private";
%csmethodmodifiers Urho3D::Graphics::GetSize "private";
%csmethodmodifiers Urho3D::Graphics::GetFullscreen "private";
%csmethodmodifiers Urho3D::Graphics::GetBorderless "private";
%csmethodmodifiers Urho3D::Graphics::GetResizable "private";
%csmethodmodifiers Urho3D::Graphics::GetHighDPI "private";
%csmethodmodifiers Urho3D::Graphics::GetVSync "private";
%csmethodmodifiers Urho3D::Graphics::GetRefreshRate "private";
%csmethodmodifiers Urho3D::Graphics::GetMonitor "private";
%csmethodmodifiers Urho3D::Graphics::GetTripleBuffer "private";
%csmethodmodifiers Urho3D::Graphics::GetSRGB "private";
%csmethodmodifiers Urho3D::Graphics::SetSRGB "private";
%csmethodmodifiers Urho3D::Graphics::GetDither "private";
%csmethodmodifiers Urho3D::Graphics::SetDither "private";
%csmethodmodifiers Urho3D::Graphics::GetFlushGPU "private";
%csmethodmodifiers Urho3D::Graphics::SetFlushGPU "private";
%csmethodmodifiers Urho3D::Graphics::GetForceGL2 "private";
%csmethodmodifiers Urho3D::Graphics::SetForceGL2 "private";
%csmethodmodifiers Urho3D::Graphics::GetOrientations "private";
%csmethodmodifiers Urho3D::Graphics::SetOrientations "private";
%csmethodmodifiers Urho3D::Graphics::GetNumPrimitives "private";
%csmethodmodifiers Urho3D::Graphics::GetNumBatches "private";
%csmethodmodifiers Urho3D::Graphics::GetDummyColorFormat "private";
%csmethodmodifiers Urho3D::Graphics::GetShadowMapFormat "private";
%csmethodmodifiers Urho3D::Graphics::GetHiresShadowMapFormat "private";
%csmethodmodifiers Urho3D::Graphics::GetInstancingSupport "private";
%csmethodmodifiers Urho3D::Graphics::GetLightPrepassSupport "private";
%csmethodmodifiers Urho3D::Graphics::GetDeferredSupport "private";
%csmethodmodifiers Urho3D::Graphics::GetAnisotropySupport "private";
%csmethodmodifiers Urho3D::Graphics::GetHardwareShadowSupport "private";
%csmethodmodifiers Urho3D::Graphics::GetReadableDepthSupport "private";
%csmethodmodifiers Urho3D::Graphics::GetSRGBSupport "private";
%csmethodmodifiers Urho3D::Graphics::GetSRGBWriteSupport "private";
%csmethodmodifiers Urho3D::Graphics::GetMultiSampleLevels "private";
%csmethodmodifiers Urho3D::Graphics::GetMonitorCount "private";
%csmethodmodifiers Urho3D::Graphics::GetCurrentMonitor "private";
%csmethodmodifiers Urho3D::Graphics::GetMaximized "private";
%csmethodmodifiers Urho3D::Graphics::GetIndexBuffer "private";
%csmethodmodifiers Urho3D::Graphics::SetIndexBuffer "private";
%csmethodmodifiers Urho3D::Graphics::GetVertexShader "private";
%csmethodmodifiers Urho3D::Graphics::GetPixelShader "private";
%csmethodmodifiers Urho3D::Graphics::GetShaderProgram "private";
%csmethodmodifiers Urho3D::Graphics::GetDefaultTextureFilterMode "private";
%csmethodmodifiers Urho3D::Graphics::SetDefaultTextureFilterMode "private";
%csmethodmodifiers Urho3D::Graphics::GetDefaultTextureAnisotropy "private";
%csmethodmodifiers Urho3D::Graphics::SetDefaultTextureAnisotropy "private";
%csmethodmodifiers Urho3D::Graphics::GetDepthStencil "private";
%csmethodmodifiers Urho3D::Graphics::SetDepthStencil "private";
%csmethodmodifiers Urho3D::Graphics::GetViewport "private";
%csmethodmodifiers Urho3D::Graphics::GetBlendMode "private";
%csmethodmodifiers Urho3D::Graphics::GetAlphaToCoverage "private";
%csmethodmodifiers Urho3D::Graphics::GetColorWrite "private";
%csmethodmodifiers Urho3D::Graphics::SetColorWrite "private";
%csmethodmodifiers Urho3D::Graphics::GetCullMode "private";
%csmethodmodifiers Urho3D::Graphics::SetCullMode "private";
%csmethodmodifiers Urho3D::Graphics::GetDepthConstantBias "private";
%csmethodmodifiers Urho3D::Graphics::GetDepthSlopeScaledBias "private";
%csmethodmodifiers Urho3D::Graphics::GetDepthTest "private";
%csmethodmodifiers Urho3D::Graphics::SetDepthTest "private";
%csmethodmodifiers Urho3D::Graphics::GetDepthWrite "private";
%csmethodmodifiers Urho3D::Graphics::SetDepthWrite "private";
%csmethodmodifiers Urho3D::Graphics::GetFillMode "private";
%csmethodmodifiers Urho3D::Graphics::SetFillMode "private";
%csmethodmodifiers Urho3D::Graphics::GetLineAntiAlias "private";
%csmethodmodifiers Urho3D::Graphics::SetLineAntiAlias "private";
%csmethodmodifiers Urho3D::Graphics::GetStencilTest "private";
%csmethodmodifiers Urho3D::Graphics::GetScissorTest "private";
%csmethodmodifiers Urho3D::Graphics::GetScissorRect "private";
%csmethodmodifiers Urho3D::Graphics::GetStencilTestMode "private";
%csmethodmodifiers Urho3D::Graphics::GetStencilPass "private";
%csmethodmodifiers Urho3D::Graphics::GetStencilFail "private";
%csmethodmodifiers Urho3D::Graphics::GetStencilZFail "private";
%csmethodmodifiers Urho3D::Graphics::GetStencilRef "private";
%csmethodmodifiers Urho3D::Graphics::GetStencilCompareMask "private";
%csmethodmodifiers Urho3D::Graphics::GetStencilWriteMask "private";
%csmethodmodifiers Urho3D::Graphics::GetUseClipPlane "private";
%csmethodmodifiers Urho3D::Graphics::GetShaderCacheDir "private";
%csmethodmodifiers Urho3D::Graphics::SetShaderCacheDir "private";
%csmethodmodifiers Urho3D::Graphics::GetRenderTargetDimensions "private";
%csmethodmodifiers Urho3D::Graphics::GetSDLWindow "private";
%typemap(cscode) Urho3D::IndexBuffer %{
  public $typemap(cstype, unsigned int) IndexCount {
    get { return GetIndexCount(); }
  }
  public $typemap(cstype, unsigned int) IndexSize {
    get { return GetIndexSize(); }
  }
  public $typemap(cstype, unsigned char *) ShadowData {
    get { return GetShadowData(); }
  }
  /*public _typemap(cstype, eastl::shared_array<unsigned char>) ShadowDataShared {
    get { return GetShadowDataShared(); }
  }*/
%}
%csmethodmodifiers Urho3D::IndexBuffer::GetIndexCount "private";
%csmethodmodifiers Urho3D::IndexBuffer::GetIndexSize "private";
%csmethodmodifiers Urho3D::IndexBuffer::GetShadowData "private";
%csmethodmodifiers Urho3D::IndexBuffer::GetShadowDataShared "private";
%typemap(cscode) Urho3D::Light %{
  public $typemap(cstype, Urho3D::LightType) LightType {
    get { return GetLightType(); }
    set { SetLightType(value); }
  }
  public $typemap(cstype, bool) PerVertex {
    get { return GetPerVertex(); }
    set { SetPerVertex(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) Color {
    get { return GetColor(); }
    set { SetColor(value); }
  }
  public $typemap(cstype, float) Temperature {
    get { return GetTemperature(); }
    set { SetTemperature(value); }
  }
  public $typemap(cstype, float) Radius {
    get { return GetRadius(); }
    set { SetRadius(value); }
  }
  public $typemap(cstype, float) Length {
    get { return GetLength(); }
    set { SetLength(value); }
  }
  public $typemap(cstype, bool) UsePhysicalValues {
    get { return GetUsePhysicalValues(); }
    set { SetUsePhysicalValues(value); }
  }
  public $typemap(cstype, Urho3D::Color) ColorFromTemperature {
    get { return GetColorFromTemperature(); }
  }
  public $typemap(cstype, float) SpecularIntensity {
    get { return GetSpecularIntensity(); }
    set { SetSpecularIntensity(value); }
  }
  public $typemap(cstype, float) Brightness {
    get { return GetBrightness(); }
    set { SetBrightness(value); }
  }
  public $typemap(cstype, Urho3D::Color) EffectiveColor {
    get { return GetEffectiveColor(); }
  }
  public $typemap(cstype, float) EffectiveSpecularIntensity {
    get { return GetEffectiveSpecularIntensity(); }
  }
  public $typemap(cstype, float) Range {
    get { return GetRange(); }
    set { SetRange(value); }
  }
  public $typemap(cstype, float) Fov {
    get { return GetFov(); }
    set { SetFov(value); }
  }
  public $typemap(cstype, float) AspectRatio {
    get { return GetAspectRatio(); }
    set { SetAspectRatio(value); }
  }
  public $typemap(cstype, float) FadeDistance {
    get { return GetFadeDistance(); }
    set { SetFadeDistance(value); }
  }
  public $typemap(cstype, float) ShadowFadeDistance {
    get { return GetShadowFadeDistance(); }
    set { SetShadowFadeDistance(value); }
  }
  public $typemap(cstype, const Urho3D::BiasParameters &) ShadowBias {
    get { return GetShadowBias(); }
    set { SetShadowBias(value); }
  }
  public $typemap(cstype, const Urho3D::CascadeParameters &) ShadowCascade {
    get { return GetShadowCascade(); }
    set { SetShadowCascade(value); }
  }
  public $typemap(cstype, const Urho3D::FocusParameters &) ShadowFocus {
    get { return GetShadowFocus(); }
    set { SetShadowFocus(value); }
  }
  public $typemap(cstype, float) ShadowIntensity {
    get { return GetShadowIntensity(); }
    set { SetShadowIntensity(value); }
  }
  public $typemap(cstype, float) ShadowResolution {
    get { return GetShadowResolution(); }
    set { SetShadowResolution(value); }
  }
  public $typemap(cstype, float) ShadowNearFarRatio {
    get { return GetShadowNearFarRatio(); }
    set { SetShadowNearFarRatio(value); }
  }
  public $typemap(cstype, float) ShadowMaxExtrusion {
    get { return GetShadowMaxExtrusion(); }
    set { SetShadowMaxExtrusion(value); }
  }
  public $typemap(cstype, Urho3D::Texture *) RampTexture {
    get { return GetRampTexture(); }
    set { SetRampTexture(value); }
  }
  public $typemap(cstype, Urho3D::Texture *) ShapeTexture {
    get { return GetShapeTexture(); }
    set { SetShapeTexture(value); }
  }
  public $typemap(cstype, Urho3D::Frustum) Frustum {
    get { return GetFrustum(); }
  }
  public $typemap(cstype, int) NumShadowSplits {
    get { return GetNumShadowSplits(); }
  }
  /*public _typemap(cstype, Urho3D::LightBatchQueue *) LightQueue {
    get { return GetLightQueue(); }
    set { SetLightQueue(value); }
  }*/
  public $typemap(cstype, Urho3D::ResourceRef) RampTextureAttr {
    get { return GetRampTextureAttr(); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) ShapeTextureAttr {
    get { return GetShapeTextureAttr(); }
  }
%}
%csmethodmodifiers Urho3D::Light::GetLightType "private";
%csmethodmodifiers Urho3D::Light::SetLightType "private";
%csmethodmodifiers Urho3D::Light::GetPerVertex "private";
%csmethodmodifiers Urho3D::Light::SetPerVertex "private";
%csmethodmodifiers Urho3D::Light::GetColor "private";
%csmethodmodifiers Urho3D::Light::SetColor "private";
%csmethodmodifiers Urho3D::Light::GetTemperature "private";
%csmethodmodifiers Urho3D::Light::SetTemperature "private";
%csmethodmodifiers Urho3D::Light::GetRadius "private";
%csmethodmodifiers Urho3D::Light::SetRadius "private";
%csmethodmodifiers Urho3D::Light::GetLength "private";
%csmethodmodifiers Urho3D::Light::SetLength "private";
%csmethodmodifiers Urho3D::Light::GetUsePhysicalValues "private";
%csmethodmodifiers Urho3D::Light::SetUsePhysicalValues "private";
%csmethodmodifiers Urho3D::Light::GetColorFromTemperature "private";
%csmethodmodifiers Urho3D::Light::GetSpecularIntensity "private";
%csmethodmodifiers Urho3D::Light::SetSpecularIntensity "private";
%csmethodmodifiers Urho3D::Light::GetBrightness "private";
%csmethodmodifiers Urho3D::Light::SetBrightness "private";
%csmethodmodifiers Urho3D::Light::GetEffectiveColor "private";
%csmethodmodifiers Urho3D::Light::GetEffectiveSpecularIntensity "private";
%csmethodmodifiers Urho3D::Light::GetRange "private";
%csmethodmodifiers Urho3D::Light::SetRange "private";
%csmethodmodifiers Urho3D::Light::GetFov "private";
%csmethodmodifiers Urho3D::Light::SetFov "private";
%csmethodmodifiers Urho3D::Light::GetAspectRatio "private";
%csmethodmodifiers Urho3D::Light::SetAspectRatio "private";
%csmethodmodifiers Urho3D::Light::GetFadeDistance "private";
%csmethodmodifiers Urho3D::Light::SetFadeDistance "private";
%csmethodmodifiers Urho3D::Light::GetShadowFadeDistance "private";
%csmethodmodifiers Urho3D::Light::SetShadowFadeDistance "private";
%csmethodmodifiers Urho3D::Light::GetShadowBias "private";
%csmethodmodifiers Urho3D::Light::SetShadowBias "private";
%csmethodmodifiers Urho3D::Light::GetShadowCascade "private";
%csmethodmodifiers Urho3D::Light::SetShadowCascade "private";
%csmethodmodifiers Urho3D::Light::GetShadowFocus "private";
%csmethodmodifiers Urho3D::Light::SetShadowFocus "private";
%csmethodmodifiers Urho3D::Light::GetShadowIntensity "private";
%csmethodmodifiers Urho3D::Light::SetShadowIntensity "private";
%csmethodmodifiers Urho3D::Light::GetShadowResolution "private";
%csmethodmodifiers Urho3D::Light::SetShadowResolution "private";
%csmethodmodifiers Urho3D::Light::GetShadowNearFarRatio "private";
%csmethodmodifiers Urho3D::Light::SetShadowNearFarRatio "private";
%csmethodmodifiers Urho3D::Light::GetShadowMaxExtrusion "private";
%csmethodmodifiers Urho3D::Light::SetShadowMaxExtrusion "private";
%csmethodmodifiers Urho3D::Light::GetRampTexture "private";
%csmethodmodifiers Urho3D::Light::SetRampTexture "private";
%csmethodmodifiers Urho3D::Light::GetShapeTexture "private";
%csmethodmodifiers Urho3D::Light::SetShapeTexture "private";
%csmethodmodifiers Urho3D::Light::GetFrustum "private";
%csmethodmodifiers Urho3D::Light::GetNumShadowSplits "private";
%csmethodmodifiers Urho3D::Light::GetLightQueue "private";
%csmethodmodifiers Urho3D::Light::SetLightQueue "private";
%csmethodmodifiers Urho3D::Light::GetRampTextureAttr "private";
%csmethodmodifiers Urho3D::Light::GetShapeTextureAttr "private";
%typemap(cscode) Urho3D::ShaderParameterAnimationInfo %{
  public $typemap(cstype, const eastl::string &) Name {
    get { return GetName(); }
  }
%}
%csmethodmodifiers Urho3D::ShaderParameterAnimationInfo::GetName "private";
%typemap(cscode) Urho3D::Material %{
  public $typemap(cstype, unsigned int) NumTechniques {
    get { return GetNumTechniques(); }
    set { SetNumTechniques(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::TechniqueEntry> &) Techniques {
    get { return GetTechniques(); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::TextureUnit, Urho3D::SharedPtr<Urho3D::Texture>> &) Textures {
    get { return GetTextures(); }
  }
  public $typemap(cstype, const eastl::string &) VertexShaderDefines {
    get { return GetVertexShaderDefines(); }
    set { SetVertexShaderDefines(value); }
  }
  public $typemap(cstype, const eastl::string &) PixelShaderDefines {
    get { return GetPixelShaderDefines(); }
    set { SetPixelShaderDefines(value); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::MaterialShaderParameter> &) ShaderParameters {
    get { return GetShaderParameters(); }
  }
  public $typemap(cstype, Urho3D::CullMode) CullMode {
    get { return GetCullMode(); }
    set { SetCullMode(value); }
  }
  public $typemap(cstype, Urho3D::CullMode) ShadowCullMode {
    get { return GetShadowCullMode(); }
    set { SetShadowCullMode(value); }
  }
  public $typemap(cstype, Urho3D::FillMode) FillMode {
    get { return GetFillMode(); }
    set { SetFillMode(value); }
  }
  public $typemap(cstype, const Urho3D::BiasParameters &) DepthBias {
    get { return GetDepthBias(); }
    set { SetDepthBias(value); }
  }
  public $typemap(cstype, bool) AlphaToCoverage {
    get { return GetAlphaToCoverage(); }
    set { SetAlphaToCoverage(value); }
  }
  public $typemap(cstype, bool) LineAntiAlias {
    get { return GetLineAntiAlias(); }
    set { SetLineAntiAlias(value); }
  }
  public $typemap(cstype, unsigned char) RenderOrder {
    get { return GetRenderOrder(); }
    set { SetRenderOrder(value); }
  }
  public $typemap(cstype, unsigned int) AuxViewFrameNumber {
    get { return GetAuxViewFrameNumber(); }
  }
  public $typemap(cstype, bool) Occlusion {
    get { return GetOcclusion(); }
    set { SetOcclusion(value); }
  }
  public $typemap(cstype, bool) Specular {
    get { return GetSpecular(); }
    set { SetSpecular(value); }
  }
  public $typemap(cstype, Urho3D::Scene *) Scene {
    get { return GetScene(); }
    set { SetScene(value); }
  }
  public $typemap(cstype, unsigned int) ShaderParameterHash {
    get { return GetShaderParameterHash(); }
  }
%}
%csmethodmodifiers Urho3D::Material::GetNumTechniques "private";
%csmethodmodifiers Urho3D::Material::SetNumTechniques "private";
%csmethodmodifiers Urho3D::Material::GetTechniques "private";
%csmethodmodifiers Urho3D::Material::GetTextures "private";
%csmethodmodifiers Urho3D::Material::GetVertexShaderDefines "private";
%csmethodmodifiers Urho3D::Material::SetVertexShaderDefines "private";
%csmethodmodifiers Urho3D::Material::GetPixelShaderDefines "private";
%csmethodmodifiers Urho3D::Material::SetPixelShaderDefines "private";
%csmethodmodifiers Urho3D::Material::GetShaderParameters "private";
%csmethodmodifiers Urho3D::Material::GetCullMode "private";
%csmethodmodifiers Urho3D::Material::SetCullMode "private";
%csmethodmodifiers Urho3D::Material::GetShadowCullMode "private";
%csmethodmodifiers Urho3D::Material::SetShadowCullMode "private";
%csmethodmodifiers Urho3D::Material::GetFillMode "private";
%csmethodmodifiers Urho3D::Material::SetFillMode "private";
%csmethodmodifiers Urho3D::Material::GetDepthBias "private";
%csmethodmodifiers Urho3D::Material::SetDepthBias "private";
%csmethodmodifiers Urho3D::Material::GetAlphaToCoverage "private";
%csmethodmodifiers Urho3D::Material::SetAlphaToCoverage "private";
%csmethodmodifiers Urho3D::Material::GetLineAntiAlias "private";
%csmethodmodifiers Urho3D::Material::SetLineAntiAlias "private";
%csmethodmodifiers Urho3D::Material::GetRenderOrder "private";
%csmethodmodifiers Urho3D::Material::SetRenderOrder "private";
%csmethodmodifiers Urho3D::Material::GetAuxViewFrameNumber "private";
%csmethodmodifiers Urho3D::Material::GetOcclusion "private";
%csmethodmodifiers Urho3D::Material::SetOcclusion "private";
%csmethodmodifiers Urho3D::Material::GetSpecular "private";
%csmethodmodifiers Urho3D::Material::SetSpecular "private";
%csmethodmodifiers Urho3D::Material::GetScene "private";
%csmethodmodifiers Urho3D::Material::SetScene "private";
%csmethodmodifiers Urho3D::Material::GetShaderParameterHash "private";
%typemap(cscode) Urho3D::Model %{
  public $typemap(cstype, const Urho3D::BoundingBox &) BoundingBox {
    get { return GetBoundingBox(); }
    set { SetBoundingBox(value); }
  }
  public $typemap(cstype, Urho3D::Skeleton &) Skeleton {
    get { return GetSkeleton(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::SharedPtr<Urho3D::VertexBuffer>> &) VertexBuffers {
    get { return GetVertexBuffers(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::SharedPtr<Urho3D::IndexBuffer>> &) IndexBuffers {
    get { return GetIndexBuffers(); }
    set { SetIndexBuffers(value); }
  }
  public $typemap(cstype, unsigned int) NumGeometries {
    get { return GetNumGeometries(); }
    set { SetNumGeometries(value); }
  }
  public $typemap(cstype, const eastl::vector<eastl::vector<Urho3D::SharedPtr<Urho3D::Geometry>>> &) Geometries {
    get { return GetGeometries(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Vector3> &) GeometryCenters {
    get { return GetGeometryCenters(); }
  }
  public $typemap(cstype, const eastl::vector<eastl::vector<unsigned int>> &) GeometryBoneMappings {
    get { return GetGeometryBoneMappings(); }
    set { SetGeometryBoneMappings(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::ModelMorph> &) Morphs {
    get { return GetMorphs(); }
    set { SetMorphs(value); }
  }
  public $typemap(cstype, unsigned int) NumMorphs {
    get { return GetNumMorphs(); }
  }
%}
%csmethodmodifiers Urho3D::Model::GetBoundingBox "private";
%csmethodmodifiers Urho3D::Model::SetBoundingBox "private";
%csmethodmodifiers Urho3D::Model::GetSkeleton "private";
%csmethodmodifiers Urho3D::Model::GetVertexBuffers "private";
%csmethodmodifiers Urho3D::Model::GetIndexBuffers "private";
%csmethodmodifiers Urho3D::Model::SetIndexBuffers "private";
%csmethodmodifiers Urho3D::Model::GetNumGeometries "private";
%csmethodmodifiers Urho3D::Model::SetNumGeometries "private";
%csmethodmodifiers Urho3D::Model::GetGeometries "private";
%csmethodmodifiers Urho3D::Model::GetGeometryCenters "private";
%csmethodmodifiers Urho3D::Model::GetGeometryBoneMappings "private";
%csmethodmodifiers Urho3D::Model::SetGeometryBoneMappings "private";
%csmethodmodifiers Urho3D::Model::GetMorphs "private";
%csmethodmodifiers Urho3D::Model::SetMorphs "private";
%csmethodmodifiers Urho3D::Model::GetNumMorphs "private";
%typemap(cscode) Urho3D::OcclusionBuffer %{
  public $typemap(cstype, void *) Buffer {
    get { return GetBuffer(); }
  }
  public $typemap(cstype, const Urho3D::Matrix3x4 &) View {
    get { return GetView(); }
  }
  public $typemap(cstype, const Urho3D::Matrix4 &) Projection {
    get { return GetProjection(); }
  }
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
  }
  public $typemap(cstype, unsigned int) NumTriangles {
    get { return GetNumTriangles(); }
  }
  public $typemap(cstype, unsigned int) MaxTriangles {
    get { return GetMaxTriangles(); }
    set { SetMaxTriangles(value); }
  }
  public $typemap(cstype, Urho3D::CullMode) CullMode {
    get { return GetCullMode(); }
    set { SetCullMode(value); }
  }
  public $typemap(cstype, unsigned int) UseTimer {
    get { return GetUseTimer(); }
  }
%}
%csmethodmodifiers Urho3D::OcclusionBuffer::GetBuffer "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::GetView "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::GetProjection "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::GetWidth "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::GetHeight "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::GetNumTriangles "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::GetMaxTriangles "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::SetMaxTriangles "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::GetCullMode "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::SetCullMode "private";
%csmethodmodifiers Urho3D::OcclusionBuffer::GetUseTimer "private";
%typemap(cscode) Urho3D::Octant %{
  public $typemap(cstype, const Urho3D::BoundingBox &) WorldBoundingBox {
    get { return GetWorldBoundingBox(); }
  }
  public $typemap(cstype, const Urho3D::BoundingBox &) CullingBox {
    get { return GetCullingBox(); }
  }
  public $typemap(cstype, unsigned int) Level {
    get { return GetLevel(); }
  }
  public $typemap(cstype, Urho3D::Octant *) Parent {
    get { return GetParent(); }
  }
  public $typemap(cstype, Urho3D::Octree *) Root {
    get { return GetRoot(); }
  }
  public $typemap(cstype, unsigned int) NumDrawables {
    get { return GetNumDrawables(); }
  }
%}
/*
%csmethodmodifiers Urho3D::Octant::GetWorldBoundingBox "private";
%csmethodmodifiers Urho3D::Octant::GetCullingBox "private";
%csmethodmodifiers Urho3D::Octant::GetLevel "private";
%csmethodmodifiers Urho3D::Octant::GetParent "private";
%csmethodmodifiers Urho3D::Octant::GetRoot "private";
%csmethodmodifiers Urho3D::Octant::GetNumDrawables "private";
*/
%typemap(cscode) Urho3D::Octree %{
  public $typemap(cstype, unsigned int) NumLevels {
    get { return GetNumLevels(); }
  }
%}
%csmethodmodifiers Urho3D::Octree::GetNumLevels "private";
%typemap(cscode) Urho3D::ParticleEffect %{
  public $typemap(cstype, Urho3D::Material *) Material {
    get { return GetMaterial(); }
    set { SetMaterial(value); }
  }
  public $typemap(cstype, unsigned int) NumParticles {
    get { return GetNumParticles(); }
    set { SetNumParticles(value); }
  }
  public $typemap(cstype, bool) UpdateInvisible {
    get { return GetUpdateInvisible(); }
    set { SetUpdateInvisible(value); }
  }
  public $typemap(cstype, float) AnimationLodBias {
    get { return GetAnimationLodBias(); }
    set { SetAnimationLodBias(value); }
  }
  public $typemap(cstype, Urho3D::EmitterType) EmitterType {
    get { return GetEmitterType(); }
    set { SetEmitterType(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) EmitterSize {
    get { return GetEmitterSize(); }
    set { SetEmitterSize(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) MinDirection {
    get { return GetMinDirection(); }
    set { SetMinDirection(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) MaxDirection {
    get { return GetMaxDirection(); }
    set { SetMaxDirection(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) ConstantForce {
    get { return GetConstantForce(); }
    set { SetConstantForce(value); }
  }
  public $typemap(cstype, float) DampingForce {
    get { return GetDampingForce(); }
    set { SetDampingForce(value); }
  }
  public $typemap(cstype, float) ActiveTime {
    get { return GetActiveTime(); }
    set { SetActiveTime(value); }
  }
  public $typemap(cstype, float) InactiveTime {
    get { return GetInactiveTime(); }
    set { SetInactiveTime(value); }
  }
  public $typemap(cstype, float) MinEmissionRate {
    get { return GetMinEmissionRate(); }
    set { SetMinEmissionRate(value); }
  }
  public $typemap(cstype, float) MaxEmissionRate {
    get { return GetMaxEmissionRate(); }
    set { SetMaxEmissionRate(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) MinParticleSize {
    get { return GetMinParticleSize(); }
    set { SetMinParticleSize(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) MaxParticleSize {
    get { return GetMaxParticleSize(); }
    set { SetMaxParticleSize(value); }
  }
  public $typemap(cstype, float) MinTimeToLive {
    get { return GetMinTimeToLive(); }
    set { SetMinTimeToLive(value); }
  }
  public $typemap(cstype, float) MaxTimeToLive {
    get { return GetMaxTimeToLive(); }
    set { SetMaxTimeToLive(value); }
  }
  public $typemap(cstype, float) MinVelocity {
    get { return GetMinVelocity(); }
    set { SetMinVelocity(value); }
  }
  public $typemap(cstype, float) MaxVelocity {
    get { return GetMaxVelocity(); }
    set { SetMaxVelocity(value); }
  }
  public $typemap(cstype, float) MinRotation {
    get { return GetMinRotation(); }
    set { SetMinRotation(value); }
  }
  public $typemap(cstype, float) MaxRotation {
    get { return GetMaxRotation(); }
    set { SetMaxRotation(value); }
  }
  public $typemap(cstype, float) MinRotationSpeed {
    get { return GetMinRotationSpeed(); }
    set { SetMinRotationSpeed(value); }
  }
  public $typemap(cstype, float) MaxRotationSpeed {
    get { return GetMaxRotationSpeed(); }
    set { SetMaxRotationSpeed(value); }
  }
  public $typemap(cstype, float) SizeAdd {
    get { return GetSizeAdd(); }
    set { SetSizeAdd(value); }
  }
  public $typemap(cstype, float) SizeMul {
    get { return GetSizeMul(); }
    set { SetSizeMul(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::ColorFrame> &) ColorFrames {
    get { return GetColorFrames(); }
    set { SetColorFrames(value); }
  }
  public $typemap(cstype, unsigned int) NumColorFrames {
    get { return GetNumColorFrames(); }
    set { SetNumColorFrames(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::TextureFrame> &) TextureFrames {
    get { return GetTextureFrames(); }
    set { SetTextureFrames(value); }
  }
  public $typemap(cstype, unsigned int) NumTextureFrames {
    get { return GetNumTextureFrames(); }
    set { SetNumTextureFrames(value); }
  }
  public $typemap(cstype, Urho3D::FaceCameraMode) FaceCameraMode {
    get { return GetFaceCameraMode(); }
    set { SetFaceCameraMode(value); }
  }
  public $typemap(cstype, Urho3D::Vector3) RandomDirection {
    get { return GetRandomDirection(); }
  }
  public $typemap(cstype, Urho3D::Vector2) RandomSize {
    get { return GetRandomSize(); }
  }
  public $typemap(cstype, float) RandomVelocity {
    get { return GetRandomVelocity(); }
  }
  public $typemap(cstype, float) RandomTimeToLive {
    get { return GetRandomTimeToLive(); }
  }
  public $typemap(cstype, float) RandomRotationSpeed {
    get { return GetRandomRotationSpeed(); }
  }
  public $typemap(cstype, float) RandomRotation {
    get { return GetRandomRotation(); }
  }
%}
%csmethodmodifiers Urho3D::ParticleEffect::GetMaterial "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMaterial "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetNumParticles "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetNumParticles "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetUpdateInvisible "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetUpdateInvisible "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetAnimationLodBias "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetAnimationLodBias "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetEmitterType "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetEmitterType "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetEmitterSize "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetEmitterSize "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMinDirection "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMinDirection "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMaxDirection "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMaxDirection "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetConstantForce "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetConstantForce "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetDampingForce "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetDampingForce "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetActiveTime "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetActiveTime "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetInactiveTime "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetInactiveTime "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMinEmissionRate "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMinEmissionRate "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMaxEmissionRate "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMaxEmissionRate "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMinParticleSize "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMinParticleSize "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMaxParticleSize "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMaxParticleSize "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMinTimeToLive "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMinTimeToLive "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMaxTimeToLive "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMaxTimeToLive "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMinVelocity "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMinVelocity "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMaxVelocity "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMaxVelocity "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMinRotation "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMinRotation "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMaxRotation "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMaxRotation "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMinRotationSpeed "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMinRotationSpeed "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetMaxRotationSpeed "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetMaxRotationSpeed "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetSizeAdd "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetSizeAdd "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetSizeMul "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetSizeMul "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetColorFrames "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetColorFrames "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetNumColorFrames "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetNumColorFrames "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetTextureFrames "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetTextureFrames "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetNumTextureFrames "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetNumTextureFrames "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetFaceCameraMode "private";
%csmethodmodifiers Urho3D::ParticleEffect::SetFaceCameraMode "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetRandomDirection "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetRandomSize "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetRandomVelocity "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetRandomTimeToLive "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetRandomRotationSpeed "private";
%csmethodmodifiers Urho3D::ParticleEffect::GetRandomRotation "private";
%typemap(cscode) Urho3D::ParticleEmitter %{
  public $typemap(cstype, Urho3D::ParticleEffect *) Effect {
    get { return GetEffect(); }
    set { SetEffect(value); }
  }
  public $typemap(cstype, unsigned int) NumParticles {
    get { return GetNumParticles(); }
    set { SetNumParticles(value); }
  }
  public $typemap(cstype, bool) SerializeParticles {
    get { return GetSerializeParticles(); }
    set { SetSerializeParticles(value); }
  }
  public $typemap(cstype, Urho3D::AutoRemoveMode) AutoRemoveMode {
    get { return GetAutoRemoveMode(); }
    set { SetAutoRemoveMode(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) EffectAttr {
    get { return GetEffectAttr(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) ParticlesAttr {
    get { return GetParticlesAttr(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) ParticleBillboardsAttr {
    get { return GetParticleBillboardsAttr(); }
  }
%}
%csmethodmodifiers Urho3D::ParticleEmitter::GetEffect "private";
%csmethodmodifiers Urho3D::ParticleEmitter::SetEffect "private";
%csmethodmodifiers Urho3D::ParticleEmitter::GetNumParticles "private";
%csmethodmodifiers Urho3D::ParticleEmitter::SetNumParticles "private";
%csmethodmodifiers Urho3D::ParticleEmitter::GetSerializeParticles "private";
%csmethodmodifiers Urho3D::ParticleEmitter::SetSerializeParticles "private";
%csmethodmodifiers Urho3D::ParticleEmitter::GetAutoRemoveMode "private";
%csmethodmodifiers Urho3D::ParticleEmitter::SetAutoRemoveMode "private";
%csmethodmodifiers Urho3D::ParticleEmitter::GetEffectAttr "private";
%csmethodmodifiers Urho3D::ParticleEmitter::GetParticlesAttr "private";
%csmethodmodifiers Urho3D::ParticleEmitter::GetParticleBillboardsAttr "private";
%typemap(cscode) Urho3D::RenderPath %{
  public $typemap(cstype, unsigned int) NumRenderTargets {
    get { return GetNumRenderTargets(); }
  }
  public $typemap(cstype, unsigned int) NumCommands {
    get { return GetNumCommands(); }
  }
%}
%csmethodmodifiers Urho3D::RenderPath::GetNumRenderTargets "private";
%csmethodmodifiers Urho3D::RenderPath::GetNumCommands "private";
%typemap(cscode) Urho3D::RenderSurface %{
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
  }
  public $typemap(cstype, Urho3D::TextureUsage) Usage {
    get { return GetUsage(); }
  }
  public $typemap(cstype, int) MultiSample {
    get { return GetMultiSample(); }
  }
  public $typemap(cstype, bool) AutoResolve {
    get { return GetAutoResolve(); }
  }
  public $typemap(cstype, unsigned int) NumViewports {
    get { return GetNumViewports(); }
    set { SetNumViewports(value); }
  }
  public $typemap(cstype, Urho3D::RenderSurfaceUpdateMode) UpdateMode {
    get { return GetUpdateMode(); }
    set { SetUpdateMode(value); }
  }
  public $typemap(cstype, Urho3D::RenderSurface *) LinkedRenderTarget {
    get { return GetLinkedRenderTarget(); }
    set { SetLinkedRenderTarget(value); }
  }
  public $typemap(cstype, Urho3D::RenderSurface *) LinkedDepthStencil {
    get { return GetLinkedDepthStencil(); }
    set { SetLinkedDepthStencil(value); }
  }
  public $typemap(cstype, Urho3D::Texture *) ParentTexture {
    get { return GetParentTexture(); }
  }
  public $typemap(cstype, void *) Surface {
    get { return GetSurface(); }
  }
  public $typemap(cstype, void *) RenderTargetView {
    get { return GetRenderTargetView(); }
  }
  public $typemap(cstype, void *) ReadOnlyView {
    get { return GetReadOnlyView(); }
  }
  public $typemap(cstype, unsigned int) Target {
    get { return GetTarget(); }
  }
  public $typemap(cstype, unsigned int) RenderBuffer {
    get { return GetRenderBuffer(); }
  }
%}
%csmethodmodifiers Urho3D::RenderSurface::GetWidth "private";
%csmethodmodifiers Urho3D::RenderSurface::GetHeight "private";
%csmethodmodifiers Urho3D::RenderSurface::GetUsage "private";
%csmethodmodifiers Urho3D::RenderSurface::GetMultiSample "private";
%csmethodmodifiers Urho3D::RenderSurface::GetAutoResolve "private";
%csmethodmodifiers Urho3D::RenderSurface::GetNumViewports "private";
%csmethodmodifiers Urho3D::RenderSurface::SetNumViewports "private";
%csmethodmodifiers Urho3D::RenderSurface::GetUpdateMode "private";
%csmethodmodifiers Urho3D::RenderSurface::SetUpdateMode "private";
%csmethodmodifiers Urho3D::RenderSurface::GetLinkedRenderTarget "private";
%csmethodmodifiers Urho3D::RenderSurface::SetLinkedRenderTarget "private";
%csmethodmodifiers Urho3D::RenderSurface::GetLinkedDepthStencil "private";
%csmethodmodifiers Urho3D::RenderSurface::SetLinkedDepthStencil "private";
%csmethodmodifiers Urho3D::RenderSurface::GetParentTexture "private";
%csmethodmodifiers Urho3D::RenderSurface::GetSurface "private";
%csmethodmodifiers Urho3D::RenderSurface::GetRenderTargetView "private";
%csmethodmodifiers Urho3D::RenderSurface::GetReadOnlyView "private";
%csmethodmodifiers Urho3D::RenderSurface::GetTarget "private";
%csmethodmodifiers Urho3D::RenderSurface::GetRenderBuffer "private";
%typemap(cscode) Urho3D::Renderer %{
  public $typemap(cstype, unsigned int) NumViewports {
    get { return GetNumViewports(); }
    set { SetNumViewports(value); }
  }
  public $typemap(cstype, Urho3D::RenderPath *) DefaultRenderPath {
    get { return GetDefaultRenderPath(); }
    set { SetDefaultRenderPath(value); }
  }
  public $typemap(cstype, Urho3D::Technique *) DefaultTechnique {
    get { return GetDefaultTechnique(); }
    set { SetDefaultTechnique(value); }
  }
  public $typemap(cstype, bool) HDRRendering {
    get { return GetHDRRendering(); }
    set { SetHDRRendering(value); }
  }
  public $typemap(cstype, bool) SpecularLighting {
    get { return GetSpecularLighting(); }
    set { SetSpecularLighting(value); }
  }
  public $typemap(cstype, bool) DrawShadows {
    get { return GetDrawShadows(); }
    set { SetDrawShadows(value); }
  }
  public $typemap(cstype, int) TextureAnisotropy {
    get { return GetTextureAnisotropy(); }
    set { SetTextureAnisotropy(value); }
  }
  public $typemap(cstype, Urho3D::TextureFilterMode) TextureFilterMode {
    get { return GetTextureFilterMode(); }
    set { SetTextureFilterMode(value); }
  }
  public $typemap(cstype, Urho3D::MaterialQuality) TextureQuality {
    get { return GetTextureQuality(); }
    set { SetTextureQuality(value); }
  }
  public $typemap(cstype, Urho3D::MaterialQuality) MaterialQuality {
    get { return GetMaterialQuality(); }
    set { SetMaterialQuality(value); }
  }
  public $typemap(cstype, int) ShadowMapSize {
    get { return GetShadowMapSize(); }
    set { SetShadowMapSize(value); }
  }
  public $typemap(cstype, Urho3D::ShadowQuality) ShadowQuality {
    get { return GetShadowQuality(); }
    set { SetShadowQuality(value); }
  }
  public $typemap(cstype, float) ShadowSoftness {
    get { return GetShadowSoftness(); }
    set { SetShadowSoftness(value); }
  }
  public $typemap(cstype, Urho3D::Vector2) VSMShadowParameters {
    get { return GetVSMShadowParameters(); }
  }
  public $typemap(cstype, int) VSMMultiSample {
    get { return GetVSMMultiSample(); }
    set { SetVSMMultiSample(value); }
  }
  public $typemap(cstype, bool) ReuseShadowMaps {
    get { return GetReuseShadowMaps(); }
    set { SetReuseShadowMaps(value); }
  }
  public $typemap(cstype, int) MaxShadowMaps {
    get { return GetMaxShadowMaps(); }
    set { SetMaxShadowMaps(value); }
  }
  public $typemap(cstype, bool) DynamicInstancing {
    get { return GetDynamicInstancing(); }
    set { SetDynamicInstancing(value); }
  }
  public $typemap(cstype, int) NumExtraInstancingBufferElements {
    get { return GetNumExtraInstancingBufferElements(); }
    set { SetNumExtraInstancingBufferElements(value); }
  }
  public $typemap(cstype, int) MinInstances {
    get { return GetMinInstances(); }
    set { SetMinInstances(value); }
  }
  public $typemap(cstype, int) MaxSortedInstances {
    get { return GetMaxSortedInstances(); }
    set { SetMaxSortedInstances(value); }
  }
  public $typemap(cstype, int) MaxOccluderTriangles {
    get { return GetMaxOccluderTriangles(); }
    set { SetMaxOccluderTriangles(value); }
  }
  public $typemap(cstype, int) OcclusionBufferSize {
    get { return GetOcclusionBufferSize(); }
    set { SetOcclusionBufferSize(value); }
  }
  public $typemap(cstype, float) OccluderSizeThreshold {
    get { return GetOccluderSizeThreshold(); }
    set { SetOccluderSizeThreshold(value); }
  }
  public $typemap(cstype, bool) ThreadedOcclusion {
    get { return GetThreadedOcclusion(); }
    set { SetThreadedOcclusion(value); }
  }
  public $typemap(cstype, float) MobileShadowBiasMul {
    get { return GetMobileShadowBiasMul(); }
    set { SetMobileShadowBiasMul(value); }
  }
  public $typemap(cstype, float) MobileShadowBiasAdd {
    get { return GetMobileShadowBiasAdd(); }
    set { SetMobileShadowBiasAdd(value); }
  }
  public $typemap(cstype, float) MobileNormalOffsetMul {
    get { return GetMobileNormalOffsetMul(); }
    set { SetMobileNormalOffsetMul(value); }
  }
  public $typemap(cstype, unsigned int) NumViews {
    get { return GetNumViews(); }
  }
  public $typemap(cstype, unsigned int) NumPrimitives {
    get { return GetNumPrimitives(); }
  }
  public $typemap(cstype, unsigned int) NumBatches {
    get { return GetNumBatches(); }
  }
  public $typemap(cstype, Urho3D::Zone *) DefaultZone {
    get { return GetDefaultZone(); }
  }
  public $typemap(cstype, Urho3D::Material *) DefaultMaterial {
    get { return GetDefaultMaterial(); }
  }
  public $typemap(cstype, Urho3D::Texture2D *) DefaultLightRamp {
    get { return GetDefaultLightRamp(); }
  }
  public $typemap(cstype, Urho3D::Texture2D *) DefaultLightSpot {
    get { return GetDefaultLightSpot(); }
  }
  public $typemap(cstype, Urho3D::TextureCube *) FaceSelectCubeMap {
    get { return GetFaceSelectCubeMap(); }
  }
  public $typemap(cstype, Urho3D::TextureCube *) IndirectionCubeMap {
    get { return GetIndirectionCubeMap(); }
  }
  public $typemap(cstype, Urho3D::VertexBuffer *) InstancingBuffer {
    get { return GetInstancingBuffer(); }
  }
  public $typemap(cstype, const Urho3D::FrameInfo &) FrameInfo {
    get { return GetFrameInfo(); }
  }
  public $typemap(cstype, Urho3D::Geometry *) QuadGeometry {
    get { return GetQuadGeometry(); }
  }
  public $typemap(cstype, Urho3D::Camera *) ShadowCamera {
    get { return GetShadowCamera(); }
  }
%}
%csmethodmodifiers Urho3D::Renderer::GetNumViewports "private";
%csmethodmodifiers Urho3D::Renderer::SetNumViewports "private";
%csmethodmodifiers Urho3D::Renderer::GetDefaultRenderPath "private";
%csmethodmodifiers Urho3D::Renderer::SetDefaultRenderPath "private";
%csmethodmodifiers Urho3D::Renderer::GetDefaultTechnique "private";
%csmethodmodifiers Urho3D::Renderer::SetDefaultTechnique "private";
%csmethodmodifiers Urho3D::Renderer::GetHDRRendering "private";
%csmethodmodifiers Urho3D::Renderer::SetHDRRendering "private";
%csmethodmodifiers Urho3D::Renderer::GetSpecularLighting "private";
%csmethodmodifiers Urho3D::Renderer::SetSpecularLighting "private";
%csmethodmodifiers Urho3D::Renderer::GetDrawShadows "private";
%csmethodmodifiers Urho3D::Renderer::SetDrawShadows "private";
%csmethodmodifiers Urho3D::Renderer::GetTextureAnisotropy "private";
%csmethodmodifiers Urho3D::Renderer::SetTextureAnisotropy "private";
%csmethodmodifiers Urho3D::Renderer::GetTextureFilterMode "private";
%csmethodmodifiers Urho3D::Renderer::SetTextureFilterMode "private";
%csmethodmodifiers Urho3D::Renderer::GetTextureQuality "private";
%csmethodmodifiers Urho3D::Renderer::SetTextureQuality "private";
%csmethodmodifiers Urho3D::Renderer::GetMaterialQuality "private";
%csmethodmodifiers Urho3D::Renderer::SetMaterialQuality "private";
%csmethodmodifiers Urho3D::Renderer::GetShadowMapSize "private";
%csmethodmodifiers Urho3D::Renderer::SetShadowMapSize "private";
%csmethodmodifiers Urho3D::Renderer::GetShadowQuality "private";
%csmethodmodifiers Urho3D::Renderer::SetShadowQuality "private";
%csmethodmodifiers Urho3D::Renderer::GetShadowSoftness "private";
%csmethodmodifiers Urho3D::Renderer::SetShadowSoftness "private";
%csmethodmodifiers Urho3D::Renderer::GetVSMShadowParameters "private";
%csmethodmodifiers Urho3D::Renderer::GetVSMMultiSample "private";
%csmethodmodifiers Urho3D::Renderer::SetVSMMultiSample "private";
%csmethodmodifiers Urho3D::Renderer::GetReuseShadowMaps "private";
%csmethodmodifiers Urho3D::Renderer::SetReuseShadowMaps "private";
%csmethodmodifiers Urho3D::Renderer::GetMaxShadowMaps "private";
%csmethodmodifiers Urho3D::Renderer::SetMaxShadowMaps "private";
%csmethodmodifiers Urho3D::Renderer::GetDynamicInstancing "private";
%csmethodmodifiers Urho3D::Renderer::SetDynamicInstancing "private";
%csmethodmodifiers Urho3D::Renderer::GetNumExtraInstancingBufferElements "private";
%csmethodmodifiers Urho3D::Renderer::SetNumExtraInstancingBufferElements "private";
%csmethodmodifiers Urho3D::Renderer::GetMinInstances "private";
%csmethodmodifiers Urho3D::Renderer::SetMinInstances "private";
%csmethodmodifiers Urho3D::Renderer::GetMaxSortedInstances "private";
%csmethodmodifiers Urho3D::Renderer::SetMaxSortedInstances "private";
%csmethodmodifiers Urho3D::Renderer::GetMaxOccluderTriangles "private";
%csmethodmodifiers Urho3D::Renderer::SetMaxOccluderTriangles "private";
%csmethodmodifiers Urho3D::Renderer::GetOcclusionBufferSize "private";
%csmethodmodifiers Urho3D::Renderer::SetOcclusionBufferSize "private";
%csmethodmodifiers Urho3D::Renderer::GetOccluderSizeThreshold "private";
%csmethodmodifiers Urho3D::Renderer::SetOccluderSizeThreshold "private";
%csmethodmodifiers Urho3D::Renderer::GetThreadedOcclusion "private";
%csmethodmodifiers Urho3D::Renderer::SetThreadedOcclusion "private";
%csmethodmodifiers Urho3D::Renderer::GetMobileShadowBiasMul "private";
%csmethodmodifiers Urho3D::Renderer::SetMobileShadowBiasMul "private";
%csmethodmodifiers Urho3D::Renderer::GetMobileShadowBiasAdd "private";
%csmethodmodifiers Urho3D::Renderer::SetMobileShadowBiasAdd "private";
%csmethodmodifiers Urho3D::Renderer::GetMobileNormalOffsetMul "private";
%csmethodmodifiers Urho3D::Renderer::SetMobileNormalOffsetMul "private";
%csmethodmodifiers Urho3D::Renderer::GetNumViews "private";
%csmethodmodifiers Urho3D::Renderer::GetNumPrimitives "private";
%csmethodmodifiers Urho3D::Renderer::GetNumBatches "private";
%csmethodmodifiers Urho3D::Renderer::GetDefaultZone "private";
%csmethodmodifiers Urho3D::Renderer::GetDefaultMaterial "private";
%csmethodmodifiers Urho3D::Renderer::GetDefaultLightRamp "private";
%csmethodmodifiers Urho3D::Renderer::GetDefaultLightSpot "private";
%csmethodmodifiers Urho3D::Renderer::GetFaceSelectCubeMap "private";
%csmethodmodifiers Urho3D::Renderer::GetIndirectionCubeMap "private";
%csmethodmodifiers Urho3D::Renderer::GetInstancingBuffer "private";
%csmethodmodifiers Urho3D::Renderer::GetFrameInfo "private";
%csmethodmodifiers Urho3D::Renderer::GetQuadGeometry "private";
%csmethodmodifiers Urho3D::Renderer::GetShadowCamera "private";
%typemap(cscode) Urho3D::RibbonTrail %{
  public $typemap(cstype, Urho3D::Material *) Material {
    get { return GetMaterial(); }
    set { SetMaterial(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) MaterialAttr {
    get { return GetMaterialAttr(); }
  }
  public $typemap(cstype, float) VertexDistance {
    get { return GetVertexDistance(); }
    set { SetVertexDistance(value); }
  }
  public $typemap(cstype, float) Width {
    get { return GetWidth(); }
    set { SetWidth(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) StartColor {
    get { return GetStartColor(); }
    set { SetStartColor(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) EndColor {
    get { return GetEndColor(); }
    set { SetEndColor(value); }
  }
  public $typemap(cstype, float) StartScale {
    get { return GetStartScale(); }
    set { SetStartScale(value); }
  }
  public $typemap(cstype, float) EndScale {
    get { return GetEndScale(); }
    set { SetEndScale(value); }
  }
  public $typemap(cstype, float) Lifetime {
    get { return GetLifetime(); }
    set { SetLifetime(value); }
  }
  public $typemap(cstype, float) AnimationLodBias {
    get { return GetAnimationLodBias(); }
    set { SetAnimationLodBias(value); }
  }
  public $typemap(cstype, Urho3D::TrailType) TrailType {
    get { return GetTrailType(); }
    set { SetTrailType(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) BaseVelocity {
    get { return GetBaseVelocity(); }
    set { SetBaseVelocity(value); }
  }
  public $typemap(cstype, unsigned int) TailColumn {
    get { return GetTailColumn(); }
    set { SetTailColumn(value); }
  }
  public $typemap(cstype, bool) UpdateInvisible {
    get { return GetUpdateInvisible(); }
    set { SetUpdateInvisible(value); }
  }
%}
%csmethodmodifiers Urho3D::RibbonTrail::GetMaterial "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetMaterial "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetMaterialAttr "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetVertexDistance "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetVertexDistance "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetWidth "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetWidth "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetStartColor "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetStartColor "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetEndColor "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetEndColor "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetStartScale "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetStartScale "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetEndScale "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetEndScale "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetLifetime "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetLifetime "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetAnimationLodBias "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetAnimationLodBias "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetTrailType "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetTrailType "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetBaseVelocity "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetBaseVelocity "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetTailColumn "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetTailColumn "private";
%csmethodmodifiers Urho3D::RibbonTrail::GetUpdateInvisible "private";
%csmethodmodifiers Urho3D::RibbonTrail::SetUpdateInvisible "private";
%typemap(cscode) Urho3D::Shader %{
  public $typemap(cstype, unsigned int) TimeStamp {
    get { return GetTimeStamp(); }
  }
%}
%csmethodmodifiers Urho3D::Shader::GetTimeStamp "private";
%typemap(cscode) Urho3D::ShaderVariation %{
  public $typemap(cstype, Urho3D::Shader *) Owner {
    get { return GetOwner(); }
  }
  public $typemap(cstype, Urho3D::ShaderType) ShaderType {
    get { return GetShaderType(); }
  }
  public $typemap(cstype, const eastl::string &) Name {
    get { return GetName(); }
    set { SetName(value); }
  }
  public $typemap(cstype, eastl::string) FullName {
    get { return GetFullName(); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::ShaderParameter> &) Parameters {
    get { return GetParameters(); }
  }
  public $typemap(cstype, unsigned long long) ElementHash {
    get { return GetElementHash(); }
  }
  public $typemap(cstype, const eastl::vector<unsigned char> &) ByteCode {
    get { return GetByteCode(); }
  }
  public $typemap(cstype, const eastl::string &) Defines {
    get { return GetDefines(); }
    set { SetDefines(value); }
  }
  public $typemap(cstype, const eastl::string &) CompilerOutput {
    get { return GetCompilerOutput(); }
  }
  /*public _typemap(cstype, const unsigned int *) ConstantBufferSizes {
    get { return GetConstantBufferSizes(); }
  }*/
  public $typemap(cstype, const eastl::string &) DefinesClipPlane {
    get { return GetDefinesClipPlane(); }
  }
%}
%csmethodmodifiers Urho3D::ShaderVariation::GetOwner "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetShaderType "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetName "private";
%csmethodmodifiers Urho3D::ShaderVariation::SetName "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetFullName "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetParameters "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetElementHash "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetByteCode "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetDefines "private";
%csmethodmodifiers Urho3D::ShaderVariation::SetDefines "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetCompilerOutput "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetConstantBufferSizes "private";
%csmethodmodifiers Urho3D::ShaderVariation::GetDefinesClipPlane "private";
%typemap(cscode) Urho3D::Skeleton %{
  public $typemap(cstype, const eastl::vector<Urho3D::Bone> &) Bones {
    get { return GetBones(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Bone> &) ModifiableBones {
    get { return GetModifiableBones(); }
  }
  public $typemap(cstype, unsigned int) NumBones {
    get { return GetNumBones(); }
  }
  public $typemap(cstype, Urho3D::Bone *) RootBone {
    get { return GetRootBone(); }
  }
%}
%csmethodmodifiers Urho3D::Skeleton::GetBones "private";
%csmethodmodifiers Urho3D::Skeleton::GetModifiableBones "private";
%csmethodmodifiers Urho3D::Skeleton::GetNumBones "private";
%csmethodmodifiers Urho3D::Skeleton::GetRootBone "private";
%typemap(cscode) Urho3D::StaticModel %{
  public $typemap(cstype, Urho3D::Model *) Model {
    get { return GetModel(); }
  }
  public $typemap(cstype, unsigned int) NumGeometries {
    get { return GetNumGeometries(); }
  }
  public $typemap(cstype, unsigned int) OcclusionLodLevel {
    get { return GetOcclusionLodLevel(); }
    set { SetOcclusionLodLevel(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) ModelAttr {
    get { return GetModelAttr(); }
  }
  public $typemap(cstype, const Urho3D::ResourceRefList &) MaterialsAttr {
    get { return GetMaterialsAttr(); }
    set { SetMaterialsAttr(value); }
  }
%}
%csmethodmodifiers Urho3D::StaticModel::GetModel "private";
%csmethodmodifiers Urho3D::StaticModel::GetNumGeometries "private";
%csmethodmodifiers Urho3D::StaticModel::GetOcclusionLodLevel "private";
%csmethodmodifiers Urho3D::StaticModel::SetOcclusionLodLevel "private";
%csmethodmodifiers Urho3D::StaticModel::GetModelAttr "private";
%csmethodmodifiers Urho3D::StaticModel::GetMaterialsAttr "private";
%csmethodmodifiers Urho3D::StaticModel::SetMaterialsAttr "private";
%typemap(cscode) Urho3D::StaticModelGroup %{
  public $typemap(cstype, unsigned int) NumInstanceNodes {
    get { return GetNumInstanceNodes(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Variant> &) NodeIDsAttr {
    get { return GetNodeIDsAttr(); }
    set { SetNodeIDsAttr(value); }
  }
%}
%csmethodmodifiers Urho3D::StaticModelGroup::GetNumInstanceNodes "private";
%csmethodmodifiers Urho3D::StaticModelGroup::GetNodeIDsAttr "private";
%csmethodmodifiers Urho3D::StaticModelGroup::SetNodeIDsAttr "private";
%typemap(cscode) Urho3D::Pass %{
  public $typemap(cstype, const eastl::string &) Name {
    get { return GetName(); }
  }
  public $typemap(cstype, unsigned int) Index {
    get { return GetIndex(); }
  }
  public $typemap(cstype, Urho3D::BlendMode) BlendMode {
    get { return GetBlendMode(); }
    set { SetBlendMode(value); }
  }
  public $typemap(cstype, Urho3D::CullMode) CullMode {
    get { return GetCullMode(); }
    set { SetCullMode(value); }
  }
  public $typemap(cstype, Urho3D::CompareMode) DepthTestMode {
    get { return GetDepthTestMode(); }
    set { SetDepthTestMode(value); }
  }
  public $typemap(cstype, Urho3D::PassLightingMode) LightingMode {
    get { return GetLightingMode(); }
    set { SetLightingMode(value); }
  }
  public $typemap(cstype, unsigned int) ShadersLoadedFrameNumber {
    get { return GetShadersLoadedFrameNumber(); }
  }
  public $typemap(cstype, bool) DepthWrite {
    get { return GetDepthWrite(); }
    set { SetDepthWrite(value); }
  }
  public $typemap(cstype, bool) AlphaToCoverage {
    get { return GetAlphaToCoverage(); }
    set { SetAlphaToCoverage(value); }
  }
  public $typemap(cstype, const eastl::string &) VertexShader {
    get { return GetVertexShader(); }
    set { SetVertexShader(value); }
  }
  public $typemap(cstype, const eastl::string &) PixelShader {
    get { return GetPixelShader(); }
    set { SetPixelShader(value); }
  }
  public $typemap(cstype, const eastl::string &) VertexShaderDefines {
    get { return GetVertexShaderDefines(); }
    set { SetVertexShaderDefines(value); }
  }
  public $typemap(cstype, const eastl::string &) PixelShaderDefines {
    get { return GetPixelShaderDefines(); }
    set { SetPixelShaderDefines(value); }
  }
  public $typemap(cstype, const eastl::string &) VertexShaderDefineExcludes {
    get { return GetVertexShaderDefineExcludes(); }
    set { SetVertexShaderDefineExcludes(value); }
  }
  public $typemap(cstype, const eastl::string &) PixelShaderDefineExcludes {
    get { return GetPixelShaderDefineExcludes(); }
    set { SetPixelShaderDefineExcludes(value); }
  }
  public $typemap(cstype, eastl::string) EffectiveVertexShaderDefines {
    get { return GetEffectiveVertexShaderDefines(); }
  }
  public $typemap(cstype, eastl::string) EffectivePixelShaderDefines {
    get { return GetEffectivePixelShaderDefines(); }
  }
%}
%csmethodmodifiers Urho3D::Pass::GetName "private";
%csmethodmodifiers Urho3D::Pass::GetIndex "private";
%csmethodmodifiers Urho3D::Pass::GetBlendMode "private";
%csmethodmodifiers Urho3D::Pass::SetBlendMode "private";
%csmethodmodifiers Urho3D::Pass::GetCullMode "private";
%csmethodmodifiers Urho3D::Pass::SetCullMode "private";
%csmethodmodifiers Urho3D::Pass::GetDepthTestMode "private";
%csmethodmodifiers Urho3D::Pass::SetDepthTestMode "private";
%csmethodmodifiers Urho3D::Pass::GetLightingMode "private";
%csmethodmodifiers Urho3D::Pass::SetLightingMode "private";
%csmethodmodifiers Urho3D::Pass::GetShadersLoadedFrameNumber "private";
%csmethodmodifiers Urho3D::Pass::GetDepthWrite "private";
%csmethodmodifiers Urho3D::Pass::SetDepthWrite "private";
%csmethodmodifiers Urho3D::Pass::GetAlphaToCoverage "private";
%csmethodmodifiers Urho3D::Pass::SetAlphaToCoverage "private";
%csmethodmodifiers Urho3D::Pass::GetVertexShader "private";
%csmethodmodifiers Urho3D::Pass::SetVertexShader "private";
%csmethodmodifiers Urho3D::Pass::GetPixelShader "private";
%csmethodmodifiers Urho3D::Pass::SetPixelShader "private";
%csmethodmodifiers Urho3D::Pass::GetVertexShaderDefines "private";
%csmethodmodifiers Urho3D::Pass::SetVertexShaderDefines "private";
%csmethodmodifiers Urho3D::Pass::GetPixelShaderDefines "private";
%csmethodmodifiers Urho3D::Pass::SetPixelShaderDefines "private";
%csmethodmodifiers Urho3D::Pass::GetVertexShaderDefineExcludes "private";
%csmethodmodifiers Urho3D::Pass::SetVertexShaderDefineExcludes "private";
%csmethodmodifiers Urho3D::Pass::GetPixelShaderDefineExcludes "private";
%csmethodmodifiers Urho3D::Pass::SetPixelShaderDefineExcludes "private";
%csmethodmodifiers Urho3D::Pass::GetEffectiveVertexShaderDefines "private";
%csmethodmodifiers Urho3D::Pass::GetEffectivePixelShaderDefines "private";
%typemap(cscode) Urho3D::Technique %{
  public $typemap(cstype, unsigned int) NumPasses {
    get { return GetNumPasses(); }
  }
  public $typemap(cstype, eastl::vector<eastl::string>) PassNames {
    get { return GetPassNames(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Pass *>) Passes {
    get { return GetPasses(); }
  }
%}
%csmethodmodifiers Urho3D::Technique::GetNumPasses "private";
%csmethodmodifiers Urho3D::Technique::GetPassNames "private";
%csmethodmodifiers Urho3D::Technique::GetPasses "private";
%typemap(cscode) Urho3D::Terrain %{
  public $typemap(cstype, int) PatchSize {
    get { return GetPatchSize(); }
    set { SetPatchSize(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) Spacing {
    get { return GetSpacing(); }
    set { SetSpacing(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) NumVertices {
    get { return GetNumVertices(); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) NumPatches {
    get { return GetNumPatches(); }
  }
  public $typemap(cstype, unsigned int) MaxLodLevels {
    get { return GetMaxLodLevels(); }
    set { SetMaxLodLevels(value); }
  }
  public $typemap(cstype, unsigned int) OcclusionLodLevel {
    get { return GetOcclusionLodLevel(); }
    set { SetOcclusionLodLevel(value); }
  }
  public $typemap(cstype, bool) Smoothing {
    get { return GetSmoothing(); }
    set { SetSmoothing(value); }
  }
  public $typemap(cstype, Urho3D::Image *) HeightMap {
    get { return GetHeightMap(); }
    set { SetHeightMap(value); }
  }
  public $typemap(cstype, Urho3D::Material *) Material {
    get { return GetMaterial(); }
    set { SetMaterial(value); }
  }
  public $typemap(cstype, Urho3D::Terrain *) NorthNeighbor {
    get { return GetNorthNeighbor(); }
    set { SetNorthNeighbor(value); }
  }
  public $typemap(cstype, Urho3D::Terrain *) SouthNeighbor {
    get { return GetSouthNeighbor(); }
    set { SetSouthNeighbor(value); }
  }
  public $typemap(cstype, Urho3D::Terrain *) WestNeighbor {
    get { return GetWestNeighbor(); }
    set { SetWestNeighbor(value); }
  }
  public $typemap(cstype, Urho3D::Terrain *) EastNeighbor {
    get { return GetEastNeighbor(); }
    set { SetEastNeighbor(value); }
  }
  /*public _typemap(cstype, eastl::shared_array<float>) HeightData {
    get { return GetHeightData(); }
  }*/
  public $typemap(cstype, float) DrawDistance {
    get { return GetDrawDistance(); }
    set { SetDrawDistance(value); }
  }
  public $typemap(cstype, float) ShadowDistance {
    get { return GetShadowDistance(); }
    set { SetShadowDistance(value); }
  }
  public $typemap(cstype, float) LodBias {
    get { return GetLodBias(); }
    set { SetLodBias(value); }
  }
  public $typemap(cstype, unsigned int) ViewMask {
    get { return GetViewMask(); }
    set { SetViewMask(value); }
  }
  public $typemap(cstype, unsigned int) LightMask {
    get { return GetLightMask(); }
    set { SetLightMask(value); }
  }
  public $typemap(cstype, unsigned int) ShadowMask {
    get { return GetShadowMask(); }
    set { SetShadowMask(value); }
  }
  public $typemap(cstype, unsigned int) ZoneMask {
    get { return GetZoneMask(); }
    set { SetZoneMask(value); }
  }
  public $typemap(cstype, unsigned int) MaxLights {
    get { return GetMaxLights(); }
    set { SetMaxLights(value); }
  }
  public $typemap(cstype, bool) CastShadows {
    get { return GetCastShadows(); }
    set { SetCastShadows(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) HeightMapAttr {
    get { return GetHeightMapAttr(); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) MaterialAttr {
    get { return GetMaterialAttr(); }
  }
%}
%csmethodmodifiers Urho3D::Terrain::GetPatchSize "private";
%csmethodmodifiers Urho3D::Terrain::SetPatchSize "private";
%csmethodmodifiers Urho3D::Terrain::GetSpacing "private";
%csmethodmodifiers Urho3D::Terrain::SetSpacing "private";
%csmethodmodifiers Urho3D::Terrain::GetNumVertices "private";
%csmethodmodifiers Urho3D::Terrain::GetNumPatches "private";
%csmethodmodifiers Urho3D::Terrain::GetMaxLodLevels "private";
%csmethodmodifiers Urho3D::Terrain::SetMaxLodLevels "private";
%csmethodmodifiers Urho3D::Terrain::GetOcclusionLodLevel "private";
%csmethodmodifiers Urho3D::Terrain::SetOcclusionLodLevel "private";
%csmethodmodifiers Urho3D::Terrain::GetSmoothing "private";
%csmethodmodifiers Urho3D::Terrain::SetSmoothing "private";
%csmethodmodifiers Urho3D::Terrain::GetHeightMap "private";
%csmethodmodifiers Urho3D::Terrain::SetHeightMap "private";
%csmethodmodifiers Urho3D::Terrain::GetMaterial "private";
%csmethodmodifiers Urho3D::Terrain::SetMaterial "private";
%csmethodmodifiers Urho3D::Terrain::GetNorthNeighbor "private";
%csmethodmodifiers Urho3D::Terrain::SetNorthNeighbor "private";
%csmethodmodifiers Urho3D::Terrain::GetSouthNeighbor "private";
%csmethodmodifiers Urho3D::Terrain::SetSouthNeighbor "private";
%csmethodmodifiers Urho3D::Terrain::GetWestNeighbor "private";
%csmethodmodifiers Urho3D::Terrain::SetWestNeighbor "private";
%csmethodmodifiers Urho3D::Terrain::GetEastNeighbor "private";
%csmethodmodifiers Urho3D::Terrain::SetEastNeighbor "private";
%csmethodmodifiers Urho3D::Terrain::GetHeightData "private";
%csmethodmodifiers Urho3D::Terrain::GetDrawDistance "private";
%csmethodmodifiers Urho3D::Terrain::SetDrawDistance "private";
%csmethodmodifiers Urho3D::Terrain::GetShadowDistance "private";
%csmethodmodifiers Urho3D::Terrain::SetShadowDistance "private";
%csmethodmodifiers Urho3D::Terrain::GetLodBias "private";
%csmethodmodifiers Urho3D::Terrain::SetLodBias "private";
%csmethodmodifiers Urho3D::Terrain::GetViewMask "private";
%csmethodmodifiers Urho3D::Terrain::SetViewMask "private";
%csmethodmodifiers Urho3D::Terrain::GetLightMask "private";
%csmethodmodifiers Urho3D::Terrain::SetLightMask "private";
%csmethodmodifiers Urho3D::Terrain::GetShadowMask "private";
%csmethodmodifiers Urho3D::Terrain::SetShadowMask "private";
%csmethodmodifiers Urho3D::Terrain::GetZoneMask "private";
%csmethodmodifiers Urho3D::Terrain::SetZoneMask "private";
%csmethodmodifiers Urho3D::Terrain::GetMaxLights "private";
%csmethodmodifiers Urho3D::Terrain::SetMaxLights "private";
%csmethodmodifiers Urho3D::Terrain::GetCastShadows "private";
%csmethodmodifiers Urho3D::Terrain::SetCastShadows "private";
%csmethodmodifiers Urho3D::Terrain::GetHeightMapAttr "private";
%csmethodmodifiers Urho3D::Terrain::GetMaterialAttr "private";
%typemap(cscode) Urho3D::TerrainPatch %{
  public $typemap(cstype, Urho3D::Geometry *) Geometry {
    get { return GetGeometry(); }
  }
  public $typemap(cstype, Urho3D::Geometry *) MaxLodGeometry {
    get { return GetMaxLodGeometry(); }
  }
  public $typemap(cstype, Urho3D::Geometry *) OcclusionGeometry {
    get { return GetOcclusionGeometry(); }
  }
  public $typemap(cstype, Urho3D::VertexBuffer *) VertexBuffer {
    get { return GetVertexBuffer(); }
  }
  public $typemap(cstype, Urho3D::Terrain *) Owner {
    get { return GetOwner(); }
    set { SetOwner(value); }
  }
  public $typemap(cstype, Urho3D::TerrainPatch *) NorthPatch {
    get { return GetNorthPatch(); }
  }
  public $typemap(cstype, Urho3D::TerrainPatch *) SouthPatch {
    get { return GetSouthPatch(); }
  }
  public $typemap(cstype, Urho3D::TerrainPatch *) WestPatch {
    get { return GetWestPatch(); }
  }
  public $typemap(cstype, Urho3D::TerrainPatch *) EastPatch {
    get { return GetEastPatch(); }
  }
  public $typemap(cstype, eastl::vector<float> &) LodErrors {
    get { return GetLodErrors(); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) Coordinates {
    get { return GetCoordinates(); }
    set { SetCoordinates(value); }
  }
  public $typemap(cstype, unsigned int) LodLevel {
    get { return GetLodLevel(); }
  }
%}
%csmethodmodifiers Urho3D::TerrainPatch::GetGeometry "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetMaxLodGeometry "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetOcclusionGeometry "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetVertexBuffer "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetOwner "private";
%csmethodmodifiers Urho3D::TerrainPatch::SetOwner "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetNorthPatch "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetSouthPatch "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetWestPatch "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetEastPatch "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetLodErrors "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetCoordinates "private";
%csmethodmodifiers Urho3D::TerrainPatch::SetCoordinates "private";
%csmethodmodifiers Urho3D::TerrainPatch::GetLodLevel "private";
%typemap(cscode) Urho3D::Texture %{
  public $typemap(cstype, unsigned int) Format {
    get { return GetFormat(); }
  }
  public $typemap(cstype, unsigned int) Levels {
    get { return GetLevels(); }
  }
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
  }
  public $typemap(cstype, int) Depth {
    get { return GetDepth(); }
  }
  public $typemap(cstype, Urho3D::TextureFilterMode) FilterMode {
    get { return GetFilterMode(); }
    set { SetFilterMode(value); }
  }
  public $typemap(cstype, unsigned int) Anisotropy {
    get { return GetAnisotropy(); }
    set { SetAnisotropy(value); }
  }
  public $typemap(cstype, bool) ShadowCompare {
    get { return GetShadowCompare(); }
    set { SetShadowCompare(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) BorderColor {
    get { return GetBorderColor(); }
    set { SetBorderColor(value); }
  }
  public $typemap(cstype, bool) SRGB {
    get { return GetSRGB(); }
    set { SetSRGB(value); }
  }
  public $typemap(cstype, int) MultiSample {
    get { return GetMultiSample(); }
  }
  public $typemap(cstype, bool) AutoResolve {
    get { return GetAutoResolve(); }
  }
  public $typemap(cstype, bool) LevelsDirty {
    get { return GetLevelsDirty(); }
  }
  public $typemap(cstype, Urho3D::Texture *) BackupTexture {
    get { return GetBackupTexture(); }
    set { SetBackupTexture(value); }
  }
  public $typemap(cstype, Urho3D::TextureUsage) Usage {
    get { return GetUsage(); }
  }
  public $typemap(cstype, unsigned int) Components {
    get { return GetComponents(); }
  }
  public $typemap(cstype, bool) ParametersDirty {
    get { return GetParametersDirty(); }
  }
  public $typemap(cstype, void *) ShaderResourceView {
    get { return GetShaderResourceView(); }
  }
  public $typemap(cstype, void *) Sampler {
    get { return GetSampler(); }
  }
  public $typemap(cstype, void *) ResolveTexture {
    get { return GetResolveTexture(); }
  }
  public $typemap(cstype, unsigned int) Target {
    get { return GetTarget(); }
  }
%}
%csmethodmodifiers Urho3D::Texture::GetFormat "private";
%csmethodmodifiers Urho3D::Texture::GetLevels "private";
%csmethodmodifiers Urho3D::Texture::GetWidth "private";
%csmethodmodifiers Urho3D::Texture::GetHeight "private";
%csmethodmodifiers Urho3D::Texture::GetDepth "private";
%csmethodmodifiers Urho3D::Texture::GetFilterMode "private";
%csmethodmodifiers Urho3D::Texture::SetFilterMode "private";
%csmethodmodifiers Urho3D::Texture::GetAnisotropy "private";
%csmethodmodifiers Urho3D::Texture::SetAnisotropy "private";
%csmethodmodifiers Urho3D::Texture::GetShadowCompare "private";
%csmethodmodifiers Urho3D::Texture::SetShadowCompare "private";
%csmethodmodifiers Urho3D::Texture::GetBorderColor "private";
%csmethodmodifiers Urho3D::Texture::SetBorderColor "private";
%csmethodmodifiers Urho3D::Texture::GetSRGB "private";
%csmethodmodifiers Urho3D::Texture::SetSRGB "private";
%csmethodmodifiers Urho3D::Texture::GetMultiSample "private";
%csmethodmodifiers Urho3D::Texture::GetAutoResolve "private";
%csmethodmodifiers Urho3D::Texture::GetLevelsDirty "private";
%csmethodmodifiers Urho3D::Texture::GetBackupTexture "private";
%csmethodmodifiers Urho3D::Texture::SetBackupTexture "private";
%csmethodmodifiers Urho3D::Texture::GetUsage "private";
%csmethodmodifiers Urho3D::Texture::GetComponents "private";
%csmethodmodifiers Urho3D::Texture::GetParametersDirty "private";
%csmethodmodifiers Urho3D::Texture::GetShaderResourceView "private";
%csmethodmodifiers Urho3D::Texture::GetSampler "private";
%csmethodmodifiers Urho3D::Texture::GetResolveTexture "private";
%csmethodmodifiers Urho3D::Texture::GetTarget "private";
%typemap(cscode) Urho3D::Texture2D %{
  public $typemap(cstype, Urho3D::RenderSurface *) RenderSurface {
    get { return GetRenderSurface(); }
  }
%}
%csmethodmodifiers Urho3D::Texture2D::GetRenderSurface "private";
%typemap(cscode) Urho3D::Texture2DArray %{
  public $typemap(cstype, unsigned int) Layers {
    get { return GetLayers(); }
    set { SetLayers(value); }
  }
  public $typemap(cstype, Urho3D::RenderSurface *) RenderSurface {
    get { return GetRenderSurface(); }
  }
%}
%csmethodmodifiers Urho3D::Texture2DArray::GetLayers "private";
%csmethodmodifiers Urho3D::Texture2DArray::SetLayers "private";
%csmethodmodifiers Urho3D::Texture2DArray::GetRenderSurface "private";
%typemap(cscode) Urho3D::VertexBuffer %{
  public $typemap(cstype, unsigned int) VertexCount {
    get { return GetVertexCount(); }
  }
  public $typemap(cstype, Urho3D::VertexMask) ElementMask {
    get { return GetElementMask(); }
  }
  public $typemap(cstype, unsigned char *) ShadowData {
    get { return GetShadowData(); }
  }
  /*public _typemap(cstype, eastl::shared_array<unsigned char>) ShadowDataShared {
    get { return GetShadowDataShared(); }
  }*/
%}
%csmethodmodifiers Urho3D::VertexBuffer::GetVertexCount "private";
%csmethodmodifiers Urho3D::VertexBuffer::GetElementMask "private";
%csmethodmodifiers Urho3D::VertexBuffer::GetShadowData "private";
%csmethodmodifiers Urho3D::VertexBuffer::GetShadowDataShared "private";
%typemap(cscode) Urho3D::View %{
  public $typemap(cstype, Urho3D::Scene *) Scene {
    get { return GetScene(); }
  }
  public $typemap(cstype, Urho3D::Octree *) Octree {
    get { return GetOctree(); }
  }
  public $typemap(cstype, Urho3D::Camera *) Camera {
    get { return GetCamera(); }
  }
  public $typemap(cstype, Urho3D::Camera *) CullCamera {
    get { return GetCullCamera(); }
  }
  public $typemap(cstype, const Urho3D::FrameInfo &) FrameInfo {
    get { return GetFrameInfo(); }
  }
  public $typemap(cstype, Urho3D::RenderSurface *) RenderTarget {
    get { return GetRenderTarget(); }
  }
  public $typemap(cstype, bool) DrawDebug {
    get { return GetDrawDebug(); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) ViewRect {
    get { return GetViewRect(); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) ViewSize {
    get { return GetViewSize(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Drawable *> &) Geometries {
    get { return GetGeometries(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Drawable *> &) Occluders {
    get { return GetOccluders(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Light *> &) Lights {
    get { return GetLights(); }
  }
  /*public _typemap(cstype, const eastl::vector<Urho3D::LightBatchQueue> &) LightQueues {
    get { return GetLightQueues(); }
  }*/
  public $typemap(cstype, Urho3D::OcclusionBuffer *) OcclusionBuffer {
    get { return GetOcclusionBuffer(); }
  }
  public $typemap(cstype, unsigned int) NumActiveOccluders {
    get { return GetNumActiveOccluders(); }
  }
  public $typemap(cstype, Urho3D::View *) SourceView {
    get { return GetSourceView(); }
  }
%}
%csmethodmodifiers Urho3D::View::GetScene "private";
%csmethodmodifiers Urho3D::View::GetOctree "private";
%csmethodmodifiers Urho3D::View::GetCamera "private";
%csmethodmodifiers Urho3D::View::GetCullCamera "private";
%csmethodmodifiers Urho3D::View::GetFrameInfo "private";
%csmethodmodifiers Urho3D::View::GetRenderTarget "private";
%csmethodmodifiers Urho3D::View::GetDrawDebug "private";
%csmethodmodifiers Urho3D::View::GetViewRect "private";
%csmethodmodifiers Urho3D::View::GetViewSize "private";
%csmethodmodifiers Urho3D::View::GetGeometries "private";
%csmethodmodifiers Urho3D::View::GetOccluders "private";
%csmethodmodifiers Urho3D::View::GetLights "private";
%csmethodmodifiers Urho3D::View::GetLightQueues "private";
%csmethodmodifiers Urho3D::View::GetOcclusionBuffer "private";
%csmethodmodifiers Urho3D::View::GetNumActiveOccluders "private";
%csmethodmodifiers Urho3D::View::GetSourceView "private";
%typemap(cscode) Urho3D::Viewport %{
  public $typemap(cstype, Urho3D::Scene *) Scene {
    get { return GetScene(); }
    set { SetScene(value); }
  }
  public $typemap(cstype, Urho3D::Camera *) Camera {
    get { return GetCamera(); }
    set { SetCamera(value); }
  }
  public $typemap(cstype, Urho3D::View *) View {
    get { return GetView(); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) Rect {
    get { return GetRect(); }
    set { SetRect(value); }
  }
  public $typemap(cstype, Urho3D::RenderPath *) RenderPath {
    get { return GetRenderPath(); }
    set { SetRenderPath(value); }
  }
  public $typemap(cstype, bool) DrawDebug {
    get { return GetDrawDebug(); }
    set { SetDrawDebug(value); }
  }
  public $typemap(cstype, Urho3D::Camera *) CullCamera {
    get { return GetCullCamera(); }
    set { SetCullCamera(value); }
  }
%}
%csmethodmodifiers Urho3D::Viewport::GetScene "private";
%csmethodmodifiers Urho3D::Viewport::SetScene "private";
%csmethodmodifiers Urho3D::Viewport::GetCamera "private";
%csmethodmodifiers Urho3D::Viewport::SetCamera "private";
%csmethodmodifiers Urho3D::Viewport::GetView "private";
%csmethodmodifiers Urho3D::Viewport::GetRect "private";
%csmethodmodifiers Urho3D::Viewport::SetRect "private";
%csmethodmodifiers Urho3D::Viewport::GetRenderPath "private";
%csmethodmodifiers Urho3D::Viewport::SetRenderPath "private";
%csmethodmodifiers Urho3D::Viewport::GetDrawDebug "private";
%csmethodmodifiers Urho3D::Viewport::SetDrawDebug "private";
%csmethodmodifiers Urho3D::Viewport::GetCullCamera "private";
%csmethodmodifiers Urho3D::Viewport::SetCullCamera "private";
%typemap(cscode) Urho3D::Zone %{
  public $typemap(cstype, const Urho3D::Matrix3x4 &) InverseWorldTransform {
    get { return GetInverseWorldTransform(); }
  }
  public $typemap(cstype, const Urho3D::Color &) AmbientColor {
    get { return GetAmbientColor(); }
    set { SetAmbientColor(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) AmbientStartColor {
    get { return GetAmbientStartColor(); }
  }
  public $typemap(cstype, const Urho3D::Color &) AmbientEndColor {
    get { return GetAmbientEndColor(); }
  }
  public $typemap(cstype, const Urho3D::Color &) FogColor {
    get { return GetFogColor(); }
    set { SetFogColor(value); }
  }
  public $typemap(cstype, float) FogStart {
    get { return GetFogStart(); }
    set { SetFogStart(value); }
  }
  public $typemap(cstype, float) FogEnd {
    get { return GetFogEnd(); }
    set { SetFogEnd(value); }
  }
  public $typemap(cstype, float) FogHeight {
    get { return GetFogHeight(); }
    set { SetFogHeight(value); }
  }
  public $typemap(cstype, float) FogHeightScale {
    get { return GetFogHeightScale(); }
    set { SetFogHeightScale(value); }
  }
  public $typemap(cstype, int) Priority {
    get { return GetPriority(); }
    set { SetPriority(value); }
  }
  public $typemap(cstype, bool) HeightFog {
    get { return GetHeightFog(); }
    set { SetHeightFog(value); }
  }
  public $typemap(cstype, bool) Override {
    get { return GetOverride(); }
    set { SetOverride(value); }
  }
  public $typemap(cstype, bool) AmbientGradient {
    get { return GetAmbientGradient(); }
    set { SetAmbientGradient(value); }
  }
  public $typemap(cstype, Urho3D::Texture *) ZoneTexture {
    get { return GetZoneTexture(); }
    set { SetZoneTexture(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) ZoneTextureAttr {
    get { return GetZoneTextureAttr(); }
  }
%}
%csmethodmodifiers Urho3D::Zone::GetInverseWorldTransform "private";
%csmethodmodifiers Urho3D::Zone::GetAmbientColor "private";
%csmethodmodifiers Urho3D::Zone::SetAmbientColor "private";
%csmethodmodifiers Urho3D::Zone::GetAmbientStartColor "private";
%csmethodmodifiers Urho3D::Zone::GetAmbientEndColor "private";
%csmethodmodifiers Urho3D::Zone::GetFogColor "private";
%csmethodmodifiers Urho3D::Zone::SetFogColor "private";
%csmethodmodifiers Urho3D::Zone::GetFogStart "private";
%csmethodmodifiers Urho3D::Zone::SetFogStart "private";
%csmethodmodifiers Urho3D::Zone::GetFogEnd "private";
%csmethodmodifiers Urho3D::Zone::SetFogEnd "private";
%csmethodmodifiers Urho3D::Zone::GetFogHeight "private";
%csmethodmodifiers Urho3D::Zone::SetFogHeight "private";
%csmethodmodifiers Urho3D::Zone::GetFogHeightScale "private";
%csmethodmodifiers Urho3D::Zone::SetFogHeightScale "private";
%csmethodmodifiers Urho3D::Zone::GetPriority "private";
%csmethodmodifiers Urho3D::Zone::SetPriority "private";
%csmethodmodifiers Urho3D::Zone::GetHeightFog "private";
%csmethodmodifiers Urho3D::Zone::SetHeightFog "private";
%csmethodmodifiers Urho3D::Zone::GetOverride "private";
%csmethodmodifiers Urho3D::Zone::SetOverride "private";
%csmethodmodifiers Urho3D::Zone::GetAmbientGradient "private";
%csmethodmodifiers Urho3D::Zone::SetAmbientGradient "private";
%csmethodmodifiers Urho3D::Zone::GetZoneTexture "private";
%csmethodmodifiers Urho3D::Zone::SetZoneTexture "private";
%csmethodmodifiers Urho3D::Zone::GetZoneTextureAttr "private";
