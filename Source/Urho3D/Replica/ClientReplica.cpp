// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Replica/ClientReplica.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Core/Exception.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Network/Connection.h"
#include "Urho3D/Network/MessageUtils.h"
#include "Urho3D/Network/Network.h"
#include "Urho3D/Network/NetworkEvents.h"
#include "Urho3D/Replica/NetworkObject.h"
#include "Urho3D/Replica/NetworkSettingsConsts.h"
#include "Urho3D/Replica/ReplicationManager.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

ClientReplicaClock::ClientReplicaClock(
    Scene* scene, AbstractConnection* connection, const MsgSceneClock& initialClock, const VariantMap& serverSettings)
    : Object(scene->GetContext())
    , scene_(scene)
    , connection_(connection)
    , serverSettings_(serverSettings)
    , thisConnectionId_(GetSetting(NetworkSettings::ConnectionId).GetUInt())
    , updateFrequency_(GetSetting(NetworkSettings::UpdateFrequency).GetUInt())
    , inputDelay_(initialClock.inputDelay_)
    , replicaTime_(InitializeSoftTime())
    , inputTime_(InitializeSoftTime())
    , updateSync_(MakeShared<SceneUpdateSynchronizer>(scene_, SceneUpdateSynchronizer::Params{false, updateFrequency_}))
{
    UpdateServerTime(initialClock, false);
    replicaTime_.Reset(ToReplicaTime(serverTime_));
    inputTime_.Reset(ToInputTime(serverTime_));
    latestScaledInputTime_ = replicaTime_.GetTime();
}

ClientReplicaClock::~ClientReplicaClock()
{
}

void ClientReplicaClock::UpdateClientClocks(float timeStep, const ea::vector<MsgSceneClock>& pendingClockUpdates)
{
    serverTime_ += SecondsToFrames(timeStep);
    for (const MsgSceneClock& msg : pendingClockUpdates)
        UpdateServerTime(msg, true);

    replicaTimeStep_ = replicaTime_.Update(timeStep, ToReplicaTime(serverTime_));

    const NetworkTime previousInputTime = inputTime_.GetTime();
    inputTimeStep_ = inputTime_.Update(timeStep, ToInputTime(serverTime_));

    if (timeStep != inputTimeStep_)
        latestScaledInputTime_ = inputTime_.GetTime();

    isNewInputFrame_ = previousInputTime.Frame() != inputTime_.GetTime().Frame();
    if (isNewInputFrame_)
        updateSync_->Synchronize(inputTime_.GetTime().Frame(), inputTime_.GetTime().Fraction() / updateFrequency_);
    else
        updateSync_->Update(inputTimeStep_);
}

const Variant& ClientReplicaClock::GetSetting(const NetworkSetting& setting) const
{
    return GetNetworkSetting(serverSettings_, setting);
}

SoftNetworkTime ClientReplicaClock::InitializeSoftTime() const
{
    const float timeSnapThreshold = GetSetting(NetworkSettings::TimeSnapThreshold).GetFloat();
    const float timeErrorTolerance = GetSetting(NetworkSettings::TimeErrorTolerance).GetFloat();
    const float minTimeDilation = GetSetting(NetworkSettings::MinTimeDilation).GetFloat();
    const float maxTimeDilation = GetSetting(NetworkSettings::MaxTimeDilation).GetFloat();
    return SoftNetworkTime{updateFrequency_, timeSnapThreshold, timeErrorTolerance, minTimeDilation, maxTimeDilation};
}

void ClientReplicaClock::UpdateServerTime(const MsgSceneClock& msg, bool skipOutdated)
{
    if (skipOutdated && (msg.latestFrame_ < latestServerFrame_))
        return;

    const unsigned serverFrameTime = connection_->RemoteToLocalTime(msg.latestFrameTime_);
    const auto offsetMs = static_cast<int>(connection_->GetLocalTime() - serverFrameTime);

    inputDelay_ = msg.inputDelay_;
    latestServerFrame_ = msg.latestFrame_;
    serverTime_ = NetworkTime{msg.latestFrame_};
    serverTime_ += SecondsToFrames(offsetMs * 0.001);
}

