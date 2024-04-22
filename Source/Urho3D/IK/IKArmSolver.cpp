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
    URHO3D_ATTRIBUTE("Bend Direction", Vector3, bendDirection_, Vector3::FORWARD, AM_DEFAULT);
}

void IKArmSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    IKNode* armBone = chain_.GetBeginNode();
    IKNode* forearmBone = chain_.GetMiddleNode();
    IKNode* handBone = chain_.GetEndNode();

    if (armBone && forearmBone && handBone)
    {
        DrawIKNode(debug, *armBone, false);
        DrawIKNode(debug, *forearmBone, false);
        DrawIKNode(debug, *handBone, false);
        DrawIKSegment(debug, *armBone, *forearmBone);
        DrawIKSegment(debug, *forearmBone, *handBone);

        const Vector3 currentBendDirection =
            chain_.GetCurrentChainRotation() * node_->GetWorldRotation() * bendDirection_;
        DrawDirection(debug, forearmBone->position_, currentBendDirection);
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

    IKNode* armBone = AddSolverNode(nodeCache, armBoneName_);
    if (!armBone)
        return false;

    IKNode* forearmBone = AddSolverNode(nodeCache, forearmBoneName_);
    if (!forearmBone)
        return false;

    IKNode* handBone = AddSolverNode(nodeCache, handBoneName_);
    if (!handBone)
        return false;

    SetParentAsFrameOfReference(*armBone);
    chain_.Initialize(armBone, forearmBone, handBone);
    return true;
}

void IKArmSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLengths();

    local_.bendDirection_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * bendDirection_;
    local_.targetDirection_ = inverseFrameOfReference.rotation_
        * (chain_.GetEndNode()->position_ - chain_.GetBeginNode()->position_).Normalized();

    local_.armOffset_ = inverseFrameOfReference * chain_.GetBeginNode()->position_;
    local_.armRotation_ = inverseFrameOfReference * chain_.GetBeginNode()->rotation_;
}

void IKArmSolver::EnsureInitialized()
{
    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    bendWeight_ = Clamp(bendWeight_, 0.0f, 1.0f);
    minElbowAngle_ = Clamp(minElbowAngle_, 0.0f, 180.0f);
    maxElbowAngle_ = Clamp(maxElbowAngle_, 0.0f, 180.0f);
}

void IKArmSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    // Store original rotation
    IKNode& armBone = *chain_.GetBeginNode();
    IKNode& forearmBone = *chain_.GetMiddleNode();
    IKNode& handBone = *chain_.GetEndNode();

    const Quaternion armBoneRotation = armBone.rotation_;
    const Quaternion forearmBoneRotation = forearmBone.rotation_;
    const Quaternion handBoneRotation = handBone.rotation_;

    // Solve rotations for full solver weight
    armBone.position_ = frameOfReference * local_.armOffset_;
    armBone.rotation_ = frameOfReference * local_.armRotation_;

    const Vector3 handTargetPosition = target_->GetWorldPosition();
    const auto [originalDirection, currentDirection] = CalculateBendDirections(frameOfReference, handTargetPosition);
    chain_.Solve(handTargetPosition, originalDirection, currentDirection, minElbowAngle_, maxElbowAngle_);

    // Interpolate rotation to apply solver weight
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

ea::pair<Vector3, Vector3> IKArmSolver::CalculateBendDirections(
    const Transform& frameOfReference, const Vector3& handTargetPosition) const
{
    BendCalculationParams params;

    params.parentNodeRotation_ = node_->GetWorldRotation();
    params.startPosition_ = chain_.GetBeginNode()->position_;
    params.targetPosition_ = handTargetPosition;
    params.targetDirectionInLocalSpace_ = local_.targetDirection_;
    params.bendDirectionInNodeSpace_ = bendDirection_;
    params.bendDirectionInLocalSpace_ = local_.bendDirection_;
    params.bendTarget_ = bendTarget_;
    params.bendTargetWeight_ = bendWeight_;

    return CalculateBendDirectionsInternal(frameOfReference, params);
}

} // namespace Urho3D
