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

#pragma once

#include "../IK/IKSolverComponent.h"
#include "../Scene/LogicComponent.h"
#include "../Scene/SceneEvents.h"

namespace Urho3D
{

class IKSolver : public LogicComponent
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

    void PostUpdate(float timeStep) override;
    StringHash GetPostUpdateEvent() const override { return E_SCENEDRAWABLEUPDATEFINISHED; }

    /// Attributes.
    /// @{
    void SetSolveWhenPaused(bool value) { solveWhenPaused_ = value; }
    bool IsSolveWhenPaused() const { return solveWhenPaused_; }
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
    IKSettings settings_;

    bool solversDirty_{};

    ea::vector<WeakPtr<IKSolverComponent>> solvers_;

    IKNodeCache solverNodes_;
};

} // namespace Urho3D
