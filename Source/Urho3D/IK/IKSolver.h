// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../IK/IKSolverComponent.h"
#include "../Scene/LogicComponent.h"
#include "../Scene/SceneEvents.h"

namespace Urho3D
{

class URHO3D_API IKSolver : public LogicComponent
{
    URHO3D_OBJECT(IKSolver, LogicComponent);

public:
    explicit IKSolver(Context* context);
    ~IKSolver() override;
    static void RegisterObject(Context* context);

    /// Notify host component that the list of solvers is dirty and needs to be rebuilt.
    void MarkSolversDirty() { solversDirty_ = true; }
    /// Solve the IK forcibly.
    void Solve(float timeStep);

    /// Implement LogicComponent.
    /// @{
    void PostUpdate(float timeStep) override;
    StringHash GetPostUpdateEvent() const override { return E_SCENEDRAWABLEUPDATEFINISHED; }
    void UpdateWorldOrigin(const IntVector3& oldOrigin, const IntVector3& newOrigin, const IntVector3& delta) override;
    /// @}

    /// Attributes.
    /// @{
    void SetSolveWhenPaused(bool value) { solveWhenPaused_ = value; }
    bool IsSolveWhenPaused() const { return solveWhenPaused_; }
    void SetSolveFromOriginal(bool value) { solveFromOriginal_ = value; }
    bool IsSolveFromOriginal() const { return solveFromOriginal_; }
    void SetContinuousRotation(bool value) { settings_.continuousRotations_ = value; }
    bool IsContinuousRotation() const { return settings_.continuousRotations_; }
    /// @}

    /// Find bone data by Node.
    const IKNode* GetNodeData(Node* node) const;

private:
    void OnNodeSet(Node* previousNode, Node* currentNode) override;

    bool IsChainTreeExpired() const;
    void RebuildSolvers();
    void SetOriginalTransforms();
    void UpdateOriginalTransforms();
    void SendIKEvent(bool preSolve);

    bool solveWhenPaused_{};
    bool solveFromOriginal_{true};
    IKSettings settings_;

    bool solversDirty_{};

    ea::vector<WeakPtr<IKSolverComponent>> solvers_;

    IKNodeCache solverNodes_;
};

} // namespace Urho3D
