// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKRotateTo.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

IKRotateTo::IKRotateTo(Context* context)
    : IKSolverComponent(context)
{
}

IKRotateTo::~IKRotateTo()
{
}

void IKRotateTo::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKRotateTo>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone 0 Name", ea::string, firstBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bone 1 Name", ea::string, secondBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Weight", float, weight_, 1.0f, AM_DEFAULT);
}

void IKRotateTo::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (chain_.beginNode_ && chain_.endNode_)
    {
        DrawIKNode(debug, *chain_.beginNode_, false);
        DrawIKNode(debug, *chain_.endNode_, false);
        DrawIKSegment(debug, *chain_.beginNode_, *chain_.endNode_);
    }
    if (target_)
        DrawIKTarget(debug, latestTargetPosition_, Quaternion::IDENTITY, false);
}

bool IKRotateTo::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    IKNode* firstBone = AddSolverNode(nodeCache, firstBoneName_);
    if (!firstBone)
        return false;

    IKNode* secondBone = AddSolverNode(nodeCache, secondBoneName_);
    if (!secondBone)
        return false;

    SetParentAsFrameOfReference(*firstBone);
    chain_ = {firstBone, secondBone};
    return true;
}

void IKRotateTo::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLength();
}

void IKRotateTo::EnsureInitialized()
{
    weight_ = Clamp(weight_, 0.0f, 1.0f);
}

void IKRotateTo::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    // Store original rotation
    IKNode& firstBone = *chain_.beginNode_;
    IKNode& secondBone = *chain_.endNode_;

    const Quaternion firstBoneRotation = firstBone.rotation_;
    const Quaternion secondBoneRotation = secondBone.rotation_;

    // Solve rotations for full solver weight
    latestTargetPosition_ = target_->GetWorldPosition();

    const Vector3 pos0 = firstBone.position_;
    const Vector3 oldPos1 = secondBone.position_;
    const Vector3 oldDirection = (oldPos1 - pos0).Normalized();
    const Vector3 newDirection = (latestTargetPosition_ - pos0).Normalized();
    const Quaternion rotation{oldDirection, newDirection};
    firstBone.RotateAround(pos0, rotation);
    secondBone.RotateAround(pos0, rotation);

    // Interpolate rotation to apply solver weight
    firstBone.rotation_ = firstBoneRotation.Slerp(firstBone.rotation_, weight_);
    secondBone.rotation_ = secondBoneRotation.Slerp(secondBone.rotation_, weight_);
}

} // namespace Urho3D
