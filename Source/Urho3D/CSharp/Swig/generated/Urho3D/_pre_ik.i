%csconstvalue("0") Urho3D::IKSolver::ONE_BONE;
%csattribute(Urho3D::IKConstraint, %arg(float), Stiffness, GetStiffness, SetStiffness);
%csattribute(Urho3D::IKConstraint, %arg(float), Stretchiness, GetStretchiness, SetStretchiness);
%csattribute(Urho3D::IKConstraint, %arg(Urho3D::Vector2), LengthConstraints, GetLengthConstraints, SetLengthConstraints);
%csattribute(Urho3D::IKEffector, %arg(Urho3D::Node *), TargetNode, GetTargetNode, SetTargetNode);
%csattribute(Urho3D::IKEffector, %arg(ea::string), TargetName, GetTargetName, SetTargetName);
%csattribute(Urho3D::IKEffector, %arg(Urho3D::Vector3), TargetPosition, GetTargetPosition, SetTargetPosition);
%csattribute(Urho3D::IKEffector, %arg(Urho3D::Quaternion), TargetRotation, GetTargetRotation, SetTargetRotation);
%csattribute(Urho3D::IKEffector, %arg(Urho3D::Vector3), TargetRotationEuler, GetTargetRotationEuler, SetTargetRotationEuler);
%csattribute(Urho3D::IKEffector, %arg(unsigned int), ChainLength, GetChainLength, SetChainLength);
%csattribute(Urho3D::IKEffector, %arg(float), Weight, GetWeight, SetWeight);
%csattribute(Urho3D::IKEffector, %arg(float), RotationWeight, GetRotationWeight, SetRotationWeight);
%csattribute(Urho3D::IKEffector, %arg(float), RotationDecay, GetRotationDecay, SetRotationDecay);
%csattribute(Urho3D::IKEffector, %arg(bool), WeightNlerp, GetWEIGHT_NLERP, SetWEIGHT_NLERP);
%csattribute(Urho3D::IKEffector, %arg(bool), InheritParentRotation, GetINHERIT_PARENT_ROTATION, SetINHERIT_PARENT_ROTATION);
%csattribute(Urho3D::IKSolver, %arg(unsigned int), MaximumIterations, GetMaximumIterations, SetMaximumIterations);
%csattribute(Urho3D::IKSolver, %arg(float), Tolerance, GetTolerance, SetTolerance);
%csattribute(Urho3D::IKSolver, %arg(bool), JointRotations, GetJOINT_ROTATIONS, SetJOINT_ROTATIONS);
%csattribute(Urho3D::IKSolver, %arg(bool), TargetRotations, GetTARGET_ROTATIONS, SetTARGET_ROTATIONS);
%csattribute(Urho3D::IKSolver, %arg(bool), UpdateOriginalPose, GetUPDATE_ORIGINAL_POSE, SetUPDATE_ORIGINAL_POSE);
%csattribute(Urho3D::IKSolver, %arg(bool), UpdateActivePose, GetUPDATE_ACTIVE_POSE, SetUPDATE_ACTIVE_POSE);
%csattribute(Urho3D::IKSolver, %arg(bool), UseOriginalPose, GetUSE_ORIGINAL_POSE, SetUSE_ORIGINAL_POSE);
%csattribute(Urho3D::IKSolver, %arg(bool), Constraints, GetCONSTRAINTS, SetCONSTRAINTS);
%csattribute(Urho3D::IKSolver, %arg(bool), AutoSolve, GetAUTO_SOLVE, SetAUTO_SOLVE);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class IKEffectorTargetChangedEvent {
        private StringHash _event = new StringHash("IKEffectorTargetChanged");
        public StringHash EffectorNode = new StringHash("EffectorNode");
        public StringHash TargetNode = new StringHash("TargetNode");
        public IKEffectorTargetChangedEvent() { }
        public static implicit operator StringHash(IKEffectorTargetChangedEvent e) { return e._event; }
    }
    public static IKEffectorTargetChangedEvent IKEffectorTargetChanged = new IKEffectorTargetChangedEvent();
} %}
