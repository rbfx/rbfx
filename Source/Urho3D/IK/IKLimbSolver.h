// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IK/IKSolverComponent.h"

namespace Urho3D
{

class URHO3D_API IKLimbSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKLimbSolver, IKSolverComponent);

public:
    explicit IKLimbSolver(Context* context);
    ~IKLimbSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    // clang-format off
    void SetFirstBoneName(const ea::string& name) { firstBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetFirstBoneName() const { return firstBoneName_; }
    void SetSecondBoneName(const ea::string& name) { secondBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetSecondBoneName() const { return secondBoneName_; }
    void SetThirdBoneName(const ea::string& name) { thirdBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetThirdBoneName() const { return thirdBoneName_; }

    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    void SetBendTargetName(const ea::string& name) { bendTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetBendTargetName() const { return bendTargetName_; }

    void SetPositionWeight(float weight) { positionWeight_ = weight; }
    float GetPositionWeight() const { return positionWeight_; }
    void SetRotationWeight(float weight) { rotationWeight_ = weight; }
    float GetRotationWeight() const { return rotationWeight_; }
    void SetBendWeight(float weight) { bendWeight_ = weight; }
    float GetBendWeight() const { return bendWeight_; }
    void SetMinAngle(float angle) { minAngle_ = angle; }
    float GetMinAngle() const { return minAngle_; }
    void SetMaxAngle(float angle) { maxAngle_ = angle; }
    float GetMaxAngle() const { return maxAngle_; }
    void SetBendDirection(const Vector3& direction) { bendDirection_ = direction; }
    const Vector3& GetBendDirection() const { return bendDirection_; }
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
    Vector3 GetTargetPosition() const;
    ea::pair<Vector3, Vector3> CalculateBendDirections(
        const Transform& frameOfReference, const Vector3& targetPosition) const;

    ea::string firstBoneName_;
    ea::string secondBoneName_;
    ea::string thirdBoneName_;

    ea::string targetName_;
    ea::string bendTargetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float bendWeight_{1.0f};
    float minAngle_{0.0f};
    float maxAngle_{180.0f};
    Vector3 bendDirection_{Vector3::FORWARD};

    IKTrigonometricChain chain_;
    WeakPtr<Node> target_;
    WeakPtr<Node> bendTarget_;

    struct LocalCache
    {
        Vector3 bendDirection_;
        Vector3 targetDirection_;
    } local_;

    Vector3 latestTargetPosition_;
};

} // namespace Urho3D
