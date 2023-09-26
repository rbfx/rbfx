// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKSpineSolver.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

IKSpineSolver::IKSpineSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKSpineSolver::~IKSpineSolver()
{
}

void IKSpineSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKSpineSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone Names", StringVector, boneNames_, OnTreeDirty, Variant::emptyStringVector, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Twist Target Name", ea::string, twistTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Weight", float, positionWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Weight", float, rotationWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Twist Weight", float, twistWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxAngle_, 90.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Tweak", float, bendTweak_, 0.0f, AM_DEFAULT);

    URHO3D_ACTION_STATIC_LABEL(
        "Update Properties", UpdateProperties, "Set properties below from current bone positions");
    URHO3D_ATTRIBUTE("Twist Rotation Offset", Quaternion, twistRotationOffset_, Quaternion::ZERO, AM_DEFAULT);
}

void IKSpineSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    const auto& segments = chain_.GetSegments();
    for (const IKNodeSegment& segment : segments)
    {
        const bool isLastSegment = &segment == &segments.back();
        DrawIKNode(debug, *segment.beginNode_, isLastSegment);
        DrawIKSegment(debug, *segment.beginNode_, *segment.endNode_);
        if (isLastSegment)
            DrawIKNode(debug, *segment.endNode_, false);
    }

    if (twistTarget_)
        DrawIKTarget(debug, twistTarget_, true);
    if (target_)
        DrawIKTarget(debug, target_, false);
}

void IKSpineSolver::UpdateProperties()
{
    UpdateTwistRotationOffset();
}

bool IKSpineSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    if (boneNames_.size() < 2)
    {
        URHO3D_LOGERROR("Spine solver must have at least 2 bones");
        return false;
    }

    IKSpineChain chain;
    for (const ea::string& boneName : boneNames_)
    {
        IKNode* bone = AddSolverNode(nodeCache, boneName);
        if (!bone)
            return false;

        chain.AddNode(bone);
    }

    if (!twistTargetName_.empty())
    {
        twistTarget_ = AddCheckedNode(nodeCache, twistTargetName_);
        if (!twistTarget_)
            return false;
    }

    SetParentAsFrameOfReference(*chain.GetNodes().front());
    chain_ = ea::move(chain);
    return true;
}

void IKSpineSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    if (twistRotationOffset_ == Quaternion::ZERO)
        UpdateTwistRotationOffset();

    chain_.UpdateLengths();

    const auto& bones = chain_.GetNodes();
    local_.defaultTransforms_.resize(bones.size());
    for (size_t i = 0; i < bones.size(); ++i)
    {
        const IKNode& bone = *bones[i];
        local_.defaultTransforms_[i] = inverseFrameOfReference * Transform{bone.position_, bone.rotation_};
    }

    const Vector3 baseDirection = (bones[1]->position_ - bones[0]->position_).Normalized();
    local_.baseDirection_ = inverseFrameOfReference.rotation_ * baseDirection;
    local_.zeroTwistRotation_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * twistRotationOffset_;
}

void IKSpineSolver::SetOriginalTransforms(const Transform& frameOfReference)
{
    auto& nodes = chain_.GetNodes();
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        IKNode& bone = *nodes[i];
        const Transform& defaultTransform = local_.defaultTransforms_[i];
        bone.position_ = frameOfReference * defaultTransform.position_;
        bone.rotation_ = frameOfReference * defaultTransform.rotation_;
    }
}

void IKSpineSolver::EnsureInitialized()
{
    if (twistRotationOffset_ == Quaternion::ZERO)
        UpdateTwistRotationOffset();

    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    twistWeight_ = Clamp(twistWeight_, 0.0f, 1.0f);
    maxAngle_ = Clamp(maxAngle_, 0.0f, 180.0f);
}

void IKSpineSolver::UpdateTwistRotationOffset()
{
    if (boneNames_.size() >= 2)
    {
        const ea::string& twistBoneName = boneNames_[boneNames_.size() - 2];
        Node* boneNode = node_->GetChild(twistBoneName, true);
        if (boneNode)
            twistRotationOffset_ = node_->GetWorldRotation().Inverse() * boneNode->GetWorldRotation();
    }
}

float IKSpineSolver::WeightFunction(float x) const
{
    if (bendTweak_ == 0.0f)
        return 1.0f;
    else if (bendTweak_ > 0.0f)
    {
        if (bendTweak_ < 1.0f)
            return Lerp(1.0f - bendTweak_, 1.0f, x);
        else
            return Pow(x, bendTweak_);
    }
    else
    {
        if (bendTweak_ > -1.0f)
            return Lerp(1.0f - (-bendTweak_), 1.0f, 1.0f - x);
        else
            return Pow(1.0f - x, -bendTweak_);
    }
}

void IKSpineSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    auto& bones = chain_.GetNodes();
    if (bones.size() < 2)
        return;

    // Store original rotation
    originalBoneRotations_.resize(bones.size());
    for (size_t i = 0; i < bones.size(); ++i)
        originalBoneRotations_[i] = bones[i]->rotation_;

    // Solve rotations for full solver weight for position target
    SetOriginalTransforms(frameOfReference);
    const Vector3 baseDirection = frameOfReference.rotation_ * local_.baseDirection_;
    const auto weightFunction = [this](float x) { return WeightFunction(x); };
    chain_.Solve(target_->GetWorldPosition(), baseDirection, maxAngle_, settings, weightFunction);

    // Interpolate rotation to apply solver weight
    for (size_t i = 0; i < bones.size(); ++i)
        bones[i]->rotation_ = originalBoneRotations_[i].Slerp(bones[i]->rotation_, positionWeight_);

    // Solve rotations for partial solver weight for twist target
    const float twistAngle =
        twistTarget_ ? GetTwistAngle(frameOfReference, chain_.GetSegments().back(), twistTarget_) : 0.0f;
    chain_.Twist(twistAngle * twistWeight_, settings);

    // Apply target rotation if needed
    if (rotationWeight_ > 0.0f)
    {
        IKNode& lastBone = *bones.back();
        const Quaternion targetRotation = target_->GetWorldRotation() * lastBone.localOriginalRotation_;
        lastBone.rotation_ = lastBone.rotation_.Slerp(targetRotation, rotationWeight_);
    }
}

float IKSpineSolver::GetTwistAngle(
    const Transform& frameOfReference, const IKNodeSegment& segment, Node* targetNode) const
{
    const Quaternion zeroTwistBoneRotation = frameOfReference.rotation_ * local_.zeroTwistRotation_;
    const Quaternion targetBoneRotation = targetNode->GetWorldRotation() * segment.beginNode_->localOriginalRotation_;
    const Quaternion deltaRotation = targetBoneRotation * zeroTwistBoneRotation.Inverse();

    const Vector3 direction = (segment.endNode_->position_ - segment.beginNode_->position_).Normalized();
    const auto [_, twist] = deltaRotation.ToSwingTwist(direction);
    const float angle = twist.Angle();
    const float sign = twist.Axis().DotProduct(direction) > 0.0f ? 1.0f : -1.0f;
    return sign * (angle > 180.0f ? angle - 360.0f : angle);
}

} // namespace Urho3D
