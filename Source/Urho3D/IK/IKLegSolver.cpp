// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKLegSolver.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

IKLegSolver::IKLegSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKLegSolver::~IKLegSolver()
{
}

void IKLegSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKLegSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Thigh Bone Name", ea::string, thighBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Calf Bone Name", ea::string, calfBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Heel Bone Name", ea::string, heelBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Toe Bone Name", ea::string, toeBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Heel Target Name", ea::string, heelTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bend Target Name", ea::string, bendTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Ground Target Name", ea::string, groundTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Weight", float, positionWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Weight", float, rotationWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Heel Weight", float, heelWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Weight", float, bendWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Min Angle", float, minKneeAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxKneeAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Tiptoe Factor", float, tiptoeFactor_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Direction", Vector3, bendDirection_, Vector3::FORWARD, AM_DEFAULT);

    URHO3D_ACTION_STATIC_LABEL(
        "Update Properties", UpdateProperties, "Set properties below from current bone positions");
    URHO3D_ATTRIBUTE("Heel Ground Offset", float, heelGroundOffset_, -1.0f, AM_DEFAULT);
}

void IKLegSolver::UpdateProperties()
{
    UpdateHeelGroundOffset();
}

void IKLegSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    IKNode* thighBone = legChain_.GetBeginNode();
    IKNode* calfBone = legChain_.GetMiddleNode();
    IKNode* heelBone = legChain_.GetEndNode();
    IKNode* toeBone = footSegment_.endNode_;

    if (thighBone && calfBone && heelBone)
    {
        DrawIKNode(debug, *thighBone, false);
        DrawIKNode(debug, *calfBone, false);
        DrawIKNode(debug, *heelBone, false);
        DrawIKSegment(debug, *thighBone, *calfBone);
        DrawIKSegment(debug, *calfBone, *heelBone);

        const Vector3 currentBendDirection =
            legChain_.GetCurrentChainRotation() * node_->GetWorldRotation() * bendDirection_;
        DrawDirection(debug, calfBone->position_, currentBendDirection);
    }
    if (heelBone && toeBone)
    {
        DrawIKNode(debug, *toeBone, false);
        DrawIKSegment(debug, *heelBone, *toeBone);
    }
    if (target_)
    {
        DrawIKTarget(debug, latestTargetPosition_, Quaternion::IDENTITY, false);
    }
    if (heelTarget_)
    {
        DrawIKTarget(debug, heelTarget_, false);
    }
    if (bendTarget_)
    {
        DrawIKTarget(debug, bendTarget_, false);
    }

    {
        Node* groundNode = groundTarget_ ? groundTarget_ : node_;
        const BoundingBox groundBox{Vector3{-0.5f, -0.2f, -0.5f}, Vector3{0.5f, 0.0f, 0.5f}};
        const Matrix3x4 groundTransform = groundNode->GetWorldTransform();
        debug->AddBoundingBox(groundBox, groundTransform, Color::GREEN, false);
    }
}

bool IKLegSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    heelTarget_ = AddCheckedNode(nodeCache, heelTargetName_);
    bendTarget_ = AddCheckedNode(nodeCache, bendTargetName_);
    groundTarget_ = AddCheckedNode(nodeCache, groundTargetName_);

    IKNode* thighBone = AddSolverNode(nodeCache, thighBoneName_);
    if (!thighBone)
        return false;

    IKNode* calfBone = AddSolverNode(nodeCache, calfBoneName_);
    if (!calfBone)
        return false;

    IKNode* heelBone = AddSolverNode(nodeCache, heelBoneName_);
    if (!heelBone)
        return false;

    IKNode* toeBone = AddSolverNode(nodeCache, toeBoneName_);
    if (!toeBone)
        return false;

    SetParentAsFrameOfReference(*thighBone);
    legChain_.Initialize(thighBone, calfBone, heelBone);
    footSegment_ = {heelBone, toeBone};
    return true;
}

void IKLegSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    legChain_.UpdateLengths();
    footSegment_.UpdateLength();

    const IKNode& thighBone = *legChain_.GetBeginNode();
    const IKNode& calfBone = *legChain_.GetMiddleNode();
    const IKNode& heelBone = *legChain_.GetEndNode();
    const IKNode& toeBone = *footSegment_.endNode_;

    local_.toeToHeel_ = node_->GetWorldRotation().Inverse() * (heelBone.position_ - toeBone.position_);
    local_.defaultThighToToeDistance_ = (toeBone.position_ - thighBone.position_).Length();

    local_.bendDirection_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * bendDirection_;
    local_.targetDirection_ = inverseFrameOfReference.rotation_
        * (legChain_.GetEndNode()->position_ - legChain_.GetBeginNode()->position_).Normalized();
    local_.defaultFootRotation_ = calfBone.rotation_.Inverse() * heelBone.rotation_;
    local_.defaultToeOffset_ = heelBone.rotation_.Inverse() * (toeBone.position_ - heelBone.position_);
    local_.defaultToeRotation_ = heelBone.rotation_.Inverse() * toeBone.rotation_;
    local_.toeRotation_ = inverseFrameOfReference.rotation_ * toeBone.rotation_;
}

void IKLegSolver::EnsureInitialized()
{
    if (heelGroundOffset_ < 0.0f)
        UpdateHeelGroundOffset();

    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    heelWeight_ = Clamp(heelWeight_, 0.0f, 1.0f);
    bendWeight_ = Clamp(bendWeight_, 0.0f, 1.0f);
    minKneeAngle_ = Clamp(minKneeAngle_, 0.0f, 180.0f);
    maxKneeAngle_ = Clamp(maxKneeAngle_, 0.0f, 180.0f);
    tiptoeFactor_ = Clamp(tiptoeFactor_, 0.0f, 1.0f);
}

void IKLegSolver::UpdateHeelGroundOffset()
{
    Node* heelNode = node_->GetChild(heelBoneName_, true);
    if (heelNode)
    {
        const Vector3 heelOffset = heelNode->GetWorldPosition() - node_->GetWorldPosition();
        heelGroundOffset_ = ea::max(0.0f, heelOffset.y_);
    }
}

Vector3 IKLegSolver::GetTargetPosition() const
{
    IKNode& firstBone = *legChain_.GetBeginNode();

    const float minDistance = 0.001f;
    const float maxDistance = GetToeReachDistance();
    const Vector3& origin = firstBone.position_;
    const Vector3& target = target_->GetWorldPosition();
    return origin + (target - origin).ReNormalized(minDistance, maxDistance);
}

Plane IKLegSolver::GetGroundPlane() const
{
    Node* groundNode = groundTarget_ ? groundTarget_ : node_;
    return Plane{groundNode->GetWorldUp(), groundNode->GetWorldPosition()};
}

Vector2 IKLegSolver::ProjectOnGround(const Vector3& position) const
{
    Node* groundNode = groundTarget_ ? groundTarget_ : node_;
    const Vector3 right = groundNode->GetWorldRotation() * Vector3::RIGHT;
    const Vector3 forward = groundNode->GetWorldRotation() * Vector3::FORWARD;
    const Vector3 localPos = position - groundNode->GetWorldPosition();
    return {right.DotProduct(localPos), forward.DotProduct(localPos)};
}

float IKLegSolver::GetHeelReachDistance() const
{
    return GetMaxDistance(legChain_, maxKneeAngle_);
}

float IKLegSolver::GetToeReachDistance() const
{
    return GetHeelReachDistance() + footSegment_.length_;
}

