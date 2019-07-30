%typemap(cscode) Urho3D::IKConstraint %{
  public $typemap(cstype, float) Stiffness {
    get { return GetStiffness(); }
    set { SetStiffness(value); }
  }
  public $typemap(cstype, float) Stretchiness {
    get { return GetStretchiness(); }
    set { SetStretchiness(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) LengthConstraints {
    get { return GetLengthConstraints(); }
    set { SetLengthConstraints(value); }
  }
%}
%csmethodmodifiers Urho3D::IKConstraint::GetStiffness "private";
%csmethodmodifiers Urho3D::IKConstraint::SetStiffness "private";
%csmethodmodifiers Urho3D::IKConstraint::GetStretchiness "private";
%csmethodmodifiers Urho3D::IKConstraint::SetStretchiness "private";
%csmethodmodifiers Urho3D::IKConstraint::GetLengthConstraints "private";
%csmethodmodifiers Urho3D::IKConstraint::SetLengthConstraints "private";
%typemap(cscode) Urho3D::IKEffector %{
  public $typemap(cstype, Urho3D::Node *) TargetNode {
    get { return GetTargetNode(); }
    set { SetTargetNode(value); }
  }
  public $typemap(cstype, const eastl::string &) TargetName {
    get { return GetTargetName(); }
    set { SetTargetName(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) TargetPosition {
    get { return GetTargetPosition(); }
    set { SetTargetPosition(value); }
  }
  public $typemap(cstype, const Urho3D::Quaternion &) TargetRotation {
    get { return GetTargetRotation(); }
    set { SetTargetRotation(value); }
  }
  public $typemap(cstype, Urho3D::Vector3) TargetRotationEuler {
    get { return GetTargetRotationEuler(); }
  }
  public $typemap(cstype, unsigned int) ChainLength {
    get { return GetChainLength(); }
    set { SetChainLength(value); }
  }
  public $typemap(cstype, float) Weight {
    get { return GetWeight(); }
    set { SetWeight(value); }
  }
  public $typemap(cstype, float) RotationWeight {
    get { return GetRotationWeight(); }
    set { SetRotationWeight(value); }
  }
  public $typemap(cstype, float) RotationDecay {
    get { return GetRotationDecay(); }
    set { SetRotationDecay(value); }
  }
  public $typemap(cstype, bool) WEIGHT_NLERP {
    get { return GetWEIGHT_NLERP(); }
    set { SetWEIGHT_NLERP(value); }
  }
  public $typemap(cstype, bool) INHERIT_PARENT_ROTATION {
    get { return GetINHERIT_PARENT_ROTATION(); }
    set { SetINHERIT_PARENT_ROTATION(value); }
  }
%}
%csmethodmodifiers Urho3D::IKEffector::GetTargetNode "private";
%csmethodmodifiers Urho3D::IKEffector::SetTargetNode "private";
%csmethodmodifiers Urho3D::IKEffector::GetTargetName "private";
%csmethodmodifiers Urho3D::IKEffector::SetTargetName "private";
%csmethodmodifiers Urho3D::IKEffector::GetTargetPosition "private";
%csmethodmodifiers Urho3D::IKEffector::SetTargetPosition "private";
%csmethodmodifiers Urho3D::IKEffector::GetTargetRotation "private";
%csmethodmodifiers Urho3D::IKEffector::SetTargetRotation "private";
%csmethodmodifiers Urho3D::IKEffector::GetTargetRotationEuler "private";
%csmethodmodifiers Urho3D::IKEffector::GetChainLength "private";
%csmethodmodifiers Urho3D::IKEffector::SetChainLength "private";
%csmethodmodifiers Urho3D::IKEffector::GetWeight "private";
%csmethodmodifiers Urho3D::IKEffector::SetWeight "private";
%csmethodmodifiers Urho3D::IKEffector::GetRotationWeight "private";
%csmethodmodifiers Urho3D::IKEffector::SetRotationWeight "private";
%csmethodmodifiers Urho3D::IKEffector::GetRotationDecay "private";
%csmethodmodifiers Urho3D::IKEffector::SetRotationDecay "private";
%csmethodmodifiers Urho3D::IKEffector::GetWEIGHT_NLERP "private";
%csmethodmodifiers Urho3D::IKEffector::SetWEIGHT_NLERP "private";
%csmethodmodifiers Urho3D::IKEffector::GetINHERIT_PARENT_ROTATION "private";
%csmethodmodifiers Urho3D::IKEffector::SetINHERIT_PARENT_ROTATION "private";
%typemap(cscode) Urho3D::IKSolver %{
  public $typemap(cstype, unsigned int) MaximumIterations {
    get { return GetMaximumIterations(); }
    set { SetMaximumIterations(value); }
  }
  public $typemap(cstype, float) Tolerance {
    get { return GetTolerance(); }
    set { SetTolerance(value); }
  }
  public $typemap(cstype, bool) JOINT_ROTATIONS {
    get { return GetJOINT_ROTATIONS(); }
    set { SetJOINT_ROTATIONS(value); }
  }
  public $typemap(cstype, bool) TARGET_ROTATIONS {
    get { return GetTARGET_ROTATIONS(); }
    set { SetTARGET_ROTATIONS(value); }
  }
  public $typemap(cstype, bool) UPDATE_ORIGINAL_POSE {
    get { return GetUPDATE_ORIGINAL_POSE(); }
    set { SetUPDATE_ORIGINAL_POSE(value); }
  }
  public $typemap(cstype, bool) UPDATE_ACTIVE_POSE {
    get { return GetUPDATE_ACTIVE_POSE(); }
    set { SetUPDATE_ACTIVE_POSE(value); }
  }
  public $typemap(cstype, bool) USE_ORIGINAL_POSE {
    get { return GetUSE_ORIGINAL_POSE(); }
    set { SetUSE_ORIGINAL_POSE(value); }
  }
  public $typemap(cstype, bool) CONSTRAINTS {
    get { return GetCONSTRAINTS(); }
    set { SetCONSTRAINTS(value); }
  }
  public $typemap(cstype, bool) AUTO_SOLVE {
    get { return GetAUTO_SOLVE(); }
    set { SetAUTO_SOLVE(value); }
  }
%}
%csmethodmodifiers Urho3D::IKSolver::GetMaximumIterations "private";
%csmethodmodifiers Urho3D::IKSolver::SetMaximumIterations "private";
%csmethodmodifiers Urho3D::IKSolver::GetTolerance "private";
%csmethodmodifiers Urho3D::IKSolver::SetTolerance "private";
%csmethodmodifiers Urho3D::IKSolver::GetJOINT_ROTATIONS "private";
%csmethodmodifiers Urho3D::IKSolver::SetJOINT_ROTATIONS "private";
%csmethodmodifiers Urho3D::IKSolver::GetTARGET_ROTATIONS "private";
%csmethodmodifiers Urho3D::IKSolver::SetTARGET_ROTATIONS "private";
%csmethodmodifiers Urho3D::IKSolver::GetUPDATE_ORIGINAL_POSE "private";
%csmethodmodifiers Urho3D::IKSolver::SetUPDATE_ORIGINAL_POSE "private";
%csmethodmodifiers Urho3D::IKSolver::GetUPDATE_ACTIVE_POSE "private";
%csmethodmodifiers Urho3D::IKSolver::SetUPDATE_ACTIVE_POSE "private";
%csmethodmodifiers Urho3D::IKSolver::GetUSE_ORIGINAL_POSE "private";
%csmethodmodifiers Urho3D::IKSolver::SetUSE_ORIGINAL_POSE "private";
%csmethodmodifiers Urho3D::IKSolver::GetCONSTRAINTS "private";
%csmethodmodifiers Urho3D::IKSolver::SetCONSTRAINTS "private";
%csmethodmodifiers Urho3D::IKSolver::GetAUTO_SOLVE "private";
%csmethodmodifiers Urho3D::IKSolver::SetAUTO_SOLVE "private";
