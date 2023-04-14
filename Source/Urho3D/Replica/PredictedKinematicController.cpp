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
#include "../Replica/ReplicatedTransform.h"

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
    context->AddFactoryReflection<PredictedKinematicController>(Category_Network);
}

void PredictedKinematicController::SetWalkVelocity(const Vector3& velocity)
{
    if (!IsConnectedToStandaloneComponents())
        return;

    NetworkObject* networkObject = GetNetworkObject();
    URHO3D_ASSERT(networkObject);

    if (networkObject->IsStandalone() || networkObject->IsOwnedByThisClient())
    {
        client_.walkVelocity_ = velocity;
    }
    else
    {
        URHO3D_LOGWARNING(
            "PredictedKinematicController::SetWalkVelocity is called for object {} by the client that doesn't own this object.",
            ToString(networkObject->GetNetworkId()));
    }
}

void PredictedKinematicController::SetJump()
{
    if (!IsConnectedToStandaloneComponents())
        return;

    NetworkObject* networkObject = GetNetworkObject();
    URHO3D_ASSERT(networkObject);

    if (networkObject->IsStandalone() || networkObject->IsOwnedByThisClient())
    {
        client_.needJump_ = true;
    }
    else
    {
        URHO3D_LOGWARNING(
            "PredictedKinematicController::SetJump is called for object {} by the client that doesn't own this object.",
            ToString(networkObject->GetNetworkId()));
    }
}

void PredictedKinematicController::InitializeStandalone()
{
    InitializeCommon();
    if (!IsConnectedToStandaloneComponents())
        return;

    SubscribeToEvent(physicsWorld_, E_PHYSICSPRESTEP,
        [this]
    {
        UpdateEffectiveVelocity(physicsStepTime_);
        ApplyActionsOnClient();
    });
}

void PredictedKinematicController::InitializeOnServer()
{
    InitializeCommon();
    if (!IsConnectedToComponents())
        return;

    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

    server_.input_.Resize(maxInputFrames_);

    SubscribeToEvent(E_BEGINSERVERNETWORKFRAME,
        [this](VariantMap& eventData)
    {
        using namespace BeginServerNetworkFrame;
        const auto frame = static_cast<NetworkFrame>(eventData[P_FRAME].GetInt64());
        OnServerFrameBegin(frame);
    });
}

void PredictedKinematicController::InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned)
{
    InitializeCommon();
    if (!IsConnectedToComponents())
        return;

    if (isOwned)
    {
        replicatedTransform_->SetPositionTrackOnly(true);

        client_.input_.set_capacity(maxInputFrames_);

        SubscribeToEvent(physicsWorld_, E_PHYSICSPRESTEP,
            [this](VariantMap& eventData)
        {
            const Variant& networkFrame = eventData[PhysicsPreStep::P_NETWORKFRAME];
            if (!networkFrame.IsEmpty())
                OnPhysicsSynchronizedOnClient(static_cast<NetworkFrame>(networkFrame.GetInt64()));
        });
    }
    else
        kinematicController_->SetGravity(Vector3::ZERO);
}

void PredictedKinematicController::InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    if (!IsConnectedToComponents())
        return;

    NetworkObject* networkObject = GetNetworkObject();
    if (!networkObject->IsOwnedByThisClient())
    {
        // Get velocity without interpolation within frame
        ReplicationManager* replicationManager = networkObject->GetReplicationManager();
        const float derivativeTimeStep = 1.0f / replicationManager->GetUpdateFrequency();
        const auto positionAndVelocity = replicatedTransform_->SampleTemporalPosition(NetworkTime{replicaTime.Frame()});
        effectiveVelocity_ = positionAndVelocity.derivative_ * derivativeTimeStep;
    }
    else
    {
        client_.desiredRedundancy_ = ea::max(1, FloorToInt(inputTime - replicaTime));
    }
}

void PredictedKinematicController::InitializeCommon()
{
    UnsubscribeFromEvent(E_BEGINSERVERNETWORKFRAME);
    UnsubscribeFromEvent(E_PHYSICSPRESTEP);

    replicatedTransform_ = node_->GetComponent<ReplicatedTransform>();
    kinematicController_ = node_->GetComponent<KinematicCharacterController>();

    previousPosition_ = node_->GetWorldPosition();

    if (Scene* scene = node_->GetScene())
    {
        physicsWorld_ = scene->GetComponent<PhysicsWorld>();
        physicsStepTime_ = physicsWorld_ ? 1.0f / physicsWorld_->GetFps() : 0.0f;
    }

    if (NetworkObject* networkObject = GetNetworkObject())
    {
        const auto replicationManager = networkObject->GetReplicationManager();
        maxInputFrames_ = replicationManager->GetSetting(NetworkSettings::MaxInputFrames).GetUInt();
        maxRedundancy_ = replicationManager->GetSetting(NetworkSettings::MaxInputRedundancy).GetUInt();
        networkStepTime_ = 1.0f / replicationManager->GetUpdateFrequency();
    }
}

bool PredictedKinematicController::PrepareUnreliableFeedback(NetworkFrame frame)
{
    return true;
}

