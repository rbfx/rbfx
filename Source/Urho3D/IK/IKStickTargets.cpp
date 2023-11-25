// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKStickTargets.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

IKStickTargets::IKStickTargets(Context* context)
    : IKSolverComponent(context)
{
}

IKStickTargets::~IKStickTargets()
{
}

void IKStickTargets::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKStickTargets>(Category_IK);

    URHO3D_ATTRIBUTE_EX(
        "Target Names", StringVector, targetNames_, OnTreeDirty, Variant::emptyStringVector, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Is Active", bool, isActive_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Is Position Sticky", bool, isPositionSticky_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Is Rotation Sticky", bool, isRotationSticky_, true, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Threshold", float, positionThreshold_, DefaultPositionThreshold, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Threshold", float, rotationThreshold_, DefaultRotationThreshold, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Time Threshold", float, timeThreshold_, DefaultTimeThreshold, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Recover Time", float, recoverTime_, DefaultRecoverTime, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Min Target Distances", float, minTargetDistance_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Simultaneous Recoveries", unsigned, maxSimultaneousRecoveries_, 0, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Base World Velocity", Vector3, baseWorldVelocity_, Vector3::ZERO, AM_DEFAULT);
}

void IKStickTargets::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
}

bool IKStickTargets::InitializeNodes(IKNodeCache& nodeCache)
{
    ea::vector<TargetInfo> targets;
    for (const ea::string& targetName : targetNames_)
    {
        Node* targetNode = AddCheckedNode(nodeCache, targetName);
        if (!targetNode)
            return false;

        targets.emplace_back(TargetInfo{WeakPtr<Node>{targetNode}});
    }

    targets_ = ea::move(targets);
    return true;
}

void IKStickTargets::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
}

void IKStickTargets::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    CollectDesiredWorldTransforms();
    ApplyWorldMovement(timeStep);
    UpdateOverrideWeights(timeStep);
    UpdateStuckTimers(timeStep);
    ApplyDeactivation();
    ApplyActivation();
    UpdateRecovery();
    CommitWorldTransforms();
}

void IKStickTargets::CollectDesiredWorldTransforms()
{
    for (TargetInfo& info : targets_)
        info.desiredWorldTransform_ = Transform{info.node_->GetWorldPosition(), info.node_->GetWorldRotation()};
}

void IKStickTargets::ApplyWorldMovement(float timeStep)
{
    for (TargetInfo& info : targets_)
    {
        if (!info.IsEffectivelyInactive() && info.overrideWorldTransform_)
            info.overrideWorldTransform_->position_ += baseWorldVelocity_ * timeStep;
    }
}

void IKStickTargets::UpdateOverrideWeights(float timeStep)
{
    for (TargetInfo& info : targets_)
    {
        if (info.state_ == TargetState::Inactive || info.state_ == TargetState::Recovering)
            info.SubtractWeight(timeStep / recoverTime_);
    }
}

void IKStickTargets::UpdateStuckTimers(float timeStep)
{
    for (TargetInfo& info : targets_)
    {
        if (info.state_ == TargetState::Stuck)
            info.stuckTimer_ += timeStep;
    }
}

void IKStickTargets::ApplyDeactivation()
{
    for (TargetInfo& info : targets_)
    {
        if (isActive_)
            continue;

        if (info.state_ != TargetState::Inactive)
        {
            info.state_ = TargetState::Inactive;
            info.OverrideToCurrent();
        }
    }
}

void IKStickTargets::ApplyActivation()
{
    for (TargetInfo& info : targets_)
    {
        if (!isActive_)
            continue;

        if (info.state_ == TargetState::Inactive)
            info.Stick();
    }
}

void IKStickTargets::UpdateRecovery()
{
    // Stop recoveries first, count how many are still going
    unsigned numOngoingRecoveries = 0;
    for (TargetInfo& info : targets_)
    {
        if (info.state_ == TargetState::Recovering)
        {
            const bool isCompleted = info.overrideWeight_ == 0.0f;
            const bool shouldAbort = minTargetDistance_ > 0.0f
                && GetDistanceToNearestStuckTarget(info.GetCurrentTransform().position_) < minTargetDistance_;

            if (isCompleted || shouldAbort)
                info.Stick();
            else
                ++numOngoingRecoveries;
        }
    }

    // Start new recoveries
    const unsigned numTargets = targets_.size();
    const unsigned startIndex = recoveryStartIndex_;
    for (unsigned i = 0; i < numTargets; ++i)
    {
        // Out of budget
        if (maxSimultaneousRecoveries_ > 0 && numOngoingRecoveries >= maxSimultaneousRecoveries_)
            break;

        TargetInfo& info = targets_[(i + startIndex) % numTargets];

        const bool isPositionExpired = info.GetStuckPositionError() > positionThreshold_;
        const bool isRotationExpired = info.GetStuckRotationError() > rotationThreshold_;
        const bool isTimedOut = timeThreshold_ > 0.0f && info.GetStuckTime() > timeThreshold_;

        if (isPositionExpired || isRotationExpired || isTimedOut)
        {
            info.state_ = TargetState::Recovering;
            ++numOngoingRecoveries;

            // Next time we start from the next target
            recoveryStartIndex_ = i + 1;
        }
    }
}

void IKStickTargets::CommitWorldTransforms()
{
    for (TargetInfo& info : targets_)
    {
        if (info.overrideWeight_ > 0.0f && info.overrideWorldTransform_)
        {
            const Transform transform = info.GetCurrentTransform();
            if (isPositionSticky_)
                info.node_->SetWorldPosition(transform.position_);
            if (isRotationSticky_)
                info.node_->SetWorldRotation(transform.rotation_);
        }
    }
}

float IKStickTargets::GetDistanceToNearestStuckTarget(const Vector3& worldPosition) const
{
    float minDistance = M_INFINITY;
    for (const TargetInfo& info : targets_)
    {
        if (info.state_ == TargetState::Stuck && info.overrideWorldTransform_)
        {
            const float distance = (info.overrideWorldTransform_->position_ - worldPosition).Length();
            minDistance = ea::min(minDistance, distance);
        }
    }
    return minDistance;
}

} // namespace Urho3D
