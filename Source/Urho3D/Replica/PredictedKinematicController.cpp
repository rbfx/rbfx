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
#include "../Replica/NetworkSettingsConsts.h"
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

    walkVelocity_ = velocity;
}

void PredictedKinematicController::InitializeOnServer()
{
    InitializeCommon();
    if (!IsConnectedToComponents())
        return;

    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

    feedbackVelocity_.Resize(traceDuration);

    SubscribeToEvent(E_BEGINSERVERNETWORKFRAME,
        [this](StringHash, VariantMap& eventData)
    {
        using namespace BeginServerNetworkFrame;
        const unsigned frame = eventData[P_FRAME].GetUInt();
        OnServerFrameBegin(frame);
    });
}

void PredictedKinematicController::InitializeFromSnapshot(unsigned frame, Deserializer& src)
{
    InitializeCommon();
    if (!IsConnectedToComponents())
        return;

    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const bool isOwned = GetNetworkObject()->GetNetworkMode() == NetworkObjectMode::ClientOwned;

    if (isOwned)
    {
        networkTransform_->SetTrackOnly(true);

        const unsigned maxInputFrames = replicationManager->GetSetting(NetworkSettings::MaxInputFrames).GetUInt();
        client_.input_.set_capacity(maxInputFrames);

        SubscribeToEvent(physicsWorld_, E_PHYSICSPRESTEP,
            [this](StringHash, VariantMap& eventData)
        {
            const Variant& networkFrame = eventData[PhysicsPreStep::P_NETWORKFRAME];
            if (!networkFrame.IsEmpty())
                OnPhysicsSynchronizedOnClient(networkFrame.GetUInt());
        });
    }
}

void PredictedKinematicController::InitializeCommon()
{
    networkTransform_ = node_->GetComponent<ReplicatedNetworkTransform>();
    kinematicController_ = node_->GetComponent<KinematicCharacterController>();

    physicsWorld_ = node_->GetScene()->GetComponent<PhysicsWorld>();
    physicsStepTime_ = physicsWorld_ ? 1.0f / physicsWorld_->GetFps() : 0.0f;
}

bool PredictedKinematicController::PrepareUnreliableFeedback(unsigned frame)
{
    return true;
}

/// TODO(network): rewrite
void PredictedKinematicController::ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src)
{
    const unsigned n = src.ReadVLE();
    for (unsigned i = 0; i < n; ++i)
    {
        const Vector3 newVelocity = src.ReadVector3();
        feedbackVelocity_.Set(feedbackFrame - n + i + 1, newVelocity);
    }
}

/// TODO(network): rewrite
void PredictedKinematicController::WriteUnreliableFeedback(unsigned frame, Serializer& dest)
{
    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned maxRedundancy = replicationManager->GetSetting(NetworkSettings::MaxInputFrames).GetUInt();
    const unsigned maxSize = client_.input_.size();
    const unsigned desiredRedundancy = 16;
    const unsigned inputBufferSize = ea::min({desiredRedundancy, maxRedundancy, maxSize});

    inputBuffer_.clear();
    for (unsigned i = 0; i < inputBufferSize; ++i)
        inputBuffer_.push_back(client_.input_[maxSize - inputBufferSize + i].walkVelocity_);

    /*(URHO3D_ASSERT(client_.input_.empty() || client_.input_.back().frame_ == frame);

    inputBuffer_.push_back(walkVelocity_);
    if (inputBuffer_.size() >= 8)
        inputBuffer_.pop_front();*/

    dest.WriteVLE(inputBuffer_.size());
    for (const Vector3& v : inputBuffer_)
        dest.WriteVector3(v);
}

void PredictedKinematicController::OnUnreliableDelta(unsigned frame)
{
}

void PredictedKinematicController::OnServerFrameBegin(unsigned serverFrame)
{
    if (AbstractConnection* owner = GetNetworkObject()->GetOwnerConnection())
    {
        // TODO(network): Use prior values as well
        if (const auto newVelocity = feedbackVelocity_.GetRaw(serverFrame))
        {
            auto kinematicController = node_->GetComponent<KinematicCharacterController>();
            kinematicController->SetWalkDirection(*newVelocity * physicsStepTime_);
        }
        else
            // TODO(network): Revisit logging
            URHO3D_LOGWARNING("PredictedKinematicController skipped input frame #{} on server", serverFrame);
    }
}

void PredictedKinematicController::OnPhysicsSynchronizedOnClient(unsigned frame)
{
    if (!IsConnectedToComponents())
        return;

    CheckAndCorrectController(frame);
    TrackCurrentInput(frame);

    // Apply actions
    kinematicController_->SetWalkDirection(walkVelocity_ * physicsStepTime_);
}

void PredictedKinematicController::CheckAndCorrectController(unsigned frame)
{
    // Skip if not ready
    const auto latestConfirmedFrame = networkTransform_->GetLatestReceivedFrame();
    if (client_.input_.empty() || !latestConfirmedFrame)
        return;

    // Apply only latest confirmed state only once.
    if (client_.latestConfirmedFrame_ && *client_.latestConfirmedFrame_ == *latestConfirmedFrame)
        return;

    // Avoid re-adjusting affected frames to stabilize behavior.
    if (client_.latestAffectedFrame_ && NetworkTime{*latestConfirmedFrame} - NetworkTime{*client_.latestAffectedFrame_} <= 0.0)
        return;

    // Skip if cannot find matching input frame for whatever reason
    const auto nextInputFrameIter = ea::find_if(client_.input_.begin(), client_.input_.end(),
        [&](const InputFrame& inputFrame) { return inputFrame.frame_ == *latestConfirmedFrame + 1; });
    if (nextInputFrameIter == client_.input_.end())
        return;

    const unsigned nextInputFrameIndex = nextInputFrameIter - client_.input_.begin();
    if (AdjustConfirmedFrame(*latestConfirmedFrame, nextInputFrameIndex))
        client_.latestAffectedFrame_ = frame;
    client_.latestConfirmedFrame_ = *latestConfirmedFrame;
}

bool PredictedKinematicController::AdjustConfirmedFrame(unsigned confirmedFrame, unsigned nextInputFrameIndex)
{
    const float movementThreshold = networkTransform_->GetMovementThreshold();
    const float smoothingConstant = networkTransform_->GetSmoothingConstant();

    const auto confirmedPosition = networkTransform_->GetRawTemporalWorldPosition(confirmedFrame);
    URHO3D_ASSERT(confirmedPosition);

    const InputFrame& nextInput = client_.input_[nextInputFrameIndex];
    const Vector3 offset = *confirmedPosition - nextInput.startPosition_;
    if (!offset.Equals(Vector3::ZERO, movementThreshold))
    {
        kinematicController_->AdjustRawPosition(offset, smoothingConstant);
        return true;
    }
    return false;
}

void PredictedKinematicController::TrackCurrentInput(unsigned frame)
{
    InputFrame currentInput;
    currentInput.frame_ = frame;
    currentInput.walkVelocity_ = walkVelocity_;
    currentInput.startPosition_ = kinematicController_->GetRawPosition();

    if (!client_.input_.empty())
    {
        InputFrame lastFrame = client_.input_.back();
        while (lastFrame.frame_ + 1 != frame)
        {
            ++lastFrame.frame_;
            // TODO(network): Revisit logging
            // TODO(network): Consider this on correction
            URHO3D_LOGWARNING("PredictedKinematicController skipped input frame #{} on client", lastFrame.frame_);
            client_.input_.push_back(lastFrame);
        }
    }
    client_.input_.push_back(currentInput);
}

}
#endif