NetworkTime ClientReplicaClock::ToReplicaTime(const NetworkTime& serverTime) const
{
    const double interpolationDelay = GetSetting(NetworkSettings::InterpolationDelay).GetDouble();
    const double interpolationLimit = GetSetting(NetworkSettings::InterpolationLimit).GetDouble();
    const double clientDelay = ea::min(interpolationLimit, interpolationDelay + connection_->GetPing() * 0.001);
    return serverTime - SecondsToFrames(clientDelay);
}

NetworkTime ClientReplicaClock::ToInputTime(const NetworkTime& serverTime) const
{
    return serverTime + static_cast<double>(inputDelay_);
}

ClientReplica::ClientReplica(
    Scene* scene, AbstractConnection* connection, const MsgSceneClock& initialClock, const VariantMap& serverSettings)
    : ClientReplicaClock(scene, connection, initialClock, serverSettings)
    , network_(GetSubsystem<Network>())
    , objectRegistry_(scene->GetComponent<ReplicationManager>())
{
    URHO3D_ASSERT(objectRegistry_);

    SubscribeToEvent(E_INPUTREADY, [this](VariantMap& eventData)
    {
        using namespace InputReady;
        const float timeStep = eventData[P_TIMESTEP].GetFloat();
        OnInputReady(timeStep);
    });

    SubscribeToEvent(network_, E_NETWORKUPDATE, [this](VariantMap& eventData)
    {
        using namespace NetworkUpdate;
        const bool isServer = eventData[P_ISSERVER].GetBool();
        if (!isServer)
            OnNetworkUpdate();
    });
}

ClientReplica::~ClientReplica() {}

bool ClientReplica::ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData)
{
    switch (messageId)
    {
    case MSG_SCENE_CLOCK:
    {
        const auto msg = ReadSerializedMessage<MsgSceneClock>(messageData);
        connection_->LogMessagePayload(messageId, msg);

        ProcessSceneClock(msg);
        return true;
    }

    case MSG_REMOVE_OBJECTS:
    {
        ProcessRemoveObjects(messageData);
        return true;
    }

    case MSG_ADD_OBJECTS:
    case MSG_ADD_OBJECTS_INCOMPLETE:
    {
        LargeMessageReader reader{*connection_, MSG_ADD_OBJECTS_INCOMPLETE, MSG_ADD_OBJECTS};
        reader.OnMessage(messageId, messageData, //
            [this](MemoryBuffer& fullMessageData) { ProcessAddObjects(fullMessageData); });

        return true;
    }

    case MSG_UPDATE_OBJECTS_RELIABLE:
    case MSG_UPDATE_OBJECTS_RELIABLE_INCOMPLETE:
    {
        LargeMessageReader reader{*connection_, MSG_UPDATE_OBJECTS_RELIABLE_INCOMPLETE, MSG_UPDATE_OBJECTS_RELIABLE};
        reader.OnMessage(messageId, messageData,
            [this](MemoryBuffer& fullMessageData) { ProcessUpdateObjectsReliable(fullMessageData); });

        return true;
    }

    case MSG_UPDATE_OBJECTS_UNRELIABLE:
    {
        ProcessUpdateObjectsUnreliable(messageData);
        return true;
    }

    default:
        return false;
    }
}

void ClientReplica::ProcessSceneUpdate()
{
    VariantMap& eventData = scene_->GetEventDataMap();

    using namespace SceneNetworkUpdate;
    eventData[P_SCENE] = scene_;
    eventData[P_TIMESTEP_REPLICA] = GetReplicaTimeStep();
    eventData[P_TIMESTEP_INPUT] = GetInputTimeStep();
    scene_->SendEvent(E_SCENENETWORKUPDATE, eventData);
}

void ClientReplica::ProcessSceneClock(const MsgSceneClock& msg)
{
    pendingClockUpdates_.push_back(msg);
}

