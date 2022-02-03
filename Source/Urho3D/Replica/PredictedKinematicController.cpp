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
    server_.input_.Resize(maxInputFrames_);

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

        client_.input_.set_capacity(maxInputFrames_);

        SubscribeToEvent(physicsWorld_, E_PHYSICSPRESTEP,
            [this](StringHash, VariantMap& eventData)
        {
            const Variant& networkFrame = eventData[PhysicsPreStep::P_NETWORKFRAME];
            if (!networkFrame.IsEmpty())
                OnPhysicsSynchronizedOnClient(networkFrame.GetUInt());
        });
    }
}

void PredictedKinematicController::InterpolateState(float timeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    client_.desiredRedundancy_ = ea::max(1, FloorToInt(inputTime - replicaTime));
}

void PredictedKinematicController::InitializeCommon()
{
    const auto replicationManager = GetNetworkObject()->GetReplicationManager();

    networkTransform_ = node_->GetComponent<ReplicatedNetworkTransform>();
    kinematicController_ = node_->GetComponent<KinematicCharacterController>();

    physicsWorld_ = node_->GetScene()->GetComponent<PhysicsWorld>();
    physicsStepTime_ = physicsWorld_ ? 1.0f / physicsWorld_->GetFps() : 0.0f;

    maxInputFrames_ = replicationManager->GetSetting(NetworkSettings::MaxInputFrames).GetUInt();
    maxRedundancy_ = replicationManager->GetSetting(NetworkSettings::MaxInputRedundancy).GetUInt();
}

bool PredictedKinematicController::PrepareUnreliableFeedback(unsigned frame)
{
    return true;
}

void PredictedKinematicController::WriteUnreliableFeedback(unsigned frame, Serializer& dest)
{
    if (client_.input_.empty() || client_.input_.back().frame_ != frame)
    {
        URHO3D_ASSERTLOG(0, "NetworkObject {}: Unexpected call to WriteUnreliableFeedback",
            ToString(GetNetworkObject()->GetNetworkId()));
        return;
    }

    const unsigned maxSize = client_.input_.size();
    const unsigned inputBufferSize = ea::min({client_.desiredRedundancy_, maxRedundancy_, maxSize});

    dest.WriteVLE(inputBufferSize);
    ea::for_each_n(client_.input_.rbegin(), inputBufferSize,
        [&](const InputFrame& inputFrame) { WriteInputFrame(inputFrame, dest); });
}

void PredictedKinematicController::ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src)
{
    const unsigned numInputFrames = ea::min(src.ReadVLE(), maxRedundancy_);
    for (unsigned i = 0; i < numInputFrames; ++i)
        ReadInputFrame(feedbackFrame - i, src);
}

void PredictedKinematicController::OnServerFrameBegin(unsigned serverFrame)
{
    if (!IsConnectedToComponents())
        return;

    if (const auto frameDataAndIndex = server_.input_.GetRawOrPrior(serverFrame))
    {
        const auto& [currentInput, currentInputFrame] = *frameDataAndIndex;
        kinematicController_->SetWalkDirection(currentInput.walkVelocity_ * physicsStepTime_);

        if (currentInputFrame != serverFrame)
            ++server_.lostFrames_;
    }
    else
    {
        ++server_.lostFrames_;
    }
    ++server_.totalFrames_;

    static const unsigned batchSize = 100;
    if (server_.totalFrames_ >= batchSize)
    {
        const float loss = static_cast<float>(server_.lostFrames_) / server_.totalFrames_;
        server_.lostFrames_ = 0;
        server_.totalFrames_ = 0;

        const auto replicationManager = GetNetworkObject()->GetReplicationManager();
        ServerReplicator* serverReplicator = replicationManager->GetServerReplicator();
        AbstractConnection* ownerConnection = GetNetworkObject()->GetOwnerConnection();
        serverReplicator->ReportInputLoss(ownerConnection, loss);
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
        [&](const InputFrame& inputFrame) { return !inputFrame.isLost_ && inputFrame.frame_ == *latestConfirmedFrame + 1; });
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
    if (!client_.input_.empty())
    {
        const unsigned previousInputFrame = client_.input_.back().frame_;
        const auto numSkippedFrames = static_cast<int>(frame - previousInputFrame) - 1;
        if (numSkippedFrames > 0)
        {
            if (numSkippedFrames >= client_.input_.capacity())
                client_.input_.clear();
            else
            {
                InputFrame lastFrame = client_.input_.back();
                lastFrame.isLost_ = true;
                for (int i = 1; i <= numSkippedFrames; ++i)
                {
                    lastFrame.frame_ = previousInputFrame;
                    client_.input_.push_back(lastFrame);
                }
            }

            URHO3D_LOGWARNING("NetworkObject {}: skipped {} input frames on client starting from #{}",
                ToString(GetNetworkObject()->GetNetworkId()), numSkippedFrames, previousInputFrame);
        }
    }

    InputFrame currentInput;
    currentInput.frame_ = frame;
    currentInput.walkVelocity_ = walkVelocity_;
    currentInput.startPosition_ = kinematicController_->GetRawPosition();
    client_.input_.push_back(currentInput);
}

void PredictedKinematicController::WriteInputFrame(const InputFrame& inputFrame, Serializer& dest) const
{
    dest.WriteVector3(inputFrame.walkVelocity_);
}

void PredictedKinematicController::ReadInputFrame(unsigned frame, Deserializer& src)
{
    const Vector3 walkVelocity = src.ReadVector3();

    InputFrame inputFrame;
    inputFrame.frame_ = frame;
    inputFrame.walkVelocity_ = walkVelocity;
    if (!server_.input_.Has(frame))
        server_.input_.Set(frame, inputFrame);

    feedbackVelocity_.Set(frame, walkVelocity);
}

}
#endif
