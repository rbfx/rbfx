// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKIdentitySolver.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

IKIdentitySolver::IKIdentitySolver(Context* context)
    : IKSolverComponent(context)
{
}

IKIdentitySolver::~IKIdentitySolver()
{
}

void IKIdentitySolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKIdentitySolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone Name", ea::string, boneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ACTION_STATIC_LABEL(
        "Update Properties", UpdateProperties, "Set properties below from current bone positions");
    URHO3D_ATTRIBUTE("Rotation Offset", Quaternion, rotationOffset_, Quaternion::ZERO, AM_DEFAULT);
}

void IKIdentitySolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (boneNode_)
        DrawIKNode(debug, *boneNode_, true);
    if (target_)
        DrawIKTarget(debug, target_, true);
}

void IKIdentitySolver::UpdateProperties()
{
    UpdateRotationOffset();
}

void IKIdentitySolver::UpdateRotationOffset()
{
    Node* boneNode = node_->GetChild(boneName_, true);
    if (boneNode)
        rotationOffset_ = node_->GetWorldRotation().Inverse() * boneNode->GetWorldRotation();
}

void IKIdentitySolver::EnsureInitialized()
{
    if (rotationOffset_ == Quaternion::ZERO)
        UpdateRotationOffset();
}

bool IKIdentitySolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    boneNode_ = AddSolverNode(nodeCache, boneName_);
    if (!boneNode_)
        return false;

    return true;
}

void IKIdentitySolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
}

void IKIdentitySolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    boneNode_->position_ = target_->GetWorldPosition();
    boneNode_->rotation_ = target_->GetWorldRotation() * rotationOffset_;

    boneNode_->MarkPositionDirty();
    boneNode_->MarkRotationDirty();
}

} // namespace Urho3D