void PredictedKinematicController::WriteUnreliableFeedback(NetworkFrame frame, Serializer& dest)
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

void PredictedKinematicController::ReadUnreliableFeedback(NetworkFrame feedbackFrame, Deserializer& src)
{
    const unsigned numInputFrames = ea::min(src.ReadVLE(), maxRedundancy_);
    for (unsigned i = 0; i < numInputFrames; ++i)
        ReadInputFrame(feedbackFrame - i, src);
}

void PredictedKinematicController::OnServerFrameBegin(NetworkFrame serverFrame)
{
    if (!IsConnectedToComponents())
        return;

    UpdateEffectiveVelocity(networkStepTime_);

    if (const auto frameDataAndIndex = server_.input_.GetRawOrPrior(serverFrame))
    {
        const auto& [currentInput, currentInputFrame] = *frameDataAndIndex;

        node_->SetWorldRotation(currentInput.rotation_);
        kinematicController_->SetWalkIncrement(currentInput.walkVelocity_ * physicsStepTime_);
        if (currentInput.needJump_ && kinematicController_->OnGround())
            kinematicController_->Jump();

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
        if (AbstractConnection* ownerConnection = GetNetworkObject()->GetOwnerConnection())
            serverReplicator->ReportInputLoss(ownerConnection, loss);
    }
}

void PredictedKinematicController::OnPhysicsSynchronizedOnClient(NetworkFrame frame)
{
    if (!IsConnectedToComponents())
        return;

    CheckAndCorrectController(frame);
    TrackCurrentInput(frame);
    ApplyActionsOnClient();
    UpdateEffectiveVelocity(networkStepTime_);
}

void PredictedKinematicController::ApplyActionsOnClient()
{
    kinematicController_->SetWalkIncrement(client_.walkVelocity_ * physicsStepTime_);
    if (client_.needJump_ && kinematicController_->OnGround())
        kinematicController_->Jump();
    client_.needJump_ = false;
}

void PredictedKinematicController::UpdateEffectiveVelocity(float timeStep)
{
    const Vector3 currentPosition = node_->GetWorldPosition();
    effectiveVelocity_ = (currentPosition - previousPosition_) / timeStep;
    previousPosition_ = currentPosition;
}

void PredictedKinematicController::CheckAndCorrectController(NetworkFrame frame)
{
    // Skip if not ready
    const auto latestConfirmedFrame = replicatedTransform_->GetLatestFrame();
    if (client_.input_.empty() || !latestConfirmedFrame)
        return;

    // Apply only latest confirmed state only once.
    if (client_.latestConfirmedFrame_ && *client_.latestConfirmedFrame_ == *latestConfirmedFrame)
        return;

    // Avoid re-adjusting affected frames to stabilize behavior.
    if (client_.latestAffectedFrame_ && (*latestConfirmedFrame <= *client_.latestAffectedFrame_))
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

bool PredictedKinematicController::AdjustConfirmedFrame(NetworkFrame confirmedFrame, unsigned nextInputFrameIndex)
{
    const float movementThreshold = replicatedTransform_->GetMovementThreshold();
    const float smoothingConstant = replicatedTransform_->GetSmoothingConstant();

    const auto confirmedPosition = replicatedTransform_->GetTemporalPosition(confirmedFrame);
    URHO3D_ASSERT(confirmedPosition);

    const InputFrame& nextInput = client_.input_[nextInputFrameIndex];
    const Vector3 offset = confirmedPosition->value_ - nextInput.startPosition_;
    if (!offset.Equals(Vector3::ZERO, movementThreshold))
    {
        kinematicController_->AdjustRawPosition(offset, smoothingConstant);
        return true;
    }
    return false;
}

void PredictedKinematicController::TrackCurrentInput(NetworkFrame frame)
{
    if (!client_.input_.empty())
    {
        const NetworkFrame previousInputFrame = client_.input_.back().frame_;
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
    currentInput.walkVelocity_ = client_.walkVelocity_;
    currentInput.startPosition_ = kinematicController_->GetRawPosition();
    currentInput.needJump_ = client_.needJump_;
    currentInput.rotation_ = node_->GetWorldRotation();
    client_.input_.push_back(currentInput);
}

void PredictedKinematicController::WriteInputFrame(const InputFrame& inputFrame, Serializer& dest) const
{
    dest.WriteVector3(inputFrame.walkVelocity_);
    dest.WriteBool(inputFrame.needJump_);
    dest.WriteQuaternion(inputFrame.rotation_);
}

void PredictedKinematicController::ReadInputFrame(NetworkFrame frame, Deserializer& src)
{
    const Vector3 walkVelocity = src.ReadVector3();
    const bool needJump = src.ReadBool();
    const Quaternion rotation = src.ReadQuaternion();

    InputFrame inputFrame;
    inputFrame.frame_ = frame;
    inputFrame.walkVelocity_ = walkVelocity;
    inputFrame.needJump_ = needJump;
    inputFrame.rotation_ = rotation;

    if (!server_.input_.Has(frame))
        server_.input_.Set(frame, inputFrame);
}

}
#endif
