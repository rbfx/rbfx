// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IK/IKSolverComponent.h"

namespace Urho3D
{

class URHO3D_API IKStickTargets : public IKSolverComponent
{
    URHO3D_OBJECT(IKStickTargets, IKSolverComponent);

public:
    static constexpr float DefaultPositionThreshold = 0.3f;
    static constexpr float DefaultRotationThreshold = 45.0f;
    static constexpr float DefaultTimeThreshold = 0.8f;
    static constexpr float DefaultRecoverTime = 0.2f;

    explicit IKStickTargets(Context* context);
    ~IKStickTargets() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    // clang-format off
    void SetTargetNames(const StringVector& names) { targetNames_ = names; OnTreeDirty(); }
    const StringVector& GetTargetNames() const { return targetNames_; }
    void SetActive(bool value) { isActive_ = value; }
    bool IsActive() const { return isActive_; }
    void SetPositionSticky(bool value) { isPositionSticky_ = value; }
    bool IsPositionSticky() const { return isPositionSticky_; }
    void SetRotationSticky(bool value) { isRotationSticky_ = value; }
    bool IsRotationSticky() const { return isRotationSticky_; }
    void SetPositionThreshold(float threshold) { positionThreshold_ = threshold; }
    float GetPositionThreshold() const { return positionThreshold_; }
    void SetRotationThreshold(float threshold) { rotationThreshold_ = threshold; }
    float GetRotationThreshold() const { return rotationThreshold_; }
    void SetTimeThreshold(float threshold) { timeThreshold_ = threshold; }
    float GetTimeThreshold() const { return timeThreshold_; }
    void SetRecoverTime(float time) { recoverTime_ = time; }
    float GetRecoverTime() const { return recoverTime_; }
    void SetMinTargetDistance(float distance) { minTargetDistance_ = distance; }
    float GetMinTargetDistance() const { return minTargetDistance_; }
    void SetMaxSimultaneousRecoveries(unsigned max) { maxSimultaneousRecoveries_ = max; }
    unsigned GetMaxSimultaneousRecoveries() const { return maxSimultaneousRecoveries_; }
    void SetBaseWorldVelocity(const Vector3& velocity) { baseWorldVelocity_ = velocity; }
    const Vector3& GetBaseWorldVelocity() const { return baseWorldVelocity_; }
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
    enum class TargetState
    {
        Inactive,
        Stuck,
        Recovering,
    };

    struct TargetInfo
    {
        WeakPtr<Node> node_;

        TargetState state_{};
        Transform desiredWorldTransform_;

        ea::optional<Transform> overrideWorldTransform_;
        float overrideWeight_{};
        float stuckTimer_{};

        void Stick()
        {
            state_ = TargetState::Stuck;
            stuckTimer_ = 0.0f;
            OverrideToCurrent();
        }

        void OverrideToCurrent()
        {
            overrideWorldTransform_ = GetCurrentTransform();
            overrideWeight_ = 1.0f;
        }

        void SubtractWeight(float delta) { overrideWeight_ = ea::max(overrideWeight_ - delta, 0.0f); }

        bool IsEffectivelyInactive() const { return state_ == TargetState::Inactive && overrideWeight_ == 0.0f; }

        Transform GetCurrentTransform() const
        {
            if (!overrideWorldTransform_ || overrideWeight_ == 0.0f)
                return desiredWorldTransform_;
            return desiredWorldTransform_.Lerp(*overrideWorldTransform_, overrideWeight_);
        }

        float GetStuckPositionError() const
        {
            if (!overrideWorldTransform_ || state_ != TargetState::Stuck)
                return 0.0f;

            return (overrideWorldTransform_->position_ - desiredWorldTransform_.position_).Length();
        }

        float GetStuckRotationError() const
        {
            if (!overrideWorldTransform_ || state_ != TargetState::Stuck)
                return 0.0f;

            return (overrideWorldTransform_->rotation_ * desiredWorldTransform_.rotation_.Inverse()).Angle();
        }

        float GetStuckTime() const
        {
            if (!overrideWorldTransform_ || state_ != TargetState::Stuck)
                return 0.0f;

            return stuckTimer_;
        }
    };

    void CollectDesiredWorldTransforms();
    void ApplyWorldMovement(float timeStep);
    void UpdateOverrideWeights(float timeStep);
    void UpdateStuckTimers(float timeStep);
    void ApplyDeactivation();
    void ApplyActivation();
    void UpdateRecovery();
    void CommitWorldTransforms();

    float GetDistanceToNearestStuckTarget(const Vector3& worldPosition) const;

    StringVector targetNames_;
    bool isActive_{true};
    bool isPositionSticky_{true};
    bool isRotationSticky_{true};
    float positionThreshold_{DefaultPositionThreshold};
    float rotationThreshold_{DefaultRotationThreshold};
    float timeThreshold_{DefaultTimeThreshold};
    float recoverTime_{DefaultRecoverTime};
    float minTargetDistance_{};
    unsigned maxSimultaneousRecoveries_{};
    Vector3 baseWorldVelocity_;

    ea::vector<TargetInfo> targets_;
    unsigned recoveryStartIndex_{};
};

} // namespace Urho3D
