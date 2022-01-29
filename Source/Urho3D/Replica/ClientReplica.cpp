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

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Exception.h"
#include "../IO/Log.h"
#include "../Network/Connection.h"
#include "../Network/Network.h"
#include "../Network/NetworkEvents.h"
#include "../Replica/NetworkObject.h"
#include "../Replica/NetworkManager.h"
#include "../Replica/NetworkSettingsConsts.h"
#include "../Replica/ClientReplica.h"
#include "../Scene/Scene.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

ClientReplicaClock::ClientReplicaClock(Scene* scene, AbstractConnection* connection,
    const MsgSceneClock& initialClock, const VariantMap& serverSettings)
    : Object(scene->GetContext())
    , scene_(scene)
    , connection_(connection)
    , serverSettings_(serverSettings)
    , thisConnectionId_(GetSetting(NetworkSettings::ConnectionId).GetUInt())
    , updateFrequency_(GetSetting(NetworkSettings::UpdateFrequency).GetUInt())
    , inputDelay_(initialClock.inputDelay_)
    , replicaTime_(InitializeSoftTime())
    , inputTime_(InitializeSoftTime())
    , physicsSync_(scene_, updateFrequency_, false)
{
    UpdateServerTime(initialClock, false);
    replicaTime_.Reset(ToClientTime(serverTime_));
    inputTime_.Reset(ToInputTime(serverTime_));
    latestScaledInputTime_ = replicaTime_.Get();
}

ClientReplicaClock::~ClientReplicaClock()
{
}

float ClientReplicaClock::UpdateClientClocks(float timeStep, const ea::vector<MsgSceneClock>& pendingClockUpdates)
{
    serverTime_ += SecondsToFrames(timeStep);
    for (const MsgSceneClock& msg : pendingClockUpdates)
        UpdateServerTime(msg, true);

    replicaTime_.Update(timeStep, ToClientTime(serverTime_));

    const NetworkTime previousInputTime = inputTime_.Get();
    const float scaledTimeStep = inputTime_.Update(timeStep, ToInputTime(serverTime_));

    if (timeStep != scaledTimeStep)
        latestScaledInputTime_ = inputTime_.Get();

    isNewInputFrame_ = previousInputTime.GetFrame() != inputTime_.Get().GetFrame();
    if (isNewInputFrame_)
        physicsSync_.Synchronize(inputTime_.Get().GetFrame(), inputTime_.Get().GetSubFrame() / updateFrequency_);
    else
        physicsSync_.Update(scaledTimeStep);

    return scaledTimeStep;
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
    if (skipOutdated && NetworkTime{msg.latestFrame_} - NetworkTime{latestServerFrame_} < 0.0)
        return;

    const unsigned serverFrameTime = connection_->RemoteToLocalTime(msg.latestFrameTime_);
    const auto offsetMs = static_cast<int>(connection_->GetLocalTime() - serverFrameTime);

    inputDelay_ = msg.inputDelay_;
    latestServerFrame_ = msg.latestFrame_;
    serverTime_ = NetworkTime{msg.latestFrame_};
    serverTime_ += SecondsToFrames(offsetMs * 0.001);
}

NetworkTime ClientReplicaClock::ToClientTime(const NetworkTime& serverTime) const
{
    const double interpolationDelay = GetSetting(NetworkSettings::InterpolationDelay).GetDouble();
    const double clientDelay = interpolationDelay + connection_->GetPing() * 0.001;
    return serverTime - SecondsToFrames(clientDelay);
}

NetworkTime ClientReplicaClock::ToInputTime(const NetworkTime& serverTime) const
{
    return serverTime + static_cast<double>(inputDelay_);
}

ClientReplica::ClientReplica(
    Scene* scene, AbstractConnection* connection, const MsgSceneClock& initialClock, const VariantMap& serverSettings)
    : ClientReplicaClock(scene, connection, initialClock, serverSettings)
    , replicationManager_(scene->GetComponent<NetworkManager>())
{
    URHO3D_ASSERT(replicationManager_);

    SubscribeToEvent(E_INPUTREADY, [this](StringHash, VariantMap& eventData)
    {
        using namespace InputReady;
        const float timeStep = eventData[P_TIMESTEP].GetFloat();
        OnInputReady(timeStep);
    });
}

bool ClientReplica::ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData)
{
    switch (messageId)
    {
    case MSG_SCENE_CLOCK:
    {
        const auto msg = ReadNetworkMessage<MsgSceneClock>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        ProcessSceneClock(msg);
        return true;
    }

    case MSG_REMOVE_OBJECTS:
    {
        connection_->OnMessageReceived(messageId, messageData);

        ProcessRemoveObjects(messageData);
        return true;
    }

    case MSG_ADD_OBJECTS:
    {
        connection_->OnMessageReceived(messageId, messageData);

        ProcessAddObjects(messageData);
        return true;
    }

    case MSG_UPDATE_OBJECTS_RELIABLE:
    {
        connection_->OnMessageReceived(messageId, messageData);

        ProcessUpdateObjectsReliable(messageData);
        return true;
    }

    case MSG_UPDATE_OBJECTS_UNRELIABLE:
    {
        connection_->OnMessageReceived(messageId, messageData);

        ProcessUpdateObjectsUnreliable(messageData);
        return true;
    }

    default:
        return false;
    }
}

void ClientReplica::ProcessSceneClock(const MsgSceneClock& msg)
{
    pendingClockUpdates_.push_back(msg);
}

