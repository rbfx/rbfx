// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKChainSolver.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

IKChainSolver::IKChainSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKChainSolver::~IKChainSolver()
{
}

void IKChainSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKChainSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone Names", StringVector, boneNames_, OnTreeDirty, Variant::emptyStringVector, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
}

void IKChainSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
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

    if (target_)
        DrawIKTarget(debug, target_, false);
}

bool IKChainSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    if (boneNames_.size() < 2)
        return false;

    IKFabrikChain chain;
    for (const ea::string& boneName : boneNames_)
    {
        IKNode* bone = AddSolverNode(nodeCache, boneName);
        if (!bone)
            return false;

        chain.AddNode(bone);
    }

    chain_ = ea::move(chain);
    return true;
}

void IKChainSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLengths();
}

void IKChainSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    chain_.Solve(target_->GetWorldPosition(), settings);
}

} // namespace Urho3D
