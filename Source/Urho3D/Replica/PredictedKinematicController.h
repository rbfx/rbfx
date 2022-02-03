//
// Copyright (c) 2008-2020 the Urho3D project.
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
class ReplicatedNetworkTransform;

/// Kinematic controller of the player replicated over network.
class URHO3D_API PredictedKinematicController : public NetworkBehavior
{
    URHO3D_OBJECT(PredictedKinematicController, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallback::UnreliableFeedback | NetworkCallback::InterpolateState;

    explicit PredictedKinematicController(Context* context);
    ~PredictedKinematicController() override;

    static void RegisterObject(Context* context);

    /// Set desired walk velocity on the owner client.
    void SetWalkVelocity(const Vector3& velocity);
    /// Return whether the behavior is properly connected to components.
    bool IsConnectedToComponents() const { return physicsWorld_ && networkTransform_ && kinematicController_; }

    /// Implementation of NetworkObject
    /// @{
    void InitializeOnServer() override;
    void InitializeFromSnapshot(unsigned frame, Deserializer& src) override;

    void InterpolateState(float timeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime) override;

    bool PrepareUnreliableFeedback(unsigned frame) override;
    void WriteUnreliableFeedback(unsigned frame, Serializer& dest) override;
    void ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src) override;
    /// @}

private:
    struct InputFrame
    {
        bool isLost_{};
        unsigned frame_{};
        Vector3 startPosition_;
        Vector3 walkVelocity_;
    };

    void InitializeCommon();

    void OnServerFrameBegin(unsigned serverFrame);

    void OnPhysicsSynchronizedOnClient(unsigned frame);
    void CheckAndCorrectController(unsigned frame);
    bool AdjustConfirmedFrame(unsigned confirmedFrame, unsigned nextInputFrameIndex);
    void TrackCurrentInput(unsigned frame);

    void WriteInputFrame(const InputFrame& inputFrame, Serializer& dest) const;
    void ReadInputFrame(unsigned frame, Deserializer& src);

    WeakPtr<ReplicatedNetworkTransform> networkTransform_;
    WeakPtr<KinematicCharacterController> kinematicController_;
    WeakPtr<PhysicsWorld> physicsWorld_;

    /// Dynamic attributes.
    /// @{
    Vector3 walkVelocity_;
    /// @}

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
        unsigned desiredRedundancy_{};
        ea::ring_buffer<InputFrame> input_;
        ea::optional<unsigned> latestConfirmedFrame_;
        ea::optional<unsigned> latestAffectedFrame_;
    } client_;


    /// Server only: feedback from client.
    NetworkValue<Vector3> feedbackVelocity_;
};

#endif

}
