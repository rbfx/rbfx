%typemap(cscode) Urho3D::AnimatedSprite2D %{
  public $typemap(cstype, Urho3D::AnimationSet2D *) AnimationSet {
    get { return GetAnimationSet(); }
    set { SetAnimationSet(value); }
  }
  public $typemap(cstype, const eastl::string &) Entity {
    get { return GetEntity(); }
    set { SetEntity(value); }
  }
  public $typemap(cstype, const eastl::string &) Animation {
    get { return GetAnimation(); }
  }
  public $typemap(cstype, LoopMode2D) LoopMode {
    get { return GetLoopMode(); }
    set { SetLoopMode(value); }
  }
  public $typemap(cstype, float) Speed {
    get { return GetSpeed(); }
    set { SetSpeed(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) AnimationSetAttr {
    get { return GetAnimationSetAttr(); }
  }
%}
%csmethodmodifiers Urho3D::AnimatedSprite2D::GetAnimationSet "private";
%csmethodmodifiers Urho3D::AnimatedSprite2D::SetAnimationSet "private";
%csmethodmodifiers Urho3D::AnimatedSprite2D::GetEntity "private";
%csmethodmodifiers Urho3D::AnimatedSprite2D::SetEntity "private";
%csmethodmodifiers Urho3D::AnimatedSprite2D::GetAnimation "private";
%csmethodmodifiers Urho3D::AnimatedSprite2D::GetLoopMode "private";
%csmethodmodifiers Urho3D::AnimatedSprite2D::SetLoopMode "private";
%csmethodmodifiers Urho3D::AnimatedSprite2D::GetSpeed "private";
%csmethodmodifiers Urho3D::AnimatedSprite2D::SetSpeed "private";
%csmethodmodifiers Urho3D::AnimatedSprite2D::GetAnimationSetAttr "private";
%typemap(cscode) Urho3D::AnimationSet2D %{
  public $typemap(cstype, unsigned int) NumAnimations {
    get { return GetNumAnimations(); }
  }
  public $typemap(cstype, Urho3D::Sprite2D *) Sprite {
    get { return GetSprite(); }
  }
  /*public _typemap(cstype, Urho3D::Spriter::SpriterData *) SpriterData {
    get { return GetSpriterData(); }
  }*/
%}
%csmethodmodifiers Urho3D::AnimationSet2D::GetNumAnimations "private";
%csmethodmodifiers Urho3D::AnimationSet2D::GetSprite "private";
%csmethodmodifiers Urho3D::AnimationSet2D::GetSpriterData "private";
%typemap(cscode) Urho3D::CollisionBox2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) Size {
    get { return GetSize(); }
    set { SetSize(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Center {
    get { return GetCenter(); }
    set { SetCenter(value); }
  }
  public $typemap(cstype, float) Angle {
    get { return GetAngle(); }
    set { SetAngle(value); }
  }
%}
%csmethodmodifiers Urho3D::CollisionBox2D::GetSize "private";
%csmethodmodifiers Urho3D::CollisionBox2D::SetSize "private";
%csmethodmodifiers Urho3D::CollisionBox2D::GetCenter "private";
%csmethodmodifiers Urho3D::CollisionBox2D::SetCenter "private";
%csmethodmodifiers Urho3D::CollisionBox2D::GetAngle "private";
%csmethodmodifiers Urho3D::CollisionBox2D::SetAngle "private";
%typemap(cscode) Urho3D::CollisionChain2D %{
  public $typemap(cstype, bool) Loop {
    get { return GetLoop(); }
    set { SetLoop(value); }
  }
  public $typemap(cstype, unsigned int) VertexCount {
    get { return GetVertexCount(); }
    set { SetVertexCount(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Vector2> &) Vertices {
    get { return GetVertices(); }
    set { SetVertices(value); }
  }
  public $typemap(cstype, eastl::vector<unsigned char>) VerticesAttr {
    get { return GetVerticesAttr(); }
  }
%}
%csmethodmodifiers Urho3D::CollisionChain2D::GetLoop "private";
%csmethodmodifiers Urho3D::CollisionChain2D::SetLoop "private";
%csmethodmodifiers Urho3D::CollisionChain2D::GetVertexCount "private";
%csmethodmodifiers Urho3D::CollisionChain2D::SetVertexCount "private";
%csmethodmodifiers Urho3D::CollisionChain2D::GetVertices "private";
%csmethodmodifiers Urho3D::CollisionChain2D::SetVertices "private";
%csmethodmodifiers Urho3D::CollisionChain2D::GetVerticesAttr "private";
%typemap(cscode) Urho3D::CollisionCircle2D %{
  public $typemap(cstype, float) Radius {
    get { return GetRadius(); }
    set { SetRadius(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Center {
    get { return GetCenter(); }
    set { SetCenter(value); }
  }
%}
%csmethodmodifiers Urho3D::CollisionCircle2D::GetRadius "private";
%csmethodmodifiers Urho3D::CollisionCircle2D::SetRadius "private";
%csmethodmodifiers Urho3D::CollisionCircle2D::GetCenter "private";
%csmethodmodifiers Urho3D::CollisionCircle2D::SetCenter "private";
%typemap(cscode) Urho3D::CollisionEdge2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) Vertex1 {
    get { return GetVertex1(); }
    set { SetVertex1(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Vertex2 {
    get { return GetVertex2(); }
    set { SetVertex2(value); }
  }
%}
%csmethodmodifiers Urho3D::CollisionEdge2D::GetVertex1 "private";
%csmethodmodifiers Urho3D::CollisionEdge2D::SetVertex1 "private";
%csmethodmodifiers Urho3D::CollisionEdge2D::GetVertex2 "private";
%csmethodmodifiers Urho3D::CollisionEdge2D::SetVertex2 "private";
%typemap(cscode) Urho3D::CollisionPolygon2D %{
  public $typemap(cstype, unsigned int) VertexCount {
    get { return GetVertexCount(); }
    set { SetVertexCount(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Vector2> &) Vertices {
    get { return GetVertices(); }
    set { SetVertices(value); }
  }
  public $typemap(cstype, eastl::vector<unsigned char>) VerticesAttr {
    get { return GetVerticesAttr(); }
  }
%}
%csmethodmodifiers Urho3D::CollisionPolygon2D::GetVertexCount "private";
%csmethodmodifiers Urho3D::CollisionPolygon2D::SetVertexCount "private";
%csmethodmodifiers Urho3D::CollisionPolygon2D::GetVertices "private";
%csmethodmodifiers Urho3D::CollisionPolygon2D::SetVertices "private";
%csmethodmodifiers Urho3D::CollisionPolygon2D::GetVerticesAttr "private";
%typemap(cscode) Urho3D::CollisionShape2D %{
  public $typemap(cstype, int) CategoryBits {
    get { return GetCategoryBits(); }
    set { SetCategoryBits(value); }
  }
  public $typemap(cstype, int) MaskBits {
    get { return GetMaskBits(); }
    set { SetMaskBits(value); }
  }
  public $typemap(cstype, int) GroupIndex {
    get { return GetGroupIndex(); }
    set { SetGroupIndex(value); }
  }
  public $typemap(cstype, float) Density {
    get { return GetDensity(); }
    set { SetDensity(value); }
  }
  public $typemap(cstype, float) Friction {
    get { return GetFriction(); }
    set { SetFriction(value); }
  }
  public $typemap(cstype, float) Restitution {
    get { return GetRestitution(); }
    set { SetRestitution(value); }
  }
  public $typemap(cstype, float) Mass {
    get { return GetMass(); }
  }
  public $typemap(cstype, float) Inertia {
    get { return GetInertia(); }
  }
  public $typemap(cstype, Urho3D::Vector2) MassCenter {
    get { return GetMassCenter(); }
  }
  public $typemap(cstype, b2Fixture *) Fixture {
    get { return GetFixture(); }
  }
%}
%csmethodmodifiers Urho3D::CollisionShape2D::GetCategoryBits "private";
%csmethodmodifiers Urho3D::CollisionShape2D::SetCategoryBits "private";
%csmethodmodifiers Urho3D::CollisionShape2D::GetMaskBits "private";
%csmethodmodifiers Urho3D::CollisionShape2D::SetMaskBits "private";
%csmethodmodifiers Urho3D::CollisionShape2D::GetGroupIndex "private";
%csmethodmodifiers Urho3D::CollisionShape2D::SetGroupIndex "private";
%csmethodmodifiers Urho3D::CollisionShape2D::GetDensity "private";
%csmethodmodifiers Urho3D::CollisionShape2D::SetDensity "private";
%csmethodmodifiers Urho3D::CollisionShape2D::GetFriction "private";
%csmethodmodifiers Urho3D::CollisionShape2D::SetFriction "private";
%csmethodmodifiers Urho3D::CollisionShape2D::GetRestitution "private";
%csmethodmodifiers Urho3D::CollisionShape2D::SetRestitution "private";
%csmethodmodifiers Urho3D::CollisionShape2D::GetMass "private";
%csmethodmodifiers Urho3D::CollisionShape2D::GetInertia "private";
%csmethodmodifiers Urho3D::CollisionShape2D::GetMassCenter "private";
%csmethodmodifiers Urho3D::CollisionShape2D::GetFixture "private";
%typemap(cscode) Urho3D::Constraint2D %{
  public $typemap(cstype, Urho3D::RigidBody2D *) OwnerBody {
    get { return GetOwnerBody(); }
  }
  public $typemap(cstype, Urho3D::RigidBody2D *) OtherBody {
    get { return GetOtherBody(); }
    set { SetOtherBody(value); }
  }
  public $typemap(cstype, bool) CollideConnected {
    get { return GetCollideConnected(); }
    set { SetCollideConnected(value); }
  }
  public $typemap(cstype, Urho3D::Constraint2D *) AttachedConstraint {
    get { return GetAttachedConstraint(); }
    set { SetAttachedConstraint(value); }
  }
  public $typemap(cstype, b2Joint *) Joint {
    get { return GetJoint(); }
  }
%}
%csmethodmodifiers Urho3D::Constraint2D::GetOwnerBody "private";
%csmethodmodifiers Urho3D::Constraint2D::GetOtherBody "private";
%csmethodmodifiers Urho3D::Constraint2D::SetOtherBody "private";
%csmethodmodifiers Urho3D::Constraint2D::GetCollideConnected "private";
%csmethodmodifiers Urho3D::Constraint2D::SetCollideConnected "private";
%csmethodmodifiers Urho3D::Constraint2D::GetAttachedConstraint "private";
%csmethodmodifiers Urho3D::Constraint2D::SetAttachedConstraint "private";
%csmethodmodifiers Urho3D::Constraint2D::GetJoint "private";
%typemap(cscode) Urho3D::ConstraintDistance2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) OwnerBodyAnchor {
    get { return GetOwnerBodyAnchor(); }
    set { SetOwnerBodyAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) OtherBodyAnchor {
    get { return GetOtherBodyAnchor(); }
    set { SetOtherBodyAnchor(value); }
  }
  public $typemap(cstype, float) FrequencyHz {
    get { return GetFrequencyHz(); }
    set { SetFrequencyHz(value); }
  }
  public $typemap(cstype, float) DampingRatio {
    get { return GetDampingRatio(); }
    set { SetDampingRatio(value); }
  }
  public $typemap(cstype, float) Length {
    get { return GetLength(); }
    set { SetLength(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintDistance2D::GetOwnerBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintDistance2D::SetOwnerBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintDistance2D::GetOtherBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintDistance2D::SetOtherBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintDistance2D::GetFrequencyHz "private";
%csmethodmodifiers Urho3D::ConstraintDistance2D::SetFrequencyHz "private";
%csmethodmodifiers Urho3D::ConstraintDistance2D::GetDampingRatio "private";
%csmethodmodifiers Urho3D::ConstraintDistance2D::SetDampingRatio "private";
%csmethodmodifiers Urho3D::ConstraintDistance2D::GetLength "private";
%csmethodmodifiers Urho3D::ConstraintDistance2D::SetLength "private";
%typemap(cscode) Urho3D::ConstraintFriction2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) Anchor {
    get { return GetAnchor(); }
    set { SetAnchor(value); }
  }
  public $typemap(cstype, float) MaxForce {
    get { return GetMaxForce(); }
    set { SetMaxForce(value); }
  }
  public $typemap(cstype, float) MaxTorque {
    get { return GetMaxTorque(); }
    set { SetMaxTorque(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintFriction2D::GetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintFriction2D::SetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintFriction2D::GetMaxForce "private";
%csmethodmodifiers Urho3D::ConstraintFriction2D::SetMaxForce "private";
%csmethodmodifiers Urho3D::ConstraintFriction2D::GetMaxTorque "private";
%csmethodmodifiers Urho3D::ConstraintFriction2D::SetMaxTorque "private";
%typemap(cscode) Urho3D::ConstraintGear2D %{
  public $typemap(cstype, Urho3D::Constraint2D *) OwnerConstraint {
    get { return GetOwnerConstraint(); }
    set { SetOwnerConstraint(value); }
  }
  public $typemap(cstype, Urho3D::Constraint2D *) OtherConstraint {
    get { return GetOtherConstraint(); }
    set { SetOtherConstraint(value); }
  }
  public $typemap(cstype, float) Ratio {
    get { return GetRatio(); }
    set { SetRatio(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintGear2D::GetOwnerConstraint "private";
%csmethodmodifiers Urho3D::ConstraintGear2D::SetOwnerConstraint "private";
%csmethodmodifiers Urho3D::ConstraintGear2D::GetOtherConstraint "private";
%csmethodmodifiers Urho3D::ConstraintGear2D::SetOtherConstraint "private";
%csmethodmodifiers Urho3D::ConstraintGear2D::GetRatio "private";
%csmethodmodifiers Urho3D::ConstraintGear2D::SetRatio "private";
%typemap(cscode) Urho3D::ConstraintMotor2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) LinearOffset {
    get { return GetLinearOffset(); }
    set { SetLinearOffset(value); }
  }
  public $typemap(cstype, float) AngularOffset {
    get { return GetAngularOffset(); }
    set { SetAngularOffset(value); }
  }
  public $typemap(cstype, float) MaxForce {
    get { return GetMaxForce(); }
    set { SetMaxForce(value); }
  }
  public $typemap(cstype, float) MaxTorque {
    get { return GetMaxTorque(); }
    set { SetMaxTorque(value); }
  }
  public $typemap(cstype, float) CorrectionFactor {
    get { return GetCorrectionFactor(); }
    set { SetCorrectionFactor(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintMotor2D::GetLinearOffset "private";
%csmethodmodifiers Urho3D::ConstraintMotor2D::SetLinearOffset "private";
%csmethodmodifiers Urho3D::ConstraintMotor2D::GetAngularOffset "private";
%csmethodmodifiers Urho3D::ConstraintMotor2D::SetAngularOffset "private";
%csmethodmodifiers Urho3D::ConstraintMotor2D::GetMaxForce "private";
%csmethodmodifiers Urho3D::ConstraintMotor2D::SetMaxForce "private";
%csmethodmodifiers Urho3D::ConstraintMotor2D::GetMaxTorque "private";
%csmethodmodifiers Urho3D::ConstraintMotor2D::SetMaxTorque "private";
%csmethodmodifiers Urho3D::ConstraintMotor2D::GetCorrectionFactor "private";
%csmethodmodifiers Urho3D::ConstraintMotor2D::SetCorrectionFactor "private";
%typemap(cscode) Urho3D::ConstraintMouse2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) Target {
    get { return GetTarget(); }
    set { SetTarget(value); }
  }
  public $typemap(cstype, float) MaxForce {
    get { return GetMaxForce(); }
    set { SetMaxForce(value); }
  }
  public $typemap(cstype, float) FrequencyHz {
    get { return GetFrequencyHz(); }
    set { SetFrequencyHz(value); }
  }
  public $typemap(cstype, float) DampingRatio {
    get { return GetDampingRatio(); }
    set { SetDampingRatio(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintMouse2D::GetTarget "private";
%csmethodmodifiers Urho3D::ConstraintMouse2D::SetTarget "private";
%csmethodmodifiers Urho3D::ConstraintMouse2D::GetMaxForce "private";
%csmethodmodifiers Urho3D::ConstraintMouse2D::SetMaxForce "private";
%csmethodmodifiers Urho3D::ConstraintMouse2D::GetFrequencyHz "private";
%csmethodmodifiers Urho3D::ConstraintMouse2D::SetFrequencyHz "private";
%csmethodmodifiers Urho3D::ConstraintMouse2D::GetDampingRatio "private";
%csmethodmodifiers Urho3D::ConstraintMouse2D::SetDampingRatio "private";
%typemap(cscode) Urho3D::ConstraintPrismatic2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) Anchor {
    get { return GetAnchor(); }
    set { SetAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Axis {
    get { return GetAxis(); }
    set { SetAxis(value); }
  }
  public $typemap(cstype, bool) EnableLimit {
    get { return GetEnableLimit(); }
    set { SetEnableLimit(value); }
  }
  public $typemap(cstype, float) LowerTranslation {
    get { return GetLowerTranslation(); }
    set { SetLowerTranslation(value); }
  }
  public $typemap(cstype, float) UpperTranslation {
    get { return GetUpperTranslation(); }
    set { SetUpperTranslation(value); }
  }
  public $typemap(cstype, bool) EnableMotor {
    get { return GetEnableMotor(); }
    set { SetEnableMotor(value); }
  }
  public $typemap(cstype, float) MaxMotorForce {
    get { return GetMaxMotorForce(); }
    set { SetMaxMotorForce(value); }
  }
  public $typemap(cstype, float) MotorSpeed {
    get { return GetMotorSpeed(); }
    set { SetMotorSpeed(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::GetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::SetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::GetAxis "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::SetAxis "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::GetEnableLimit "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::SetEnableLimit "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::GetLowerTranslation "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::SetLowerTranslation "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::GetUpperTranslation "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::SetUpperTranslation "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::GetEnableMotor "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::SetEnableMotor "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::GetMaxMotorForce "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::SetMaxMotorForce "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::GetMotorSpeed "private";
%csmethodmodifiers Urho3D::ConstraintPrismatic2D::SetMotorSpeed "private";
%typemap(cscode) Urho3D::ConstraintPulley2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) OwnerBodyGroundAnchor {
    get { return GetOwnerBodyGroundAnchor(); }
    set { SetOwnerBodyGroundAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) OtherBodyGroundAnchor {
    get { return GetOtherBodyGroundAnchor(); }
    set { SetOtherBodyGroundAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) OwnerBodyAnchor {
    get { return GetOwnerBodyAnchor(); }
    set { SetOwnerBodyAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) OtherBodyAnchor {
    get { return GetOtherBodyAnchor(); }
    set { SetOtherBodyAnchor(value); }
  }
  public $typemap(cstype, float) Ratio {
    get { return GetRatio(); }
    set { SetRatio(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintPulley2D::GetOwnerBodyGroundAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPulley2D::SetOwnerBodyGroundAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPulley2D::GetOtherBodyGroundAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPulley2D::SetOtherBodyGroundAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPulley2D::GetOwnerBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPulley2D::SetOwnerBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPulley2D::GetOtherBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPulley2D::SetOtherBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintPulley2D::GetRatio "private";
%csmethodmodifiers Urho3D::ConstraintPulley2D::SetRatio "private";
%typemap(cscode) Urho3D::ConstraintRevolute2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) Anchor {
    get { return GetAnchor(); }
    set { SetAnchor(value); }
  }
  public $typemap(cstype, bool) EnableLimit {
    get { return GetEnableLimit(); }
    set { SetEnableLimit(value); }
  }
  public $typemap(cstype, float) LowerAngle {
    get { return GetLowerAngle(); }
    set { SetLowerAngle(value); }
  }
  public $typemap(cstype, float) UpperAngle {
    get { return GetUpperAngle(); }
    set { SetUpperAngle(value); }
  }
  public $typemap(cstype, bool) EnableMotor {
    get { return GetEnableMotor(); }
    set { SetEnableMotor(value); }
  }
  public $typemap(cstype, float) MotorSpeed {
    get { return GetMotorSpeed(); }
    set { SetMotorSpeed(value); }
  }
  public $typemap(cstype, float) MaxMotorTorque {
    get { return GetMaxMotorTorque(); }
    set { SetMaxMotorTorque(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintRevolute2D::GetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::SetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::GetEnableLimit "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::SetEnableLimit "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::GetLowerAngle "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::SetLowerAngle "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::GetUpperAngle "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::SetUpperAngle "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::GetEnableMotor "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::SetEnableMotor "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::GetMotorSpeed "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::SetMotorSpeed "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::GetMaxMotorTorque "private";
%csmethodmodifiers Urho3D::ConstraintRevolute2D::SetMaxMotorTorque "private";
%typemap(cscode) Urho3D::ConstraintRope2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) OwnerBodyAnchor {
    get { return GetOwnerBodyAnchor(); }
    set { SetOwnerBodyAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) OtherBodyAnchor {
    get { return GetOtherBodyAnchor(); }
    set { SetOtherBodyAnchor(value); }
  }
  public $typemap(cstype, float) MaxLength {
    get { return GetMaxLength(); }
    set { SetMaxLength(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintRope2D::GetOwnerBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintRope2D::SetOwnerBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintRope2D::GetOtherBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintRope2D::SetOtherBodyAnchor "private";
%csmethodmodifiers Urho3D::ConstraintRope2D::GetMaxLength "private";
%csmethodmodifiers Urho3D::ConstraintRope2D::SetMaxLength "private";
%typemap(cscode) Urho3D::ConstraintWeld2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) Anchor {
    get { return GetAnchor(); }
    set { SetAnchor(value); }
  }
  public $typemap(cstype, float) FrequencyHz {
    get { return GetFrequencyHz(); }
    set { SetFrequencyHz(value); }
  }
  public $typemap(cstype, float) DampingRatio {
    get { return GetDampingRatio(); }
    set { SetDampingRatio(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintWeld2D::GetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintWeld2D::SetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintWeld2D::GetFrequencyHz "private";
%csmethodmodifiers Urho3D::ConstraintWeld2D::SetFrequencyHz "private";
%csmethodmodifiers Urho3D::ConstraintWeld2D::GetDampingRatio "private";
%csmethodmodifiers Urho3D::ConstraintWeld2D::SetDampingRatio "private";
%typemap(cscode) Urho3D::ConstraintWheel2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) Anchor {
    get { return GetAnchor(); }
    set { SetAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Axis {
    get { return GetAxis(); }
    set { SetAxis(value); }
  }
  public $typemap(cstype, bool) EnableMotor {
    get { return GetEnableMotor(); }
    set { SetEnableMotor(value); }
  }
  public $typemap(cstype, float) MaxMotorTorque {
    get { return GetMaxMotorTorque(); }
    set { SetMaxMotorTorque(value); }
  }
  public $typemap(cstype, float) MotorSpeed {
    get { return GetMotorSpeed(); }
    set { SetMotorSpeed(value); }
  }
  public $typemap(cstype, float) FrequencyHz {
    get { return GetFrequencyHz(); }
    set { SetFrequencyHz(value); }
  }
  public $typemap(cstype, float) DampingRatio {
    get { return GetDampingRatio(); }
    set { SetDampingRatio(value); }
  }
%}
%csmethodmodifiers Urho3D::ConstraintWheel2D::GetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::SetAnchor "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::GetAxis "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::SetAxis "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::GetEnableMotor "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::SetEnableMotor "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::GetMaxMotorTorque "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::SetMaxMotorTorque "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::GetMotorSpeed "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::SetMotorSpeed "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::GetFrequencyHz "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::SetFrequencyHz "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::GetDampingRatio "private";
%csmethodmodifiers Urho3D::ConstraintWheel2D::SetDampingRatio "private";
%typemap(cscode) Urho3D::Drawable2D %{
  public $typemap(cstype, int) Layer {
    get { return GetLayer(); }
    set { SetLayer(value); }
  }
  public $typemap(cstype, int) OrderInLayer {
    get { return GetOrderInLayer(); }
    set { SetOrderInLayer(value); }
  }
  /*public _typemap(cstype, const eastl::vector<Urho3D::SourceBatch2D> &) SourceBatches {
    get { return GetSourceBatches(); }
  }*/
%}
%csmethodmodifiers Urho3D::Drawable2D::GetLayer "private";
%csmethodmodifiers Urho3D::Drawable2D::SetLayer "private";
%csmethodmodifiers Urho3D::Drawable2D::GetOrderInLayer "private";
%csmethodmodifiers Urho3D::Drawable2D::SetOrderInLayer "private";
%csmethodmodifiers Urho3D::Drawable2D::GetSourceBatches "private";
%typemap(cscode) Urho3D::ParticleEffect2D %{
  public $typemap(cstype, Urho3D::Sprite2D *) Sprite {
    get { return GetSprite(); }
    set { SetSprite(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) SourcePositionVariance {
    get { return GetSourcePositionVariance(); }
    set { SetSourcePositionVariance(value); }
  }
  public $typemap(cstype, float) Speed {
    get { return GetSpeed(); }
    set { SetSpeed(value); }
  }
  public $typemap(cstype, float) SpeedVariance {
    get { return GetSpeedVariance(); }
    set { SetSpeedVariance(value); }
  }
  public $typemap(cstype, float) ParticleLifeSpan {
    get { return GetParticleLifeSpan(); }
    set { SetParticleLifeSpan(value); }
  }
  public $typemap(cstype, float) ParticleLifespanVariance {
    get { return GetParticleLifespanVariance(); }
    set { SetParticleLifespanVariance(value); }
  }
  public $typemap(cstype, float) Angle {
    get { return GetAngle(); }
    set { SetAngle(value); }
  }
  public $typemap(cstype, float) AngleVariance {
    get { return GetAngleVariance(); }
    set { SetAngleVariance(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Gravity {
    get { return GetGravity(); }
    set { SetGravity(value); }
  }
  public $typemap(cstype, float) RadialAcceleration {
    get { return GetRadialAcceleration(); }
    set { SetRadialAcceleration(value); }
  }
  public $typemap(cstype, float) TangentialAcceleration {
    get { return GetTangentialAcceleration(); }
    set { SetTangentialAcceleration(value); }
  }
  public $typemap(cstype, float) RadialAccelVariance {
    get { return GetRadialAccelVariance(); }
    set { SetRadialAccelVariance(value); }
  }
  public $typemap(cstype, float) TangentialAccelVariance {
    get { return GetTangentialAccelVariance(); }
    set { SetTangentialAccelVariance(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) StartColor {
    get { return GetStartColor(); }
    set { SetStartColor(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) StartColorVariance {
    get { return GetStartColorVariance(); }
    set { SetStartColorVariance(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) FinishColor {
    get { return GetFinishColor(); }
    set { SetFinishColor(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) FinishColorVariance {
    get { return GetFinishColorVariance(); }
    set { SetFinishColorVariance(value); }
  }
  public $typemap(cstype, int) MaxParticles {
    get { return GetMaxParticles(); }
    set { SetMaxParticles(value); }
  }
  public $typemap(cstype, float) StartParticleSize {
    get { return GetStartParticleSize(); }
    set { SetStartParticleSize(value); }
  }
  public $typemap(cstype, float) StartParticleSizeVariance {
    get { return GetStartParticleSizeVariance(); }
    set { SetStartParticleSizeVariance(value); }
  }
  public $typemap(cstype, float) FinishParticleSize {
    get { return GetFinishParticleSize(); }
    set { SetFinishParticleSize(value); }
  }
  public $typemap(cstype, float) FinishParticleSizeVariance {
    get { return GetFinishParticleSizeVariance(); }
    set { SetFinishParticleSizeVariance(value); }
  }
  public $typemap(cstype, float) Duration {
    get { return GetDuration(); }
    set { SetDuration(value); }
  }
  public $typemap(cstype, Urho3D::EmitterType2D) EmitterType {
    get { return GetEmitterType(); }
    set { SetEmitterType(value); }
  }
  public $typemap(cstype, float) MaxRadius {
    get { return GetMaxRadius(); }
    set { SetMaxRadius(value); }
  }
  public $typemap(cstype, float) MaxRadiusVariance {
    get { return GetMaxRadiusVariance(); }
    set { SetMaxRadiusVariance(value); }
  }
  public $typemap(cstype, float) MinRadius {
    get { return GetMinRadius(); }
    set { SetMinRadius(value); }
  }
  public $typemap(cstype, float) MinRadiusVariance {
    get { return GetMinRadiusVariance(); }
    set { SetMinRadiusVariance(value); }
  }
  public $typemap(cstype, float) RotatePerSecond {
    get { return GetRotatePerSecond(); }
    set { SetRotatePerSecond(value); }
  }
  public $typemap(cstype, float) RotatePerSecondVariance {
    get { return GetRotatePerSecondVariance(); }
    set { SetRotatePerSecondVariance(value); }
  }
  public $typemap(cstype, Urho3D::BlendMode) BlendMode {
    get { return GetBlendMode(); }
    set { SetBlendMode(value); }
  }
  public $typemap(cstype, float) RotationStart {
    get { return GetRotationStart(); }
    set { SetRotationStart(value); }
  }
  public $typemap(cstype, float) RotationStartVariance {
    get { return GetRotationStartVariance(); }
    set { SetRotationStartVariance(value); }
  }
  public $typemap(cstype, float) RotationEnd {
    get { return GetRotationEnd(); }
    set { SetRotationEnd(value); }
  }
  public $typemap(cstype, float) RotationEndVariance {
    get { return GetRotationEndVariance(); }
    set { SetRotationEndVariance(value); }
  }
%}
%csmethodmodifiers Urho3D::ParticleEffect2D::GetSprite "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetSprite "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetSourcePositionVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetSourcePositionVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetSpeed "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetSpeed "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetSpeedVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetSpeedVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetParticleLifeSpan "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetParticleLifeSpan "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetParticleLifespanVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetParticleLifespanVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetAngle "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetAngle "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetAngleVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetAngleVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetGravity "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetGravity "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetRadialAcceleration "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetRadialAcceleration "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetTangentialAcceleration "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetTangentialAcceleration "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetRadialAccelVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetRadialAccelVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetTangentialAccelVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetTangentialAccelVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetStartColor "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetStartColor "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetStartColorVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetStartColorVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetFinishColor "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetFinishColor "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetFinishColorVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetFinishColorVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetMaxParticles "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetMaxParticles "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetStartParticleSize "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetStartParticleSize "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetStartParticleSizeVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetStartParticleSizeVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetFinishParticleSize "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetFinishParticleSize "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetFinishParticleSizeVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetFinishParticleSizeVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetDuration "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetDuration "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetEmitterType "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetEmitterType "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetMaxRadius "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetMaxRadius "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetMaxRadiusVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetMaxRadiusVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetMinRadius "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetMinRadius "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetMinRadiusVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetMinRadiusVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetRotatePerSecond "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetRotatePerSecond "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetRotatePerSecondVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetRotatePerSecondVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetBlendMode "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetBlendMode "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetRotationStart "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetRotationStart "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetRotationStartVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetRotationStartVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetRotationEnd "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetRotationEnd "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::GetRotationEndVariance "private";
%csmethodmodifiers Urho3D::ParticleEffect2D::SetRotationEndVariance "private";
%typemap(cscode) Urho3D::ParticleEmitter2D %{
  public $typemap(cstype, Urho3D::ParticleEffect2D *) Effect {
    get { return GetEffect(); }
    set { SetEffect(value); }
  }
  public $typemap(cstype, Urho3D::Sprite2D *) Sprite {
    get { return GetSprite(); }
    set { SetSprite(value); }
  }
  public $typemap(cstype, Urho3D::BlendMode) BlendMode {
    get { return GetBlendMode(); }
    set { SetBlendMode(value); }
  }
  public $typemap(cstype, unsigned int) MaxParticles {
    get { return GetMaxParticles(); }
    set { SetMaxParticles(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) ParticleEffectAttr {
    get { return GetParticleEffectAttr(); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) SpriteAttr {
    get { return GetSpriteAttr(); }
  }
%}
%csmethodmodifiers Urho3D::ParticleEmitter2D::GetEffect "private";
%csmethodmodifiers Urho3D::ParticleEmitter2D::SetEffect "private";
%csmethodmodifiers Urho3D::ParticleEmitter2D::GetSprite "private";
%csmethodmodifiers Urho3D::ParticleEmitter2D::SetSprite "private";
%csmethodmodifiers Urho3D::ParticleEmitter2D::GetBlendMode "private";
%csmethodmodifiers Urho3D::ParticleEmitter2D::SetBlendMode "private";
%csmethodmodifiers Urho3D::ParticleEmitter2D::GetMaxParticles "private";
%csmethodmodifiers Urho3D::ParticleEmitter2D::SetMaxParticles "private";
%csmethodmodifiers Urho3D::ParticleEmitter2D::GetParticleEffectAttr "private";
%csmethodmodifiers Urho3D::ParticleEmitter2D::GetSpriteAttr "private";
%typemap(cscode) Urho3D::PhysicsWorld2D %{
  public $typemap(cstype, bool) DrawShape {
    get { return GetDrawShape(); }
    set { SetDrawShape(value); }
  }
  public $typemap(cstype, bool) DrawJoint {
    get { return GetDrawJoint(); }
    set { SetDrawJoint(value); }
  }
  public $typemap(cstype, bool) DrawAabb {
    get { return GetDrawAabb(); }
    set { SetDrawAabb(value); }
  }
  public $typemap(cstype, bool) DrawPair {
    get { return GetDrawPair(); }
    set { SetDrawPair(value); }
  }
  public $typemap(cstype, bool) DrawCenterOfMass {
    get { return GetDrawCenterOfMass(); }
    set { SetDrawCenterOfMass(value); }
  }
  public $typemap(cstype, bool) AllowSleeping {
    get { return GetAllowSleeping(); }
    set { SetAllowSleeping(value); }
  }
  public $typemap(cstype, bool) WarmStarting {
    get { return GetWarmStarting(); }
    set { SetWarmStarting(value); }
  }
  public $typemap(cstype, bool) ContinuousPhysics {
    get { return GetContinuousPhysics(); }
    set { SetContinuousPhysics(value); }
  }
  public $typemap(cstype, bool) SubStepping {
    get { return GetSubStepping(); }
    set { SetSubStepping(value); }
  }
  public $typemap(cstype, bool) AutoClearForces {
    get { return GetAutoClearForces(); }
    set { SetAutoClearForces(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Gravity {
    get { return GetGravity(); }
    set { SetGravity(value); }
  }
  public $typemap(cstype, int) VelocityIterations {
    get { return GetVelocityIterations(); }
    set { SetVelocityIterations(value); }
  }
  public $typemap(cstype, int) PositionIterations {
    get { return GetPositionIterations(); }
    set { SetPositionIterations(value); }
  }
  public $typemap(cstype, b2World *) World {
    get { return GetWorld(); }
  }
%}
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetDrawShape "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetDrawShape "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetDrawJoint "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetDrawJoint "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetDrawAabb "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetDrawAabb "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetDrawPair "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetDrawPair "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetDrawCenterOfMass "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetDrawCenterOfMass "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetAllowSleeping "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetAllowSleeping "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetWarmStarting "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetWarmStarting "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetContinuousPhysics "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetContinuousPhysics "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetSubStepping "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetSubStepping "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetAutoClearForces "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetAutoClearForces "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetGravity "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetGravity "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetVelocityIterations "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetVelocityIterations "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetPositionIterations "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::SetPositionIterations "private";
%csmethodmodifiers Urho3D::PhysicsWorld2D::GetWorld "private";
%typemap(cscode) Urho3D::RigidBody2D %{
  public $typemap(cstype, Urho3D::BodyType2D) BodyType {
    get { return GetBodyType(); }
    set { SetBodyType(value); }
  }
  public $typemap(cstype, float) Mass {
    get { return GetMass(); }
    set { SetMass(value); }
  }
  public $typemap(cstype, float) Inertia {
    get { return GetInertia(); }
    set { SetInertia(value); }
  }
  public $typemap(cstype, Urho3D::Vector2) MassCenter {
    get { return GetMassCenter(); }
  }
  public $typemap(cstype, bool) UseFixtureMass {
    get { return GetUseFixtureMass(); }
    set { SetUseFixtureMass(value); }
  }
  public $typemap(cstype, float) LinearDamping {
    get { return GetLinearDamping(); }
    set { SetLinearDamping(value); }
  }
  public $typemap(cstype, float) AngularDamping {
    get { return GetAngularDamping(); }
    set { SetAngularDamping(value); }
  }
  public $typemap(cstype, float) GravityScale {
    get { return GetGravityScale(); }
    set { SetGravityScale(value); }
  }
  public $typemap(cstype, Urho3D::Vector2) LinearVelocity {
    get { return GetLinearVelocity(); }
  }
  public $typemap(cstype, float) AngularVelocity {
    get { return GetAngularVelocity(); }
    set { SetAngularVelocity(value); }
  }
  public $typemap(cstype, b2Body *) Body {
    get { return GetBody(); }
  }
%}
%csmethodmodifiers Urho3D::RigidBody2D::GetBodyType "private";
%csmethodmodifiers Urho3D::RigidBody2D::SetBodyType "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetMass "private";
%csmethodmodifiers Urho3D::RigidBody2D::SetMass "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetInertia "private";
%csmethodmodifiers Urho3D::RigidBody2D::SetInertia "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetMassCenter "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetUseFixtureMass "private";
%csmethodmodifiers Urho3D::RigidBody2D::SetUseFixtureMass "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetLinearDamping "private";
%csmethodmodifiers Urho3D::RigidBody2D::SetLinearDamping "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetAngularDamping "private";
%csmethodmodifiers Urho3D::RigidBody2D::SetAngularDamping "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetGravityScale "private";
%csmethodmodifiers Urho3D::RigidBody2D::SetGravityScale "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetLinearVelocity "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetAngularVelocity "private";
%csmethodmodifiers Urho3D::RigidBody2D::SetAngularVelocity "private";
%csmethodmodifiers Urho3D::RigidBody2D::GetBody "private";
%typemap(cscode) Urho3D::Sprite2D %{
  public $typemap(cstype, Urho3D::Texture2D *) Texture {
    get { return GetTexture(); }
    set { SetTexture(value); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) Rectangle {
    get { return GetRectangle(); }
    set { SetRectangle(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) HotSpot {
    get { return GetHotSpot(); }
    set { SetHotSpot(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) Offset {
    get { return GetOffset(); }
    set { SetOffset(value); }
  }
  public $typemap(cstype, float) TextureEdgeOffset {
    get { return GetTextureEdgeOffset(); }
    set { SetTextureEdgeOffset(value); }
  }
  public $typemap(cstype, Urho3D::SpriteSheet2D *) SpriteSheet {
    get { return GetSpriteSheet(); }
    set { SetSpriteSheet(value); }
  }
%}
%csmethodmodifiers Urho3D::Sprite2D::GetTexture "private";
%csmethodmodifiers Urho3D::Sprite2D::SetTexture "private";
%csmethodmodifiers Urho3D::Sprite2D::GetRectangle "private";
%csmethodmodifiers Urho3D::Sprite2D::SetRectangle "private";
%csmethodmodifiers Urho3D::Sprite2D::GetHotSpot "private";
%csmethodmodifiers Urho3D::Sprite2D::SetHotSpot "private";
%csmethodmodifiers Urho3D::Sprite2D::GetOffset "private";
%csmethodmodifiers Urho3D::Sprite2D::SetOffset "private";
%csmethodmodifiers Urho3D::Sprite2D::GetTextureEdgeOffset "private";
%csmethodmodifiers Urho3D::Sprite2D::SetTextureEdgeOffset "private";
%csmethodmodifiers Urho3D::Sprite2D::GetSpriteSheet "private";
%csmethodmodifiers Urho3D::Sprite2D::SetSpriteSheet "private";
%typemap(cscode) Urho3D::SpriteSheet2D %{
  public $typemap(cstype, Urho3D::Texture2D *) Texture {
    get { return GetTexture(); }
    set { SetTexture(value); }
  }
  public $typemap(cstype, const eastl::unordered_map<eastl::string, Urho3D::SharedPtr<Urho3D::Sprite2D>> &) SpriteMapping {
    get { return GetSpriteMapping(); }
  }
%}
%csmethodmodifiers Urho3D::SpriteSheet2D::GetTexture "private";
%csmethodmodifiers Urho3D::SpriteSheet2D::SetTexture "private";
%csmethodmodifiers Urho3D::SpriteSheet2D::GetSpriteMapping "private";
%typemap(cscode) Urho3D::Spriter::SpriterInstance %{
  public $typemap(cstype, Urho3D::Spriter::Entity *) Entity {
    get { return GetEntity(); }
  }
  public $typemap(cstype, Urho3D::Spriter::Animation *) Animation {
    get { return GetAnimation(); }
  }
  public $typemap(cstype, const Urho3D::Spriter::SpatialInfo &) SpatialInfo {
    get { return GetSpatialInfo(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Spriter::SpatialTimelineKey *> &) TimelineKeys {
    get { return GetTimelineKeys(); }
  }
%}
%csmethodmodifiers Urho3D::Spriter::SpriterInstance::GetEntity "private";
%csmethodmodifiers Urho3D::Spriter::SpriterInstance::GetAnimation "private";
%csmethodmodifiers Urho3D::Spriter::SpriterInstance::GetSpatialInfo "private";
%csmethodmodifiers Urho3D::Spriter::SpriterInstance::GetTimelineKeys "private";
%typemap(cscode) Urho3D::StaticSprite2D %{
  public $typemap(cstype, Urho3D::Sprite2D *) Sprite {
    get { return GetSprite(); }
    set { SetSprite(value); }
  }
  public $typemap(cstype, const Urho3D::Rect &) DrawRect {
    get { return GetDrawRect(); }
    set { SetDrawRect(value); }
  }
  public $typemap(cstype, const Urho3D::Rect &) TextureRect {
    get { return GetTextureRect(); }
    set { SetTextureRect(value); }
  }
  public $typemap(cstype, Urho3D::BlendMode) BlendMode {
    get { return GetBlendMode(); }
    set { SetBlendMode(value); }
  }
  public $typemap(cstype, bool) FlipX {
    get { return GetFlipX(); }
    set { SetFlipX(value); }
  }
  public $typemap(cstype, bool) FlipY {
    get { return GetFlipY(); }
    set { SetFlipY(value); }
  }
  public $typemap(cstype, bool) SwapXY {
    get { return GetSwapXY(); }
    set { SetSwapXY(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) Color {
    get { return GetColor(); }
    set { SetColor(value); }
  }
  public $typemap(cstype, float) Alpha {
    get { return GetAlpha(); }
    set { SetAlpha(value); }
  }
  public $typemap(cstype, bool) UseHotSpot {
    get { return GetUseHotSpot(); }
    set { SetUseHotSpot(value); }
  }
  public $typemap(cstype, bool) UseDrawRect {
    get { return GetUseDrawRect(); }
    set { SetUseDrawRect(value); }
  }
  public $typemap(cstype, bool) UseTextureRect {
    get { return GetUseTextureRect(); }
    set { SetUseTextureRect(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) HotSpot {
    get { return GetHotSpot(); }
    set { SetHotSpot(value); }
  }
  public $typemap(cstype, Urho3D::Material *) CustomMaterial {
    get { return GetCustomMaterial(); }
    set { SetCustomMaterial(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) SpriteAttr {
    get { return GetSpriteAttr(); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) CustomMaterialAttr {
    get { return GetCustomMaterialAttr(); }
  }
%}
%csmethodmodifiers Urho3D::StaticSprite2D::GetSprite "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetSprite "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetDrawRect "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetDrawRect "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetTextureRect "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetTextureRect "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetBlendMode "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetBlendMode "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetFlipX "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetFlipX "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetFlipY "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetFlipY "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetSwapXY "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetSwapXY "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetColor "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetColor "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetAlpha "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetAlpha "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetUseHotSpot "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetUseHotSpot "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetUseDrawRect "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetUseDrawRect "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetUseTextureRect "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetUseTextureRect "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetHotSpot "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetHotSpot "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetCustomMaterial "private";
%csmethodmodifiers Urho3D::StaticSprite2D::SetCustomMaterial "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetSpriteAttr "private";
%csmethodmodifiers Urho3D::StaticSprite2D::GetCustomMaterialAttr "private";
%typemap(cscode) Urho3D::StretchableSprite2D %{
  public $typemap(cstype, const Urho3D::IntRect &) Border {
    get { return GetBorder(); }
    set { SetBorder(value); }
  }
%}
%csmethodmodifiers Urho3D::StretchableSprite2D::GetBorder "private";
%csmethodmodifiers Urho3D::StretchableSprite2D::SetBorder "private";
%typemap(cscode) Urho3D::TileMap2D %{
  /*public _typemap(cstype, Urho3D::TmxFile2D *) TmxFile {
    get { return GetTmxFile(); }
    set { SetTmxFile(value); }
  }*/
  public $typemap(cstype, const Urho3D::TileMapInfo2D &) Info {
    get { return GetInfo(); }
  }
  public $typemap(cstype, unsigned int) NumLayers {
    get { return GetNumLayers(); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) TmxFileAttr {
    get { return GetTmxFileAttr(); }
  }
%}
%csmethodmodifiers Urho3D::TileMap2D::GetTmxFile "private";
%csmethodmodifiers Urho3D::TileMap2D::SetTmxFile "private";
%csmethodmodifiers Urho3D::TileMap2D::GetInfo "private";
%csmethodmodifiers Urho3D::TileMap2D::GetNumLayers "private";
%csmethodmodifiers Urho3D::TileMap2D::GetTmxFileAttr "private";
%typemap(cscode) Urho3D::Tile2D %{
  public $typemap(cstype, unsigned int) Gid {
    get { return GetGid(); }
  }
  public $typemap(cstype, bool) FlipX {
    get { return GetFlipX(); }
  }
  public $typemap(cstype, bool) FlipY {
    get { return GetFlipY(); }
  }
  public $typemap(cstype, bool) SwapXY {
    get { return GetSwapXY(); }
  }
  public $typemap(cstype, Urho3D::Sprite2D *) Sprite {
    get { return GetSprite(); }
  }
%}
%csmethodmodifiers Urho3D::Tile2D::GetGid "private";
%csmethodmodifiers Urho3D::Tile2D::GetFlipX "private";
%csmethodmodifiers Urho3D::Tile2D::GetFlipY "private";
%csmethodmodifiers Urho3D::Tile2D::GetSwapXY "private";
%csmethodmodifiers Urho3D::Tile2D::GetSprite "private";
%typemap(cscode) Urho3D::TileMapObject2D %{
  public $typemap(cstype, Urho3D::TileMapObjectType2D) ObjectType {
    get { return GetObjectType(); }
  }
  public $typemap(cstype, const eastl::string &) Name {
    get { return GetName(); }
  }
  public $typemap(cstype, const eastl::string &) MapType {
    get { return GetMapType(); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Position {
    get { return GetPosition(); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Size {
    get { return GetSize(); }
  }
  public $typemap(cstype, unsigned int) NumPoints {
    get { return GetNumPoints(); }
  }
  public $typemap(cstype, unsigned int) TileGid {
    get { return GetTileGid(); }
  }
  public $typemap(cstype, bool) TileFlipX {
    get { return GetTileFlipX(); }
  }
  public $typemap(cstype, bool) TileFlipY {
    get { return GetTileFlipY(); }
  }
  public $typemap(cstype, bool) TileSwapXY {
    get { return GetTileSwapXY(); }
  }
  public $typemap(cstype, Urho3D::Sprite2D *) TileSprite {
    get { return GetTileSprite(); }
  }
%}
%csmethodmodifiers Urho3D::TileMapObject2D::GetObjectType "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetName "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetType "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetPosition "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetSize "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetNumPoints "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetTileGid "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetTileFlipX "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetTileFlipY "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetTileSwapXY "private";
%csmethodmodifiers Urho3D::TileMapObject2D::GetTileSprite "private";
%typemap(cscode) Urho3D::TileMapLayer2D %{
  public $typemap(cstype, Urho3D::TileMap2D *) TileMap {
    get { return GetTileMap(); }
  }
  /*public _typemap(cstype, const Urho3D::TmxLayer2D *) TmxLayer {
    get { return GetTmxLayer(); }
  }*/
  public $typemap(cstype, int) DrawOrder {
    get { return GetDrawOrder(); }
    set { SetDrawOrder(value); }
  }
  public $typemap(cstype, Urho3D::TileMapLayerType2D) LayerType {
    get { return GetLayerType(); }
  }
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
  }
  public $typemap(cstype, unsigned int) NumObjects {
    get { return GetNumObjects(); }
  }
  public $typemap(cstype, Urho3D::Node *) ImageNode {
    get { return GetImageNode(); }
  }
%}
%csmethodmodifiers Urho3D::TileMapLayer2D::GetTileMap "private";
%csmethodmodifiers Urho3D::TileMapLayer2D::GetTmxLayer "private";
%csmethodmodifiers Urho3D::TileMapLayer2D::GetDrawOrder "private";
%csmethodmodifiers Urho3D::TileMapLayer2D::SetDrawOrder "private";
%csmethodmodifiers Urho3D::TileMapLayer2D::GetLayerType "private";
%csmethodmodifiers Urho3D::TileMapLayer2D::GetWidth "private";
%csmethodmodifiers Urho3D::TileMapLayer2D::GetHeight "private";
%csmethodmodifiers Urho3D::TileMapLayer2D::GetNumObjects "private";
%csmethodmodifiers Urho3D::TileMapLayer2D::GetImageNode "private";
%typemap(cscode) Urho3D::TmxLayer2D %{
  public $typemap(cstype, Urho3D::TmxFile2D *) TmxFile {
    get { return GetTmxFile(); }
  }
  public $typemap(cstype, Urho3D::TileMapLayerType2D) Type {
    get { return GetLayerType(); }
  }
  public $typemap(cstype, const eastl::string &) Name {
    get { return GetName(); }
  }
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
  }
%}
%csmethodmodifiers Urho3D::TmxLayer2D::GetTmxFile "private";
%csmethodmodifiers Urho3D::TmxLayer2D::GetLayerType "private";
%csmethodmodifiers Urho3D::TmxLayer2D::GetName "private";
%csmethodmodifiers Urho3D::TmxLayer2D::GetWidth "private";
%csmethodmodifiers Urho3D::TmxLayer2D::GetHeight "private";
%typemap(cscode) Urho3D::TmxObjectGroup2D %{
  public $typemap(cstype, unsigned int) NumObjects {
    get { return GetNumObjects(); }
  }
%}
%csmethodmodifiers Urho3D::TmxObjectGroup2D::GetNumObjects "private";
%typemap(cscode) Urho3D::TmxImageLayer2D %{
  public $typemap(cstype, const Urho3D::Vector2 &) Position {
    get { return GetPosition(); }
  }
  public $typemap(cstype, const eastl::string &) Source {
    get { return GetSource(); }
  }
  public $typemap(cstype, Urho3D::Sprite2D *) Sprite {
    get { return GetSprite(); }
  }
%}
%csmethodmodifiers Urho3D::TmxImageLayer2D::GetPosition "private";
%csmethodmodifiers Urho3D::TmxImageLayer2D::GetSource "private";
%csmethodmodifiers Urho3D::TmxImageLayer2D::GetSprite "private";
%typemap(cscode) Urho3D::TmxFile2D %{
  public $typemap(cstype, const Urho3D::TileMapInfo2D &) Info {
    get { return GetInfo(); }
  }
  public $typemap(cstype, unsigned int) NumLayers {
    get { return GetNumLayers(); }
  }
  public $typemap(cstype, float) SpriteTextureEdgeOffset {
    get { return GetSpriteTextureEdgeOffset(); }
    set { SetSpriteTextureEdgeOffset(value); }
  }
%}
%csmethodmodifiers Urho3D::TmxFile2D::GetInfo "private";
%csmethodmodifiers Urho3D::TmxFile2D::GetNumLayers "private";
%csmethodmodifiers Urho3D::TmxFile2D::GetSpriteTextureEdgeOffset "private";
%csmethodmodifiers Urho3D::TmxFile2D::SetSpriteTextureEdgeOffset "private";
