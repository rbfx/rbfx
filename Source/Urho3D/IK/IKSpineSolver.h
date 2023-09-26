// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IK/IKSolverComponent.h"

namespace Urho3D
{

class URHO3D_API IKSpineSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKSpineSolver, IKSolverComponent);

public:
    explicit IKSpineSolver(Context* context);
    ~IKSpineSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    void UpdateProperties();

    /// Attributes.
    /// @{
    // clang-format off
    void SetBoneNames(const StringVector& names) { boneNames_ = names; OnTreeDirty(); }
    const StringVector& GetBoneNames() const { return boneNames_; }
    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    void SetTwistTargetName(const ea::string& name) { twistTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetTwistTargetName() const { return twistTargetName_; }

    void SetPositionWeight(float weight) { positionWeight_ = weight; }
    float GetPositionWeight() const { return positionWeight_; }
    void SetRotationWeight(float weight) { rotationWeight_ = weight; }
    float GetRotationWeight() const { return rotationWeight_; }
    void SetTwistWeight(float weight) { twistWeight_ = weight; }
    float GetTwistWeight() const { return twistWeight_; }
    void SetMaxAngle(float angle) { maxAngle_ = angle; }
    float GetMaxAngle() const { return maxAngle_; }
    void SetBendTweak(float tweak) { bendTweak_ = tweak; }
    float GetBendTweak() const { return bendTweak_; }
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
    void UpdateTwistRotationOffset();

    float WeightFunction(float x) const;

    void SetOriginalTransforms(const Transform& frameOfReference);
    float GetTwistAngle(const Transform& frameOfReference, const IKNodeSegment& segment, Node* targetNode) const;

    StringVector boneNames_;
    ea::string twistTargetName_;
    ea::string targetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float twistWeight_{1.0f};
    float maxAngle_{90.0f};
    float bendTweak_{0.0f};
    /// This orientation of twist bone in object space is equivalent to having no twist.
    Quaternion twistRotationOffset_{Quaternion::ZERO};

    IKSpineChain chain_;
    WeakPtr<Node> target_;
    WeakPtr<Node> twistTarget_;

    // TODO: Rename "local cache"
    struct LocalCache
    {
        ea::vector<Transform> defaultTransforms_;
        Vector3 baseDirection_;
        Quaternion zeroTwistRotation_;
    } local_;

    ea::vector<Quaternion> originalBoneRotations_;
};

} // namespace Urho3D
