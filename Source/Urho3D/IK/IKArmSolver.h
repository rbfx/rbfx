// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IK/IKSolverComponent.h"

namespace Urho3D
{

class URHO3D_API IKArmSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKArmSolver, IKSolverComponent);

public:
    explicit IKArmSolver(Context* context);
    ~IKArmSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    // clang-format off
    void SetShoulderBoneName(const ea::string& name) { shoulderBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetShoulderBoneName() const { return shoulderBoneName_; }
    void SetArmBoneName(const ea::string& name) { armBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetArmBoneName() const { return armBoneName_; }
    void SetForearmBoneName(const ea::string& name) { forearmBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetForearmBoneName() const { return forearmBoneName_; }
    void SetHandBoneName(const ea::string& name) { handBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetHandBoneName() const { return handBoneName_; }

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
    void SetMinAngle(float angle) { minElbowAngle_ = angle; }
    float GetMinAngle() const { return minElbowAngle_; }
    void SetMaxAngle(float angle) { maxElbowAngle_ = angle; }
    float GetMaxAngle() const { return maxElbowAngle_; }
    void SetShoulderWeight(const Vector2& weight) { shoulderWeight_ = weight; }
    const Vector2& GetShoulderWeight() const { return shoulderWeight_; }
    void SetBendDirection(const Vector3& direction) { bendDirection_ = direction; }
    const Vector3& GetBendDirection() const { return bendDirection_; }
    void SetUpDirection(const Vector3& direction) { upDirection_ = direction; }
    const Vector3& GetUpDirection() const { return upDirection_; }
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

    void RotateShoulder(const Quaternion& rotation);

    ea::pair<Vector3, Vector3> CalculateBendDirections(
        const Transform& frameOfReference, const Vector3& handTargetPosition) const;
    Quaternion CalculateMaxShoulderRotation(const Vector3& handTargetPosition) const;

    /// Attributes.
    /// @{
    ea::string shoulderBoneName_;
    ea::string armBoneName_;
    ea::string forearmBoneName_;
    ea::string handBoneName_;

    ea::string targetName_;
    ea::string bendTargetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float bendWeight_{1.0f};
    float minElbowAngle_{0.0f};
    float maxElbowAngle_{180.0f};
    Vector2 shoulderWeight_{};
    Vector3 bendDirection_{Vector3::FORWARD};
    Vector3 upDirection_{Vector3::UP};
    /// @}

    /// IK nodes and effectors.
    /// @{
    IKTrigonometricChain armChain_;
    IKNodeSegment shoulderSegment_;
    WeakPtr<Node> target_;
    WeakPtr<Node> bendTarget_;
    /// @}

    struct LocalCache
    {
        Vector3 bendDirection_;
        Vector3 up_;
        Vector3 targetDirection_;

        Quaternion shoulderRotation_;
        Vector3 armOffset_;
        Quaternion armRotation_;
    } local_;
};

} // namespace Urho3D
