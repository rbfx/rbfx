// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IK/IKSolverComponent.h"

namespace Urho3D
{

class URHO3D_API IKHeadSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKHeadSolver, IKSolverComponent);

public:
    explicit IKHeadSolver(Context* context);
    ~IKHeadSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    // clang-format off
    void SetNeckBoneName(const ea::string& name) { neckBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetNeckBoneName() const { return neckBoneName_; }
    void SetHeadBoneName(const ea::string& name) { headBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetHeadBoneName() const { return headBoneName_; }

    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    void SetLookAtTargetName(const ea::string& name) { lookAtTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetLookAtTargetName() const { return lookAtTargetName_; }

    void SetPositionWeight(float weight) { positionWeight_ = weight; }
    float GetPositionWeight() const { return positionWeight_; }
    void SetRotationWeight(float weight) { rotationWeight_ = weight; }
    float GetRotationWeight() const { return rotationWeight_; }
    void SetDirectionWeight(float weight) { directionWeight_ = weight; }
    float GetDirectionWeight() const { return directionWeight_; }
    void SetLookAtWeight(float weight) { lookAtWeight_ = weight; }
    float GetLookAtWeight() const { return lookAtWeight_; }
    void SetEyeDirection(const Vector3& direction) { eyeDirection_ = direction; }
    const Vector3& GetEyeDirection() const { return eyeDirection_; }
    void SetEyeOffset(const Vector3& offset) { eyeOffset_ = offset; }
    const Vector3& GetEyeOffset() const { return eyeOffset_; }
    void SetNeckWeight(float weight) { neckWeight_ = weight; }
    float GetNeckWeight() const { return neckWeight_; }
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
    void EnsureInitialized();

    void SolvePosition();
    void SolveRotation();
    void SolveDirection();
    void SolveLookAt(const Transform& frameOfReference, const IKSettings& settings);

    Ray GetEyeRay() const;

    ea::string neckBoneName_;
    ea::string headBoneName_;
    ea::string targetName_;
    ea::string lookAtTargetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float directionWeight_{1.0f};
    float lookAtWeight_{0.0f};
    Vector3 eyeDirection_{Vector3::FORWARD};
    Vector3 eyeOffset_;
    float neckWeight_{0.5f};

    IKNodeSegment neckSegment_;
    IKEyeChain neckChain_;
    IKEyeChain headChain_;

    WeakPtr<Node> target_;
    WeakPtr<Node> lookAtTarget_;

    struct LocalCache
    {
        Transform defaultNeckTransform_;
        Transform defaultHeadTransform_;
    } local_;
};

} // namespace Urho3D
