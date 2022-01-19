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

#include "../Network/DefaultNetworkObject.h"

#include <EASTL/optional.h>

#ifdef URHO3D_PHYSICS

namespace Urho3D
{

class KinematicCharacterController;

/// Kinematic controller of the player replicated over network.
/// TODO(network): Rename to PredictedKinematicController?
class URHO3D_API KinematicPlayerNetworkObject : public NetworkBehavior
{
    URHO3D_OBJECT(KinematicPlayerNetworkObject, NetworkBehavior);

public:
    explicit KinematicPlayerNetworkObject(Context* context);
    ~KinematicPlayerNetworkObject() override;

    static void RegisterObject(Context* context);

    /// Set desired walk velocity on the owner client.
    void SetWalkVelocity(const Vector3& velocity);

    /// Implementation of NetworkObject
    /// @{
    void InitializeOnServer() override;
    void ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src) override;

    void InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime, const ea::optional<unsigned>& isNewInputFrame) override;
    void ReadSnapshot(unsigned frame, Deserializer& src) override;
    void OnUnreliableDelta(unsigned frame) override;
    unsigned GetUnreliableFeedbackMask(unsigned frame) override;
    void WriteUnreliableFeedback(unsigned frame, unsigned mask, Serializer& dest) override;
    /// @}

protected:
    /// Called when frame begins on server.
    void OnServerNetworkFrameBegin();
    /// Called when physics step is over.
    void OnPhysicsPostStepOnClient();
    void CorrectAgainstFrame(unsigned frame);

private:
    WeakPtr<ReplicatedNetworkTransform> networkTransform_;
    WeakPtr<KinematicCharacterController> kinematicController_;
    Vector3 velocity_;

    /// Client only: track of predicted positions for unprocessed.
    ea::vector<ea::pair<unsigned, Vector3>> predictedWorldPositions_;
    ea::vector<Vector3> inputBuffer_;
    /// Client only: whether to track next physics step.
    ea::optional<ea::pair<unsigned, unsigned>> trackNextStepAsFrame_;
    ea::optional<unsigned> compareNextStepToFrame_;

    /// Server only: feedback from client.
    NetworkValue<Vector3> feedbackVelocity_;
};

#endif

}
