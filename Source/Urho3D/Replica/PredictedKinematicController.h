// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Replica/BaseFeedbackBehavior.h"

#include <EASTL/optional.h>

#ifdef URHO3D_PHYSICS

namespace Urho3D
{

class KinematicCharacterController;
class ReplicatedTransform;

/// Input frame of predicted kinematic controller.
struct PredictedKinematicControllerFrame
{
    Vector3 walkVelocity_;
    Quaternion rotation_;
    bool needJump_{};

    /// Client only: body position in the beginning of the frame.
    Vector3 startPosition_;
};

/// Kinematic controller of the player replicated over network.
/// Input will be silently ignored if the client is not allowed to send it.
class URHO3D_API PredictedKinematicController : public BaseFeedbackBehavior<PredictedKinematicControllerFrame>
{
    URHO3D_OBJECT(PredictedKinematicController, BaseFeedbackBehavior<PredictedKinematicControllerFrame>);

public:
    explicit PredictedKinematicController(Context* context);
    ~PredictedKinematicController() override;

    static void RegisterObject(Context* context);

    /// Set desired walk velocity on the owner client.
    void SetWalkVelocity(const Vector3& velocity);
    /// Set whether to jump on the next update. Automatically reset on jump.
    void SetJump();

    /// Return whether the behavior is properly connected to components.
    bool IsConnectedToComponents() const { return physicsWorld_ && replicatedTransform_ && kinematicController_; }
    bool IsConnectedToStandaloneComponents() const { return physicsWorld_ && kinematicController_; }

    /// Return effective current velocity of the controller.
    /// Available for all three modes: server, replicating client and owner client.
    /// Velocity is synchronized between server and replicating client,
    /// but is not synchronized for owner client.
    const Vector3& GetVelocity() const { return effectiveVelocity_; }

    /// Implementation of NetworkObject
    /// @{
    void InitializeStandalone() override;
    void InitializeOnServer() override;
    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override;

    void InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime,
        const NetworkTime& inputTime) override;
    /// @}

protected:
    /// Implement BaseFeedbackBehavior.
    /// @{
    void OnServerFrameBegin(NetworkFrame serverFrame) override;
    void ApplyPayload(const PredictedKinematicControllerFrame& payload) override;
    void WritePayload(const PredictedKinematicControllerFrame& payload, Serializer& dest) const override;
    void ReadPayload(PredictedKinematicControllerFrame& payload, Deserializer& src) const override;
    /// @}

private:
    void InitializeCommon();

    void OnPhysicsSynchronizedOnClient(NetworkFrame frame);
    void CheckAndCorrectController(NetworkFrame frame);
    bool AdjustConfirmedFrame(NetworkFrame confirmedFrame, const PredictedKinematicControllerFrame& nextInput);
    void ApplyActionsOnClient();
    void UpdateEffectiveVelocity(float timeStep);

    WeakPtr<ReplicatedTransform> replicatedTransform_;
    WeakPtr<KinematicCharacterController> kinematicController_;
    WeakPtr<PhysicsWorld> physicsWorld_;

    Vector3 previousPosition_;
    Vector3 effectiveVelocity_;

    float networkStepTime_{};
    float physicsStepTime_{};

    struct ClientData
    {
        PredictedKinematicControllerFrame currentFrameData_;

        ea::optional<NetworkFrame> latestConfirmedFrame_;
        ea::optional<NetworkFrame> latestAffectedFrame_;
    } client_;
};

} // namespace Urho3D

#endif