void ClientReplica::ProcessRemoveObjects(MemoryBuffer& messageData)
{
    const auto messageFrame = static_cast<NetworkFrame>(messageData.ReadInt64());
    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        WeakPtr<NetworkObject> networkObject{ objectRegistry_->GetNetworkObject(networkId) };
        if (!networkObject)
        {
            URHO3D_LOGWARNING("Cannot find NetworkObject {} to remove", ToString(networkId));
            continue;
        }

        RemoveNetworkObject(networkObject);
    }
}

void ClientReplica::ProcessAddObjects(MemoryBuffer& messageData)
{
    const auto messageFrame = static_cast<NetworkFrame>(messageData.ReadInt64());
    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        const StringHash componentType = messageData.ReadStringHash();
        const unsigned ownerConnectionId = messageData.ReadVLE();

        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        const bool isOwned = ownerConnectionId == GetConnectionId();
        if (NetworkObject* networkObject = CreateNetworkObject(networkId, componentType))
        {
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);
            networkObject->InitializeFromSnapshot(messageFrame, componentBuffer_, isOwned);

            if (isOwned)
            {
                networkObject->SetNetworkMode(NetworkObjectMode::ClientOwned);
                ownedObjects_.emplace(WeakPtr<NetworkObject>(networkObject));
            }
            else
                networkObject->SetNetworkMode(NetworkObjectMode::ClientReplicated);
        }
    }
}

void ClientReplica::ProcessUpdateObjectsReliable(MemoryBuffer& messageData)
{
    const auto messageFrame = static_cast<NetworkFrame>(messageData.ReadInt64());
    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        const StringHash componentType = messageData.ReadStringHash();

        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        if (NetworkObject* networkObject = GetCheckedNetworkObject(networkId, componentType))
        {
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);
            networkObject->ReadReliableDelta(messageFrame, componentBuffer_);
        }
    }
}

void ClientReplica::ProcessUpdateObjectsUnreliable(MemoryBuffer& messageData)
{
    const auto messageFrame = static_cast<NetworkFrame>(messageData.ReadInt64());

    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        const StringHash componentType = messageData.ReadStringHash();

        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        if (NetworkObject* networkObject = GetCheckedNetworkObject(networkId, componentType))
        {
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);
            networkObject->ReadUnreliableDelta(messageFrame, componentBuffer_);
        }
    }
}

NetworkObject* ClientReplica::CreateNetworkObject(NetworkId networkId, StringHash componentType)
{
    auto networkObject = DynamicCast<NetworkObject>(context_->CreateObject(componentType));
    if (!networkObject)
    {
        URHO3D_LOGWARNING("Cannot create NetworkObject {} of type #{} '{}'",
            ToString(networkId), componentType.Value(), componentType.Reverse());
        return nullptr;
    }
    networkObject->SetNetworkId(networkId);

    if (NetworkObject* oldNetworkObject = objectRegistry_->GetNetworkObject(networkId, false))
    {
        URHO3D_LOGWARNING("NetworkObject {} overwrites existing NetworkObject {}",
            ToString(networkId),
            ToString(oldNetworkObject->GetNetworkId()));
        RemoveNetworkObject(WeakPtr<NetworkObject>(oldNetworkObject));
    }

    Node* newNode = scene_->CreateChild(EMPTY_STRING);
    newNode->AddComponent(networkObject, 0);
    return networkObject;
}

NetworkObject* ClientReplica::GetCheckedNetworkObject(NetworkId networkId, StringHash componentType)
{
    NetworkObject* networkObject = objectRegistry_->GetNetworkObject(networkId);
    if (!networkObject)
    {
        URHO3D_LOGWARNING("Cannot find existing NetworkObject {}", ToString(networkId));
        return nullptr;
    }

    if (networkObject->GetType() != componentType)
    {
        URHO3D_LOGWARNING("NetworkObject {} has unexpected type '{}', message was prepared for {}",
            ToString(networkId), networkObject->GetTypeName(),
            componentType.ToDebugString());
        return nullptr;
    }

    return networkObject;
}

