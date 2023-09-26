// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKLimbSolver.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

IKLimbSolver::IKLimbSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKLimbSolver::~IKLimbSolver()
{
}

void IKLimbSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKLimbSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone 0 Name", ea::string, firstBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bone 1 Name", ea::string, secondBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bone 2 Name", ea::string, thirdBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bend Target Name", ea::string, bendTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Weight", float, positionWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Weight", float, rotationWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Weight", float, bendWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Min Angle", float, minAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Direction", Vector3, bendDirection_, Vector3::FORWARD, AM_DEFAULT);
}

void IKLimbSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    IKNode* firstBone = chain_.GetBeginNode();
    IKNode* secondBone = chain_.GetMiddleNode();
    IKNode* thirdBone = chain_.GetEndNode();

    if (firstBone && secondBone && thirdBone)
    {
        DrawIKNode(debug, *firstBone, false);
        DrawIKNode(debug, *secondBone, false);
        DrawIKNode(debug, *thirdBone, false);
        DrawIKSegment(debug, *firstBone, *secondBone);
        DrawIKSegment(debug, *secondBone, *thirdBone);

        const Vector3 currentBendDirection =
            chain_.GetCurrentChainRotation() * node_->GetWorldRotation() * bendDirection_;
        DrawDirection(debug, secondBone->position_, currentBendDirection);
    }
    if (target_)
        DrawIKTarget(debug, latestTargetPosition_, Quaternion::IDENTITY, false);
    if (bendTarget_)
        DrawIKTarget(debug, bendTarget_, false);
}

bool IKLimbSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    bendTarget_ = AddCheckedNode(nodeCache, bendTargetName_);

    IKNode* firstBone = AddSolverNode(nodeCache, firstBoneName_);
    if (!firstBone)
        return false;

    IKNode* secondBone = AddSolverNode(nodeCache, secondBoneName_);
    if (!secondBone)
        return false;

    IKNode* thirdBone = AddSolverNode(nodeCache, thirdBoneName_);
    if (!thirdBone)
        return false;

    SetParentAsFrameOfReference(*firstBone);
    chain_.Initialize(firstBone, secondBone, thirdBone);
    return true;
}

void IKLimbSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLengths();

    local_.bendDirection_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * bendDirection_;
    local_.targetDirection_ = inverseFrameOfReference.rotation_
        * (chain_.GetEndNode()->position_ - chain_.GetBeginNode()->position_).Normalized();
}

void IKLimbSolver::EnsureInitialized()
{
    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    bendWeight_ = Clamp(bendWeight_, 0.0f, 1.0f);
    minAngle_ = Clamp(minAngle_, 0.0f, 180.0f);
    maxAngle_ = Clamp(maxAngle_, minAngle_, 180.0f);
}

void IKLimbSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    // Store original rotation
    IKNode& firstBone = *chain_.GetBeginNode();
    IKNode& secondBone = *chain_.GetMiddleNode();
    IKNode& thirdBone = *chain_.GetEndNode();

    const Quaternion firstBoneRotation = firstBone.rotation_;
    const Quaternion secondBoneRotation = secondBone.rotation_;
    const Quaternion thirdBoneRotation = thirdBone.rotation_;

    // Solve rotations for full solver weight
    latestTargetPosition_ = GetTargetPosition();
    const auto [originalDirection, currentDirection] = CalculateBendDirections(frameOfReference, latestTargetPosition_);

    chain_.Solve(latestTargetPosition_, originalDirection, currentDirection, minAngle_, maxAngle_);

    // Interpolate rotation to apply solver weight
    firstBone.rotation_ = firstBoneRotation.Slerp(firstBone.rotation_, positionWeight_);
    secondBone.rotation_ = secondBoneRotation.Slerp(secondBone.rotation_, positionWeight_);
    thirdBone.rotation_ = thirdBoneRotation.Slerp(thirdBone.rotation_, positionWeight_);

    // Apply target rotation if needed
    if (rotationWeight_ > 0.0f)
    {
        const Quaternion targetRotation = target_->GetWorldRotation() * thirdBone.localOriginalRotation_;
        thirdBone.rotation_ = thirdBone.rotation_.Slerp(targetRotation, rotationWeight_);
    }
}

Vector3 IKLimbSolver::GetTargetPosition() const
{
    IKNode& firstBone = *chain_.GetBeginNode();

    const float minDistance = 0.001f;
    const float maxDistance = GetMaxDistance(chain_, maxAngle_);
    const Vector3& origin = firstBone.position_;
    const Vector3& target = target_->GetWorldPosition();
    return origin + (target - origin).ReNormalized(minDistance, maxDistance);
}

ea::pair<Vector3, Vector3> IKLimbSolver::CalculateBendDirections(
    const Transform& frameOfReference, const Vector3& targetPosition) const
{
    BendCalculationParams params;

    params.parentNodeRotation_ = node_->GetWorldRotation();
    params.startPosition_ = chain_.GetBeginNode()->position_;
    params.targetPosition_ = targetPosition;
    params.targetDirectionInLocalSpace_ = local_.targetDirection_;
    params.bendDirectionInNodeSpace_ = bendDirection_;
    params.bendDirectionInLocalSpace_ = local_.bendDirection_;
    params.bendTarget_ = bendTarget_;
    params.bendTargetWeight_ = bendWeight_;

    return CalculateBendDirectionsInternal(frameOfReference, params);
}

} // namespace Urho3D
