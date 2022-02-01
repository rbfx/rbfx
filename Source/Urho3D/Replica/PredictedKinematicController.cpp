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

#include "../Precompiled.h"

#include "../Replica/PredictedKinematicController.h"

#ifdef URHO3D_PHYSICS
#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Physics/KinematicCharacterController.h"
#include "../Physics/PhysicsEvents.h"
#include "../Physics/PhysicsWorld.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "../Network/NetworkEvents.h"
#include "../Replica/ReplicatedNetworkTransform.h"

namespace Urho3D
{

PredictedKinematicController::PredictedKinematicController(Context* context)
    : NetworkBehavior(context, CallbackMask)
{
}

PredictedKinematicController::~PredictedKinematicController()
{
}

void PredictedKinematicController::RegisterObject(Context* context)
{
    context->RegisterFactory<PredictedKinematicController>();
}

void PredictedKinematicController::SetWalkVelocity(const Vector3& velocity)
{
    if (GetNetworkObject()->GetNetworkMode() == NetworkObjectMode::ClientReplicated)
    {
        URHO3D_LOGWARNING(
            "PredictedKinematicController::SetWalkVelocity is called for object {} even tho this client doesn't own it",
            ToString(GetNetworkObject()->GetNetworkId()));
        return;
    }

    velocity_ = velocity;
}

void PredictedKinematicController::InitializeOnServer()
{
    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();
    feedbackVelocity_.Resize(traceDuration);

    SubscribeToEvent(E_BEGINSERVERNETWORKFRAME,
        [this](StringHash, VariantMap& eventData)
    {
        using namespace BeginServerNetworkFrame;
        const unsigned serverFrame = eventData[P_FRAME].GetUInt();
        OnServerNetworkFrameBegin(serverFrame);
    });
}

void PredictedKinematicController::ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src)
{
    const unsigned n = src.ReadVLE();
    for (unsigned i = 0; i < n; ++i)
    {
        const Vector3 newVelocity = src.ReadVector3();
        feedbackVelocity_.Set(feedbackFrame - n + i + 1, newVelocity);
    }
}

void PredictedKinematicController::InitializeFromSnapshot(unsigned frame, Deserializer& src)
{
    networkTransform_ = node_->GetComponent<ReplicatedNetworkTransform>();
    kinematicController_ = node_->GetComponent<KinematicCharacterController>();

    if (GetNetworkObject()->GetNetworkMode() == NetworkObjectMode::ClientOwned)
    {
        networkTransform_->SetTrackOnly(true);

        const auto physicsWorld = node_->GetScene()->GetComponent<PhysicsWorld>();
        SubscribeToEvent(physicsWorld, E_PHYSICSPRESTEP, [this](StringHash, VariantMap& eventData)
        {
            const Variant& networkFrame = eventData[PhysicsPreStep::P_NETWORKFRAME];
            if (!networkFrame.IsEmpty())
                OnPhysicsSynchronizedOnClient(networkFrame.GetUInt());
        });
    }
}

bool PredictedKinematicController::PrepareUnreliableFeedback(unsigned frame)
{
    return true;
}

void PredictedKinematicController::WriteUnreliableFeedback(unsigned frame, Serializer& dest)
{
    inputBuffer_.push_back(velocity_);
    if (inputBuffer_.size() >= 8)
        inputBuffer_.pop_front();

    dest.WriteVLE(inputBuffer_.size());
    for (const Vector3& v : inputBuffer_)
        dest.WriteVector3(v);
}

void PredictedKinematicController::OnUnreliableDelta(unsigned frame)
{
    if (!kinematicController_ || !networkTransform_)
        return;

    compareNextStepToFrame_ = frame;
}

void PredictedKinematicController::CorrectAgainstFrame(unsigned frame)
{
    // Skip frames without confirmed data (shouldn't happen too ofter)
    const auto confirmedPosition = networkTransform_->GetRawTemporalWorldPosition(frame);
    if (!confirmedPosition)
        return;

    // Skip if no predicted frame (shouldn't happen too ofter as well)
    const auto greaterOrEqual = [&](const auto& frameAndPosition) { return frameAndPosition.first >= frame; };
    const auto iter = ea::find_if(predictedWorldPositions_.begin(), predictedWorldPositions_.end(), greaterOrEqual);
    predictedWorldPositions_.erase(predictedWorldPositions_.begin(), iter);
    if (predictedWorldPositions_.empty() || predictedWorldPositions_.front().first != frame)
        return;

    const Vector3 predictedPosition = predictedWorldPositions_.front().second;
    const Vector3 offset = *confirmedPosition - predictedPosition;
    if (!offset.Equals(Vector3::ZERO, 0.001f))
    {
        const auto replicationManager = GetNetworkObject()->GetReplicationManager();
        // TODO(network): Refactor
        const float smoothConstant = 15.0f;
        kinematicController_->AdjustRawPosition(offset, smoothConstant);
        predictedWorldPositions_.clear();
        //for (auto& [predictionFrame, otherPredictedPosition] : predictedWorldPositions_)
        //    otherPredictedPosition += offset;
    }
}

void PredictedKinematicController::OnServerNetworkFrameBegin(unsigned serverFrame)
{
    if (AbstractConnection* owner = GetNetworkObject()->GetOwnerConnection())
    {
        // TODO(network): Use prior values as well
        if (const auto newVelocity = feedbackVelocity_.GetRaw(serverFrame))
        {
            auto kinematicController = node_->GetComponent<KinematicCharacterController>();
            const float timeStep = 1.0f / GetScene()->GetComponent<PhysicsWorld>()->GetFps(); // TODO(network): Remove before merge!!!
            kinematicController->SetWalkDirection(*newVelocity * timeStep);
        }
    }
}

void PredictedKinematicController::OnPhysicsSynchronizedOnClient(unsigned networkFrame)
{
    if (kinematicController_)
    {
        const float timeStep = 1.0f / GetScene()->GetComponent<PhysicsWorld>()->GetFps(); // TODO(network): Remove before merge!!!
        kinematicController_->SetWalkDirection(velocity_ * timeStep);

        if (compareNextStepToFrame_)
        {
            CorrectAgainstFrame(*compareNextStepToFrame_);
            compareNextStepToFrame_ = ea::nullopt;
        }

        predictedWorldPositions_.emplace_back(networkFrame - 1, kinematicController_->GetRawPosition());
    }
}

}
#endif