void ClientReplica::ProcessRemoveObjects(MemoryBuffer& messageData)
{
    const unsigned messageFrame = messageData.ReadUInt();
    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        WeakPtr<NetworkObject> networkObject{ replicationManager_->GetNetworkObject(networkId) };
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
    const unsigned messageFrame = messageData.ReadUInt();
    while (!messageData.IsEof())
    {
        const auto networkId = static_cast<NetworkId>(messageData.ReadUInt());
        const StringHash componentType = messageData.ReadStringHash();
        const unsigned ownerConnectionId = messageData.ReadVLE();

        messageData.ReadBuffer(componentBuffer_.GetBuffer());

        const bool isOwned = ownerConnectionId == GetConnectionId();
        if (NetworkObject* networkObject = CreateNetworkObject(networkId, componentType, isOwned))
        {
            componentBuffer_.Resize(componentBuffer_.GetBuffer().size());
            componentBuffer_.Seek(0);
            networkObject->ReadSnapshot(messageFrame, componentBuffer_);
        }
    }
}

void ClientReplica::ProcessUpdateObjectsReliable(MemoryBuffer& messageData)
{
    const unsigned messageFrame = messageData.ReadUInt();
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
    const unsigned messageFrame = messageData.ReadUInt();

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

NetworkObject* ClientReplica::CreateNetworkObject(NetworkId networkId, StringHash componentType, bool isOwned)
{
    auto networkObject = DynamicCast<NetworkObject>(context_->CreateObject(componentType));
    if (!networkObject)
    {
        URHO3D_LOGWARNING("Cannot create NetworkObject {} of type #{} '{}'",
            ToString(networkId), componentType.Value(), componentType.Reverse());
        return nullptr;
    }
    networkObject->SetNetworkId(networkId);

    if (isOwned)
    {
        networkObject->SetNetworkMode(NetworkObjectMode::ClientOwned);
        ownedObjects_.insert(WeakPtr<NetworkObject>(networkObject));
    }
    else
        networkObject->SetNetworkMode(NetworkObjectMode::ClientReplicated);

    if (NetworkObject* oldNetworkObject = replicationManager_->GetNetworkObject(networkId, false))
    {
        URHO3D_LOGWARNING("NetworkObject {} overwrites existing NetworkObject {}",
            ToString(networkId),
            ToString(oldNetworkObject->GetNetworkId()));
        RemoveNetworkObject(WeakPtr<NetworkObject>(oldNetworkObject));
    }

    Node* newNode = scene_->CreateChild(EMPTY_STRING, LOCAL);
    newNode->AddComponent(networkObject, 0, LOCAL);
    return networkObject;
}

NetworkObject* ClientReplica::GetCheckedNetworkObject(NetworkId networkId, StringHash componentType)
{
    NetworkObject* networkObject = replicationManager_->GetNetworkObject(networkId);
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

    networkObject->PrepareToRemove();
    if (networkObject)
        networkObject->Remove();
}

unsigned ClientReplica::GetTraceCapacity() const
{
    // TODO(network): Refactor
    float traceDurationInSeconds = 1.0f;
    return CeilToInt(traceDurationInSeconds * GetUpdateFrequency());
}

unsigned ClientReplica::GetPositionExtrapolationFrames() const
{
    // TODO(network): Refactor
    float positionExtrapolationTimeInSeconds = 0.25;
    return RoundToInt(positionExtrapolationTimeInSeconds * GetUpdateFrequency());
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
        GetServerTime().GetFrame(),
        ea::max(0, CeilToInt(replicaDelayMs)),
        GetLatestScaledInputTime().GetFrame());
}

void ClientReplica::OnInputReady(float timeStep)
{
    UpdateClientClocks(timeStep, pendingClockUpdates_);
    pendingClockUpdates_.clear();

    const bool isNewInputFrame = IsNewInputFrame();

    const auto& networkObjects = replicationManager_->GetNetworkObjects();
    for (NetworkObject* networkObject : networkObjects)
    {
        networkObject->InterpolateState(GetReplicaTime(), GetInputTime(), isNewInputFrame);
    }

    if (isNewInputFrame)
    {
        auto network = GetSubsystem<Network>();
        network->SendEvent(E_BEGINCLIENTNETWORKFRAME);
        SendObjectsFeedbackUnreliable(GetInputTime().GetFrame());
    }
}

void ClientReplica::SendObjectsFeedbackUnreliable(unsigned feedbackFrame)
{
    connection_->SendGeneratedMessage(MSG_OBJECTS_FEEDBACK_UNRELIABLE, NetworkMessageFlag::None,
        [&](VectorBuffer& msg, ea::string* debugInfo)
    {
        msg.WriteUInt(feedbackFrame);

        bool sendMessage = false;
        for (NetworkObject* networkObject : ownedObjects_)
        {
            if (!networkObject)
                continue;

            componentBuffer_.Clear();
            if (const auto mask = networkObject->GetUnreliableFeedbackMask(feedbackFrame))
            {
                networkObject->WriteUnreliableFeedback(feedbackFrame, mask, componentBuffer_);
                sendMessage = true;
                msg.WriteUInt(static_cast<unsigned>(networkObject->GetNetworkId()));
                msg.WriteBuffer(componentBuffer_.GetBuffer());
            }

            if (debugInfo)
            {
                if (!debugInfo->empty())
                    debugInfo->append(", ");
                debugInfo->append(ToString(networkObject->GetNetworkId()));
            }
        }
        return sendMessage;
    });
}

}
