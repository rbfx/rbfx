//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

#pragma once

#include "../Replica/BehaviorNetworkObject.h"

#include <EASTL/optional.h>
#include <EASTL/vector.h>
#include <EASTL/bonus/ring_buffer.h>

#ifdef URHO3D_PHYSICS

namespace Urho3D
{

class KinematicCharacterController;
class ReplicatedTransform;

/// Kinematic controller of the player replicated over network.
class URHO3D_API PredictedKinematicController : public NetworkBehavior
{
    URHO3D_OBJECT(PredictedKinematicController, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallbackMask::UnreliableFeedback | NetworkCallbackMask::InterpolateState;

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

    void InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime) override;

    bool PrepareUnreliableFeedback(NetworkFrame frame) override;
    void WriteUnreliableFeedback(NetworkFrame frame, Serializer& dest) override;
    void ReadUnreliableFeedback(NetworkFrame feedbackFrame, Deserializer& src) override;
    /// @}

private:
    struct InputFrame
    {
        bool isLost_{};
        NetworkFrame frame_{};
        Vector3 startPosition_;
        Vector3 walkVelocity_;
        Quaternion rotation_;
        bool needJump_{};
    };

    void InitializeCommon();

    void OnServerFrameBegin(NetworkFrame serverFrame);

    void OnPhysicsSynchronizedOnClient(NetworkFrame frame);
    void CheckAndCorrectController(NetworkFrame frame);
    bool AdjustConfirmedFrame(NetworkFrame confirmedFrame, unsigned nextInputFrameIndex);
    void TrackCurrentInput(NetworkFrame frame);
    void ApplyActionsOnClient();
    void UpdateEffectiveVelocity(float timeStep);

    void WriteInputFrame(const InputFrame& inputFrame, Serializer& dest) const;
    void ReadInputFrame(NetworkFrame frame, Deserializer& src);

    WeakPtr<ReplicatedTransform> replicatedTransform_;
    WeakPtr<KinematicCharacterController> kinematicController_;
    WeakPtr<PhysicsWorld> physicsWorld_;

    Vector3 previousPosition_;
    Vector3 effectiveVelocity_;

    float networkStepTime_{};
    float physicsStepTime_{};
    unsigned maxRedundancy_{};
    unsigned maxInputFrames_{};

    struct ServerData
    {
        NetworkValue<InputFrame> input_;
        unsigned totalFrames_{};
        unsigned lostFrames_{};
    } server_;

    struct ClientData
    {
        Vector3 walkVelocity_;
        bool needJump_{};

        unsigned desiredRedundancy_{};
        ea::ring_buffer<InputFrame> input_;
        ea::optional<NetworkFrame> latestConfirmedFrame_;
        ea::optional<NetworkFrame> latestAffectedFrame_;
    } client_;
};

}

#endif
