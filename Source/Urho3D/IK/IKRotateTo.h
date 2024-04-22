// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IK/IKSolverComponent.h"

namespace Urho3D
{

class URHO3D_API IKRotateTo : public IKSolverComponent
{
    URHO3D_OBJECT(IKRotateTo, IKSolverComponent);

public:
    explicit IKRotateTo(Context* context);
    ~IKRotateTo() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    // clang-format off
    void SetFirstBoneName(const ea::string& name) { firstBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetFirstBoneName() const { return firstBoneName_; }
    void SetSecondBoneName(const ea::string& name) { secondBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetSecondBoneName() const { return secondBoneName_; }

    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }

    void SetWeight(float weight) { weight_ = weight; }
    float GetWeight() const { return weight_; }
    // clang-format on
    /// @}

private:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

    void EnsureInitialized();

    ea::string firstBoneName_;
    ea::string secondBoneName_;

    ea::string targetName_;

    float weight_{1.0f};

    IKNodeSegment chain_;
    WeakPtr<Node> target_;

    Vector3 latestTargetPosition_;
};

} // namespace Urho3D
