%csattribute(Urho3D::IKArmSolver, %arg(ea::string), ShoulderBoneName, GetShoulderBoneName, SetShoulderBoneName);
%csattribute(Urho3D::IKArmSolver, %arg(ea::string), ArmBoneName, GetArmBoneName, SetArmBoneName);
%csattribute(Urho3D::IKArmSolver, %arg(ea::string), ForearmBoneName, GetForearmBoneName, SetForearmBoneName);
%csattribute(Urho3D::IKArmSolver, %arg(ea::string), HandBoneName, GetHandBoneName, SetHandBoneName);
%csattribute(Urho3D::IKArmSolver, %arg(ea::string), TargetName, GetTargetName, SetTargetName);
%csattribute(Urho3D::IKArmSolver, %arg(ea::string), BendTargetName, GetBendTargetName, SetBendTargetName);
%csattribute(Urho3D::IKArmSolver, %arg(float), PositionWeight, GetPositionWeight, SetPositionWeight);
%csattribute(Urho3D::IKArmSolver, %arg(float), RotationWeight, GetRotationWeight, SetRotationWeight);
%csattribute(Urho3D::IKArmSolver, %arg(float), BendWeight, GetBendWeight, SetBendWeight);
%csattribute(Urho3D::IKArmSolver, %arg(float), MinAngle, GetMinAngle, SetMinAngle);
%csattribute(Urho3D::IKArmSolver, %arg(float), MaxAngle, GetMaxAngle, SetMaxAngle);
%csattribute(Urho3D::IKArmSolver, %arg(Urho3D::Vector2), ShoulderWeight, GetShoulderWeight, SetShoulderWeight);
%csattribute(Urho3D::IKArmSolver, %arg(Urho3D::Vector3), BendDirection, GetBendDirection, SetBendDirection);
%csattribute(Urho3D::IKArmSolver, %arg(Urho3D::Vector3), UpDirection, GetUpDirection, SetUpDirection);
%csattribute(Urho3D::IKChainSolver, %arg(Urho3D::StringVector), BoneNames, GetBoneNames, SetBoneNames);
%csattribute(Urho3D::IKChainSolver, %arg(ea::string), TargetName, GetTargetName, SetTargetName);
%csattribute(Urho3D::IKHeadSolver, %arg(ea::string), NeckBoneName, GetNeckBoneName, SetNeckBoneName);
%csattribute(Urho3D::IKHeadSolver, %arg(ea::string), HeadBoneName, GetHeadBoneName, SetHeadBoneName);
%csattribute(Urho3D::IKHeadSolver, %arg(ea::string), TargetName, GetTargetName, SetTargetName);
%csattribute(Urho3D::IKHeadSolver, %arg(ea::string), LookAtTargetName, GetLookAtTargetName, SetLookAtTargetName);
%csattribute(Urho3D::IKHeadSolver, %arg(float), PositionWeight, GetPositionWeight, SetPositionWeight);
%csattribute(Urho3D::IKHeadSolver, %arg(float), RotationWeight, GetRotationWeight, SetRotationWeight);
%csattribute(Urho3D::IKHeadSolver, %arg(float), DirectionWeight, GetDirectionWeight, SetDirectionWeight);
%csattribute(Urho3D::IKHeadSolver, %arg(float), LookAtWeight, GetLookAtWeight, SetLookAtWeight);
%csattribute(Urho3D::IKHeadSolver, %arg(Urho3D::Vector3), EyeDirection, GetEyeDirection, SetEyeDirection);
%csattribute(Urho3D::IKHeadSolver, %arg(Urho3D::Vector3), EyeOffset, GetEyeOffset, SetEyeOffset);
%csattribute(Urho3D::IKHeadSolver, %arg(float), NeckWeight, GetNeckWeight, SetNeckWeight);
%csattribute(Urho3D::IKIdentitySolver, %arg(ea::string), BoneName, GetBoneName, SetBoneName);
%csattribute(Urho3D::IKIdentitySolver, %arg(ea::string), TargetName, GetTargetName, SetTargetName);
%csattribute(Urho3D::IKIdentitySolver, %arg(Urho3D::Quaternion), RotationOffset, GetRotationOffset, SetRotationOffset);
%csattribute(Urho3D::IKLegSolver, %arg(ea::string), ThighBoneName, GetThighBoneName, SetThighBoneName);
%csattribute(Urho3D::IKLegSolver, %arg(ea::string), CalfBoneName, GetCalfBoneName, SetCalfBoneName);
%csattribute(Urho3D::IKLegSolver, %arg(ea::string), HeelBoneName, GetHeelBoneName, SetHeelBoneName);
%csattribute(Urho3D::IKLegSolver, %arg(ea::string), ToeBoneName, GetToeBoneName, SetToeBoneName);
%csattribute(Urho3D::IKLegSolver, %arg(ea::string), TargetName, GetTargetName, SetTargetName);
%csattribute(Urho3D::IKLegSolver, %arg(ea::string), BendTargetName, GetBendTargetName, SetBendTargetName);
%csattribute(Urho3D::IKLegSolver, %arg(ea::string), GroundTargetName, GetGroundTargetName, SetGroundTargetName);
%csattribute(Urho3D::IKLegSolver, %arg(float), PositionWeight, GetPositionWeight, SetPositionWeight);
%csattribute(Urho3D::IKLegSolver, %arg(float), RotationWeight, GetRotationWeight, SetRotationWeight);
%csattribute(Urho3D::IKLegSolver, %arg(float), BendWeight, GetBendWeight, SetBendWeight);
%csattribute(Urho3D::IKLegSolver, %arg(float), FootRotationWeight, GetFootRotationWeight, SetFootRotationWeight);
%csattribute(Urho3D::IKLegSolver, %arg(float), MinAngle, GetMinAngle, SetMinAngle);
%csattribute(Urho3D::IKLegSolver, %arg(float), MaxAngle, GetMaxAngle, SetMaxAngle);
%csattribute(Urho3D::IKLegSolver, %arg(Urho3D::Vector2), BaseTiptoe, GetBaseTiptoe, SetBaseTiptoe);
%csattribute(Urho3D::IKLegSolver, %arg(Urho3D::Vector4), GroundTiptoeTweaks, GetGroundTiptoeTweaks, SetGroundTiptoeTweaks);
%csattribute(Urho3D::IKLegSolver, %arg(Urho3D::Vector3), BendDirection, GetBendDirection, SetBendDirection);
%csattribute(Urho3D::IKLegSolver, %arg(float), HeelGroundOffset, GetHeelGroundOffset, SetHeelGroundOffset);
%csattribute(Urho3D::IKLimbSolver, %arg(ea::string), FirstBoneName, GetFirstBoneName, SetFirstBoneName);
%csattribute(Urho3D::IKLimbSolver, %arg(ea::string), SecondBoneName, GetSecondBoneName, SetSecondBoneName);
%csattribute(Urho3D::IKLimbSolver, %arg(ea::string), ThirdBoneName, GetThirdBoneName, SetThirdBoneName);
%csattribute(Urho3D::IKLimbSolver, %arg(ea::string), TargetName, GetTargetName, SetTargetName);
%csattribute(Urho3D::IKLimbSolver, %arg(ea::string), BendTargetName, GetBendTargetName, SetBendTargetName);
%csattribute(Urho3D::IKLimbSolver, %arg(float), PositionWeight, GetPositionWeight, SetPositionWeight);
%csattribute(Urho3D::IKLimbSolver, %arg(float), RotationWeight, GetRotationWeight, SetRotationWeight);
%csattribute(Urho3D::IKLimbSolver, %arg(float), BendWeight, GetBendWeight, SetBendWeight);
%csattribute(Urho3D::IKLimbSolver, %arg(float), MinAngle, GetMinAngle, SetMinAngle);
%csattribute(Urho3D::IKLimbSolver, %arg(float), MaxAngle, GetMaxAngle, SetMaxAngle);
%csattribute(Urho3D::IKLimbSolver, %arg(Urho3D::Vector3), BendDirection, GetBendDirection, SetBendDirection);
%csattribute(Urho3D::IKSpineSolver, %arg(Urho3D::StringVector), BoneNames, GetBoneNames, SetBoneNames);
%csattribute(Urho3D::IKSpineSolver, %arg(ea::string), TargetName, GetTargetName, SetTargetName);
%csattribute(Urho3D::IKSpineSolver, %arg(ea::string), TwistTargetName, GetTwistTargetName, SetTwistTargetName);
%csattribute(Urho3D::IKSpineSolver, %arg(float), PositionWeight, GetPositionWeight, SetPositionWeight);
%csattribute(Urho3D::IKSpineSolver, %arg(float), RotationWeight, GetRotationWeight, SetRotationWeight);
%csattribute(Urho3D::IKSpineSolver, %arg(float), TwistWeight, GetTwistWeight, SetTwistWeight);
%csattribute(Urho3D::IKSpineSolver, %arg(float), MaxAngle, GetMaxAngle, SetMaxAngle);
%csattribute(Urho3D::IKSpineSolver, %arg(float), BendTweak, GetBendTweak, SetBendTweak);
%csattribute(Urho3D::IKStickTargets, %arg(Urho3D::StringVector), TargetNames, GetTargetNames, SetTargetNames);
%csattribute(Urho3D::IKStickTargets, %arg(bool), IsActive, IsActive, SetActive);
%csattribute(Urho3D::IKStickTargets, %arg(bool), IsPositionSticky, IsPositionSticky, SetPositionSticky);
%csattribute(Urho3D::IKStickTargets, %arg(bool), IsRotationSticky, IsRotationSticky, SetRotationSticky);
%csattribute(Urho3D::IKStickTargets, %arg(float), PositionThreshold, GetPositionThreshold, SetPositionThreshold);
%csattribute(Urho3D::IKStickTargets, %arg(float), RotationThreshold, GetRotationThreshold, SetRotationThreshold);
%csattribute(Urho3D::IKStickTargets, %arg(float), TimeThreshold, GetTimeThreshold, SetTimeThreshold);
%csattribute(Urho3D::IKStickTargets, %arg(float), RecoverTime, GetRecoverTime, SetRecoverTime);
%csattribute(Urho3D::IKStickTargets, %arg(float), MinTargetDistance, GetMinTargetDistance, SetMinTargetDistance);
%csattribute(Urho3D::IKStickTargets, %arg(unsigned int), MaxSimultaneousRecoveries, GetMaxSimultaneousRecoveries, SetMaxSimultaneousRecoveries);
%csattribute(Urho3D::IKStickTargets, %arg(Urho3D::Vector3), BaseWorldVelocity, GetBaseWorldVelocity, SetBaseWorldVelocity);
%csattribute(Urho3D::IKStickTargets::TargetInfo, %arg(bool), IsEffectivelyInactive, IsEffectivelyInactive);
%csattribute(Urho3D::IKStickTargets::TargetInfo, %arg(Urho3D::Transform), CurrentTransform, GetCurrentTransform);
%csattribute(Urho3D::IKStickTargets::TargetInfo, %arg(float), StuckPositionError, GetStuckPositionError);
%csattribute(Urho3D::IKStickTargets::TargetInfo, %arg(float), StuckRotationError, GetStuckRotationError);
%csattribute(Urho3D::IKStickTargets::TargetInfo, %arg(float), StuckTime, GetStuckTime);
%csattribute(Urho3D::IKSolver, %arg(Urho3D::StringHash), PostUpdateEvent, GetPostUpdateEvent);
%csattribute(Urho3D::IKSolver, %arg(bool), IsSolveWhenPaused, IsSolveWhenPaused, SetSolveWhenPaused);
%csattribute(Urho3D::IKSolver, %arg(bool), IsContinuousRotation, IsContinuousRotation, SetContinuousRotation);
%csattribute(Urho3D::IKTargetExtractor, %arg(bool), IsExecutedOnOutput, IsExecutedOnOutput);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class IKPreSolveEvent {
        private StringHash _event = new StringHash("IKPreSolve");
        public StringHash Node = new StringHash("Node");
        public StringHash IKSolver = new StringHash("IKSolver");
        public IKPreSolveEvent() { }
        public static implicit operator StringHash(IKPreSolveEvent e) { return e._event; }
    }
    public static IKPreSolveEvent IKPreSolve = new IKPreSolveEvent();
    public class IKPostSolveEvent {
        private StringHash _event = new StringHash("IKPostSolve");
        public StringHash Node = new StringHash("Node");
        public StringHash IKSolver = new StringHash("IKSolver");
        public IKPostSolveEvent() { }
        public static implicit operator StringHash(IKPostSolveEvent e) { return e._event; }
    }
    public static IKPostSolveEvent IKPostSolve = new IKPostSolveEvent();
} %}
