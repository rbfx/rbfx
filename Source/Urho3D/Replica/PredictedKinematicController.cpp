// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Replica/PredictedKinematicController.h"

#ifdef URHO3D_PHYSICS
    #include "Urho3D/Core/Context.h"
    #include "Urho3D/Core/CoreEvents.h"
    #include "Urho3D/Network/NetworkEvents.h"
    #include "Urho3D/Physics/KinematicCharacterController.h"
    #include "Urho3D/Physics/PhysicsEvents.h"
    #include "Urho3D/Physics/PhysicsWorld.h"
    #include "Urho3D/Replica/NetworkSettingsConsts.h"
    #include "Urho3D/Replica/ReplicatedTransform.h"
    #include "Urho3D/Scene/Node.h"
    #include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

PredictedKinematicController::PredictedKinematicController(Context* context)
    : BaseFeedbackBehavior<PredictedKinematicControllerFrame>(context, CallbackMask)
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
    client_.currentFrameData_.walkVelocity_ = velocity;
}

void PredictedKinematicController::SetJump()
{
    client_.currentFrameData_.needJump_ = true;
}

void PredictedKinematicController::InitializeStandalone()
{
    InitializeCommon();
    if (!IsConnectedToStandaloneComponents())
        return;

    BaseClassName::InitializeStandalone();

    SubscribeToEvent(physicsWorld_, E_PHYSICSPRESTEP, [this]
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

    BaseClassName::InitializeOnServer();
}

void PredictedKinematicController::InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned)
{
    InitializeCommon();
    if (!IsConnectedToComponents())
        return;

    BaseClassName::InitializeFromSnapshot(frame, src, isOwned);

    if (isOwned)
    {
        replicatedTransform_->SetPositionTrackOnly(true);

        SubscribeToEvent(physicsWorld_, E_PHYSICSPRESTEP, [this](VariantMap& eventData)
        {
            const Variant& networkFrame = eventData[PhysicsPreStep::P_NETWORKFRAME];
            if (!networkFrame.IsEmpty())
                OnPhysicsSynchronizedOnClient(static_cast<NetworkFrame>(networkFrame.GetInt64()));
        });
    }
    else
        kinematicController_->SetGravity(Vector3::ZERO);
}

void PredictedKinematicController::InterpolateState(
    float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    if (!IsConnectedToComponents())
        return;

    BaseClassName::InterpolateState(replicaTimeStep, inputTimeStep, replicaTime, inputTime);

    NetworkObject* networkObject = GetNetworkObject();
    if (!networkObject->IsOwnedByThisClient())
    {
        // Get velocity without interpolation within frame
        ReplicationManager* replicationManager = networkObject->GetReplicationManager();
        const float derivativeTimeStep = 1.0f / replicationManager->GetUpdateFrequency();
        const auto positionAndVelocity = replicatedTransform_->SampleTemporalPosition(NetworkTime{replicaTime.Frame()});
        effectiveVelocity_ = positionAndVelocity.derivative_ * derivativeTimeStep;
    }
}

void PredictedKinematicController::InitializeCommon()
{
    UnsubscribeFromEvent(E_PHYSICSPRESTEP);

    replicatedTransform_ = node_->GetComponent<ReplicatedTransform>();
    kinematicController_ = node_->GetComponent<KinematicCharacterController>();

    previousPosition_ = node_->GetWorldPosition();

    if (Scene* scene = node_->GetScene())
    {
        physicsWorld_ = scene->GetComponent<PhysicsWorld>();
        physicsStepTime_ = physicsWorld_ ? 1.0f / physicsWorld_->GetFps() : 0.0f;
    }

    NetworkObject* networkObject = GetNetworkObject();
    const auto replicationManager = networkObject->GetReplicationManager();
    networkStepTime_ = 1.0f / replicationManager->GetUpdateFrequency();
}

void PredictedKinematicController::OnServerFrameBegin(NetworkFrame serverFrame)
{
    if (!IsConnectedToComponents())
        return;

    UpdateEffectiveVelocity(networkStepTime_);

    BaseClassName::OnServerFrameBegin(serverFrame);
}

void PredictedKinematicController::OnPhysicsSynchronizedOnClient(NetworkFrame frame)
{
    if (!IsConnectedToComponents())
        return;

    CheckAndCorrectController(frame);

    client_.currentFrameData_.startPosition_ = kinematicController_->GetRawPosition();
    client_.currentFrameData_.rotation_ = node_->GetWorldRotation();
    CreateFrameOnClient(frame, client_.currentFrameData_);

    ApplyActionsOnClient();
    UpdateEffectiveVelocity(networkStepTime_);
}

void PredictedKinematicController::ApplyActionsOnClient()
{
    kinematicController_->SetWalkDirection(client_.currentFrameData_.walkVelocity_);
    if (client_.currentFrameData_.needJump_ && kinematicController_->OnGround())
        kinematicController_->Jump();
    client_.currentFrameData_.needJump_ = false;
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
    if (!HasFramesOnClient() || !latestConfirmedFrame)
        return;

    // Apply only latest confirmed state only once.
    if (client_.latestConfirmedFrame_ && *client_.latestConfirmedFrame_ == *latestConfirmedFrame)
        return;

    // Avoid re-adjusting affected frames to stabilize behavior.
    if (client_.latestAffectedFrame_ && (*latestConfirmedFrame <= *client_.latestAffectedFrame_))
        return;

    // Skip if cannot find matching input frame for whatever reason
    const auto* nextInputFrame = FindFrameOnClient(*latestConfirmedFrame + 1);
    if (!nextInputFrame)
        return;

    if (AdjustConfirmedFrame(*latestConfirmedFrame, *nextInputFrame))
        client_.latestAffectedFrame_ = frame;
    client_.latestConfirmedFrame_ = *latestConfirmedFrame;
}

bool PredictedKinematicController::AdjustConfirmedFrame(
    NetworkFrame confirmedFrame, const PredictedKinematicControllerFrame& nextInput)
{
    const float movementThreshold = replicatedTransform_->GetMovementThreshold();
    const float smoothingConstant = replicatedTransform_->GetSmoothingConstant();

    const auto confirmedPosition = replicatedTransform_->GetTemporalPosition(confirmedFrame);
    URHO3D_ASSERT(confirmedPosition);

    const Vector3 offset = confirmedPosition->value_ - nextInput.startPosition_;
    if (!offset.Equals(Vector3::ZERO, movementThreshold))
    {
        kinematicController_->AdjustRawPosition(offset, smoothingConstant);
        return true;
    }
    return false;
}

void PredictedKinematicController::ApplyPayload(const PredictedKinematicControllerFrame& payload)
{
    if (!IsConnectedToComponents())
        return;

    node_->SetWorldRotation(payload.rotation_);
    kinematicController_->SetWalkDirection(payload.walkVelocity_);
    if (payload.needJump_ && kinematicController_->OnGround())
        kinematicController_->Jump();
}

void PredictedKinematicController::WritePayload(
    const PredictedKinematicControllerFrame& payload, Serializer& dest) const
{
    dest.WriteVector3(payload.walkVelocity_);
    dest.WriteBool(payload.needJump_);
    dest.WriteQuaternion(payload.rotation_);
}

void PredictedKinematicController::ReadPayload(PredictedKinematicControllerFrame& payload, Deserializer& src) const
{
    payload.walkVelocity_ = src.ReadVector3();
    payload.needJump_ = src.ReadBool();
    payload.rotation_ = src.ReadQuaternion();
}

} // namespace Urho3D
#endif
