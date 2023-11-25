// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKHeadSolver.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

IKHeadSolver::IKHeadSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKHeadSolver::~IKHeadSolver()
{
}

void IKHeadSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKHeadSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Neck Bone Name", ea::string, neckBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Head Bone Name", ea::string, headBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Look At Target Name", ea::string, lookAtTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Position Weight", float, positionWeight_, OnTreeDirty, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Direction Weight", float, directionWeight_, OnTreeDirty, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Rotation Weight", float, rotationWeight_, OnTreeDirty, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Look At Weight", float, lookAtWeight_, OnTreeDirty, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Eye Direction", Vector3, eyeDirection_, OnTreeDirty, Vector3::FORWARD, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Eye Offset", Vector3, eyeOffset_, OnTreeDirty, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Neck Weight", float, neckWeight_, OnTreeDirty, 0.5f, AM_DEFAULT);
}

void IKHeadSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    IKNode* neckBone = neckSegment_.beginNode_;
    IKNode* headBone = neckSegment_.endNode_;

    if (neckBone && headBone)
    {
        DrawIKNode(debug, *neckBone, false);
        DrawIKNode(debug, *headBone, false);
        DrawIKSegment(debug, *neckBone, *headBone);

        const Ray eyeRay = GetEyeRay();
        DrawDirection(debug, eyeRay.origin_, eyeRay.direction_, true, false);
    }
    if (target_)
        DrawIKTarget(debug, target_, true);
    if (lookAtTarget_)
        DrawIKTarget(debug, lookAtTarget_, false);
}

bool IKHeadSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    lookAtTarget_ = AddCheckedNode(nodeCache, lookAtTargetName_);
    if (!target_ && !lookAtTarget_)
    {
        URHO3D_LOGERROR("IKHeadSolver: Either head or look at target must be specified");
        return false;
    }

    IKNode* neckBone = AddSolverNode(nodeCache, neckBoneName_);
    if (!neckBone)
        return false;

    IKNode* headBone = AddSolverNode(nodeCache, headBoneName_);
    if (!headBone)
        return false;

    SetParentAsFrameOfReference(*neckBone);
    neckChain_.Initialize(neckBone);
    headChain_.Initialize(headBone);
    neckSegment_ = {neckBone, headBone};
    return true;
}

void IKHeadSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    neckSegment_.UpdateLength();

    const IKNode& neckBone = *neckSegment_.beginNode_;
    const IKNode& headBone = *neckSegment_.endNode_;

    local_.defaultNeckTransform_ = inverseFrameOfReference * Transform{neckBone.position_, neckBone.rotation_};
    local_.defaultHeadTransform_ = inverseFrameOfReference * Transform{headBone.position_, headBone.rotation_};

    const Vector3 eyeDirection = node_->GetWorldRotation() * eyeDirection_;
    const Vector3 eyeOffset = node_->GetWorldRotation() * eyeOffset_;
    neckChain_.SetWorldEyeTransform(eyeOffset, eyeDirection);
    headChain_.SetWorldEyeTransform(eyeOffset, eyeDirection);
}

void IKHeadSolver::EnsureInitialized()
{
    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    directionWeight_ = Clamp(directionWeight_, 0.0f, 1.0f);
    lookAtWeight_ = Clamp(lookAtWeight_, 0.0f, 1.0f);
    neckWeight_ = Clamp(neckWeight_, 0.0f, 1.0f);
}

void IKHeadSolver::SolvePosition()
{
    IKNode& neckBone = *neckSegment_.beginNode_;
    IKNode& headBone = *neckSegment_.endNode_;

    const Vector3 targetPosition = target_->GetWorldPosition();
    const Quaternion rotation{headBone.position_ - neckBone.position_, targetPosition - neckBone.position_};
    const Quaternion scaledRotation = Quaternion::IDENTITY.Slerp(rotation, positionWeight_);

    neckBone.RotateAround(neckBone.position_, scaledRotation);
    headBone.RotateAround(neckBone.position_, scaledRotation);

    neckBone.MarkRotationDirty();
    headBone.MarkRotationDirty();
}

void IKHeadSolver::SolveRotation()
{
    IKNode& headBone = *neckSegment_.endNode_;

    const Quaternion targetRotation = target_->GetWorldRotation() * headBone.localOriginalRotation_;

    headBone.rotation_ = headBone.rotation_.Slerp(targetRotation, rotationWeight_);

    headBone.MarkRotationDirty();
}

void IKHeadSolver::SolveDirection()
{
    IKNode& headBone = *neckSegment_.endNode_;

    const Vector3 direction = target_->GetWorldDirection();
    const Quaternion rotation = headChain_.SolveLookTo(direction);
    const Quaternion scaledRotation = Quaternion::IDENTITY.Slerp(rotation, directionWeight_);

    headBone.rotation_ = scaledRotation * headBone.rotation_;

    headBone.MarkRotationDirty();
}

void IKHeadSolver::SolveLookAt(const Transform& frameOfReference, const IKSettings& settings)
{
    // Store original rotation
    IKNode& neckBone = *neckSegment_.beginNode_;
    IKNode& headBone = *neckSegment_.endNode_;

    const Quaternion neckBoneRotation = neckBone.rotation_;
    const Quaternion headBoneRotation = headBone.rotation_;

    // Reset transforms before solving
    neckBone.rotation_ = frameOfReference * local_.defaultNeckTransform_.rotation_;
    headBone.position_ = frameOfReference * local_.defaultHeadTransform_.position_;
    headBone.rotation_ = frameOfReference * local_.defaultHeadTransform_.rotation_;
    neckBone.StorePreviousTransform();
    headBone.StorePreviousTransform();

    const Vector3 lookAtTarget = lookAtTarget_->GetWorldPosition();

    const Quaternion neckRotation = neckChain_.SolveLookAt(lookAtTarget, settings);
    const Quaternion neckRotationWeighted = Quaternion::IDENTITY.Slerp(neckRotation, neckWeight_);
    neckBone.rotation_ = neckRotationWeighted * neckBone.rotation_;
    headBone.RotateAround(neckBone.position_, neckRotationWeighted);

    const Quaternion headRotation = headChain_.SolveLookAt(lookAtTarget, settings);
    headBone.rotation_ = headRotation * headBone.rotation_;

    neckBone.MarkRotationDirty();
    headBone.MarkRotationDirty();

    // Interpolate rotation to apply solver weight
    neckBone.rotation_ = neckBoneRotation.Slerp(neckBone.rotation_, lookAtWeight_);
    headBone.rotation_ = headBoneRotation.Slerp(headBone.rotation_, lookAtWeight_);
}

void IKHeadSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    if (target_ && positionWeight_ > 0.0f)
        SolvePosition();

    if (target_ && rotationWeight_ > 0.0f)
        SolveRotation();

    if (target_ && directionWeight_ > 0.0f)
        SolveDirection();

    if (lookAtTarget_ && lookAtWeight_ > 0.0f)
        SolveLookAt(frameOfReference, settings);
}

Ray IKHeadSolver::GetEyeRay() const
{
    const IKNode& headBone = *neckSegment_.endNode_;
    const Vector3 origin = headBone.position_ + headBone.rotation_ * headChain_.GetLocalEyeOffset();
    const Vector3 direction = headBone.rotation_ * headChain_.GetLocalEyeDirection();
    return {origin, direction};
}

} // namespace Urho3D
