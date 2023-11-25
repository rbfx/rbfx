// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IK/IKSolverComponent.h"

namespace Urho3D
{

class URHO3D_API IKIdentitySolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKIdentitySolver, IKSolverComponent);

public:
    explicit IKIdentitySolver(Context* context);
    ~IKIdentitySolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    void UpdateProperties();

    /// Attributes.
    /// @{
    // clang-format off
    void SetBoneName(const ea::string& name) { boneName_ = name; OnTreeDirty(); }
    const ea::string& GetBoneName() const { return boneName_; }
    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }

    void SetRotationOffset(const Quaternion& rotation) { rotationOffset_ = rotation; }
    const Quaternion& GetRotationOffset() const { return rotationOffset_; }
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
    void UpdateRotationOffset();

    ea::string boneName_;
    ea::string targetName_;
    Quaternion rotationOffset_{Quaternion::ZERO};

    IKNode* boneNode_{};
    WeakPtr<Node> target_;
};

} // namespace Urho3D
