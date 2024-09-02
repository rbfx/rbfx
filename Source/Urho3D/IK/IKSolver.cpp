//
// Copyright (c) 2022-2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKSolver.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/IK/IKEvents.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

IKSolver::IKSolver(Context* context)
    : LogicComponent(context)
{
}

IKSolver::~IKSolver()
{
}

void IKSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKSolver>(Category_IK);

    URHO3D_ACTION_STATIC_LABEL("Set as origin", MarkSolversDirty,
        "Set current pose as original one. AnimatedModel skeleton is used if present.");

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Solve when Paused", bool, solveWhenPaused_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Continuous Rotation", bool, settings_.continuousRotations_, false, AM_DEFAULT);
}

void IKSolver::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (previousNode)
    {
        if (auto solver = previousNode->GetComponent<IKSolver>())
            solver->MarkSolversDirty();
    }
    if (currentNode)
    {
        if (auto solver = currentNode->GetComponent<IKSolver>())
            solver->MarkSolversDirty();
    }
}

void IKSolver::PostUpdate(float timeStep)
{
    if (!node_)
        return;

    auto scene = GetScene();

    // Cannot solve when paused if there's no AnimatedModel because it will disturb original pose.
    if (solveWhenPaused_ && !node_->HasComponent<AnimatedModel>())
        solveWhenPaused_ = false;

    if (scene->IsUpdateEnabled() || solveWhenPaused_)
        Solve(timeStep);
}

void IKSolver::Solve(float timeStep)
{
    if (IsChainTreeExpired())
        solversDirty_ = true;

    if (solversDirty_)
    {
        solversDirty_ = false;
        node_->FindComponents(solvers_, ComponentSearchFlag::SelfOrChildrenRecursive | ComponentSearchFlag::Derived);
        RebuildSolvers();
    }

    if (solvers_.empty() || solverNodes_.empty())
        return;

    SendIKEvent(true);
    UpdateOriginalTransforms();
    for (IKSolverComponent* solver : solvers_)
    {
        URHO3D_ASSERT(solver);
        solver->Solve(settings_, timeStep);
    }
    SendIKEvent(false);

    if (auto animatedModel = node_->GetComponent<AnimatedModel>())
        animatedModel->UpdateBoneBoundingBox();
}

void IKSolver::SendIKEvent(bool preSolve)
{
    using namespace IKPreSolve;
    node_->SendEvent(preSolve ? E_IKPRESOLVE : E_IKPOSTSOLVE, //
        ea::forward_as_tuple(P_NODE, node_), //
        ea::forward_as_tuple(P_IKSOLVER, this));
}

bool IKSolver::IsChainTreeExpired() const
{
    if (!solvers_.empty() && solverNodes_.empty())
        return true;

    return ea::any_of(
        solverNodes_.begin(), solverNodes_.end(), [](const IKNodeCache::value_type& entry) { return !entry.first; });
}

void IKSolver::RebuildSolvers()
{
    solverNodes_.clear();
    if (solvers_.empty())
        return;

    IKNodeCache solverNodes;
    for (IKSolverComponent* solver : solvers_)
    {
        if (!solver->Initialize(solverNodes))
            return;
    }

    solverNodes_ = ea::move(solverNodes);

    // Reset skeleton on initialization to get true initial pose.
    auto animationController = node_->GetComponent<AnimationController>();
    if (animationController && animationController->IsSkeletonReset())
    {
        if (auto animatedModel = GetComponent<AnimatedModel>())
            animatedModel->GetSkeleton().Reset();
    }

    SetOriginalTransforms();

    for (IKSolverComponent* solver : solvers_)
        solver->NotifyPositionsReady();
}

void IKSolver::SetOriginalTransforms()
{
    const Matrix3x4 inverseWorldTransform = node_->GetWorldTransform().Inverse();
    for (auto& [node, solverNode] : solverNodes_)
        solverNode.SetOriginalTransform(node->GetWorldPosition(), node->GetWorldRotation(), inverseWorldTransform);
}

void IKSolver::UpdateOriginalTransforms()
{
    const Matrix3x4 worldTransform = node_->GetWorldTransform();
    for (auto& [_, solverNode] : solverNodes_)
        solverNode.UpdateOriginalTransform(worldTransform);
}

const IKNode* IKSolver::GetNodeData(Node* node) const
{
    const auto iter = solverNodes_.find_as(node);
    return iter != solverNodes_.end() ? &iter->second : nullptr;
}

} // namespace Urho3D
