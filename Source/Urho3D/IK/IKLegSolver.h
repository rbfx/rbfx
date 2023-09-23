// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IK/IKSolverComponent.h"

namespace Urho3D
{

class URHO3D_API IKLegSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKLegSolver, IKSolverComponent);

public:
    explicit IKLegSolver(Context* context);
    ~IKLegSolver() override;
    static void RegisterObject(Context* context);

    void UpdateProperties();

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    // clang-format off
    void SetThighBoneName(const ea::string& name) { thighBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetThighBoneName() const { return thighBoneName_; }
    void SetCalfBoneName(const ea::string& name) { calfBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetCalfBoneName() const { return calfBoneName_; }
    void SetHeelBoneName(const ea::string& name) { heelBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetHeelBoneName() const { return heelBoneName_; }
    void SetToeBoneName(const ea::string& name) { toeBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetToeBoneName() const { return toeBoneName_; }

    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    void SetBendTargetName(const ea::string& name) { bendTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetBendTargetName() const { return bendTargetName_; }
    void SetGroundTargetName(const ea::string& name) { groundTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetGroundTargetName() const { return groundTargetName_; }

    void SetPositionWeight(float weight) { positionWeight_ = weight; }
    float GetPositionWeight() const { return positionWeight_; }
    void SetRotationWeight(float weight) { rotationWeight_ = weight; }
    float GetRotationWeight() const { return rotationWeight_; }
    void SetBendWeight(float weight) { bendWeight_ = weight; }
    float GetBendWeight() const { return bendWeight_; }
    void SetFootRotationWeight(float weight) { footRotationWeight_ = weight; }
    float GetFootRotationWeight() const { return footRotationWeight_; }
    void SetMinAngle(float angle) { minKneeAngle_ = angle; }
    float GetMinAngle() const { return minKneeAngle_; }
    void SetMaxAngle(float angle) { maxKneeAngle_ = angle; }
    float GetMaxAngle() const { return maxKneeAngle_; }
    void SetBaseTiptoe(const Vector2& value) { baseTiptoe_ = value; }
    const Vector2& GetBaseTiptoe() const { return baseTiptoe_; }
    void SetGroundTiptoeTweaks(const Vector4& value) { groundTiptoeTweaks_ = value; }
    const Vector4& GetGroundTiptoeTweaks() const { return groundTiptoeTweaks_; }
    void SetBendDirection(const Vector3& direction) { bendDirection_ = direction; }
    const Vector3& GetBendDirection() const { return bendDirection_; }

    void SetHeelGroundOffset(float offset) { heelGroundOffset_ = offset; }
    float GetHeelGroundOffset() const { return heelGroundOffset_; }
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
    void UpdateHeelGroundOffset();

    void RotateFoot(const Vector3& toeToHeel, const Quaternion& footRotation);

    Vector3 GetTargetPosition() const;
    Plane GetGroundPlane() const;
    Vector2 ProjectOnGround(const Vector3& position) const;
    float GetHeelReachDistance() const;
    float GetToeReachDistance() const;
    Vector3 RecoverFromGroundPenetration(const Vector3& toeToHeel, const Vector3& toePosition) const;
    Vector3 SnapToReachablePosition(const Vector3& toeToHeel, const Vector3& toePosition) const;

    ea::pair<Vector3, Vector3> CalculateBendDirections(
        const Transform& frameOfReference, const Vector3& toeTargetPosition) const;
    Quaternion CalculateLegRotation(
        const Vector3& toeTargetPosition, const Vector3& originalDirection, const Vector3& currentDirection) const;
    Vector2 CalculateLegOffset(
        const Transform& frameOfReference, const Vector3& toeTargetPosition, const Vector3& bendDirection);
    float CalculateTiptoeFactor(const Vector3& toeTargetPosition) const;
    Quaternion CalculateFootRotation(const Transform& frameOfReference, const Vector3& toeTargetPosition) const;

    Vector3 CalculateToeToHeel(const Transform& frameOfReference, float tiptoeFactor, const Vector3& toeTargetPosition,
        const Vector3& originalDirection, const Vector3& currentDirection, const Quaternion& footRotation) const;
    Vector3 CalculateToeToHeelBent(const Vector3& toeTargetPosition, const Vector3& approximateBendDirection) const;

    /// Attributes.
    /// @{
    ea::string thighBoneName_;
    ea::string calfBoneName_;
    ea::string heelBoneName_;
    ea::string toeBoneName_;

    ea::string targetName_;
    ea::string bendTargetName_;
    ea::string groundTargetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float bendWeight_{1.0f};
    float footRotationWeight_{0.0f};
    float minKneeAngle_{0.0f};
    float maxKneeAngle_{180.0f};
    Vector2 baseTiptoe_{0.5f, 0.0f};
    Vector4 groundTiptoeTweaks_{0.2f, 0.2f, 0.2f, 0.2f};
    Vector3 bendDirection_{Vector3::FORWARD};

    float heelGroundOffset_{-1.0f};
    /// @}

    /// IK nodes and effectors.
    /// @{
    IKTrigonometricChain legChain_;
    IKNodeSegment footSegment_;
    WeakPtr<Node> target_;
    WeakPtr<Node> bendTarget_;
    WeakPtr<Node> groundTarget_;
    /// @}

    struct LocalCache
    {
        Vector3 toeToHeel_;
        float defaultThighToToeDistance_{};
        float tiptoeTweakOffset_{};

        Vector3 bendDirection_;
        Vector3 targetDirection_;
        Quaternion defaultFootRotation_;
        Vector3 defaultToeOffset_;
        Quaternion defaultToeRotation_;

        Quaternion toeRotation_;
    } local_;

    Vector3 latestTargetPosition_;
    float latestTiptoeFactor_{};
};

} // namespace Urho3D
