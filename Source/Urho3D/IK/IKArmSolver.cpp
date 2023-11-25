// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKArmSolver.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

IKArmSolver::IKArmSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKArmSolver::~IKArmSolver()
{
}

void IKArmSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKArmSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Shoulder Bone Name", ea::string, shoulderBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Arm Bone Name", ea::string, armBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Forearm Bone Name", ea::string, forearmBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Hand Bone Name", ea::string, handBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bend Target Name", ea::string, bendTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Weight", float, positionWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Weight", float, rotationWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Weight", float, bendWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Min Angle", float, minElbowAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxElbowAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Shoulder Weight", Vector2, shoulderWeight_, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Direction", Vector3, bendDirection_, Vector3::FORWARD, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Up Direction", Vector3, upDirection_, Vector3::UP, AM_DEFAULT);
}

void IKArmSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    IKNode* shoulderBone = shoulderSegment_.beginNode_;
    IKNode* armBone = armChain_.GetBeginNode();
    IKNode* forearmBone = armChain_.GetMiddleNode();
    IKNode* handBone = armChain_.GetEndNode();

    if (armBone && forearmBone && handBone)
    {
        DrawIKNode(debug, *armBone, false);
        DrawIKNode(debug, *forearmBone, false);
        DrawIKNode(debug, *handBone, false);
        DrawIKSegment(debug, *armBone, *forearmBone);
        DrawIKSegment(debug, *forearmBone, *handBone);

        const Vector3 currentBendDirection =
            armChain_.GetCurrentChainRotation() * node_->GetWorldRotation() * bendDirection_;
        DrawDirection(debug, forearmBone->position_, currentBendDirection);
    }
    if (shoulderBone && armBone)
    {
        DrawIKNode(debug, *shoulderBone, false);
        DrawIKSegment(debug, *shoulderBone, *armBone);
    }
    if (target_)
        DrawIKTarget(debug, target_, false);
}

bool IKArmSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    bendTarget_ = AddCheckedNode(nodeCache, bendTargetName_);

    IKNode* shoulderBone = AddSolverNode(nodeCache, shoulderBoneName_);
    if (!shoulderBone)
        return false;

    IKNode* armBone = AddSolverNode(nodeCache, armBoneName_);
    if (!armBone)
        return false;

    IKNode* forearmBone = AddSolverNode(nodeCache, forearmBoneName_);
    if (!forearmBone)
        return false;

    IKNode* handBone = AddSolverNode(nodeCache, handBoneName_);
    if (!handBone)
        return false;

    SetParentAsFrameOfReference(*shoulderBone);
    armChain_.Initialize(armBone, forearmBone, handBone);
    shoulderSegment_ = {shoulderBone, armBone};
    return true;
}

void IKArmSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    armChain_.UpdateLengths();
    shoulderSegment_.UpdateLength();

    local_.bendDirection_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * bendDirection_;
    local_.up_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * upDirection_;
    local_.targetDirection_ = inverseFrameOfReference.rotation_
        * (armChain_.GetEndNode()->position_ - armChain_.GetBeginNode()->position_).Normalized();

    local_.shoulderRotation_ = inverseFrameOfReference * shoulderSegment_.beginNode_->rotation_;
    local_.armOffset_ = inverseFrameOfReference * shoulderSegment_.endNode_->position_;
    local_.armRotation_ = inverseFrameOfReference * shoulderSegment_.endNode_->rotation_;
}

void IKArmSolver::EnsureInitialized()
{
    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    bendWeight_ = Clamp(bendWeight_, 0.0f, 1.0f);
    minElbowAngle_ = Clamp(minElbowAngle_, 0.0f, 180.0f);
    maxElbowAngle_ = Clamp(maxElbowAngle_, 0.0f, 180.0f);
    shoulderWeight_ = VectorClamp(shoulderWeight_, Vector2::ZERO, Vector2::ONE);
}

void IKArmSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    // Store original rotation
    IKNode& shoulderBone = *shoulderSegment_.beginNode_;
    IKNode& armBone = *armChain_.GetBeginNode();
    IKNode& forearmBone = *armChain_.GetMiddleNode();
    IKNode& handBone = *armChain_.GetEndNode();

    const Quaternion shoulderBoneRotation = shoulderBone.rotation_;
    const Quaternion armBoneRotation = armBone.rotation_;
    const Quaternion forearmBoneRotation = forearmBone.rotation_;
    const Quaternion handBoneRotation = handBone.rotation_;

    // Solve rotations for full solver weight
    shoulderSegment_.beginNode_->rotation_ = frameOfReference * local_.shoulderRotation_;
    shoulderSegment_.endNode_->position_ = frameOfReference * local_.armOffset_;
    shoulderSegment_.endNode_->rotation_ = frameOfReference * local_.armRotation_;

    const Vector3 handTargetPosition = target_->GetWorldPosition();
    const auto [originalDirection, currentDirection] = CalculateBendDirections(frameOfReference, handTargetPosition);

    const Quaternion maxShoulderRotation = CalculateMaxShoulderRotation(handTargetPosition);
    const auto [swing, twist] = maxShoulderRotation.ToSwingTwist(frameOfReference.rotation_ * local_.up_);
    const Quaternion shoulderRotation =
        Quaternion::IDENTITY.Slerp(swing, shoulderWeight_.y_) * Quaternion::IDENTITY.Slerp(twist, shoulderWeight_.x_);
    RotateShoulder(shoulderRotation);

    armChain_.Solve(handTargetPosition, originalDirection, currentDirection, minElbowAngle_, maxElbowAngle_);

    // Interpolate rotation to apply solver weight
    shoulderBone.rotation_ = shoulderBoneRotation.Slerp(shoulderBone.rotation_, positionWeight_);
    armBone.rotation_ = armBoneRotation.Slerp(armBone.rotation_, positionWeight_);
    forearmBone.rotation_ = forearmBoneRotation.Slerp(forearmBone.rotation_, positionWeight_);
    handBone.rotation_ = handBoneRotation.Slerp(handBone.rotation_, positionWeight_);

    // Apply target rotation if needed
    if (rotationWeight_ > 0.0f)
    {
        const Quaternion targetRotation = target_->GetWorldRotation() * handBone.localOriginalRotation_;
        handBone.rotation_ = handBone.rotation_.Slerp(targetRotation, rotationWeight_);
    }
}

void IKArmSolver::RotateShoulder(const Quaternion& rotation)
{
    const Vector3 shoulderPosition = shoulderSegment_.beginNode_->position_;
    shoulderSegment_.beginNode_->RotateAround(shoulderPosition, rotation);
    shoulderSegment_.endNode_->RotateAround(shoulderPosition, rotation);
}

ea::pair<Vector3, Vector3> IKArmSolver::CalculateBendDirections(
    const Transform& frameOfReference, const Vector3& handTargetPosition) const
{
    BendCalculationParams params;

    params.parentNodeRotation_ = node_->GetWorldRotation();
    params.startPosition_ = shoulderSegment_.beginNode_->position_;
    params.targetPosition_ = handTargetPosition;
    params.targetDirectionInLocalSpace_ = local_.targetDirection_;
    params.bendDirectionInNodeSpace_ = bendDirection_;
    params.bendDirectionInLocalSpace_ = local_.bendDirection_;
    params.bendTarget_ = bendTarget_;
    params.bendTargetWeight_ = bendWeight_;

    return CalculateBendDirectionsInternal(frameOfReference, params);
}

Quaternion IKArmSolver::CalculateMaxShoulderRotation(const Vector3& handTargetPosition) const
{
    const Vector3 shoulderPosition = shoulderSegment_.beginNode_->position_;
    const Vector3 shoulderToArmMax =
        (handTargetPosition - shoulderPosition).ReNormalized(shoulderSegment_.length_, shoulderSegment_.length_);
    const Vector3 armTargetPosition = shoulderPosition + shoulderToArmMax;

    const Vector3 originalShoulderToArm = shoulderSegment_.endNode_->position_ - shoulderSegment_.beginNode_->position_;
    const Vector3 maxShoulderToArm = armTargetPosition - shoulderPosition;

    return Quaternion{originalShoulderToArm, maxShoulderToArm};
}

} // namespace Urho3D