Vector3 IKLegSolver::RecoverFromGroundPenetration(const Vector3& toeToHeel, const Vector3& toePosition) const
{
    const Plane groundPlane = GetGroundPlane();
    const Vector3& yAxis = groundPlane.normal_;
    const Vector3& xAxis = toeToHeel.Orthogonalize(yAxis);

    // Decompose the vector into vertical and horizontal components relative to ground normal
    //
    //      o-heel
    //     / } (x,y)
    //    o-toe
    //    | } y0
    // ___|_____
    const float x = xAxis.DotProduct(toeToHeel);
    const float y = yAxis.DotProduct(toeToHeel);
    const float y0 = groundPlane.Distance(toePosition);

    // Clamp heel y to the minimum distance from the ground
    const float len = footSegment_.length_;
    const float y2 = ea::min(ea::max(y, heelGroundOffset_ - y0), len);
    const float x2 = Sqrt(ea::max(len * len - y2 * y2, 0.0f)) * (x < 0.0f ? -1.0f : 1.0f);

    return xAxis * x2 + yAxis * y2;
}

Vector3 IKLegSolver::SnapToReachablePosition(const Vector3& toeToHeel, const Vector3& toePosition) const
{
    const auto& thighBone = *legChain_.GetBeginNode();
    const Sphere reachableSphere{thighBone.position_, GetHeelReachDistance()};
    if (reachableSphere.IsInside(toePosition + toeToHeel) != OUTSIDE)
        return toeToHeel;

    const Sphere availableSphere{toePosition, toeToHeel.Length()};
    const Circle availableHeelPositions = reachableSphere.Intersect(availableSphere);

    const Vector3 heelPosition = availableHeelPositions.GetPoint(toeToHeel);
    return heelPosition - toePosition;
}

ea::pair<Vector3, Vector3> IKLegSolver::CalculateBendDirections(
    const Transform& frameOfReference, const Vector3& toeTargetPosition) const
{
    BendCalculationParams params;

    params.parentNodeRotation_ = node_->GetWorldRotation();
    params.startPosition_ = legChain_.GetBeginNode()->position_;
    params.targetPosition_ = toeTargetPosition;
    params.targetDirectionInLocalSpace_ = local_.targetDirection_;
    params.bendDirectionInNodeSpace_ = bendDirection_;
    params.bendDirectionInLocalSpace_ = local_.bendDirection_;
    params.bendTarget_ = bendTarget_;
    params.bendTargetWeight_ = bendWeight_;

    return CalculateBendDirectionsInternal(frameOfReference, params);
}

Quaternion IKLegSolver::CalculateLegRotation(
    const Vector3& toeTargetPosition, const Vector3& originalDirection, const Vector3& currentDirection) const
{
    IKNode* thighBone = legChain_.GetBeginNode();
    IKNode* toeBone = footSegment_.endNode_;

    const Quaternion chainRotation = IKTrigonometricChain::CalculateRotation(thighBone->originalPosition_,
        toeBone->originalPosition_, originalDirection, thighBone->position_, toeTargetPosition, currentDirection);
    return chainRotation;
}

Vector3 IKLegSolver::CalculateToeToHeelBent(
    const Vector3& toeTargetPosition, const Vector3& approximateBendDirection) const
{
    IKNode* thighBone = legChain_.GetBeginNode();
    const auto [newPos1, newPos2] = IKTrigonometricChain::Solve(thighBone->position_, legChain_.GetFirstLength(),
        legChain_.GetSecondLength() + footSegment_.length_, toeTargetPosition, approximateBendDirection, minKneeAngle_,
        maxKneeAngle_);
    return (newPos1 - newPos2).Normalized() * footSegment_.length_;
}

