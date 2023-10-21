// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IK/IKSolverComponent.h"

namespace Urho3D
{

class URHO3D_API IKChainSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKChainSolver, IKSolverComponent);

public:
    explicit IKChainSolver(Context* context);
    ~IKChainSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    // clang-format off
    void SetBoneNames(const StringVector& names) { boneNames_ = names; OnTreeDirty(); }
    const StringVector& GetBoneNames() const { return boneNames_; }
    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    // clang-format on
    /// @}

protected:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

private:
    StringVector boneNames_;
    ea::string targetName_;

    IKFabrikChain chain_;
    WeakPtr<Node> target_;
};

} // namespace Urho3D