void ClientReplica::RemoveNetworkObject(WeakPtr<NetworkObject> networkObject)
{
    if (networkObject->GetNetworkMode() == NetworkObjectMode::ClientOwned)
        ownedObjects_.erase(networkObject);

    Node* parentNode = networkObject->GetNode()->GetParent();
    for (NetworkObject* childNetworkObject : networkObject->GetChildrenNetworkObjects())
    {
        if (childNetworkObject)
            childNetworkObject->GetNode()->SetParent(parentNode);
    }

    networkObject->PrepareToRemove();
    if (networkObject)
        networkObject->Remove();
}

ea::string ClientReplica::GetDebugInfo() const
{
    static const ea::string unnamedScene = "Unnamed";
    const ea::string& sceneName = !scene_->GetName().empty() ? scene_->GetName() : unnamedScene;

    const double inputDelayMs = (GetInputTime() - GetServerTime()) / GetUpdateFrequency() * 1000.0f;
    const double replicaDelayMs = (GetServerTime() - GetReplicaTime()) / GetUpdateFrequency() * 1000.0f;
    return Format("Scene '{}': Ping {}ms, Time {}ms+#{}-{}ms, Sync since #{}\n",
        sceneName,
        connection_->GetPing(),
        ea::max(0, CeilToInt(inputDelayMs)),
        GetServerTime().Frame(),
        ea::max(0, CeilToInt(replicaDelayMs)),
        GetLatestScaledInputTime().Frame());
}

void ClientReplica::OnInputReady(float timeStep)
{
    UpdateClientClocks(timeStep, pendingClockUpdates_);
    pendingClockUpdates_.clear();

    for (NetworkObject* networkObject : objectRegistry_->GetNetworkObjects())
        networkObject->InterpolateState(GetReplicaTimeStep(), GetInputTimeStep(), GetReplicaTime(), GetInputTime());

    if (IsNewInputFrame())
    {
        using namespace BeginClientNetworkFrame;
        auto& eventData = network_->GetEventDataMap();
        eventData[P_FRAME] = static_cast<long long>(GetInputTime().Frame());
        network_->SendEvent(E_BEGINCLIENTNETWORKFRAME);
    }
}

void ClientReplica::OnNetworkUpdate()
{
    if (IsNewInputFrame())
    {
        using namespace EndClientNetworkFrame;
        auto& eventData = network_->GetEventDataMap();
        eventData[P_FRAME] = static_cast<long long>(GetInputTime().Frame());
        network_->SendEvent(E_ENDCLIENTNETWORKFRAME);

        SendObjectsFeedbackUnreliable(GetInputTime().Frame());
    }
}

void ClientReplica::SendObjectsFeedbackUnreliable(NetworkFrame feedbackFrame)
{
    MultiMessageWriter writer{*connection_, MSG_OBJECTS_FEEDBACK_UNRELIABLE, PacketType::UnreliableUnordered};

    VectorBuffer& msg = writer.GetBuffer();
    ea::string* debugInfo = writer.GetDebugInfo();

    msg.WriteInt64(static_cast<long long>(feedbackFrame));
    writer.CompleteHeader();

    for (NetworkObject* networkObject : ownedObjects_)
    {
        if (!networkObject)
            continue;

        if (!networkObject->PrepareUnreliableFeedback(feedbackFrame))
            continue;

        componentBuffer_.Clear();
        networkObject->WriteUnreliableFeedback(feedbackFrame, componentBuffer_);
        msg.WriteUInt(static_cast<unsigned>(networkObject->GetNetworkId()));
        msg.WriteBuffer(componentBuffer_.GetBuffer());

        if (debugInfo)
        {
            if (!debugInfo->empty())
                debugInfo->append(", ");
            debugInfo->append(ToString(networkObject->GetNetworkId()));
        }

        writer.CompletePayload();
    }
}

}