Vector3 IKLegSolver::CalculateToeToHeel(const Transform& frameOfReference, float tiptoeFactor,
    const Vector3& toeTargetPosition, const Vector3& originalDirection, const Vector3& currentDirection,
    const Quaternion& footRotation) const
{
    const auto legRotation = CalculateLegRotation(toeTargetPosition, originalDirection, currentDirection);
    const Vector3 approximateBendDirection = legRotation * originalDirection;

    const Vector3 toeToHeelMin = footRotation * legRotation * node_->GetWorldRotation() * local_.toeToHeel_;
    const Vector3 toeToHeelMax = CalculateToeToHeelBent(toeTargetPosition, approximateBendDirection);
    const Vector3 toeToHeelAuto = Lerp(toeToHeelMin, toeToHeelMax, tiptoeFactor).Normalized() * footSegment_.length_;

    // If heel target is provided, use it
    Vector3 toeToHeelTarget = toeToHeelAuto;
    if (heelTarget_)
    {
        const Vector3 heelTargetPosition = heelTarget_->GetWorldPosition();
        toeToHeelTarget = (heelTargetPosition - toeTargetPosition).Normalized() * footSegment_.length_;
    }

    const Vector3 toeToHeel = Lerp(toeToHeelAuto, toeToHeelTarget, heelWeight_).Normalized() * footSegment_.length_;
    return SnapToReachablePosition(RecoverFromGroundPenetration(toeToHeel, toeTargetPosition), toeTargetPosition);
}

void IKLegSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    // Store original rotation
    IKNode& thighBone = *legChain_.GetBeginNode();
    IKNode& calfBone = *legChain_.GetMiddleNode();
    IKNode& heelBone = *legChain_.GetEndNode();
    IKNode& toeBone = *footSegment_.endNode_;

    const Quaternion thighBoneRotation = thighBone.rotation_;
    const Quaternion calfBoneRotation = calfBone.rotation_;
    const Quaternion heelBoneRotation = heelBone.rotation_;
    const Quaternion toeBoneRotation = toeBone.rotation_;

    // Solve rotations for full solver weight
    latestTargetPosition_ = GetTargetPosition();

    const auto [originalDirection, currentDirection] = CalculateBendDirections(frameOfReference, latestTargetPosition_);
    const Quaternion footRotation = Quaternion::IDENTITY;
    const Vector3 toeToHeel = CalculateToeToHeel(frameOfReference, tiptoeFactor_, latestTargetPosition_,
        originalDirection, currentDirection, footRotation);
    const Vector3 heelTargetPosition = latestTargetPosition_ + toeToHeel;

    legChain_.Solve(heelTargetPosition, originalDirection, currentDirection, minKneeAngle_, maxKneeAngle_);
    RotateFoot(toeToHeel, footRotation);

    // Interpolate rotation to apply solver weight
    thighBone.rotation_ = thighBoneRotation.Slerp(thighBone.rotation_, positionWeight_);
    calfBone.rotation_ = calfBoneRotation.Slerp(calfBone.rotation_, positionWeight_);
    heelBone.rotation_ = heelBoneRotation.Slerp(heelBone.rotation_, positionWeight_);
    toeBone.rotation_ = toeBoneRotation.Slerp(toeBone.rotation_, positionWeight_);

    // Apply target rotation if needed
    if (rotationWeight_ > 0.0f)
    {
        const Quaternion targetRotation = target_->GetWorldRotation() * toeBone.localOriginalRotation_;
        toeBone.rotation_ = toeBone.rotation_.Slerp(targetRotation, rotationWeight_);
    }
}

void IKLegSolver::RotateFoot(const Vector3& toeToHeel, const Quaternion& footRotation)
{
    IKNode& calfBone = *legChain_.GetMiddleNode();
    IKNode& heelBone = *legChain_.GetEndNode();
    IKNode& toeBone = *footSegment_.endNode_;

    // heelBone.position should already set by legChain_.Solve()
    heelBone.previousPosition_ = heelBone.position_;
    heelBone.previousRotation_ = footRotation * calfBone.rotation_ * local_.defaultFootRotation_;
    toeBone.previousPosition_ = heelBone.previousPosition_ + heelBone.previousRotation_ * local_.defaultToeOffset_;
    toeBone.previousRotation_ = heelBone.previousRotation_ * local_.defaultToeRotation_;
    toeBone.position_ = heelBone.position_ - toeToHeel;

    footSegment_.UpdateRotationInNodes(true, true);
}

} // namespace Urho3D
